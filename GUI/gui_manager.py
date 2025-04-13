"""
This file contains the code for running the GUI. The GUI receives packets from
Arduino Featherboards, decodes the packet data, and plots the data on an 
interactive map running in Qt.
"""

from map_manager import MapManager
from datetime import UTC
import sys
import serial

from PyQt5 import QtCore, QtWebEngineWidgets, QtWidgets
from PyQt5.QtSerialPort import QSerialPortInfo

class SerialPortSelector(QtWidgets.QDialog):
    """ Dialog for selecting serial port before launching main application """
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.port = None
                
        # Create button box
        self.buttonBox = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
        )
        self.initUI()
        
    def initUI(self):
        self.setWindowTitle(" Serial Port ")
        layout = QtWidgets.QVBoxLayout()
        
        # Create label
        label = QtWidgets.QLabel("Select the serial port for the map to read from:")
        layout.addWidget(label)
        
        # Create drop down menu for available serial ports
        self.portComboBox = QtWidgets.QComboBox()
        self.refreshPorts()
        
        # Create refresh button
        refreshButton = QtWidgets.QPushButton("Refresh Ports")
        refreshButton.clicked.connect(self.refreshPorts)
        
        # Layout for serial selection
        portLayout = QtWidgets.QHBoxLayout()
        portLayout.addWidget(self.portComboBox)
        portLayout.addWidget(refreshButton)
        layout.addLayout(portLayout)

        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        layout.addWidget(self.buttonBox)
        
        self.setLayout(layout)
        
    def refreshPorts(self):
        """ Refresh the list of available serial ports """

        self.portComboBox.clear()
        
        available_ports = QSerialPortInfo.availablePorts()
        
        # Add ports to combobox
        for port in available_ports:
            port_info = f"{port.portName()} - {port.description()}"
            self.portComboBox.addItem(port_info, port.portName())
            
        # If no ports are available, disable OK button
        if self.portComboBox.count() == 0:
            self.portComboBox.addItem("No serial ports available")
            self.buttonBox.button(QtWidgets.QDialogButtonBox.Ok).setEnabled(False)
        else:
            self.buttonBox.button(QtWidgets.QDialogButtonBox.Ok).setEnabled(True)
        
    def accept(self):
        """ Get the selected port when OK is clicked """

        if self.portComboBox.count() > 0 and self.portComboBox.currentData() is not None:
            self.port = self.portComboBox.currentData()
            super().accept()
            
    def getSelectedPort(self):
        """ Return the selected port """
        return self.port

