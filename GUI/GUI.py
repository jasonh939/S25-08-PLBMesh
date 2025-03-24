"""
This file contains the code for running the GUI. The GUI receives packets from
Arduino Featherboards, decodes the packet data, and plots the data on an 
interactive map running in Qt.
"""

from datetime import UTC, datetime
import time
import io
import struct
import sys
import threading
import serial
import os
import json

from PyQt5 import QtCore, QtWebEngineWidgets, QtWidgets
import folium

SERIAL_PORT = 'COM6'  # This should be changed to match Arduino serial port
BAUD_RATE = 9600
PACKET_SIZE = 16

MAX_RADIO_ID = 16
lngMin, lngMax = -180., 180.
latMin, latMax = -90., 90.

class PacketLengthError(Exception):
    """Creates a new error to throw if the packet is not the right length"""
    pass

class MapManager(QtCore.QObject):
    """Manages the map and the data points on the map"""

    htmlChanged = QtCore.pyqtSignal(str)
    closeWindow = QtCore.pyqtSignal()

    def __init__(self):
        super().__init__()
        try:
            self.serial_port = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        except serial.SerialException as err:
            print(f"Error opening serial port: {err}", file=sys.stderr)
            raise
            
        self.points_db = {}    
        self.map = folium.Map(location=[37.227779, -80.422289], zoom_start=18)
        self.live_file = "live_beacons.json"
        self.history_file = "history_beacons.json"
        self.live_data = self.load_json(self.live_file)
        self.history_data = self.load_json(self.history_file)
        self.latitudes = []
        self.longitudes = []
        self.paused = False # Flag to pause map updates
        self.show_history = False # Flag to toggle history display

        threading.Thread(target=self.exec, daemon=True).start()

    def load_json(self, json_file):
        """ Load JSON file or create one if it doesn't exist """
        if os.path.exists(json_file):
            try:
                with open(json_file, 'r') as f:
                    return json.load(f)
            except json.JSONDecodeError:
                print(f"Error reading {json_file}. Creating a new GeoJSON file.")
        
        # Create an empty GeoJSON structure if file doesn't exist or is invalid
        return {
            "type": "FeatureCollection",
            "features": []
        }
    
    def save_json(self, json_file, json_data):
        """ Save any changes to JSON file"""
        with open(json_file, 'w') as f:
            json.dump(json_data, f, indent=4)

    def set_paused(self, paused_state):
        """ Set paused state for map updates """
        self.paused = paused_state

    def set_show_history(self, show_history):
        """ Set show history flag for QT GUI """
        self.show_history = show_history

        return self.update_map()

    def exec(self):
        """Main exec loop for the worker"""
        try:
            while True:
                if self.serial_port.in_waiting >= PACKET_SIZE:          # Anytime there is a packet to read
                    data = self.serial_port.read(PACKET_SIZE + 2)       # + 2 takes in the "\r\n" which will be trimmed
                    if len(data) == PACKET_SIZE + 2:
                        data = data[2:]
                    try:
                        decodedData = self.decode(data)
                        print("Checking if packet is valid...")
                        if (self.isValidGPS(decodedData[3], decodedData[4]) and 0 < decodedData[0] < MAX_RADIO_ID): # Checks for various invalid packets
                            if self.check_point(decodedData):           # Check if point is a duplicate
                                self.add_or_update_beacon(decodedData)                 # Add or update Live file with point data
                                self.add_to_history(decodedData)                        # Add point to history file
                                
                                if not self.paused: # If not paused, update the map
                                    self.htmlChanged.emit(self.update_map())   
                    except PacketLengthError as err:
                        print(f"Error: {err}", file=sys.stderr)
        except serial.SerialException as err:
            print(f"Serial communication error: {err}", file=sys.stderr)
            self.closeWindow.emit()
        finally:
            self.serial_port.close()

    def load_HTML(self):
        """Loads the html from the map"""
        data = io.BytesIO()
        self.map.save(data, close_file=False)
        return data.getvalue().decode()
    
    def update_map(self):
        """ Update map with the current JSON data """

        # Create a new map
        self.map = folium.Map(location=[37.227779, -80.422289], zoom_start=18)

        # reset coordinate list
        self.latitudes = []
        self.longitudes = []

        display_data = self.history_data if self.show_history else self.live_data

        # Add each beacon to the map
        for feature in display_data["features"]:
            properties = feature["properties"]
            coordinates = feature["geometry"]["coordinates"]

            # Extract each data line
            radio_id = properties["Radio ID"]     
            message_id = properties["Message ID"]
            panic_state = properties["Panic State"]
            latitude = properties["Latitude"]
            longitude = properties["Longitude"]
            battery_life = properties["Battery Life"]
            utc_time = properties["Time"]

            # Add coordinates to member variables
            self.latitudes.append(latitude)
            self.longitudes.append(longitude)

            # Create tooltip HTML for hover information
            tooltip_html = f"""
            <div style="font-family: Arial; font-size: 12px; padding: 5px; width: 200px;">
                <div style="font-weight: bold; margin-bottom: 3px;">Radio ID: {radio_id}</div>
                <div>Message ID: {message_id}</div>
                <div>Panic State: {'YES' if panic_state else 'NO'}</div>
                <div>Battery: {battery_life:.1f}%</div>
                <div>Time: {utc_time} UTC</div>
            </div>
            """
            
            # Create a tooltip that shows on hover
            tooltip = folium.Tooltip(tooltip_html)

            # Create a marker for each beacon
            popup_string = (
            f"Radio ID: {radio_id}<br>"
            f"Message ID: {message_id}<br>"
            f"Panic State: {panic_state}<br>"
            f"Latitude: {latitude:.4f}<br>"
            f"Longitude: {longitude:.4f}<br>"
            f"Battery Life: {battery_life:.1f}%<br>"
            f"Time: {utc_time} UTC"
            )
    
            iframe = folium.IFrame(html=popup_string)
            popup = folium.Popup(iframe, min_width=250, max_width=250)

            # set icon color based on panic mode state
            icon_color = 'red' if panic_state else 'blue'

            # In history view, use smaller markers with timestamp-based opacity
            if self.show_history:
                # Create lines connecting points from the same radio ID
                self.add_history_lines(radio_id)
                
                # Use smaller markers for history points
                icon = folium.DivIcon(
                    icon_size=(150, 36),
                    icon_anchor=(10, 10),
                    popup_anchor=(1, -15),
                    html=f'''
                        <div style="
                            width: 20px;
                            height: 20px;
                            border-radius: 50%;
                            background-color: {icon_color};
                            display: flex;
                            justify-content: center;
                            align-items: center;
                            color: white;
                            font-size: 10px;
                            font-weight: bold;
                            border: 1px solid white;
                        ">
                            {radio_id}
                        </div>
                    '''
                )
            else:
                # Regular marker for current view
                icon = folium.DivIcon(
                    icon_size=(150, 36),
                    icon_anchor=(14, 40),
                    popup_anchor=(1, -40),
                    html=f'''
                        <div style="
                            width: 30px;
                            height: 30px;
                            border-radius: 50%;
                            background-color: {icon_color};
                            display: flex;
                            justify-content: center;
                            align-items: center;
                            color: white;
                            font-size: 14px;
                            font-weight: bold;
                            border: 2px solid white;
                        ">
                            {radio_id}
                        </div>
                    '''
                )

            # add custom marker to map
            folium.Marker(
                location=[latitude, longitude], 
                popup=popup,
                tooltip=tooltip,
                icon=icon
            ).add_to(self.map)


        # Set map bounds for zoom
        if self.latitudes and self.longitudes:
            southwest_point = (min(self.latitudes) - 0.0001, min(self.longitudes) - 0.0001)
            northeast_point = (max(self.latitudes) + 0.0001, max(self.longitudes) + 0.0001)
            self.map.fit_bounds([southwest_point, northeast_point])

        return self.load_HTML()

    def add_or_update_beacon(self, point):
        """
        Add a new beacon or update an existing one in JSON file based on radio_id
        """
        (radio_id, message_id, panic_state, latitude, longitude, 
         battery_life, utc_time) = point
        
        print(f"Adding or updating beacon with Radio ID: {radio_id}")

        # Find if radio_id already exists in JSON file
        beacon_index = -1
        for i, feature in enumerate(self.live_data["features"]):
            if feature["properties"].get("Radio ID") == radio_id:
                beacon_index = i
                break
        
        # Create JSON feature for beacon point
        beacon_data = {
            "type": "Feature",
            "properties": {
                "Radio ID": radio_id,
                "Message ID": message_id,
                "Panic State": panic_state,
                "Latitude": latitude,
                "Longitude": longitude,
                "Battery Life": battery_life,
                "Time": utc_time
            },
            "geometry": {
                "type": "Point",
                "coordinates": [longitude, latitude]  # JSON uses [long, lat] order
            }
        }
        
        # Update existing beacon point or add a new point in JSON file
        if beacon_index >= 0:
            self.live_data["features"][beacon_index] = beacon_data
            print(f"Updated beacon with Radio ID: {radio_id}")
        else:
            self.live_data["features"].append(beacon_data)
            print(f"Added new beacon with Radio ID: {radio_id}")
        
        # Save changes to file
        self.save_json(self.live_file, self.live_data)

    def add_to_history(self, point):
        """ Add packet to history file """
        (radio_id, message_id, panic_state, latitude, longitude, 
         battery_life, utc_time) = point
        
        history_point = {
            "type": "Feature",
            "properties": {
                "Radio ID": radio_id,
                "Message ID": message_id,
                "Panic State": panic_state,
                "Latitude": latitude,
                "Longitude": longitude,
                "Battery Life": battery_life,
                "Time": utc_time
            },
            "geometry": {
                "type": "Point",
                "coordinates": [longitude, latitude]  
            }
        }

        self.history_data["features"].append(history_point)

        # Save changes to file
        self.save_json(self.history_file, self.history_data)

    def add_history_lines(self, radio_id):
        """Add lines connecting the history points for a specific radio ID"""
        # Get all points for this radio ID
        points = []
        for feature in self.history_data["features"]:
            if feature["properties"]["Radio ID"] == radio_id:
                coords = feature["geometry"]["coordinates"]
                timestamp = feature["properties"]["Time"]
                points.append((coords[1], coords[0], timestamp))  # lat, lon format for polyline
        
        # Sort points by timestamp
        points.sort(key=lambda x: datetime.strptime(x[2], "%m-%d-%Y %H:%M:%S"))
        
        # Only draw lines if we have 2+ points
        if len(points) >= 2:
            locations = [(p[0], p[1]) for p in points]
            folium.PolyLine(
                locations=locations,
                weight=2,
                color=f'blue',
                opacity=0.6,
                dash_array='5,5'
            ).add_to(self.map)


    def check_point(self, packet):
        """Checks packet data against internal database to see if it is a duplicate"""
        (radio_id, message_id, panic_state, latitude, longitude, 
         battery_life, utc_time) = packet
        
        print(f"Received packet from Radio ID: {radio_id}. Checking point now...")

        if radio_id in self.points_db:
            if self.points_db[radio_id] == packet:
                return False
            else:
                self.points_db[radio_id] = packet
                return True
        else:
            self.points_db[radio_id] = packet
            return True


    def decode(self, received_data: bytes):
        """Decodes the data packet from Arduino"""
        binary_representation = ' '.join(format(byte, '08b') for byte in received_data)
        print(received_data)
        print(binary_representation)
        if (packet_length := len(received_data)) != PACKET_SIZE:
            raise PacketLengthError(
                f"Expected packet length of {PACKET_SIZE} bytes. Received {packet_length} bytes"
            )

        # Check MSB of first byte to determine packet type
        is_meshpkt = bool(received_data[0] & 0x80)

        if is_meshpkt:
            # New Mesh Packet: 8-bit radio ID
            radio_id, message_byte, latitude, longitude, battery_life, unix_time = struct.unpack(
                "!BhffBI", received_data 
            )
            radio_id = radio_id & 0x7F # Remove MSB 1
            # Mesh Packet: 15-bit message ID
            message_id = message_byte & 0x7FFF  # 0111 1111 1111 1111
            panic_state = bool(message_byte & 0x8000)  # Check MSB of message_byte
            
        else:
            # Legacy Packet: 16-bit radio ID
            radio_id, message_byte, latitude, longitude, battery_life, unix_time = struct.unpack(
                "!HbffBI", received_data 
            )
            # Legacy Packet: 7-bit message ID
            message_id = message_byte & 0x7F  # 0111 1111
            panic_state = bool(message_byte & 0x80)  # Check MSB of message_byte
        
        utc_time = datetime.fromtimestamp(unix_time, UTC).strftime("%m-%d-%Y %H:%M:%S")
        
        return (radio_id, message_id, panic_state, latitude, longitude,
                battery_life, utc_time)

    def isValidGPS(self, latitude: float, longitude: float):
        valid = lngMin <= longitude <= lngMax and latMin <= latitude <= latMax
        return valid

