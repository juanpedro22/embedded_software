/*
 * CLNJ, 07/07/2025
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
//See https://www.arduino.cc/reference/en/libraries/espasyncwebserver/
#include <LittleFS.h>
#include <Arduino.h>
#include <FS.h>
#include <time.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> // by Frank de Brabander v 1.1.2

const int Ncols = 16;
const int Nrows = 2;
LiquidCrystal_I2C lcd(0x27, Ncols, Nrows); // I2C address: 0x27; Display size: 16x2

#define LED_1_PIN 25   // ESP32 pin IO25 connected to LED 1
#define LED_2_PIN 26   // ESP32 pin IO26 connected to LED 2
#define LED_ON_BOARD  2

#define Filename "/text_box_data.txt"

//const char* ntpServer = "pool.ntp.org";
const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = -3*3600;
const int   daylightOffset_sec = 0; // para 1 h use 3600;
char Local_Date_Time[50]; //50 chars should be enough
struct tm timeinfo;

// Replace with your network credentials
const char* ssid     = "LMI_2";
const char* password = "ea254@ea254";

AsyncWebServer server(80);

String Local_Server_IP, Remote_Client_IP;

int Led_1_State = HIGH;
int Led_2_State = HIGH;
String Text_Box_1_Message = "TB1 message not defined";
String Text_Box_2_Message = "TB2 message not defined";

const char* PARAM_INPUT = "input";
String ledstate, ledstate_inverse;

// Function to replace the placeholders
String processor(const String& var){
  //Serial.println(var);
  if(var == "LED_1_STATE"){
    if(Led_1_State == LOW)
    { ledstate = "off"; } else { ledstate = "on"; }
    return ledstate;
  }
  if(var == "LED_1_STATE_INVERSE"){
    if(Led_1_State == LOW)
    { ledstate_inverse = "on"; } else { ledstate_inverse = "off"; }
    return ledstate_inverse;
  }
  if(var == "LED_2_STATE"){
    if(Led_2_State == LOW)
    { ledstate = "off"; } else { ledstate = "on"; }
    return ledstate;
  }
  if(var == "LED_2_STATE_INVERSE"){
    if(Led_2_State == LOW)
    { ledstate_inverse = "on"; } else { ledstate_inverse = "off"; }
    return ledstate_inverse;
  }
  if(var == "TEXT_BOX_1_MESSAGE"){
    return Text_Box_1_Message;
  }
  if(var == "TEXT_BOX_2_MESSAGE"){
    return Text_Box_2_Message; 
  }
  if(var == "LOCAL_DATE_TIME"){
    getLocalTime(&timeinfo);
    strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
    return Local_Date_Time;
  }
  if(var == "LOCAL_SERVER_IP"){
    return Local_Server_IP;
  }
  if(var == "REMOTE_CLIENT_IP"){
    return Remote_Client_IP;
  }

  return String();
}

void setup() {
  Serial.begin(115200);
  delay(5000);

  lcd.init();
  lcd.backlight();

  // Device configuration
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_1_PIN, Led_1_State);
  pinMode(LED_2_PIN, OUTPUT);
  digitalWrite(LED_2_PIN, Led_2_State);
  pinMode(LED_ON_BOARD, OUTPUT);
  digitalWrite(LED_ON_BOARD, HIGH);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  int ic=0; String str_blank="";
  while (WiFi.status() != WL_CONNECTED) {
    if (ic==0) {str_blank=""; ic=1;} else {str_blank=" "; ic=0;}
    Serial.println(str_blank + "Trying to connect to Wi-Fi network \"" + String(ssid) + "\"");
    lcd.clear(); lcd.setCursor(0,0); lcd.print(str_blank + "Trying Wi-Fi:");
    lcd.setCursor(0,1); lcd.print(ssid);
    delay(1000);
  }
  Serial.println("Connected to Wi-Fi network \"" + String(ssid) + "\"");
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Connected Wi-Fi:");
  lcd.setCursor(0,1); lcd.print(ssid);
  // Print the ESP32's IP address
  Local_Server_IP = WiFi.localIP().toString();
  Serial.println("ESP32's IP address: " + Local_Server_IP);
  Serial.println("ESP32's MAC address: " + String(WiFi.macAddress()));
  // IPAddress primaryDNS(192, 168, 0, 1);
  IPAddress primaryDNS(8, 8, 8, 8);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4); //optional

  // Date & time initialization
  getLocalTime(&timeinfo);
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  Serial.println("Date & time now: " + String(Local_Date_Time));
  Serial.println("Trying to update date & time...");
  while (!getLocalTime(&timeinfo)){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(1000);
    Serial.print("x");
  }
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  Serial.println(" Date & time update to: " + String(Local_Date_Time));

  // Mount filesystem LITTLEFS and check if the output file exists (if not, create it as an empty file)
  if(!LittleFS.begin()) {
     Serial.println("An Error has occurred while mounting LittleFS");
     return;
    }
   else{
     Serial.println("Little FS Mounted Successfully.");
    }
  // Check if the file already exists to prevent overwritting existing data
  bool fileexists = LittleFS.exists(Filename);
  if(!fileexists) {
    Serial.println("Log file " + String(Filename) + " does not exist. Creating log file...");
    // Create File and add header
    getLocalTime(&timeinfo);
    strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
    //appendFile(LittleFS, Filename, (String(Local_Date_Time) + ", TB2: " + inputMessage + "\r\n").c_str()); //Append data to the file
    writeFile(LittleFS, Filename, (String(Local_Date_Time) + ", Log file " + String(Filename) + " created.\r\n").c_str());
   }
   else {
    Serial.println("Log file " + String(Filename) + " already exists. Appending messagens to log file...");
    // Delete File
    // deleteFile(LittleFS, Filename);
   }
  
  // Generate an output for each possible file request or command sent to the web server
  // home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /index.html");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/index.html");
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/frame_menu.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /frame_menu.html");
    request->send(LittleFS, "/frame_menu.html", "text/html");
  });
  server.on("/Page_1.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP=request->client()->remoteIP().toString();
    Serial.println("GET /Page_1.html");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/Page_1.html");
    request->send(LittleFS, "/Page_1.html", String(), false, processor);
  });
  server.on("/Page_2.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET /Page_2.html");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/Page_2.html");
    request->send(LittleFS, "/Page_2.html", String(), false, processor);
  });
  //server.on("/teste.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
  server.on(Filename, HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("GET " + String(Filename));
    lcd.clear(); lcd.setCursor(0,0); lcd.print(Filename);
    request->send(LittleFS, Filename, String(), false, processor);
  });

  // Route to control LED 1
  server.on("/led1/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_1_State = HIGH; digitalWrite(LED_1_PIN, Led_1_State);
    Serial.println("GET /led1/on");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led1/on");
    request->send(LittleFS, "/Page_1.html", String(), false, processor);
  });
  server.on("/led1/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_1_State = LOW; digitalWrite(LED_1_PIN, Led_1_State);
    Serial.println("GET /led1/off");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led1/off");
    request->send(LittleFS, "/Page_1.html", String(), false, processor);
  });
  // Route to control LED 2
  server.on("/led2/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_2_State = HIGH; digitalWrite(LED_2_PIN, Led_2_State);
    Serial.println("GET /led2/on");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led2/on");
    request->send(LittleFS, "/Page_2.html", String(), false, processor);
  });
  server.on("/led2/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_2_State = LOW; digitalWrite(LED_2_PIN, Led_2_State);
    Serial.println("GET /led2/off");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led2/off");
    request->send(LittleFS, "/Page_2.html", String(), false, processor);
  });

  // Send a GET request to <ESP_IP>/text_box_1?input=<inputMessage>
  server.on("/text_box_1", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage, inputParam;
    // GET input value on <ESP_IP>/text_box_1?input=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      inputParam = PARAM_INPUT;
    }
    else {
      inputMessage = "Text Box 1: wrong or null parameter.";
      inputParam = "none";
    }
    Text_Box_1_Message = inputMessage;
    Serial.print("Text Box 1: "); Serial.println(inputMessage);
    lcd.clear(); lcd.setCursor(0,0); lcd.print("Text Box 1:"); lcd.setCursor(0,1); lcd.print(inputMessage);
    // Append text to the output file
    getLocalTime(&timeinfo);
    strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
    appendFile(LittleFS, Filename, (String(Local_Date_Time) + ", Text Box 1: " + inputMessage + "\r\n").c_str()); //Append data to the file
    request->send(LittleFS, "/Page_1.html", String(), false, processor);
  });
  // Send a GET request to <ESP_IP>/text_box_2?input=<inputMessage>
  server.on("/text_box_2", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage, inputParam;
    // GET input value on <ESP_IP>/text_box_2?input=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      inputParam = PARAM_INPUT;
    }
    else {
      inputMessage = "Text Box 2: wrong or null parameter.";
      inputParam = "none";
    }
    Text_Box_2_Message = inputMessage;
    Serial.print("Text Box 2: "); Serial.println(inputMessage);
    lcd.clear(); lcd.setCursor(0,0); lcd.print("Text Box 2:"); lcd.setCursor(0,1); lcd.print(inputMessage);
    // Append text to the output file
    getLocalTime(&timeinfo);
    strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
    appendFile(LittleFS, Filename, (String(Local_Date_Time) + ", Text Box 2: " + inputMessage + "\r\n").c_str()); //Append data to the file
    request->send(LittleFS, "/Page_2.html", String(), false, processor);
  });

  // Serving image files
  server.on("/led_on.gif", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("GET /led_on.gif");
    request->send(LittleFS, "/led_on.gif", "image/gif");
  });
  server.on("/led_off.gif", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("GET /led_off.gif");
    request->send(LittleFS, "/led_off.gif", "image/gif");
  });

  // Get the client's IP
  server.on("/printRemoteIP", HTTP_GET, [](AsyncWebServerRequest *request){
    Remote_Client_IP=request->client()->remoteIP().toString();
    Serial.println("Received request from client with IP: " + Remote_Client_IP);
    lcd.clear(); lcd.setCursor(0,0); lcd.print("Remote Client IP:");
    lcd.setCursor(0,1); lcd.print(Remote_Client_IP);
    request->send(200, "text/plain", "Received request from client with IP: " + Remote_Client_IP);
  });

  // Start the server
  server.begin();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void loop() {
  // Your code 
  digitalWrite(LED_ON_BOARD,HIGH);
  delay(500);
  digitalWrite(LED_ON_BOARD,LOW);
  delay(500);
}

// Functions for creating, appending, reading and deleting a file
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: \"%s\"\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing.");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written.");
    } else {
        Serial.println("- write failed.");
    }
    file.close();
}
void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending.");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended.");
    } else {
        Serial.println("- append failed.");
    }
    file.close();
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading.");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\r\n", path);
  if(fs.remove(path)){
    Serial.println("- file deleted.");
  } else {
    Serial.println("- delete failed.");
  }
}

