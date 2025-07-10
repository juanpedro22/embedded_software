/*
 * CLNJ, 13/07/2024
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
//See https://www.arduino.cc/reference/en/libraries/espasyncwebserver/
#include <LittleFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <time.h>
#include <LiquidCrystal_I2C.h> // by Frank de Brabander v 1.1.2

const int Ncols = 16;
const int Nrows = 2;
LiquidCrystal_I2C lcd(0x27, Ncols, Nrows); // I2C address: 0x27; Display size: 16x2

#define LED_1_PIN 25   // ESP32 pin IO25 connected to LED 1
#define LED_2_PIN 26   // ESP32 pin IO26 connected to LED 2
#define LED_ON_BOARD  2
// Variável global (adicione fora das funções, no topo do código)
int lastLed4Duty = -1;
// Slide Switch configuration:
#define SLIDE_SWITCH_PIN 17   // ESP32 pin IO32 connected to slide switch

// Motor CC configuration:
#define MOTOR_PIN1 5          // ESP32 pin IO5 connected to motor driver IN1
#define MOTOR_PIN2 23         // ESP32 pin IO23 connected to motor driver IN2

// Pushbutton configuration:
#define PUSHBUTTON_PIN 18     // ESP32 pin IO33 connected to pushbutton

// Buzzer configuration:
#define BUZZER_PIN 12         // ESP32 pin IO14 connected to buzzer

// Potentiometer adjusted Led:
#define POT_PIN 34
#define LED_3_PIN 27
//led PWM
const int pwmFreq = 5000;
const int pwmChannel = 0;
const int pwmResolution = 8;
int pwmDutyCycle = 0;
float potVoltage = 0.0;

//flag estado
String flagcurrentpage = "";
String flaglastaction = "";

//lcd mutex
SemaphoreHandle_t lcdMutex;

String wifiSsid = "defaultSSID";
String wifiPassword = "defaultPassword";
String ntpServer = "time.google.com";
int timezoneOffset = -3;  // GMT-3 por default
String emailAlarm = "default@exemplo.com";

//led 4 brightness control in the webpage
#define LED_4_PIN 14
// PWM
const int pwmled4Freq = 5000;
const int pwmled4Channel = 2;
const int pwmled4Resolution = 8;
int pwmled4DutyCycle = 0;

#define Filename "/text_box_data.txt"

//const char* ntpServer = "pool.ntp.org";

const long  gmtOffset_sec = -3*3600;
const int   daylightOffset_sec = 0; // para 1 h use 3600;
char Local_Date_Time[50]; //50 chars should be enough
struct tm timeinfo;




AsyncWebServer server(80);

String Local_Server_IP, Remote_Client_IP;

int Led_1_State = HIGH;
int Led_2_State = HIGH;
String Text_Box_1_Message = "TB1 message not defined";
String Text_Box_2_Message = "TB2 message not defined";

const char* PARAM_INPUT = "input";
String ledstate, ledstate_inverse;

//for lcd print
String lcdCurrentLine1 = "";
String lcdCurrentLine2 = "";
unsigned long lcdMessageExpiry = 0;

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

// Motor CC variables
String Motor_State = "STOPPED"; // Motor state (STOPPED, CLOCKWISE, COUNTERCLOCKWISE)

// Alarm system variables
volatile bool alarmTriggered = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce
const unsigned long alarmDuration = 2000; // 2 seconds alarm duration
int alarmCount = 0;
String lastAlarmTime = "Nenhum alarme";
// time in lcd 16 max
String time_for_lcd = "";
bool buzzerActive = false;
unsigned long buzzerOffTime = 0;


void actionComunication(const String& line1, const String& line2 = "", unsigned long durationMs = 8000) {
  Serial.println(line1);
  if (line2.length() > 0) {
    Serial.println(line2);
  }

  if (xSemaphoreTake(lcdMutex, portMAX_DELAY) == pdTRUE) {
    // Seu código que acessa o LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(line1);
    if (line2.length() > 0) {
      lcd.setCursor(0,1);
      lcd.print(line2);
    }
    lcdCurrentLine1 = line1;
    lcdCurrentLine2 = line2;
    lcdMessageExpiry = millis() + durationMs;

    xSemaphoreGive(lcdMutex);
  }
  lcdCurrentLine1 = line1;
  lcdCurrentLine2 = line2;
  lcdMessageExpiry = millis() + durationMs;
}


void loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("Arquivo config.json não encontrado. Usando valores padrão.");
    return;
  }

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Erro ao abrir config.json");
    return;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Arquivo config.json muito grande!");
    return;
  }

  std::unique_ptr<char[]> buf(new char[size + 1]);
  configFile.readBytes(buf.get(), size);
  buf[size] = '\0';

  StaticJsonDocument<512> json;
  DeserializationError error = deserializeJson(json, buf.get());

  if (error) {
    Serial.println("Erro ao parsear JSON");
    return;
  }

  if (json.containsKey("wifiSsid")) {
    wifiSsid = json["wifiSsid"].as<String>();
  }
  if (json.containsKey("wifiPassword")) {
    wifiPassword = json["wifiPassword"].as<String>();
  }
  if (json.containsKey("ntpServer")) {
    ntpServer = json["ntpServer"].as<String>();
  }
  if (json.containsKey("timezoneOffset")) {
    timezoneOffset = json["timezoneOffset"].as<int>();
  }
  if (json.containsKey("emailAlarm")) {
    emailAlarm = json["emailAlarm"].as<String>();
  }

  Serial.println("Configurações carregadas do JSON:");
  Serial.println("SSID: " + wifiSsid);
  Serial.println("Senha: " + wifiPassword);
  Serial.println("NTP: " + ntpServer);
  Serial.println("Timezone: " + String(timezoneOffset));
  Serial.println("Email: " + emailAlarm);
}

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
  if (var == "ESP_IP") {
    return Local_Server_IP;
  }
  if (var == "ESP_MAC") {
    return WiFi.macAddress();
  }
  if (var == "WIFI_SSID") {
    return wifiSsid;
  }
  if (var == "WIFI_PASSWORD") {
    return wifiPassword;
  }
  if (var == "NTP_SERVER") {
    return ntpServer;
  }
  if (var == "TIMEZONE") {
    return String(timezoneOffset) + " h";
  }
  if (var == "ALARM_EMAIL") {
    return emailAlarm;
  }
  if (var == "POT_VOLTAGE") {
    return String(potVoltage, 2) + " V";
  }
  if(var == "SLIDE_SWITCH_ALERT_CLASS"){
    if(Slide_Switch_State == LOW)
      return "success";
    else
      return "secondary";
  }

  if(var == "BUZZER_ICON"){
    if(Buzzer_State == HIGH)
      return "volume-up";
    else
      return "volume-mute";
  }

  if(var == "SLIDE_SWITCH_ICON"){
    if(Slide_Switch_State == LOW)
      return "check-circle";
    else
      return "dash-circle";
  }
   // Motor variables
  if(var == "MOTOR_STATE"){
    return Motor_State;
  }
  if(var == "MOTOR_DIRECTION"){
    if(Motor_State == "CLOCKWISE") {
      return "Horário";
    } else if(Motor_State == "COUNTERCLOCKWISE") {
      return "Anti-horário";
    } else {
      return "Parado";
    }
  }
  if(var == "MOTOR_BUTTON_1"){
    if(Motor_State == "STOPPED") {
      return "<a href=\"/motor/clockwise\" class=\"btn btn-success btn-sm\">Acionar sentido horário</a>";
    } else {
      return "<a href=\"/motor/reverse\" class=\"btn btn-warning btn-sm\">Inverter rotação</a>";
    }
  }
  if(var == "MOTOR_BUTTON_2"){
    if(Motor_State == "STOPPED") {
      return "<a href=\"/motor/counterclockwise\" class=\"btn btn-primary btn-sm\">Acionar sentido anti-horário</a>";
    } else {
      return "<a href=\"/motor/stop\" class=\"btn btn-danger btn-sm\">Parar</a>";
    }
  }
  if(var == "MOTOR_ICON"){
    if(Motor_State == "CLOCKWISE") {
      return "bi-arrow-clockwise text-success";
    } else if(Motor_State == "COUNTERCLOCKWISE") {
      return "bi-arrow-counterclockwise text-primary";
    } else {
      return "bi-dash-circle text-secondary";
    }
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

// Motor control functions
void motorClockwise() {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  Motor_State = "CLOCKWISE";
  Serial.println("Motor: Sentido horário");
  actionComunication("Motor: Horario");
}

void motorCounterClockwise() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, HIGH);
  Motor_State = "COUNTERCLOCKWISE";
  Serial.println("Motor: Sentido anti-horário");
  actionComunication("Motor: Anti-horario");
}

void motorStop() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  Motor_State = "STOPPED";
  Serial.println("Motor: Parado");
  actionComunication("Motor: Parado");
}

void motorReverse() {
  if (Motor_State == "CLOCKWISE") {
    motorCounterClockwise();
  } else if (Motor_State == "COUNTERCLOCKWISE") {
    motorClockwise();
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
  actionComunication("ALARME #" + String(alarmCount), lastAlarmTime.substring(0,16));

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

  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }
  loadConfig();

  lcd.init();
  lcd.backlight();

  //semaphore lcd
  lcdMutex = xSemaphoreCreateMutex();
  if (lcdMutex == NULL) {
    Serial.println("Erro ao criar lcdMutex!");
  }

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
  pinMode(SLIDE_SWITCH_PIN, INPUT_PULLUP);
  // PWM setup
  ledcAttachChannel(LED_3_PIN, pwmFreq, pwmResolution, pwmChannel);
  //led
  pinMode(LED_4_PIN, OUTPUT);
  ledcAttachChannel(LED_4_PIN, pwmled4Freq, pwmled4Resolution, pwmled4Channel);

 
  // Motor configuration
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  motorStop(); // Initialize motor in stopped state

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
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  
  int ic=0; String str_blank="";
  while (WiFi.status() != WL_CONNECTED) {
    if (ic==0) {str_blank=""; ic=1;} else {str_blank=" "; ic=0;}
    actionComunication(str_blank + "Trying Wi-Fi:", wifiSsid);
    delay(1000);
  }
  
  actionComunication("Connected Wi-Fi:", wifiSsid);
  delay(3000);
  // Print the ESP32's IP address
  Local_Server_IP = WiFi.localIP().toString();
  actionComunication("IP Servidor:", Local_Server_IP);
  Serial.println("ESP32's MAC address: " + String(WiFi.macAddress()));
  // IPAddress primaryDNS(192, 168, 0, 1);
  IPAddress primaryDNS(8, 8, 8, 8);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4); //optional

  // Date & time initialization
  getLocalTime(&timeinfo);
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  Serial.println("Date & time now: " + String(Local_Date_Time));
  Serial.println("Trying to update date & time...");
  actionComunication("Trying to update", "date & time...");
  while (!getLocalTime(&timeinfo)){
    long gmtOffset_sec = timezoneOffset * 3600;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
    Serial.print("x");
  }
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  Serial.println("Date&time update" + String(Local_Date_Time));
  char Short_Date_Time[30];
  strftime(Short_Date_Time, sizeof(Short_Date_Time), "%d/%m/%Y,%H:%M", &timeinfo);
  actionComunication("NTP atualizado:", String(Short_Date_Time),3000);
  delay(3000);
  
  // Mount filesystem LITTLEFS and check if the output file exists (if not, create it as an empty file)
  if(!LittleFS.begin()) {
     Serial.println("An Error has occurred while mounting LittleFS");
     actionComunication("Error LittleFS");
     delay(3000);
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
  actionComunication("web server", "started!", 3000);
  // Generate an output for each possible file request or command sent to the web server
  // home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();  
    if(flagcurrentpage != "/home.html"){
      actionComunication(Remote_Client_IP, "/home.html");
      flagcurrentpage = "/home.html";
    }
    request->send(LittleFS, "/home.html", String(), false, processor);
  });

  server.on("/home.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP=request->client()->remoteIP().toString();
    if(flagcurrentpage != "/home.html"){
      actionComunication(Remote_Client_IP,"/home.html");
      flagcurrentpage = "/home.html";
    }
    request->send(LittleFS, "/home.html", String(), false, processor);
  });
   server.on("/history.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    request->send(LittleFS, "/history.html", String(), false, processor);
    if(flagcurrentpage != "/history.html"){
      actionComunication(Remote_Client_IP, "/history.html");
      flagcurrentpage = "/history.html";
    }
  });
  server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    request->send(LittleFS, "/settings.html", String(), false, processor);
    if(flagcurrentpage != "/settings.html"){
      actionComunication(Remote_Client_IP, "/settings.html");
      flagcurrentpage = "/settings.html";
    }
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
    actionComunication(Filename);
    request->send(LittleFS, Filename, String(), false, processor);
  });

  // Route to control LED 1
  server.on("/led1/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    Led_1_State = HIGH; digitalWrite(LED_1_PIN, Led_1_State);
    request->send(LittleFS, "/home.html", String(), false, processor);
    if(flaglastaction != "led1/on"){
      actionComunication(Remote_Client_IP, "led1 on");
      flaglastaction = "led1/on";
    }
  });
  server.on("/led1/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    Led_1_State = LOW; digitalWrite(LED_1_PIN, Led_1_State);
    request->send(LittleFS, "/home.html", String(), false, processor);
    if(flaglastaction != "led1/off"){
      actionComunication(Remote_Client_IP, "led1 off");
      flaglastaction = "led1/off";
    }
  });
  // Route to control LED 2
  server.on("/led2/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    Led_2_State = HIGH; digitalWrite(LED_2_PIN, Led_2_State);
    request->send(LittleFS, "/home.html", String(), false, processor);
    if(flaglastaction != "led2/on"){
      actionComunication(Remote_Client_IP, "led2 on");
      flaglastaction = "led2/on";
    }
  });
  server.on("/led2/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    Led_2_State = LOW; digitalWrite(LED_2_PIN, Led_2_State);
    request->send(LittleFS, "/home.html", String(), false, processor);
    if(flaglastaction != "led2/off"){
      actionComunication(Remote_Client_IP, "led2 off");
      flaglastaction = "led2/off";
    }
  });

  // Serving image files
  server.on("/led_on.gif", HTTP_GET, [](AsyncWebServerRequest *request){
    //Serial.println("GET /led_on.gif");
    request->send(LittleFS, "/led_on.gif", "image/gif");
  });
  server.on("/led_off.gif", HTTP_GET, [](AsyncWebServerRequest *request){
    //Serial.println("GET /led_off.gif");
    request->send(LittleFS, "/led_off.gif", "image/gif");
  });

  // Get the client's IP
  server.on("/printRemoteIP", HTTP_GET, [](AsyncWebServerRequest *request){
    Remote_Client_IP=request->client()->remoteIP().toString();
    Serial.println("Received request from client with IP: " + Remote_Client_IP);
    actionComunication("Remote Client IP:", Remote_Client_IP);
    lcd.setCursor(0,1); lcd.print(Remote_Client_IP);
    request->send(200, "text/plain", "Received request from client with IP: " + Remote_Client_IP);
  });

  server.on("/log_last10", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = LittleFS.open(Filename, "r");
    if (!file) {
      request->send(500, "text/plain", "Erro ao abrir o arquivo de log.");
      return;
    }

    // Lê todo o conteúdo
    String content = file.readString();
    file.close();

    // Divide em linhas
    std::vector<String> lines;
    int start = 0;
    int idx = content.indexOf('\n');
    while (idx >= 0) {
      lines.push_back(content.substring(start, idx));
      start = idx + 1;
      idx = content.indexOf('\n', start);
    }
    // Pode ter linha final sem \n
    if (start < content.length()) {
      lines.push_back(content.substring(start));
    }

    // Pega últimas 10 linhas
    String output;
    int total = lines.size();
    int from = total > 10 ? total - 10 : 0;
    for (int i = from; i < total; i++) {
      output += lines[i] + "\n";
    }

    request->send(200, "text/plain", output);
  });

  server.on("/buzzer/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    Buzzer_State = HIGH;
    digitalWrite(BUZZER_PIN, Buzzer_State);

    buzzerActive = true;
    buzzerOffTime = millis() + 5000;

    actionComunication(Remote_Client_IP, "Buzzer acionado");

    request->send(LittleFS, "/home.html", String(), false, processor);
      // Redireciona imediatamente para /home.html
    request->redirect("/home.html");

  });

  server.on("/buzzer_page", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Liga o buzzer
    Buzzer_State = HIGH;
    digitalWrite(BUZZER_PIN, Buzzer_State);

    // Exibe mensagem no LCD
    actionComunication("Buzzer ON", "Desliga em 5 seg");

    // HTML temporário
    String html = R"rawliteral(
      <html>
        <head>
          <meta http-equiv="refresh" content="5; URL=/buzzer/off">
          <title>Buzzer Ativado</title>
        </head>
        <body style="text-align:center;font-family:sans-serif;">
          <h1 style="color:red;">🔊 Buzzer Ativado</h1>
          <p>O buzzer ficará ligado por 5 segundos.</p>
        </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });
  
  server.on("/buzzer/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    Buzzer_State = LOW;
    digitalWrite(BUZZER_PIN, Buzzer_State);
    buzzerActive = false; // Caso use ainda a flag

    Serial.println("Buzzer desligado após 5 segundos.");

    // Opcional: LCD mensagem
    actionComunication("Buzzer desligado");

    // Redireciona
    request->redirect("/home.html");
  });

  server.on("/set_led4", [](AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    if (request->hasParam("duty")) {
      String dutyStr = request->getParam("duty")->value();
      int duty = dutyStr.toInt();
      if (duty < 0) duty = 0;
      if (duty > 100) duty = 100;

      // Atualiza PWM
      pwmled4DutyCycle = map(duty, 0, 100, 0, 255);
      ledcWriteChannel(pwmled4Channel, pwmled4DutyCycle);

      // Verifica diferença para decidir imprimir
      if (lastLed4Duty == -1 || abs(duty - lastLed4Duty) >= 10) {
        // Se primeira vez ou diferença ≥10%, imprime no LCD
        actionComunication(Remote_Client_IP, "LED4 DC: " + String(duty) + "%");
        lastLed4Duty = duty;
      } else {
        // Não imprime, só atualiza variável
        lastLed4Duty = duty;
      }

      // Atualiza flag da página
      flagcurrentpage = "/home.html";

      request->send(LittleFS, "/home.html", String(), false, processor);
    } else {
      request->send(400, "text/plain", "Missing 'duty' parameter");
    }
  });

  // Routes to control Motor
  server.on("/motor/clockwise", HTTP_GET, [](AsyncWebServerRequest *request) {
    motorClockwise();
    request->send(LittleFS, "/home.html", String(), false, processor);
  });

  server.on("/motor/counterclockwise", HTTP_GET, [](AsyncWebServerRequest *request) {
    motorCounterClockwise();
    request->send(LittleFS, "/home.html", String(), false, processor);
  });

  server.on("/motor/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    motorStop();
    request->send(LittleFS, "/home.html", String(), false, processor);
  });

  server.on("/motor/reverse", HTTP_GET, [](AsyncWebServerRequest *request) {
    motorReverse();
    request->send(LittleFS, "/home.html", String(), false, processor);
  });

  // Route to control ServoMotor angle
  server.on("/servo", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Remote_Client_IP = request->client()->remoteIP().toString();
    String inputMessage, inputParam;
    // GET input value on <ESP_IP>/servo?input=<angle>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      inputParam = PARAM_INPUT;

      // Validation: check if input is empty
      if (inputMessage.length() == 0) {
        actionComunication("Servo: Campo vazio");
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
          actionComunication("Servo: Erro", "Apenas numeros");
        }
        else {
          int angle = inputMessage.toInt();
          // Validate angle range (0-180 degrees)
          if (angle >= 0 && angle <= 180) {
            Servo_Angle = angle;
            // Convert angle to duty cycle
            int dutyCycle = map(angle, 0, 180, servoDutyCycleMin, servoDutyCycleMax);
            ledcWriteChannel(ServoChannel, dutyCycle);

            actionComunication(Remote_Client_IP, "Servo: " + String(angle) + " graus");
          } else {
            actionComunication("Servo: Erro", "Range: 0-180");
          }
        }
      }
    }
    else {
      actionComunication("Servo: Erro", "Param ausente");
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
  if (lcdMessageExpiry > 0 && millis() > lcdMessageExpiry) {
    // Mensagem expirou, mostra status default
    lcdMessageExpiry = 0;
    actionComunication("ESP32 Control");
  }
  // Piscar LED onboard com millis()
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_ON_BOARD, ledState ? HIGH : LOW);
  }
  
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

  // Update PWM brightness
  int potValue = analogRead(POT_PIN);
  pwmDutyCycle = map(potValue, 0, 4095, 0, 255);
  ledcWriteChannel(pwmChannel, pwmDutyCycle);
  potVoltage = (potValue / 4095.0) * 3.3;
  static int lastPercent = pwmDutyCycle;
  
  if (abs(lastPercent - pwmDutyCycle) >= 100){
    
    lastPercent = pwmDutyCycle;
    int percent = map(pwmDutyCycle, 0, 255, 0, 100);
    actionComunication("LED 4 Duty Cycle:", String(percent) + "%");

  }
  if (buzzerActive && millis() > buzzerOffTime) {
  Buzzer_State = LOW;
  digitalWrite(BUZZER_PIN, Buzzer_State);
  buzzerActive = false;
  Serial.println("Buzzer desligado após 5 segundos.");
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