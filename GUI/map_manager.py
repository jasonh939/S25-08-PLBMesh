
from PyQt5 import QtCore
from datetime import UTC, datetime
import time
import io
import struct
import sys
import threading
import serial
import os
import json
import folium 

BAUD_RATE = 9600
PACKET_SIZE = 16

MAX_RADIO_ID = 16
lngMin, lngMax = -180., 180.
latMin, latMax = -90., 90.

class PacketLengthError(Exception):
    """ Creates a new error to throw if the packet is not the right length """
    pass

class MapManager(QtCore.QObject):
    """ Manages the map and the data points on the map """

    htmlChanged = QtCore.pyqtSignal(str)
    closeWindow = QtCore.pyqtSignal()

    def __init__(self, SERIAL_PORT):
        super().__init__()
        try:
            self.serial_port = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        except serial.SerialException as err:
            print(f"Error opening serial port: {err}", file=sys.stderr)
            raise
              
        self.map = folium.Map(location=[37.227779, -80.422289], zoom_start=18)
        self.live_file = "live_beacons.json"
        self.history_file = "history_beacons.json"
        self.live_data = self.load_json(self.live_file)
        self.history_data = self.load_json(self.history_file)
        self.latitudes = []
        self.longitudes = []
        self.paused = False # Flag to pause updates
        self.show_history = False # Flag to toggle GUI display
        self.id_filter = None 
        self.time_filter = None
        self.time_filter_state = True # Flag for time filter to check before or after time

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
        """ Save any changes to JSON file """
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
        """ Main exec loop for the worker to read data from the serial port """

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
        """ Loads the html from the map """

        data = io.BytesIO()
        self.map.save(data, close_file=False)
        return data.getvalue().decode()
    
    def update_map(self):
        """ Update Folium map with the relevant JSON data """

        # Create a new map
        self.map = folium.Map(location=[37.227779, -80.422289], zoom_start=18)

        # reset coordinate list
        self.latitudes = []
        self.longitudes = []

        # Data to display based on GUI state
        display_data = self.history_data if self.show_history else self.live_data

        # For loop to add each beacon data point to the map
        for feature in display_data["features"]:
            properties = feature["properties"]
            radio_id = properties["Radio ID"]

            # Check state of filters to modify data displayed on map as needed
            # If ID filter is set, only display beacons of that ID
            if self.id_filter is not None and radio_id != self.id_filter:
                continue
            # If Date/Time filter is set, only display beacons after that time
            if self.time_filter is not None:
                beacon_time = datetime.strptime(properties["Time"], "%m-%d-%Y %H:%M:%S")
                filter_time = datetime.strptime(self.time_filter, "%m-%d-%Y %H:%M:%S")
                if self.time_filter_state:
                    if beacon_time < filter_time:
                        continue
                else:
                    if beacon_time > filter_time:
                        continue

            coordinates = feature["geometry"]["coordinates"]

            # Extract remaning data line
            message_id = properties["Message ID"]
            panic_state = properties["Panic State"]
            latitude = properties["Latitude"]
            longitude = properties["Longitude"]
            battery_life = properties["Battery Life"]
            utc_time = properties["Time"]

            # Add coordinates to member variables
            self.latitudes.append(latitude)
            self.longitudes.append(longitude)

            # Create the tooltip's HTML for hover event
            tooltip_html = f"""
            <div style="font-family: Arial; font-size: 20px; padding: 5px; width: 300px;">
                <div>Radio ID: {radio_id}</div>
                <div>Message ID: {message_id}</div>
                <div>Panic State: {'YES' if panic_state else 'NO'}</div>
                <div>Latitude: {latitude:.5f}</div>
                <div>Longitude: {longitude:.5f}</div>
            </div>
            """
            
            # Create a tooltip that shows on hover
            tooltip = folium.Tooltip(tooltip_html)

            # Create popup's html for click event
            popup_string = f"""
            <div style="font-family: Arial; font-size: 26px; padding: 5px; width: 375px;">
                <div>Radio ID: {radio_id}</div>
                <div>Message ID: {message_id}</div>
                <div>Panic State: {'YES' if panic_state else 'NO'}</div>
                <div>Latitude: {latitude:.5f}</div>
                <div>Longitude: {longitude:.5f}</div>
                <div>Battery: {battery_life:.1f}%</div>
                <div>Time: {utc_time} UTC</div>
            """
    
            # Create a popup to contain the HTML string
            iframe = folium.IFrame(html=popup_string)
            popup = folium.Popup(iframe, min_width=450, max_width=400)

            # set icon color of marker based on panic mode state
            icon_color = 'red' if panic_state else 'blue'

            # In history view, use smaller markers with timestamp-based opacity
            if self.show_history:
                # Create lines connecting points from the same radio ID
                self.add_history_lines(radio_id)

                # Get the opacity from the feature properties
                opacity = properties.get("opacity", 1.0)
                
                icon = folium.DivIcon(
                    icon_size=(150, 36),
                    icon_anchor=(21, 20),
                    popup_anchor=(2, -15),
                    html=f'''
                        <div style="
                            width: 45px;
                            height: 45px;
                            border-radius: 50%;
                            background-color: {icon_color};
                            opacity: {opacity};
                            display: flex;
                            justify-content: center;
                            align-items: center;
                            color: white;
                            font-size: 20px;
                            font-weight: bold;
                            border: 1px solid white;
                        ">
                            {radio_id}
                        </div>
                    '''
                )
            else:

                icon = folium.DivIcon(
                    icon_size=(150, 36),
                    icon_anchor=(14, 40),
                    popup_anchor=(14, -35),
                    html=f'''
                        <div style="
                            width: 60px;
                            height: 60px;
                            border-radius: 50%;
                            background-color: {icon_color};
                            display: flex;
                            justify-content: center;
                            align-items: center;
                            color: white;
                            font-size: 26px;
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

    def add_or_update_beacon(self, packet):
        """ Add a new beacon or update an existing to their respective JSON files based on radio_id """

        (radio_id, message_id, panic_state, latitude, longitude, 
         battery_life, utc_time) = packet
        
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

        # Add beacon to history data no matter whats
        self.history_data["features"].append(beacon_data)
        self.save_json(self.history_file, self.history_data)
        
        # Update existing beacon point or add a new point to live data 
        if beacon_index >= 0:
            self.live_data["features"][beacon_index] = beacon_data
            print(f"Updated beacon with Radio ID: {radio_id}")
        else:
            self.live_data["features"].append(beacon_data)
            print(f"Added new beacon with Radio ID: {radio_id}")
        
        # Save changes to file
        self.save_json(self.live_file, self.live_data)

    def add_history_lines(self, radio_id):
        """ Add lines connecting markers for a specific radio ID together in history view """

        beacon_points = []
        for feature in self.history_data["features"]:
            if feature["properties"]["Radio ID"] == radio_id:

                # Check if Time filter is set to ignore points outside of time range
                if self.time_filter is not None:
                    beacon_time = datetime.strptime(feature["properties"]["Time"], "%m-%d-%Y %H:%M:%S")
                    filter_time = datetime.strptime(self.time_filter, "%m-%d-%Y %H:%M:%S")
                    # If state is set, filter beacons before filter time
                    if self.time_filter_state:
                        if beacon_time < filter_time:
                            continue
                    # If state is not set, filter beacons after filter time
                    else:
                        if beacon_time > filter_time:
                            continue

                coords = feature["geometry"]["coordinates"]
                timestamp = feature["properties"]["Time"]
                beacon_points.append((coords[1], coords[0], timestamp, feature))  # lat, lon format for polyline
        
        # Sort points by timestamp
        beacon_points.sort(key=lambda x: datetime.strptime(x[2], "%m-%d-%Y %H:%M:%S"))
        
        # Only draw lines if we have 2+ points
        if len(beacon_points) >= 2:
            locations = [(p[0], p[1]) for p in beacon_points]
            folium.PolyLine(
                locations=locations,
                weight=2,
                color=f'blue',
                opacity=0.6,
                dash_array='5,5'
            ).add_to(self.map)

        # Adjust feature properties to include the opacity based on timestamp
        if beacon_points:
            # Make the most recent point fully opaque
            most_recent = beacon_points[-1][3]
            most_recent["properties"]["opacity"] = 1.0

            # Remaining points are at half opacity
            for point in beacon_points[:-1]:
                point[3]["properties"]["opacity"] = 0.5

    def set_id_filter(self, radio_id):
        """ Set ID of radio to filter on map """
        self.id_filter = radio_id
        return self.update_map()
    
    def set_time_filter(self, date_time, time_state):
        """ Set date/time to filter beacons on map by """
        self.time_filter = date_time
        self.time_filter_state = time_state
        return self.update_map()


    def check_point(self, packet):
        """Checks packet data against internal database to see if it is a duplicate"""
        (radio_id, message_id, panic_state, latitude, longitude, 
         battery_life, utc_time) = packet
        
        for feature in self.history_data["features"]:
            properties = feature["properties"]
            
            # Check if this feature has the same radio_id
            if properties.get("Radio ID") == radio_id:                    
                # Check if coordinates and other relevant data match
                coords = feature["geometry"]["coordinates"]
                if (properties.get("Message ID") == message_id and
                    coords[0] == longitude and 
                    coords[1] == latitude and 
                    properties.get("Panic") == panic_state and
                    properties.get("Battery") == battery_life and
                    properties.get("UTCTime") == utc_time):
                    print(f"Duplicate position data detected for Radio ID: {radio_id}")
                    return False
    
        # no duplicate was found
        print(f"No duplicate found for Radio ID: {radio_id}")
        return True


    def decode(self, received_data: bytes):
        """ Decodes the data packet from Arduino """
        binary_representation = ' '.join(format(byte, '08b') for byte in received_data)
        print(f"Received data: {received_data}")
        print(f"Binary Representation: {binary_representation}")
        
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