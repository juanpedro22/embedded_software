/*
 * ESP32 Email Alert System - Minimal Version
 * Sistema de Alarme com Email Automático (Versão Reduzida)
 * 
 * Funcionalidades incluídas:
 * - Detecção de alarme via botão
 * - Envio de email a cada 5 alarmes
 * - Display LCD com feedback
 * - Buzzer para alarme
 * - Configuração via JSON
 * - Log de eventos
 */

#include <WiFi.h>
#include <LittleFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP_Mail_Client.h>

// LCD Configuration
const int Ncols = 16;
const int Nrows = 2;
LiquidCrystal_I2C lcd(0x27, Ncols, Nrows);

// Pin Definitions
#define LED_ON_BOARD  2
#define PUSHBUTTON_PIN 18     // ESP32 pin for alarm button
#define BUZZER_PIN 12         // ESP32 pin for buzzer

// File for logging
#define Filename "/alarm_log.txt"

// LCD mutex for thread safety
SemaphoreHandle_t lcdMutex;

// Configuration variables
String wifiSsid = "defaultSSID";
String wifiPassword = "defaultPassword";
String ntpServer = "time.google.com";
int timezoneOffset = -3;
String emailAlarm = "default@exemplo.com";
String smtpServer = "smtp.gmail.com";
int smtpPort = 587;
String emailSender = "sender@gmail.com";
String emailPassword = "app_password";
String emailSubject = "Alerta ESP32 - Sistema de Alarme";

// Email objects
SMTPSession smtp;
ESP_Mail_Session session;
SMTP_Message message;

// Time variables
const long gmtOffset_sec = -3*3600;
const int daylightOffset_sec = 0;
char Local_Date_Time[50];
struct tm timeinfo;
String Local_Server_IP;

// Alarm system variables
volatile bool alarmTriggered = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200;
const unsigned long alarmDuration = 2000;
int alarmCount = 0;
String lastAlarmTime = "Nenhum alarme";
int emailsSent = 0;
String lastEmailTime = "Nenhum email enviado";

// LCD display variables
String lcdCurrentLine1 = "";
String lcdCurrentLine2 = "";
unsigned long lcdMessageExpiry = 0;

// Function prototypes
void actionComunication(const String& line1, const String& line2 = "", unsigned long durationMs = 5000);
void loadConfig();
void sendAlarmEmail();
void sendTestEmail();
void activateAlarm();
void IRAM_ATTR pushbuttonISR();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== ESP32 Email Alert System - Minimal Version ===");

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }
  
  // Load configuration
  loadConfig();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Create LCD mutex
  lcdMutex = xSemaphoreCreateMutex();
  if (lcdMutex == NULL) {
    Serial.println("Erro ao criar lcdMutex!");
  }

  // Initialize pins
  pinMode(LED_ON_BOARD, OUTPUT);
  digitalWrite(LED_ON_BOARD, HIGH);
  pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Attach interrupt for alarm button
  attachInterrupt(digitalPinToInterrupt(PUSHBUTTON_PIN), pushbuttonISR, FALLING);

  // Connect to WiFi
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  
  actionComunication("Conectando WiFi", wifiSsid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Local_Server_IP = WiFi.localIP().toString();
  actionComunication("WiFi conectado", Local_Server_IP);
  Serial.println("IP: " + Local_Server_IP);
  Serial.println("MAC: " + WiFi.macAddress());

  // Initialize NTP time
  actionComunication("Sincronizando", "horario NTP...");
  long gmtOffset_sec = timezoneOffset * 3600;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(1000);
  }
  
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y %H:%M:%S", &timeinfo);
  actionComunication("Horário sincronizado", String(Local_Date_Time).substring(0,16));
  Serial.println("Data/Hora: " + String(Local_Date_Time));

  // Initialize log file
  if (!LittleFS.exists(Filename)) {
    Serial.println("Criando arquivo de log...");
    writeFile(LittleFS, Filename, (String(Local_Date_Time) + " - Sistema iniciado\r\n").c_str());
  } else {
    Serial.println("Arquivo de log existente encontrado");
    appendFile(LittleFS, Filename, (String(Local_Date_Time) + " - Sistema reiniciado\r\n").c_str());
  }

  actionComunication("Sistema pronto!", "Pressione botão");
  
  Serial.println("=== Sistema de Email Automático Pronto ===");
  Serial.println("Pressione o botão para gerar alarme");
  Serial.println("Email será enviado a cada 5 alarmes");
}

void loop() {
  // Handle LCD message expiry
  if (lcdMessageExpiry > 0 && millis() > lcdMessageExpiry) {
    lcdMessageExpiry = 0;
    actionComunication("Sistema ativo", "Alarmes: " + String(alarmCount));
  }
  
  // Blink onboard LED
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_ON_BOARD, ledState ? HIGH : LOW);
  }
  
  // Check if alarm was triggered
  if (alarmTriggered) {
    alarmTriggered = false;
    activateAlarm();
  }
  
  delay(10); // Small delay to prevent watchdog issues
}

