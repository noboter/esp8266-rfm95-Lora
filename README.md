# esp8266-rfm95-Lora
Location tracker sending macaddresses of surrounding WiFi routers via LoRa

Used on a esp8266 12-E connected to an RFM95

The esp scans for WiFi Routers around and sends this informaton via Lora.
Combined with a database like Wigle.net one may track the device quite well.

Uses ABP but can be configured to use OTAA as well.

By defaut the channels are configured to use TheThingsNetwork. Therefore there are 8 channels configured, other networks typically support only the first 3.

Triangulation or trilateration of the location of base basestations provides the estimate. Backend work is still in progress.
Location estimation facilitating the LoRa signal works as well, with lower accuracy.
However, there are probably more WiFi stations, compared to LoRa gateways around.
