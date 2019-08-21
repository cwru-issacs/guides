# ESP32 Simple Web Server

This project uses the [ESP32 WebServer Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer) to serve a simple webpage that keeps track of the number of times the page has been loaded.

The project demonstrates how to connect to a Wifi network, register a mDNS host name, and server a simple HTML webpage.

### Libraries Used:

* [ESP32 WebServer Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)
* [ESPmDNS](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESPmDNS)
* [Wifi](https://www.arduino.cc/en/Reference/WiFi)

### Hardware Needed:

* ESP32 Board

### Software Needed:

* Arduino IDE
* Any web browser

## Running the Demo

1. Connect the ESP32 board through USB to the computer.
2. Load the sketch in Arduino IDE.
3. Modify the network parameters near the top of the sketch for your network.
4. Set the correct ESP32 Board and Port settings in the Tool menu. 
5. Compile and load the image on to the ESP32 board.
6. The ESP32 will flash the on-board LED until it makes a network connection.  If the LED remains flashing, double check your network ssid and password are correct.
7. Connect the serial monitor to find the ip address assigned to the device.  A successful connection will look similar to the following:

	```
	............	Connected to CaseGuest	IP address: 172.33.1.217	MDNS responder started with Hostname: esp32.local	HTTP server started	```
8. Open a web browser on your computer and enter the ip address in the url field.  The web browser should open the web page served from the ESP32.


## Debugging

* When using the ***CaseGuest*** wifi, if the ESP32 connects to the network but the assigned ip can not be opened from your laptop, double check that your laptop is also on the ***CaseGuest*** wifi, not the ***CaseWireless*** wifi.  These networks are not on the same subnet so ip packets can not be routed between the two networks.
* If the image does not appear on the webpage, your computer may not be connected to the internet.  The image is retrieved from GitHub, not hosted by the ESP32 (due to limited flash memory).
