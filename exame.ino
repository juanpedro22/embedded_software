/*
 * CLNJ, 13/07/2024
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

// Slide Switch configuration:
#define SLIDE_SWITCH_PIN 32   // ESP32 pin IO32 connected to slide switch

// Pushbutton configuration:
#define PUSHBUTTON_PIN 18     // ESP32 pin IO33 connected to pushbutton

// Buzzer configuration:
#define BUZZER_PIN 14         // ESP32 pin IO14 connected to buzzer

// Potentiometer adjusted Led:
#define POT_PIN 34
#define LED_3_PIN 27
// PWM
const int pwmFreq = 5000;
const int pwmChannel = 0;
const int pwmResolution = 8;
int pwmDutyCycle = 0;
//led 4 brightness control in the webpage
#define LED_4_PIN 14
// PWM
const int pwmled4Freq = 5000;
const int pwmled4Channel = 1;
const int pwmled4Resolution = 8;
int pwmled4DutyCycle = 0;

#define Filename "/text_box_data.txt"

//const char* ntpServer = "pool.ntp.org";
const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = -3*3600;
const int   daylightOffset_sec = 0; // para 1 h use 3600;
char Local_Date_Time[50]; //50 chars should be enough
struct tm timeinfo;

// Replace with your network credentials
const char* ssid     = "ggg";
const char* password = "gggg";

AsyncWebServer server(80);

String Local_Server_IP, Remote_Client_IP;

int Led_1_State = HIGH;
int Led_2_State = HIGH;
String Text_Box_1_Message = "TB1 message not defined";
String Text_Box_2_Message = "TB2 message not defined";

const char* PARAM_INPUT = "input";
String ledstate, ledstate_inverse;

// For PWM info display
String pwmDutyCycleStr;
String pwmFrequencyStr;

// Servomotor configuration:
const int Servo = 19;

//PWM Servo: 
const int ServoChannel = 1;
const int ServoResolution = 10;
const int Nsteps = pow(2,ServoResolution);  // duty cycle de 0 a Nsteps-1
const float T = 20.0;                       // Periodo do sinal de PWM (em ms)
const float Tmin = 0.5;                    // Tmin do pulso (em ms)
const float Tmax = 2.5;                    // Tmax do pulso (em ms)
const float Servofreq = 1000.0/T;         // frequência do sinal de PWM (em Hz)
const int servoDutyCycleMin = Nsteps*Tmin/T;
const int servoDutyCycleMax = Nsteps*Tmax/T;

int Servo_Angle = 90; // Servo angle state (0-180 degrees)
int Slide_Switch_State = LOW; // Slide switch state (HIGH/LOW)
int Pushbutton_State = HIGH; // Pushbutton state (HIGH/LOW) - INPUT_PULLUP inverts logic
int Buzzer_State = LOW; // Buzzer state (HIGH/LOW)

// Alarm system variables
volatile bool alarmTriggered = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce
const unsigned long alarmDuration = 2000; // 2 seconds alarm duration
int alarmCount = 0;
String lastAlarmTime = "Nenhum alarme";

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
  if (var == "PWM_DUTY_CYCLE") {
    int percent = map(pwmDutyCycle, 0, 255, 0, 100);
    Serial.print("DEBUG PWM: ");
    Serial.println(percent);
    return String(percent);
  }
  if (var == "PWM_DUTY_STARS") {
    int percent = map(pwmDutyCycle, 0, 255, 0, 10);
    String stars = "";
    for (int i = 0; i < percent; i++) stars += "▮";
    for (int i = percent; i < 10; i++) stars += "▯";
    return stars;
  }
  if (var == "PWM_FREQUENCY"){
    return String(pwmFreq);
  }
  if (var == "LED4_DUTY_CYCLE") {
    int percent = map(pwmled4DutyCycle, 0, 255, 0, 100);
    return String(percent);
  }
  if (var == "LED4_FREQUENCY") {
    return String(pwmled4Freq);
  }
  if(var == "SERVO_ANGLE"){
    return String(Servo_Angle);
  }
  if(var == "SLIDE_SWITCH_STATE"){
    if(Slide_Switch_State == LOW)  // INPUT_PULLUP inverts logic
    { return "on"; } else { return "off"; }
  }
  if(var == "PUSHBUTTON_STATE"){
    if(Pushbutton_State == LOW)  // INPUT_PULLUP inverts logic
    { return "pressed"; } else { return "released"; }
  }
  if(var == "BUZZER_STATE"){
    if(Buzzer_State == HIGH)
    { return "on"; } else { return "off"; }
  }
  if(var == "BUZZER_STATE_INVERSE"){
    if(Buzzer_State == HIGH)
    { return "off"; } else { return "on"; }
  }
  if(var == "ALARM_COUNT"){
    return String(alarmCount);
  }
  if(var == "LAST_ALARM_TIME"){
    return lastAlarmTime;
  }

  return String();
}

// Interrupt Service Routine for pushbutton
void IRAM_ATTR pushbuttonISR() {
  unsigned long interruptTime = millis();
  // Debounce: ignore if interrupt occurred too recently
  if (interruptTime - lastInterruptTime > debounceDelay) {
    alarmTriggered = true;
    lastInterruptTime = interruptTime;
  }
}

// Alarm function
void activateAlarm() {
  // Increment alarm counter
  alarmCount++;

  // Get current date and time for logging
  getLocalTime(&timeinfo);
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  lastAlarmTime = String(Local_Date_Time);

  // 1. First: Display on LCD
  Serial.printf("ALARM #%d TRIGGERED! Time: %s\n", alarmCount, lastAlarmTime.c_str());
  lcd.clear(); 
  lcd.setCursor(0,0); 
  lcd.print("ALARME #" + String(alarmCount));
  lcd.setCursor(0,1); 
  lcd.print(lastAlarmTime.substring(0, 16)); // Display date/time (truncated to fit)

  // 2. Second: Log to file
  String logEntry = String(Local_Date_Time) + ", Alarme #" + String(alarmCount) + " - Botao pressionado\r\n";
  appendFile(LittleFS, Filename, logEntry.c_str());

  // 3. Third: Start buzzer, wait, then stop
  Buzzer_State = HIGH;
  digitalWrite(BUZZER_PIN, Buzzer_State);
  delay(alarmDuration); // Wait for 2 seconds
  Buzzer_State = LOW;
  digitalWrite(BUZZER_PIN, Buzzer_State);

  Serial.println("Buzzer deactivated after 2 seconds");
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
  //LED3 and PWM pot
  pinMode(LED_3_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  // PWM setup
  ledcAttachChannel(LED_3_PIN, pwmFreq, pwmResolution, pwmChannel);
  //led
  pinMode(LED_4_PIN, OUTPUT);
  ledcAttachChannel(LED_4_PIN, pwmled4Freq, pwmled4Resolution, pwmled4Channel);


  // Slide switch configuration
  pinMode(SLIDE_SWITCH_PIN, INPUT_PULLUP);
  // Pushbutton configuration
  pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
  // Attach interrupt to pushbutton (trigger on falling edge - when pressed)
  attachInterrupt(digitalPinToInterrupt(PUSHBUTTON_PIN), pushbuttonISR, FALLING);
  // Buzzer configuration
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, Buzzer_State);

  ledcAttachChannel(Servo, Servofreq, ServoResolution, ServoChannel); // Servomotor

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
    Remote_Client_IP = request->client()->remoteIP().toString();
    Serial.println("GET /home.html");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/home.html");
    request->send(LittleFS, "/home.html", String(), false, processor);
  });

  server.on("/home.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP=request->client()->remoteIP().toString();
    Serial.println("GET /home.html");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/home.html");
    request->send(LittleFS, "/home.html", String(), false, processor);
  });
   server.on("/history.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    request->send(LittleFS, "/history.html", "text/html");
    request->send(LittleFS, "/history.html", String(), false, processor);
  });
  server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    request->send(LittleFS, "/settings.html", "text/html");
  });

  //pot
server.on("/get_pwm", HTTP_GET, [](AsyncWebServerRequest *request){
  int percent = map(pwmDutyCycle, 0, 255, 0, 100);
    String html = 
        "<meta http-equiv='refresh' content='2'>"
        "<div class='progress' style='height:25px;'>"
          "<div class='progress-bar bg-warning' role='progressbar' style='width:" + String(percent) + "%;'>"
          + String(percent) + "%</div>"
        "</div>";
    request->send(200, "text/html", html);
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
    request->send(LittleFS, "/home.html", String(), false, processor);
  });
  server.on("/led1/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_1_State = LOW; digitalWrite(LED_1_PIN, Led_1_State);
    Serial.println("GET /led1/off");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led1/off");
    request->send(LittleFS, "/home.html", String(), false, processor);
  });
  // Route to control LED 2
  server.on("/led2/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_2_State = HIGH; digitalWrite(LED_2_PIN, Led_2_State);
    Serial.println("GET /led2/on");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led2/on");
    request->send(LittleFS, "/home.html", String(), false, processor);
  });
  server.on("/led2/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Led_2_State = LOW; digitalWrite(LED_2_PIN, Led_2_State);
    Serial.println("GET /led2/off");
    lcd.clear(); lcd.setCursor(0,0); lcd.print("/led2/off");
    request->send(LittleFS, "/home.html", String(), false, processor);
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
  server.on("/set_led4", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("duty")) {
      String dutyStr = request->getParam("duty")->value();
      int duty = dutyStr.toInt();
      if (duty < 0) duty = 0;
      if (duty > 100) duty = 100;
      pwmled4DutyCycle = map(duty, 0, 100, 0, 255);

      // Atualiza PWM
  
      ledcWriteChannel(pwmled4Channel, pwmled4DutyCycle);

      // Mensagem serial
      Serial.print("LED4 Duty set to: ");
      Serial.print(duty);
      Serial.println("%");

      // Atualiza LCD
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("LED4 Duty Cycle:");
      lcd.setCursor(0,1);
      lcd.print(String(duty) + "%");

      request->send(LittleFS, "/home.html", String(), false, processor);
    } else {
      request->send(400, "text/plain", "Missing 'duty' parameter");
    }
  });
  // Route to control ServoMotor angle
  server.on("/servo", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage, inputParam;
    // GET input value on <ESP_IP>/servo?input=<angle>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      inputParam = PARAM_INPUT;

      // Validation: check if input is empty
      if (inputMessage.length() == 0) {
        Serial.println("Servo: Empty input");
        lcd.clear(); 
        lcd.setCursor(0,0); 
        lcd.print("Servo: Campo vazio");
      }
      // Validation: check if input contains only digits
      else {
        bool isValidNumber = true;
        for (int i = 0; i < inputMessage.length(); i++) {
          if (!isDigit(inputMessage.charAt(i))) {
            isValidNumber = false;
            break;
          }
        }

        if (!isValidNumber) {
          Serial.println("Servo: Invalid characters in input: " + inputMessage);
          lcd.clear(); 
          lcd.setCursor(0,0); 
          lcd.print("Servo: Erro");
          lcd.setCursor(0,1); 
          lcd.print("Apenas numeros");
        }
        else {
          int angle = inputMessage.toInt();
          // Validate angle range (0-180 degrees)
          if (angle >= 0 && angle <= 180) {
            Servo_Angle = angle;
            // Convert angle to duty cycle
            int dutyCycle = map(angle, 0, 180, servoDutyCycleMin, servoDutyCycleMax);
            ledcWriteChannel(ServoChannel, dutyCycle);

            Serial.println("Servo angle set to: " + String(angle) + "°");
            lcd.clear(); 
            lcd.setCursor(0,0); 
            lcd.print("Servo: " + String(angle) + " graus");
          } else {
            Serial.println("Servo: Angle out of range: " + String(angle));
            lcd.clear(); 
            lcd.setCursor(0,0); 
            lcd.print("Servo: Erro");
            lcd.setCursor(0,1); 
            lcd.print("Range: 0-180");
          }
        }
      }
    }
    else {
      Serial.println("Servo: Missing parameter");
      lcd.clear(); 
      lcd.setCursor(0,0); 
      lcd.print("Servo: Erro");
      lcd.setCursor(0,1); 
      lcd.print("Param ausente");
    }

    request->send(LittleFS, "/home.html", String(), false, processor);
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

  // Update PWM brightness
  int potValue = analogRead(POT_PIN);
  pwmDutyCycle = map(potValue, 0, 4095, 0, 255);
  ledcWriteChannel(pwmChannel, pwmDutyCycle);
  Serial.println(analogRead(POT_PIN));
  
  // Check if alarm was triggered by interrupt
  if (alarmTriggered) {
    alarmTriggered = false; // Reset flag
    activateAlarm();
  }
  // Read slide switch state
  int newSwitchState = digitalRead(SLIDE_SWITCH_PIN);
  if (newSwitchState != Slide_Switch_State) {
    Slide_Switch_State = newSwitchState;
    Serial.print("Slide Switch: ");
    Serial.println(Slide_Switch_State == LOW ? "ON" : "OFF");
  }

  // Exibir no Serial
  //Serial.print("PWM Duty Cycle: ");
  //Serial.print(percent);
  //Serial.println("%");

  // Exibir no LCD
  //lcd.clear();
  //lcd.setCursor(0,0);
  //lcd.print("PWM Duty Cycle:");
  //lcd.setCursor(0,1);
  //lcd.print(String(percent) + "%");

  // Calcular percentual (0-100%)
  int percent = map(pwmDutyCycle, 0, 255, 0, 100);

  // Variável estática para armazenar último valor
  static int lastPercent = -1;

  // Atualizar apenas se diferença >= 3
  if (abs(percent - lastPercent) >= 3) {
    lastPercent = percent;

    // Serial
    Serial.print("PWM Duty Cycle: ");
    Serial.print(percent);
    Serial.println("%");

    // LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("PWM Duty Cycle:");
    lcd.setCursor(0,1);
    lcd.print(String(percent) + "%");
  }

  




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
