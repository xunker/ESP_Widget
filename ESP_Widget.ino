#include "ESP8266WiFi.h"
#include <Wire.h>

char buffer[20];
char* password = "";
char* ssid     = "dm2p";
String MyNetworkSSID = "dm2p"; // SSID you want to connect to Same as SSID
bool Fl_MyNetwork = false; // Used to flag specific network has been found
bool Fl_NetworkUP = false; // Used to flag network connected and operational.

extern "C" {
#include "user_interface.h"
}


void setup() {
  Serial.begin(115200);
  delay(2000); // wait for uart to settle and print Espressif blurb..
  // print out all system information
  Serial.print("Heap: "); Serial.println(system_get_free_heap_size());
  Serial.print("Boot Vers: "); Serial.println(system_get_boot_version());
  Serial.print("CPU: "); Serial.println(system_get_cpu_freq());
  Serial.println();
  Serial.println("OLED network Acquire Application Started....");
  //Wire.pins(int sda, int scl), etc
  Wire.pins(0, 2); //on ESP-01.
  Wire.begin();
  StartUp_OLED(); // Init Oled and fire up!
  Serial.println("OLED Init...");
  clear_display();
  sendStrXY(" DANBICKS WIFI ", 0, 1); // 16 Character max per line with font set
  sendStrXY("   SCANNER     ", 2, 1);
  sendStrXY("  STARTING UP  ", 4, 1);
  delay(1000);
  Serial.println("Setup done");
}


void loop()
{
  if (!Fl_NetworkUP) {
    connectToNetwork();
  } else {
    getData();
  }
  delay(5000);    // Wait a little before trying again
}

void getData() {
  clear_display();
  sendStrXY("GETTING DATA...", 3, 0);
  delay(500);

  const char* host = "dbepubs-mnielsen-development.s3.amazonaws.com";

  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    clear_display();
    sendStrXY("  CONNECTION   ", 1, 0);
    sendStrXY("    FAILED     ", 3, 0);
    sendStrXY("   RETRYING    ", 5, 0);
    enableScroll(0x00, 0x05, 0x05, 0x03);
    delay(5000);
    disableScroll();
    return;
  }

  String url = "/";
  url += "test_1.txt";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);


  sendStrXY("   CONNECTED.  ", 1, 0);
  sendStrXY(" LOADING DATA. ", 5, 0);
  delay(500);

  boolean textMarkerFound = false;

  while(client.available()){
    String line = client.readStringUntil('\n');
    Serial.print("CLIENT: "); Serial.println(line);
    Serial.println("Checking for data marker.");
    if (line.indexOf("TEXT:") > -1) {
      Serial.print("Heap: "); Serial.println(system_get_free_heap_size());
      Serial.println("Data line found.");
      textMarkerFound = true;
      clear_display();
    }

    if (textMarkerFound) {
      uint8 lineNumber = 0;
      while(client.available()){
        Serial.println("Reading data.");
        line = client.readStringUntil('\n');
        Serial.print("Line Number: "); Serial.println(lineNumber);
        Serial.print("Data line: "); Serial.println(line);
        char lineBuffer[17];
        line.toCharArray(lineBuffer, 17);
        sendStrXY(lineBuffer, lineNumber, 0);
        lineNumber++;
      }

      if (lineNumber == 0) {
        Serial.println("Response contained no data.");
        sendStrXY("    RESPONSE   ", 1, 0);
        sendStrXY("  CONTAINED NO ", 3, 0);
        sendStrXY("      DATA.    ", 5, 0);
      }
    } else {
      Serial.println("Not a data line.");
    }

  }
  client.flush();

  Serial.println();
  Serial.println("closing connection");
  client.stop();

  if (textMarkerFound) {
    uint8 sleepTimer = 60;
    Serial.println("Sleeping for 60 seconds.");
    while (sleepTimer > 0) {
      if ((sleepTimer % 10) == 0) {
        Serial.println(sleepTimer);
      } else {
        Serial.print(".");
      }
      delay(1000);
      sleepTimer--;
    }
    Serial.println("Done sleeping.");
  }
}

void connectToNetwork() {
  Serial.println("Starting Process Scanning...");
  Scan_Wifi_Networks();
  Draw_WAVES();
  delay(2000);

  if (Fl_MyNetwork)
  {
    // Yep we have our network lets try and connect to it..
    Serial.println(MyNetworkSSID + " has been Found....");
    Serial.println("Attempting Connection to Network..");
    Do_Connect();

    if (Fl_NetworkUP)
    {
      // Connection success
      Serial.println("Connected OK");
      Draw_WIFI();
      delay(4000);
      clear_display(); // Clear OLED
      IPAddress ip = WiFi.localIP(); // // Convert IP Here
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      ipStr.toCharArray(buffer, 20);
      sendStrXY("NETWORK DETAILS", 0, 1);
      sendStrXY("NET: ", 3, 1);
      sendStrXY((ssid), 3, 6);
      sendStrXY((buffer), 6, 1); // Print IP to yellow part OLED
    }
    else
    {
      // Connection failure
      Serial.println("Not Connected");
      clear_display(); // Clear OLED
      sendStrXY("CHECK   NETWORK", 0, 1);
      sendStrXY("   DETAILS     ", 2, 1);
      sendStrXY("NO CONNECTION: ", 4, 1);
      sendStrXY(ssid, 6, 1); // YELLOW LINE DISPLAY
      delay(3000);
    }
  }
  else
  {
    // Nope my network not identified in Scan
    Serial.println("Not Connected");
    clear_display(); // Clear OLED
    sendStrXY("    NETWORK    ", 0, 1);
    sendStrXY(ssid, 2, 1);
    sendStrXY("  NOT FOUND IN ", 4, 1);
    sendStrXY("     SCAN      ", 6, 1);
    delay(3000);
  }
}

void enableScroll(byte direction, byte startPage, byte endPage, byte scrollSpeed){
  if (direction == 0x01) {
     Serial.println("scrolling right");
    //Scroll Right
    sendcommand(0x26);
  } else {
    Serial.println("scrolling left");
    //Scroll Left
    sendcommand(0x27);
  }

  sendcommand(0x00); // dummy
  sendcommand(startPage); // eg line, 0x00-0x07
  sendcommand(scrollSpeed);
  sendcommand(endPage); // eg line, 0x00-0x07
  sendcommand(0X00); //dummy
  sendcommand(0XFF);
  sendcommand(0x2F); // enable scroll

}

void disableScroll(){
  Serial.println("scrolling off");
  sendcommand(0X00); //dummy
  sendcommand(0XFF);
  sendcommand(0x2E); // disable scroll
}
