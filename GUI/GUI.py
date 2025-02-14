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

from PyQt5 import QtCore, QtWebEngineWidgets, QtWidgets
import folium

SERIAL_PORT = 'COM7'  # This should be changed to match Arduino serial port
BAUD_RATE = 9600
PACKET_SIZE = 16

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
            
        self.map = folium.Map(location=[37.227779, -80.422289], zoom_start=13)
        self.latitudes = []
        self.longitudes = []
        threading.Thread(target=self.exec, daemon=True).start()

    def exec(self):
        """Main exec loop for the worker"""
        try:
            while True:
                if self.serial_port.in_waiting >= PACKET_SIZE:          # Anytime there is a packet to read
                    data = self.serial_port.read(PACKET_SIZE)
                    try:
                        self.add_point(self.decode(data))               # Decode packet and add point to map
                        self.htmlChanged.emit(self.load_HTML())
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

    def add_point(self, point):
        """Adds a point to the map"""
        (radio_id, message_id, panic_state, latitude, longitude, 
         battery_life, utc_time) = point
        
        self.latitudes.append(latitude)
        self.longitudes.append(longitude)
        
        popup_string = (
            f"Radio ID: {radio_id}<br>"
            f"Message ID: {message_id}<br>"
            f"Panic State: {panic_state}<br>"
            f"Latitude: {latitude:.4f}<br>"
            f"Longitude: {longitude:.4f}<br>"
            f"Battery Life: {battery_life:.1f}%<br>"
            f"Time: {utc_time} UTC"
        )
        
        print("\nPacket Received:\n{}".format(popup_string.replace("<br>", "\n")))
        
        iframe = folium.IFrame(popup_string)
        popup = folium.Popup(iframe, min_width=250, max_width=250)
        folium.Marker(location=(latitude, longitude), popup=popup).add_to(self.map)
        
        southwest_point = (min(self.latitudes) - 0.01, min(self.longitudes) - 0.01)
        northeast_point = (max(self.latitudes) + 0.01, max(self.longitudes) + 0.01)
        self.map.fit_bounds((southwest_point, northeast_point))

    def decode(self, received_data: bytes):
        """Decodes the data packet from Arduino"""
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
            radio_id, message_byte, latitude, longitude, unix_time = struct.unpack(
                "!BhffxI", received_data 
            )
            # Mesh Packet: 15-bit message ID
            message_id = message_byte & 0x7FFF  # 0111 1111 1111 1111
            panic_state = bool(message_byte & 0x8000)  # Check MSB of message_byte
            
        else:
            # Legacy Packet: 16-bit radio ID
            radio_id, message_byte, latitude, longitude, unix_time = struct.unpack(
                "!HbffxI", received_data 
            )
            # Legacy Packet: 7-bit message ID
            message_id = message_byte & 0x7F  # 0111 1111
            panic_state = bool(message_byte & 0x80)  # Check MSB of message_byte
        
        battery_life = received_data[11]
        utc_time = datetime.fromtimestamp(unix_time, UTC).strftime("%m-%d-%Y %H:%M:%S")
        
        return (radio_id, message_id, panic_state, latitude, longitude,
                battery_life, utc_time)

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
