# esp8266-rfm95-Lora
Location tracker sending macaddresses of surrounding WiFi routers via LoRa

Used on the probably smallest and cheapest Lora device right now.
A esp8266 12-E connected to an RFM95

The esp scans for WiFi Routers around and sends this informaton via Lora.
Combined with a database like Wigle.net one may track the device quite well.

Uses ABP but can be configured to use OTAA as well.

By defaut the channels are configured to use TheThingsNetwork. Therefore there are 8 channels configured, other networks typically support only the first 3. 
