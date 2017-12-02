# cloudlamp
A weather connected cloudlamp running on ESP-12 (NodeMCU Board)

This code connects to wifi, and then connects to a public MQTT broker (iot.eclipse.org). Every 10 seconds, it publishes a state request to a topic on that broker, which is received by my home automation server (Node-RED). The server looks at the weather in my area, and responds with a single integer character, corresponding to the weather. This code receives that, and changes the "state" of the cloud accordingly. 

It currently has four states:
"Happy Cloud" - ID 1 - A sky colored cloud
"Thunder Cloud" - ID 2 - Off with random lightning
"Sunny Cloud" - ID 3 - Happy cloud but a single pixel glows yellow for the sun
"Red Cloud" - ID 9 - Glows evil red for no reason

If the character that it gets is not one of those four ID characters, it turns off.

It also hosts an OTA page at its IP address. Go there, and upload the .bin file for your code to update it over OTA.

