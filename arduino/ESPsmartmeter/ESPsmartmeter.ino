// https://wolles-elektronikkiste.de/wemos-d1-mini-boards
// libs:
// https://www.arduino.cc/reference/en/libraries/espsoftwareserial/


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>

#include <algorithm>  // std::min

#ifndef STASSID
#define STASSID "insert_here_your_wifi_name"
#define STAPSK  "insert_here_your_wifi_password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

SoftwareSerial myPort;

int incomingByte = 0;

int last_incomingByte = 0;
int frame_length = 0;

#define FLAG_FIELD_MASK_FOR_LENGTH 0x0FFF
#define FRAME_BUFFER_SIZE 256
uint8_t frame_buffer[FRAME_BUFFER_SIZE] = {};

// debug_prox_server forwards serial stream 1:1 to port 1337 for debug
// on port 1338 you get always one exact hdlc frame (last received) for parsing
WiFiServer debug_proxy_server(1337);
WiFiServer get_one_frame_server(1342);


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
  ArduinoOTA.setPassword("insert_here_your_ota_password");

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
  
  Serial.println("Starting tcp servers...");   
  debug_proxy_server.begin(); 
  get_one_frame_server.begin();

  Serial.println("Setup done!"); 

  // serial1 for testing 
  // no rx? D4 is TX
  Serial1.begin(115200);

  // serial2?
  // D7 RX
  // D8 TX
}

void os_tasks() {
  // wifi reconnect
  //----------------
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Wifi disconnect, try to reconnect..."));
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    delay(5000);
  }

  ArduinoOTA.handle();

  //delay(1);
}

void loop() {
  
  os_tasks();

  // foward soft serial to hardware serial (usb)
  //---------------------------------------------
  /*
  if (myPort.available() > 0) {    
    incomingByte = myPort.read();
    Serial.print((char)incomingByte);    
  }
  */

  // new tcp client conenction for debug_proxy_server (port 1337)
  // forwards endles serial stream
  // be aware blocking!  
  WiFiClient debug_proxy_client = debug_proxy_server.available();
  if (debug_proxy_client) {
    while (debug_proxy_client.connected()) {
      if (myPort.available() > 0) {    
        incomingByte = myPort.read();
        debug_proxy_client.write(incomingByte);
      }
      os_tasks();
    }
    debug_proxy_client.stop();
  }

  // read one byte and check for frame
  if (myPort.available() > 0) {
    // check for twp delimiter of hdlc frame -> is start of a frame
    last_incomingByte = incomingByte;
    incomingByte = myPort.read();    
    if (incomingByte == 0x7e && last_incomingByte == 0x7e) {
      frame_buffer[0] = incomingByte;
      // readout 2byte flag field (lower 12bit contain length)
      int bytes_read = myPort.readBytes(&(frame_buffer[1]), 2);
      if(bytes_read == 2) {
        frame_length = (frame_buffer[1]*256+frame_buffer[2]) & FLAG_FIELD_MASK_FOR_LENGTH;
           
        // readout whole frame
        // -2 cause we read already 2 bytes for length
        // +1 for delimiter at end 
        int bytes_to_read = min(frame_length -2 + 1, FRAME_BUFFER_SIZE -3);
        int bytes_already_read = 0;
        while (bytes_to_read != 0) {        
          bytes_read = myPort.readBytes(&(frame_buffer[3 + bytes_already_read]), bytes_to_read);
          bytes_to_read -= bytes_read;
          bytes_already_read += bytes_read;
          os_tasks();
        }        
      }
    }
  } 

  // new tcp client conenction for get_one_frame_server (port 1342)
  WiFiClient get_one_frame_client = get_one_frame_server.available();


  
  if (get_one_frame_client) {
    while (get_one_frame_client.connected()) {
      // send one frame
      int transmitted = 0;      
      int total_length = frame_length + 2; // +2 for delimiter at start and end
      while (transmitted != total_length) {                  
        int len = total_length - transmitted;
        len = std::min(get_one_frame_client.availableForWrite(),len);
        if (len > 0) {
          transmitted += get_one_frame_client.write(&(frame_buffer[transmitted]), len);
          //get_one_frame_client.println(transmitted);
        }
        get_one_frame_client.flush();
        os_tasks();
      }
      get_one_frame_client.stop();
    }
  }

  /*
  if (Serial1.available() > 0) {    
    incomingByte = Serial1.read();
    Serial.print((char)incomingByte);
  }  
  Serial1.print(".");
  */  
}