// Interrupt Service Routine for alarm button
void IRAM_ATTR pushbuttonISR() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceDelay) {
    alarmTriggered = true;
    lastInterruptTime = interruptTime;
  }
}

// Main alarm function
void activateAlarm() {
  // Increment alarm counter
  alarmCount++;

  // Get current time
  getLocalTime(&timeinfo);
  strftime(Local_Date_Time, sizeof(Local_Date_Time), "%d/%m/%Y %H:%M:%S", &timeinfo);
  lastAlarmTime = String(Local_Date_Time);

  // Display on LCD
  Serial.printf("ALARME #%d DISPARADO! Hora: %s\n", alarmCount, lastAlarmTime.c_str());
  actionComunication("ALARME #" + String(alarmCount), lastAlarmTime.substring(11,19)); // Show only time

  // Log event
  String logEntry = String(Local_Date_Time) + " - Alarme #" + String(alarmCount) + " - Botão pressionado\r\n";
  appendFile(LittleFS, Filename, logEntry.c_str());

  // Activate buzzer
  digitalWrite(BUZZER_PIN, HIGH);
  delay(alarmDuration);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Buzzer desativado");

  // Check if should send email (every 5 alarms)
  if (alarmCount % 5 == 0) {
    Serial.printf("Enviando email - 5 alarmes atingidos (Total: %d)\n", alarmCount);
    emailsSent++;
    lastEmailTime = String(Local_Date_Time);
    sendAlarmEmail();
  } else {
    int remaining = 5 - (alarmCount % 5);
    actionComunication("Faltam " + String(remaining), "para proximo email");
  }
}

// Email sending function
void sendAlarmEmail() {
  message.clear();
  
  // Configure session
  session.server.host_name = smtpServer.c_str();
  session.server.port = smtpPort;
  session.login.email = emailSender.c_str();
  session.login.password = emailPassword.c_str();
  session.login.user_domain = "";
  
  session.time.ntp_server = ntpServer.c_str();
  session.time.gmt_offset = timezoneOffset;
  session.time.day_light_offset = 0;
  
  // Configure message
  message.sender.name = "ESP32 Sistema de Alarme";
  message.sender.email = emailSender.c_str();
  message.subject = emailSubject.c_str();
  message.addRecipient("", emailAlarm.c_str());
  
  // Create email content
  String emailContent = "🚨 ALERTA DO SISTEMA ESP32 🚨\n\n";
  emailContent += "RESUMO DO ALERTA:\n";
  emailContent += "- Total de alarmes: " + String(alarmCount) + "\n";
  emailContent += "- Última ocorrência: " + lastAlarmTime + "\n";
  emailContent += "- IP do ESP32: " + Local_Server_IP + "\n";
  emailContent += "- MAC Address: " + WiFi.macAddress() + "\n\n";
  emailContent += "PRÓXIMO EMAIL:\n";
  emailContent += "- Será enviado no alarme #" + String(alarmCount + 5) + "\n\n";
  emailContent += "Sistema ESP32 - Gerado automaticamente\n";
  emailContent += "Hora: " + String(Local_Date_Time);
  
  message.text.content = emailContent.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  // Send email
  Serial.println("Enviando email de alerta...");
  actionComunication("Enviando email", "Alarme #" + String(alarmCount));
  
  if (!smtp.connect(&session)) {
    String errorMsg = smtp.errorReason();
    Serial.println("Erro SMTP: " + errorMsg);
    actionComunication("Erro SMTP", errorMsg.substring(0,16));
    
    String logEntry = String(Local_Date_Time) + " - Erro SMTP: " + errorMsg + "\r\n";
    appendFile(LittleFS, Filename, logEntry.c_str());
    return;
  }
  
  if (!MailClient.sendMail(&smtp, &message)) {
    String errorMsg = smtp.errorReason();
    Serial.println("Erro ao enviar: " + errorMsg);
    actionComunication("Erro envio", errorMsg.substring(0,16));
    
    String logEntry = String(Local_Date_Time) + " - Erro envio: " + errorMsg + "\r\n";
    appendFile(LittleFS, Filename, logEntry.c_str());
  } else {
    Serial.println("Email enviado com sucesso!");
    actionComunication("Email enviado!", "Total: " + String(emailsSent));
    
    String logEntry = String(Local_Date_Time) + " - Email enviado - Alarme #" + String(alarmCount) + "\r\n";
    appendFile(LittleFS, Filename, logEntry.c_str());
  }
  
  smtp.closeSession();
  message.clear();
}