class BaseStationGUI(QtWidgets.QWidget):
    """Main QT GUI class that connects to base station via serial port"""

    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        self.webEngineView = QtWebEngineWidgets.QWebEngineView()

        try:
            self.mapManager = MapManager()
        except serial.SerialException:
            QtWidgets.QMessageBox.critical(
                self, 
                "Connection Error",
                f"Could not connect to Arduino on {SERIAL_PORT}. Please check the connection."
            )
            sys.exit(1)

        # Set HTML content and connect signal
        self.webEngineView.setHtml(self.mapManager.load_HTML())
        self.mapManager.htmlChanged.connect(self.webEngineView.setHtml)
        self.mapManager.closeWindow.connect(self.close)

        # HBox layout for buttons
        controlPanel = QtWidgets.QWidget()
        controlLayout = QtWidgets.QHBoxLayout(controlPanel)

        # Pause, Resume, and Force Update button
        self.pauseButton = QtWidgets.QPushButton("Pause Map Updates")
        self.pauseButton.clicked.connect(self.pauseMap)
        
        self.resumeButton = QtWidgets.QPushButton("Resume Map Updates")
        self.resumeButton.clicked.connect(self.resumeMap)
        self.resumeButton.setEnabled(False)

        self.forceUpdateButton = QtWidgets.QPushButton("Force Map Update")
        self.forceUpdateButton.clicked.connect(self.forceMapUpdate)
        self.forceUpdateButton.setEnabled(False)

        # History/Live toggle buttons
        self.viewLiveButton = QtWidgets.QPushButton("View Live Beacons")
        self.viewLiveButton.clicked.connect(self.showLiveView)
        self.viewLiveButton.setEnabled(False)

        self.viewHistoryButton = QtWidgets.QPushButton("View Beacon History")
        self.viewHistoryButton.clicked.connect(self.showHistoryView)

        # Labels
        self.statusLabel = QtWidgets.QLabel("Status: updates Active")
        self.viewLabel = QtWidgets.QLabel("View: Current Locations")

        # Add buttons and labels to layout
        controlLayout.addWidget(self.pauseButton)
        controlLayout.addWidget(self.resumeButton)
        controlLayout.addWidget(self.forceUpdateButton)
        controlLayout.addWidget(self.statusLabel)
        controlLayout.addWidget(self.viewLiveButton)
        controlLayout.addWidget(self.viewHistoryButton)
        controlLayout.addWidget(self.viewLabel)

        # Add button to clear all beacons
        self.clearBeaconsButton = QtWidgets.QPushButton("Clear All Beacons")
        self.clearBeaconsButton.clicked.connect(self.clearBeacons)

        # Main layout
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.webEngineView)
        layout.addWidget(controlPanel)
        layout.addWidget(self.clearBeaconsButton)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setStretchFactor(self.webEngineView, 10)
        layout.setStretchFactor(controlPanel, 1)

        self.resize(1920, 1080)
        self.setWindowTitle("Base station GUI")
        self.show()

    def pauseMap(self):
        """ Pause map updates """
        self.mapManager.set_paused(True)
        self.pauseButton.setEnabled(False)
        self.resumeButton.setEnabled(True)
        self.forceUpdateButton.setEnabled(True)
        self.statusLabel.setText("Status: updates PAUSED")

    def resumeMap(self):
        """Resume map updates"""
        self.mapManager.set_paused(False)
        self.pauseButton.setEnabled(True)
        self.resumeButton.setEnabled(False)
        self.forceUpdateButton.setEnabled(False)
        self.statusLabel.setText("Status: updates ACTIVE")
        
        # Force an immediate update when resuming
        self.forceMapUpdate()
    
    def forceMapUpdate(self):
        """Force a map update even when paused"""
        self.webEngineView.setHtml(self.mapManager.update_map())

    def showHistoryView(self):
        """ Show History of beacon locations """
        html = self.mapManager.set_show_history(True)
        self.webEngineView.setHtml(html)

        self.viewLiveButton.setEnabled(True)
        self.viewHistoryButton.setEnabled(False)
        self.viewLabel.setText("View: Past Locations")

    def showLiveView(self):
        """ Show Live beacon locations """
        html = self.mapManager.set_show_history(False)
        self.webEngineView.setHtml(html)

        self.viewLiveButton.setEnabled(False)
        self.viewHistoryButton.setEnabled(True)
        self.viewLabel.setText("View: Current Locations")

    def clearBeacons(self):
        self.mapManager.live_data["features"] = []
        self.mapManager.history_data["features"] = []
        self.mapManager.save_json(self.mapManager.live_file, self.mapManager.live_data)
        self.mapManager.save_json(self.mapManager.history_file, self.mapManager.history_data)
        self.mapManager.points_db = {}

        self.forceMapUpdate()

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    gui = BaseStationGUI()
    sys.exit(app.exec())