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
        self.json_file = "beacon_data.json"
        self.json_data = self.load_json()
        self.latitudes = []
        self.longitudes = []
        threading.Thread(target=self.exec, daemon=True).start()

    def load_json(self):
        """ Load JSON file or create one if it doesn't exist """
        if os.path.exists(self.json_file):
            try:
                with open(self.json_file, 'r') as f:
                    return json.load(f)
            except json.JSONDecodeError:
                print(f"Error reading {self.json_file}. Creating a new GeoJSON file.")
        
        # Create an empty GeoJSON structure if file doesn't exist or is invalid
        return {
            "type": "FeatureCollection",
            "features": []
        }
    
    def save_json(self):
        """ Save any changes to JSON file"""
        with open(self.json_file, 'w') as f:
            json.dump(self.json_data, f, indent=4)


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
                                self.add_or_update_beacon(decodedData)                 # Add or update JSON file with point data
                                self.htmlChanged.emit(self.update_map())     # Reload the html map
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

        # Add each beacon to the map
        for feature in self.json_data["features"]:
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

            # set icon marker based on radio_id
            icon = folium.DivIcon(
                icon_size=(150, 36),
                icon_anchor=(14, 40),
                html=f'''
                    <div style="
                        width: 30px;
                        height: 30px;
                        border-radius: 50%;
                        background-color: {icon_color};
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
                icon=icon
            ).add_to(self.map)


        # Set map bounds for zoom
        if self.latitudes and self.longitudes:
            southwest_point = (min(self.latitudes) - 0.01, min(self.longitudes) - 0.01)
            northeast_point = (max(self.latitudes) + 0.01, max(self.longitudes) + 0.01)
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
        for i, feature in enumerate(self.json_data["features"]):
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
            self.json_data["features"][beacon_index] = beacon_data
            print(f"Updated beacon with Radio ID: {radio_id}")
        else:
            self.json_data["features"].append(beacon_data)
            print(f"Added new beacon with Radio ID: {radio_id}")
        
        # Save changes to file
        self.save_json()


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
        
        #######################################################################################
        # IMPORTANT: Currently set to unpack data in little-endian format for testing purposes
        #            Need to change to big-endian format for final implementation
        #            Change '<' to '!' in struct.unpack() to change to big-endian format for featherboards
        #
        #            (Or from '!' to '<' for testing)
        #######################################################################################

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
    """Main GUI class that connects to base station via serial port"""

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

        self.webEngineView.setHtml(self.mapManager.load_HTML())
        self.mapManager.htmlChanged.connect(self.webEngineView.setHtml)
        self.mapManager.closeWindow.connect(self.close)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.webEngineView)
        layout.setContentsMargins(0, 0, 0, 0)

        self.resize(1280, 720)
        self.setWindowTitle("Base station GUI")
        self.show()

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    gui = BaseStationGUI()
    sys.exit(app.exec_())