// Test email function
void sendTestEmail() {
  message.clear();
  
  session.server.host_name = smtpServer.c_str();
  session.server.port = smtpPort;
  session.login.email = emailSender.c_str();
  session.login.password = emailPassword.c_str();
  session.login.user_domain = "";
  
  session.time.ntp_server = ntpServer.c_str();
  session.time.gmt_offset = timezoneOffset;
  session.time.day_light_offset = 0;
  
  message.sender.name = "ESP32 Sistema - TESTE";
  message.sender.email = emailSender.c_str();
  message.subject = "TESTE - Sistema ESP32";
  message.addRecipient("", emailAlarm.c_str());
  
  String emailContent = "🧪 TESTE DO SISTEMA ESP32 🧪\n\n";
  emailContent += "Sistema funcionando corretamente!\n\n";
  emailContent += "INFORMAÇÕES:\n";
  emailContent += "- IP: " + Local_Server_IP + "\n";
  emailContent += "- MAC: " + WiFi.macAddress() + "\n";
  emailContent += "- Data/Hora: " + String(Local_Date_Time) + "\n";
  emailContent += "- Total de alarmes: " + String(alarmCount) + "\n\n";
  emailContent += "Teste realizado com sucesso!";
  
  message.text.content = emailContent.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  Serial.println("Enviando email de teste...");
  actionComunication("Enviando teste", "Aguarde...");
  
  if (!smtp.connect(&session)) {
    String errorMsg = smtp.errorReason();
    Serial.println("Erro SMTP teste: " + errorMsg);
    actionComunication("Erro teste", errorMsg.substring(0,16));
    return;
  }
  
  if (!MailClient.sendMail(&smtp, &message)) {
    String errorMsg = smtp.errorReason();
    Serial.println("Erro teste: " + errorMsg);
    actionComunication("Erro teste", errorMsg.substring(0,16));
  } else {
    Serial.println("Email de teste enviado!");
    actionComunication("Teste enviado!", "Email OK");
    
    String logEntry = String(Local_Date_Time) + " - Teste de email enviado\r\n";
    appendFile(LittleFS, Filename, logEntry.c_str());
  }
  
  smtp.closeSession();
  message.clear();
}

// LCD communication function with mutex protection
void actionComunication(const String& line1, const String& line2, unsigned long durationMs) {
  Serial.println(line1);
  if (line2.length() > 0) {
    Serial.println(line2);
  }

  if (xSemaphoreTake(lcdMutex, portMAX_DELAY) == pdTRUE) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    if (line2.length() > 0) {
      lcd.setCursor(0, 1);
      lcd.print(line2);
    }
    xSemaphoreGive(lcdMutex);
  }
  
  lcdCurrentLine1 = line1;
  lcdCurrentLine2 = line2;
  lcdMessageExpiry = millis() + durationMs;
}

// Load configuration from JSON file
void loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("Config.json não encontrado. Usando valores padrão.");
    return;
  }

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Erro ao abrir config.json");
    return;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config.json muito grande!");
    configFile.close();
    return;
  }

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();

  StaticJsonDocument<512> json;
  DeserializationError error = deserializeJson(json, buf.get());

  if (error) {
    Serial.println("Erro ao parsear JSON");
    return;
  }

  // Load configuration values
  if (json.containsKey("wifiSsid")) wifiSsid = json["wifiSsid"].as<String>();
  if (json.containsKey("wifiPassword")) wifiPassword = json["wifiPassword"].as<String>();
  if (json.containsKey("ntpServer")) ntpServer = json["ntpServer"].as<String>();
  if (json.containsKey("timezoneOffset")) timezoneOffset = json["timezoneOffset"].as<int>();
  if (json.containsKey("emailAlarm")) emailAlarm = json["emailAlarm"].as<String>();
  if (json.containsKey("smtpServer")) smtpServer = json["smtpServer"].as<String>();
  if (json.containsKey("smtpPort")) smtpPort = json["smtpPort"].as<int>();
  if (json.containsKey("emailSender")) emailSender = json["emailSender"].as<String>();
  if (json.containsKey("emailPassword")) emailPassword = json["emailPassword"].as<String>();

  Serial.println("Configuração carregada:");
  Serial.println("SSID: " + wifiSsid);
  Serial.println("Email: " + emailAlarm);
  Serial.println("SMTP: " + smtpServer);
}

// File system functions
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Escrevendo arquivo: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Erro ao criar arquivo");
    return;
  }
  if (file.print(message)) {
    Serial.println("Arquivo criado com sucesso");
  } else {
    Serial.println("Erro ao escrever");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Adicionando ao arquivo: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Erro ao abrir arquivo");
    return;
  }
  if (file.print(message)) {
    Serial.println("Mensagem adicionada");
  } else {
    Serial.println("Erro ao adicionar");
  }
  file.close();
}
