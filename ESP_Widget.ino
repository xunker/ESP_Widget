#include "ESP8266WiFi.h"
#include <Wire.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

struct eepromCharArray
{
  boolean isSet;
  char value[64];
} widgetNameStruct, widgetIdStruct, ssidStruct, passwordStruct, hostnameStruct, pathStruct;

char buffer[20];
char* password = "some-password";
char* ssid     = "some-network-ssid";
String MyNetworkSSID = "some-network-ssid"; // SSID you want to connect to Same as SSID
bool Fl_MyNetwork = false; // Used to flag specific network has been found
bool Fl_NetworkUP = false; // Used to flag network connected and operational.

char* host_name = "some.host";
char* path = "/some/path.txt";

char widget_name[16] = "New Widget"; // default name of widget

#define EEPROM_SIZE 768 // Max is 4096.
#define EEPROM_WIDGET_NAME_POSITION 0
#define EEPROM_WIDGET_ID_POSITION 128
#define EEPROM_SSID_POSITION 256
#define EEPROM_PASSWORD_POSITION 384
#define EEPROM_HOST_POSITION 512
#define EEPROM_PATH_POSITION 640

boolean configMode = false;

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
  Serial.println("EEPROM begin.");
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

  Serial.println("Press any key in 5 seconds to enter configuration mode.");

  int counter = 5;
  while (counter > 0) {
    delay(1000);
    Serial.println(counter);
    if (Serial.available()) {
      configMode = true;
      counter = 0;
    } else {
      counter--;
    }
  }

  if (!configMode) {
    loadWidgetName();
    loadSsid();
    loadPassword();
    loadHostName();
    loadPath();
  }

  Serial.println("Setup done");
}

void loop() {
  if (configMode) {
    enterConfigMode();
  } else {
    if (!Fl_NetworkUP) {
      connectToNetwork();
    } else {
      getData();
    }
    delay(5000);    // Wait a little before trying again
  }
}

