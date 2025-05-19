# ESP8266-door-control
Simple ESP8266 code to open or close a door (i.e. garage door) with a relay.

# General
The ESP connects to an MQTT-Broker via MQTT over TLS and reacts on a "OPEN" or "CLOSE" command. Once the command has been received, a specified pin will be set to high. This can be used to activate a relay on which wires i.e. of your garage door are connected.
The code can be used to remote control things (i.e. doors) in your home.

# Security concerns
It'ss highly recommended not to use a public MQTT-Broker. The broker and topic should be secured with a password and an ACL. Because in theory anyone can send an "OPEN" or "CLOSE" you should not use this code to control front- or flat doors.

# Use
You have to set your WiFi SSID and password as well as an MQTT-Broker, credentials and a topic.
Because MQTT over TLS is used, you also have to set the Root certificate of your broker's SSL-certificate

# KeepAlive
In my case, for some reason the MQTT-Session timeouts after a round about half of a minute. To modify the keepAlive interval did not help. As a workaround, the client sends a "keepAlive" message to a specific topic every 20 seconds. The interval can be modified with the "mqtt_KeepAlive" variable. 
