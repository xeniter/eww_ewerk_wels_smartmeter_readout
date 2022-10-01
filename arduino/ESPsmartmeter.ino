// https://wolles-elektronikkiste.de/wemos-d1-mini-boards
// libs:
// https://www.arduino.cc/reference/en/libraries/espsoftwareserial/
// -> you have to patch this library! -> default buffer size is only 64bytes which is not enough!
// -> you have to increase buffer size to 256

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>

#ifndef STASSID
#define STASSID "insert_here_your_wifi_name"
#define STAPSK  "insert_here_your_wifi_password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

SoftwareSerial myPort;

int incomingByte = 0; 

WiFiServer server(1337);


void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("smartmeter");

  // No authentication by default
  ArduinoOTA.setPassword("nios1337");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Setup Software Serial..."); 
  myPort.begin(115200, SWSERIAL_8N1, 5, 4, true); // 5=D1=RX 4=D2=TX inverted=true
  
  Serial.println("Starting tcp server...");   
  server.begin(); 

  Serial.println("Setup done!"); 

  // serial1 for testing 
  // no rx? D4 is TX
  Serial1.begin(115200);

  // serial2?
  // D7 RX
  // D8 TX
}

void loop() {
  
  // wifi reconnect
  //----------------
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Wifi disconnect, try to reconnect..."));
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    delay(5000);
  }

  ArduinoOTA.handle();

  // foward soft serial to hardware serial (usb)
  //---------------------------------------------
  /*
  if (myPort.available() > 0) {    
    incomingByte = myPort.read();
    Serial.print((char)incomingByte);    
  }
  */

  // wifi client
  // be aware blocking!  
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (myPort.available() > 0) {    
        incomingByte = myPort.read();
        client.write(incomingByte);
      }
      delay(1);
    }
    client.stop();
  }

            

  /*
  if (Serial1.available() > 0) {    
    incomingByte = Serial1.read();
    Serial.print((char)incomingByte);
  }  
  Serial1.print(".");
  */  
}