void getData() {
  clear_display();
  sendStrXY("GETTING DATA...", 3, 0);
  delay(500);

  // const char* host = "dbepubs-mnielsen-development.s3.amazonaws.com";

  Serial.print("connecting to ");
  Serial.println(host_name);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host_name, httpPort)) {
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

  // String url = "/";
  // url += "test_1.txt";

  Serial.print("Requesting URL: ");
  Serial.println(path);

  // This will send the request to the server
  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host_name + "\r\n" +
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

void enterConfigMode(){
  sendStringSerialXY("Config Mode", 7, 0);

  String inputString;
  String screenBuffer[7];

  Serial.print("cmd> ");
  while (configMode) {
    while (Serial.available()) {
      char inChar = (char)Serial.read();
      if (inChar == '\n') {
        clear_display();

        for(int i=0;i<6;i++) {
          // 7 is one less than number of lines on screen
          screenBuffer[i] = screenBuffer[i+1];
          sendStringXY(screenBuffer[i], i, 0);
        }

        screenBuffer[6] = processConfigCommand(inputString);
        sendStringSerialXY(screenBuffer[6], 6, 0);
        inputString = "";
        Serial.print("cmd> ");
      } else if (inChar == '\r') {
        // ignore!
      } else {
        inputString += inChar;
        sendStringXY(inputString, 7, 0);
      }
    }
  }
}

String processConfigCommand(String commandString) {
  String command = commandString.substring(0, commandString.indexOf(" "));
  if (command == "exit") {
    return "Reboot instead.";
  } else if (command == "name") {
    if (commandString.indexOf(" ") > -1) {
      String new_name = commandString.substring(commandString.indexOf(" ")+1);
      Serial.println("New name: \"" + String(new_name) + "\"");

      char new_name_chars[64];
      new_name.toCharArray(new_name_chars, 63);

      Serial.println("New name chars:");
      Serial.println(String(new_name_chars));
      strcpy( widgetNameStruct.value, new_name_chars );
      widgetNameStruct.isSet = true;
      Serial.println(widgetNameStruct.value);
      Serial.print("Writing to  EEPROM... ");
      EEPROM.begin(EEPROM_SIZE);
      EEPROM_writeAnything( EEPROM_WIDGET_NAME_POSITION, widgetNameStruct );
      EEPROM.end();
      Serial.println("Done.");
      strcpy(widget_name, widgetNameStruct.value);
    } else {
      loadWidgetName();
    }
    return "\"" + String(widget_name) + "\"";
  } else if (command == "ssid") {
    if (commandString.indexOf(" ") > -1) {
      String new_ssid = commandString.substring(commandString.indexOf(" ")+1);
      Serial.println("New ssid: \"" + String(new_ssid) + "\"");

      char new_ssid_chars[64];
      new_ssid.toCharArray(new_ssid_chars, 63);

      Serial.println("New ssid chars:");
      Serial.println(String(new_ssid_chars));
      strcpy( ssidStruct.value, new_ssid_chars );
      ssidStruct.isSet = true;
      Serial.println(ssidStruct.value);
      Serial.print("Writing to  EEPROM... ");
      EEPROM.begin(EEPROM_SIZE);
      EEPROM_writeAnything( EEPROM_SSID_POSITION, ssidStruct );
      EEPROM.end();
      Serial.println("Done.");
      strcpy(ssid, ssidStruct.value);
    } else {
      loadSsid();
    }
    return "\"" + String(ssid) + "\"";
  } else if (command == "password") {
    if (commandString.indexOf(" ") > -1) {
      String new_password = commandString.substring(commandString.indexOf(" ")+1);
      Serial.println("New password: \"" + String(new_password) + "\"");

      char new_password_chars[64];
      new_password.toCharArray(new_password_chars, 63);

      Serial.println("New password chars:");
      Serial.println(String(new_password_chars));
      strcpy( passwordStruct.value, new_password_chars );
      passwordStruct.isSet = true;
      Serial.println(passwordStruct.value);
      Serial.print("Writing to  EEPROM... ");
      EEPROM.begin(EEPROM_SIZE);
      EEPROM_writeAnything( EEPROM_PASSWORD_POSITION, passwordStruct );
      EEPROM.end();
      Serial.println("Done.");
      strcpy(password, passwordStruct.value);
    } else {
      loadPassword();
    }
    return "\"" + String(password) + "\"";
  } else if (command == "hostname") {
    if (commandString.indexOf(" ") > -1) {
      String new_host_name = commandString.substring(commandString.indexOf(" ")+1);
      Serial.println("New hostname: \"" + String(new_host_name) + "\"");

      char new_host_name_chars[64];
      new_host_name.toCharArray(new_host_name_chars, 63);

      Serial.println("New hostname chars:");
      Serial.println(String(new_host_name_chars));
      strcpy( hostnameStruct.value, new_host_name_chars );
      hostnameStruct.isSet = true;
      Serial.println(hostnameStruct.value);
      Serial.print("Writing to  EEPROM... ");
      EEPROM.begin(EEPROM_SIZE);
      EEPROM_writeAnything( EEPROM_HOST_POSITION, hostnameStruct );
      EEPROM.end();
      Serial.println("Done.");
      strcpy(host_name, hostnameStruct.value);
    } else {
      loadHostName();
    }
    return "\"" + String(host_name) + "\"";
  } else if (command == "path") {
    if (commandString.indexOf(" ") > -1) {
      String new_path = commandString.substring(commandString.indexOf(" ")+1);
      Serial.println("New path: \"" + String(new_path) + "\"");

      char new_path_chars[64];
      new_path.toCharArray(new_path_chars, 63);

      Serial.println("New path chars:");
      Serial.println(String(new_path_chars));
      strcpy( pathStruct.value, new_path_chars );
      pathStruct.isSet = true;
      Serial.println(pathStruct.value);
      Serial.print("Writing to  EEPROM... ");
      EEPROM.begin(EEPROM_SIZE);
      EEPROM_writeAnything( EEPROM_PATH_POSITION, pathStruct );
      EEPROM.end();
      Serial.println("Done.");
      strcpy(path, pathStruct.value);
    } else {
      loadPath();
    }
    return "\"" + String(path) + "\"";
  } else if (command == "heap") {
    return "Heap: " + String(system_get_free_heap_size());
  } else if (command == "boot") {
    return "Boot Version: " + String(system_get_boot_version());
  } else if (command == "cpu") {
    return "CPU Freq: " + String(system_get_cpu_freq()) + "MHz";
  } else if (command == "reset!") {
    EEPROM.begin(EEPROM_SIZE);
    Serial.print("Resetting EEPROM");
    for( int i = 0; i <= EEPROM_SIZE; i++ ) {
      Serial.print(".");
      EEPROM.write(i, false);
    }
    EEPROM.end();
    Serial.println("Done.");
    return "EEPROM reset.";
  } else {
    return "\"" + command + "\" unknown.";
  }
}

void sendStringXY(String line, int X, int Y) {
  char lineBuffer[17];
  line.toCharArray(lineBuffer, 17);
  sendStrXY(lineBuffer, X, Y);
}

void sendStringSerialXY(String line, int X, int Y) {
  Serial.println(line);
  sendStringXY(line, X, Y);
}

void loadWidgetName() {
  Serial.println("Loading widget name from EEPROM.");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM_readAnything( EEPROM_WIDGET_NAME_POSITION, widgetNameStruct );
  Serial.print("widgetNameStruct.isSet: "); Serial.println(widgetNameStruct.isSet);

  if (widgetNameStruct.isSet) {
    Serial.println("Setting Widget Name to: \"" + String(widgetNameStruct.value) + "\".");
    strcpy(widget_name, widgetNameStruct.value);
  } else {
    Serial.println("Widget Name not set. Using default.");
  }
  EEPROM.end();
}

void loadSsid() {
  Serial.println("Loading ssid from EEPROM.");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM_readAnything( EEPROM_SSID_POSITION, ssidStruct );
  Serial.print("ssidStruct.isSet: "); Serial.println(ssidStruct.isSet);

  if (ssidStruct.isSet) {
    Serial.println("Setting SSID to: \"" + String(ssidStruct.value) + "\".");
    strcpy(ssid, ssidStruct.value);
    MyNetworkSSID = String(ssid);
  } else {
    Serial.println("SSID not set. Using default.");
  }
  EEPROM.end();
}

void loadPassword() {
  Serial.println("Loading password from EEPROM.");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM_readAnything( EEPROM_PASSWORD_POSITION, passwordStruct );
  Serial.print("passwordStruct.isSet: "); Serial.println(passwordStruct.isSet);

  if (passwordStruct.isSet) {
    Serial.println("Setting password to: \"" + String(passwordStruct.value) + "\".");
    strcpy(password, passwordStruct.value);
  } else {
    Serial.println("password not set. Using default.");
  }
  EEPROM.end();
}

void loadHostName() {
  Serial.println("Loading host from EEPROM.");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM_readAnything( EEPROM_HOST_POSITION, hostnameStruct );
  Serial.print("hostnameStruct.isSet: "); Serial.println(hostnameStruct.isSet);

  if (hostnameStruct.isSet) {
    Serial.println("Setting host to: \"" + String(hostnameStruct.value) + "\".");
    strcpy(host_name, hostnameStruct.value);
  } else {
    Serial.println("host not set. Using default.");
  }
  EEPROM.end();
}

void loadPath() {
  Serial.println("Loading path from EEPROM.");
  EEPROM.begin(EEPROM_SIZE);
  EEPROM_readAnything( EEPROM_PATH_POSITION, pathStruct );
  Serial.print("pathStruct.isSet: "); Serial.println(pathStruct.isSet);

  if (pathStruct.isSet) {
    Serial.println("Setting path to: \"" + String(pathStruct.value) + "\".");
    strcpy(path, pathStruct.value);
  } else {
    Serial.println("path not set. Using default.");
  }
  EEPROM.end();
}