class BaseStationGUI(QtWidgets.QWidget):
    """Main QT GUI class that connects to base station via serial port"""

    def __init__(self, serial_port):
        super().__init__()
        self.serial_port = serial_port
        self.initUI()

    def initUI(self):
        self.webEngineView = QtWebEngineWidgets.QWebEngineView()

        try:
            self.mapManager = MapManager(self.serial_port)
        except serial.SerialException:
            QtWidgets.QMessageBox.critical(
                self, 
                "Connection Error",
                f"Could not connect to Arduino on {self.serial_port}. Please check the connection."
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
        self.pauseButton = QtWidgets.QPushButton("Pause Updates")
        self.pauseButton.clicked.connect(self.pauseMap)
        
        self.resumeButton = QtWidgets.QPushButton("Resume Updates")
        self.resumeButton.clicked.connect(self.resumeMap)
        self.resumeButton.setEnabled(False)

        self.forceUpdateButton = QtWidgets.QPushButton("Force Update")
        self.forceUpdateButton.clicked.connect(self.forceMapUpdate)
        self.forceUpdateButton.setEnabled(False)

        # History/Live toggle buttons
        self.viewLiveButton = QtWidgets.QPushButton("View Live Beacons")
        self.viewLiveButton.clicked.connect(self.showLiveView)
        self.viewLiveButton.setEnabled(False)

        self.viewHistoryButton = QtWidgets.QPushButton("View Beacon History")
        self.viewHistoryButton.clicked.connect(self.showHistoryView)

        # Labels
        self.statusLabel = QtWidgets.QLabel("Status: updates ACTIVE")
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

        # Filtering options layout 
        filterLayout = QtWidgets.QHBoxLayout(self)
                    # Filtering by Radio ID
        self.idFilterBox = QtWidgets.QCheckBox(" Radio ID Filter: ")
        self.idFilterBox.stateChanged.connect(self.updateIDFilter)
        self.idEdit = QtWidgets.QLineEdit()
        self.idEdit.setPlaceholderText("Enter Radio ID")
        self.idEdit.textChanged.connect(self.updateIDFilter)
        self.idEdit.setInputMask("0000")
                    # Filtering by Date/Time
        self.timeFilterBox = QtWidgets.QCheckBox(" Date/Time Filter: ")
        self.timeFilterBox.stateChanged.connect(self.updateTimeFilter)
        self.dateTimeEdit = QtWidgets.QDateTimeEdit()
        self.dateTimeEdit.dateTimeChanged.connect(self.updateTimeFilter)
        self.dateTimeEdit.setDisplayFormat("MM-dd-yyyy HH:mm:ss")
        self.dateTimeEdit.setDateTime(QtCore.QDateTime.currentDateTime())
        self.beforeButton = QtWidgets.QPushButton(" Before ")
        self.beforeButton.clicked.connect(self.updateTimeState)
        self.afterButton = QtWidgets.QPushButton(" After ")
        self.afterButton.setEnabled(False)
        self.afterButton.clicked.connect(self.updateTimeState)
                    # Add Filter widgets to layout
        filterLayout.insertSpacerItem(0, QtWidgets.QSpacerItem(200, 0, QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Minimum))
        filterLayout.addWidget(self.idFilterBox)
        filterLayout.addWidget(self.idEdit)
        filterLayout.insertSpacerItem(3, QtWidgets.QSpacerItem(100, 0, QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Minimum))
        filterLayout.addWidget(self.timeFilterBox)
        filterLayout.addWidget(self.beforeButton)
        filterLayout.addWidget(self.afterButton)
        filterLayout.addWidget(self.dateTimeEdit)
        filterLayout.addStretch(2)

        # Put layout in a container widget to add to main layout
        filterPanel = QtWidgets.QWidget()
        filterPanel.setLayout(filterLayout)


        # Main layout
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.webEngineView)
        layout.addWidget(controlPanel)
        layout.addWidget(filterPanel)
        layout.addWidget(self.clearBeaconsButton)
        layout.setStretchFactor(self.webEngineView, 15)

        self.resize(1280, 1024)
        self.setWindowTitle(f"Base station GUI - {self.serial_port}")
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
        """Clear all beacons from the map"""
        box = QtWidgets.QMessageBox()
        box.setIcon(QtWidgets.QMessageBox.Warning)
        box.setWindowTitle("WARNING")
        box.setText("Are you sure you want to clear all beacons? This will erase all current beacon data.")
        box.setStandardButtons(QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No)

        if box.exec() == QtWidgets.QMessageBox.Yes:
            self.mapManager.live_data["features"] = []
            self.mapManager.history_data["features"] = []
            self.mapManager.save_json(self.mapManager.live_file, self.mapManager.live_data)
            self.mapManager.save_json(self.mapManager.history_file, self.mapManager.history_data)
            self.forceMapUpdate()

    def updateIDFilter(self):
        """ Check state of ID filter to update map """
        # Check to ensure map doesn't update if ID filter is not checked
        filter_check = self.mapManager.id_filter

        if self.idFilterBox.isChecked() and self.idEdit.text():
            try:
                id = int(self.idEdit.text())
                print(f"Filtering map by Radio ID: {id}")
                self.mapManager.set_id_filter(id)
            except ValueError:
                pass
        else:
            self.mapManager.set_id_filter(None)

        if self.idFilterBox.isChecked() or filter_check is not None:
            self.forceMapUpdate()

    def updateTimeFilter(self):
        """ Check state of Date/Time filter to update map """

        # Check to ensure map doesn't update if Date/Time filter is not checked
        filter_check = self.mapManager.time_filter

        if self.timeFilterBox.isChecked():
            date_time = self.dateTimeEdit.dateTime().toString("MM-dd-yyyy HH:mm:ss")

            if self.beforeButton.isEnabled():
                self.mapManager.set_time_filter(date_time, time_state=True)
                print("After time state")
            else:
                self.mapManager.set_time_filter(date_time, time_state=False)
                print("Before time state")
        else:
            self.mapManager.set_time_filter(None, time_state=None)

        if self.timeFilterBox.isChecked() or filter_check is not None:
            self.forceMapUpdate()

    def updateTimeState(self):
        """ Update the state of the date/time filter """
        if self.beforeButton.isEnabled():
            self.beforeButton.setEnabled(False)
            self.afterButton.setEnabled(True)
        else:
            self.beforeButton.setEnabled(True)
            self.afterButton.setEnabled(False)
        self.updateTimeFilter()


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)

    port = SerialPortSelector()
    if port.exec() == QtWidgets.QDialog.Accepted and port.port:
        gui = BaseStationGUI(port.port)
        sys.exit(app.exec())
    else:
        sys.exit(0)