  /*******************************************************************************
    Settings for Arduino IDE when using an esp12e
   Board:      Generic esp8266 module
   Flash mode: QIO
   Flash freq: 40Mhz
   CPU frequn: 80MHz
   Flash size: 4M (1M SPIFFS)
   Debug port: disabled
   Debug level none
   Reset Meth: ck
   upload Spe: 115200
    ???
 *******************************************************************************/

#include "ESP8266WiFi.h"

#define WAIT_SECS 30

#if defined(__AVR__)
#include <avr/pgmspace.h>
#include <arduino.h>
#elif defined(ARDUINO_ARCH_ESP8266)
#include <Esp.h>
#elif defined(__MKL26Z64__)
#include <arduino.h>
#else
#error Unknown architecture in aes.cpp
#endif

#include <lmic.h>
//#include "lmic.h"
#include "hal/hal.h"
#include <SPI.h>

//---------------------------------------------------------
// Sensor declarations
//---------------------------------------------------------

// LoRaWAN NwkSKey, network session key
unsigned char NwkSkey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// LoRaWAN AppSKey, application session key
unsigned char AppSkey[16] =	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define msbf4_read(p)   (u4_t)((u4_t)(p)[0]<<24 | (u4_t)(p)[1]<<16 | (p)[2]<<8 | (p)[3])
unsigned char DevAddr[4] = { 0x00, 0x00, 0x00, 0x00 };

String message = "-";
// ----------------------------------------------------------------------------
// APPLICATION CALLBACKS
// ----------------------------------------------------------------------------
// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
//  memcpy(buf, APPEUI, 8);
}
// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
//  memcpy(buf, DEVEUI, 8);
}
// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
  memcpy(buf, NwkSkey, 16);
}

uint8_t mydata[64];
int c;
static osjob_t sendjob;

// Pin mapping
// These settings should be set to the GPIO pins
//
const lmic_pinmap lmic_pins = {
  .nss = 15,			          // Connected to pin D8,GPIO15
  .rxtx = LMIC_UNUSED_PIN,  // For placeholder only, Do not connected on RFM92/RFM95
  .rst = LMIC_UNUSED_PIN,  	// Needed on RFM92/RFM95? (probably not) D0/GPIO16
  .dio = {5, 4, LMIC_UNUSED_PIN},		// Specify pin numbers for DIO0, 1, 2
  // MISO D6/GPIO12, MOSI D7/GPIO13, SCK D5/GPIO14
};

void do_send(osjob_t* j) {
  delay(1);                          // XXX delay is added for Serial
  Serial.print("Time: ");  Serial.println(millis() / 1000);
  // Show TX channel (channel numbers are local to LMIC)
  Serial.print("Send, txCnhl: ");  Serial.println(LMIC.txChnl);
  Serial.println("Opmode check: ");
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println("OP_TXRXPEND, not sending");
  } else {
    Serial.print("ready to send "); Serial.print(c);Serial.println(" byte");
    LMIC_setTxData2(1, mydata, c, 0);
  }
}


void scan_Aps() {
  Serial.println("scan start");
  //WiFi.mode(WIFI_STA);
  //delay(100);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  boolean add = true;
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else {
    message = "";
    Serial.print(n);
    c = 0; // global variable to know the length of mydata to send...
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found

      //debug - send 5 addresses 
      if (i > 4)
        add = false;

      if (add && i>0)
        message += ",";

      Serial.print(i + 1);
      Serial.print(": ");

      char mac[13] = {0};
      uint8_t *bssid = WiFi.BSSID(i);
      sprintf( mac, "%02X%02X%02X%02X%02X%02X", bssid[0],  bssid[1],  bssid[2], bssid[3], bssid[4], bssid[5] );
      if (add){
        message += String( mac );
        mydata[c]=bssid[0]; c++; mydata[c]=bssid[1]; c++; mydata[c]=bssid[2]; c++;
        mydata[c]=bssid[3]; c++; mydata[c]=bssid[4]; c++; mydata[c]=bssid[5]; c++;
      }
      Serial.print(WiFi.BSSIDstr(i)); // String MAC / BSSID of scanned wifi
      Serial.print(" - ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      if (add){
        message += abs(WiFi.RSSI(i));
        mydata[c] = abs(WiFi.RSSI(i)); c++;
      }
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
}

// read serial input...
void read_Serial() {
  //read serial as a character
  message = "";
  while (Serial.available()) {
    message += (char)Serial.read();
  }
  Serial.println(message);
  scan_Aps();
  do_send(&sendjob);
}

// to be called every WAIT_SECS
void timerevnt(osjob_t* j){
  scan_Aps();
  do_send(&sendjob);
  // Schedule next transmission
}

void onEvent (ev_t ev) {
  Serial.print(millis() / 1000);
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE"));
      if (LMIC.dataLen) {
        // data received in rx slot after tx
        Serial.print(F("Data Received: "));
        Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        Serial.println();
      }
      //this sendjob is not j!! does that work anyway?
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(WAIT_SECS), timerevnt);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

// ====================================================================
// SETUP
//
void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Wifi Setup done");

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  LMIC_setSession (0x1, msbf4_read(DevAddr), (uint8_t*)NwkSkey, (uint8_t*)AppSkey);

  // data rate adaptation
  LMIC_setAdrMode(0);
  // link check validation
  LMIC_setLinkCheckMode(0);

  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band - 10%
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band 1%
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band 0.1%

  //if you only want one channel, deactivate the others:
  //LMIC_disableChannel(0);
  //LMIC_disableChannel(1);
  //LMIC_disableChannel(2);
  //LMIC_disableChannel(3);
  //LMIC_disableChannel(4);
  //LMIC_disableChannel(5);
  //LMIC_disableChannel(7);
  //LMIC_disableChannel(8);
  
  // Disable beacon tracking
  LMIC_disableTracking ();
  // Stop listening for downstream data (periodical reception)
  // LMIC_stopPingable();
  // Set data rate and transmit power (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF11, 14);

// from raw.ino //
  // Maximum TX power
//  LMIC.txpow = 27;
  // Use a medium spread factor. This can be increased up to SF12
//  LMIC.datarate = DR_SF9;
  // This sets CR 4/5, BW125 (except for DR_SF7B, which uses BW250)
//  LMIC.rps = updr2rps(LMIC.datarate);
  
  //Serial.flush();
  Serial.print(F("A: ")); Serial.println(msbf4_read(DevAddr), HEX);

  timerevnt(&sendjob);
}

// ================================================================
// LOOP
//
void loop() {

  while (1) {
    if (Serial.available()) {
      read_Serial();
    }
    os_runloop_once();
    //yield();
    delay(100);
  }
}

