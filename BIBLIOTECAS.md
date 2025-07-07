# Bibliotecas Necessárias - Versão Minimal

## Lista de Bibliotecas

### Bibliotecas Principais
```
1. ESP Mail Client
   - Versão: 3.4.0 ou superior
   - Autor: Mobizt
   - Descrição: Cliente SMTP/IMAP para ESP32
   - Instalação: Library Manager → "ESP Mail Client"

2. ArduinoJson
   - Versão: 6.21.0 ou superior
   - Autor: Benoit Blanchon
   - Descrição: Biblioteca JSON para Arduino
   - Instalação: Library Manager → "ArduinoJson"

3. LiquidCrystal I2C
   - Versão: 1.1.2 ou superior
   - Autor: Frank de Brabander
   - Descrição: Controle LCD via I2C
   - Instalação: Library Manager → "LiquidCrystal I2C"
```

### Bibliotecas do Sistema (Incluídas)
```
4. WiFi - Incluída no ESP32 Core
5. LittleFS - Incluída no ESP32 Core
6. Wire - Incluída no ESP32 Core
7. time.h - Incluída no ESP32 Core
```

## Instalação Passo a Passo

### 1. Configurar Board Manager
```
1. Abrir Arduino IDE
2. File → Preferences
3. Em "Additional Board Manager URLs", adicionar:
   https://dl.espressif.com/dl/package_esp32_index.json
4. Tools → Board → Board Manager
5. Buscar "ESP32" e instalar "ESP32 by Espressif Systems"
```

### 2. Instalar Bibliotecas
```
1. Tools → Manage Libraries
2. Buscar e instalar cada biblioteca:
   - "ESP Mail Client" by Mobizt
   - "ArduinoJson" by Benoit Blanchon
   - "LiquidCrystal I2C" by Frank de Brabander
```

### 3. Verificar Instalação
```cpp
// Código de teste para verificar bibliotecas
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <ESP_Mail_Client.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Todas as bibliotecas carregadas com sucesso!");
}

void loop() {
    // Vazio
}
```

## Configurações de Compilação

### Board Settings
```
Board: ESP32 Dev Module
Upload Speed: 921600
CPU Frequency: 240MHz
Flash Frequency: 80MHz
Flash Mode: DIO
Flash Size: 4MB (32Mb)
Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
Core Debug Level: None
PSRAM: Disabled
```

## Versões Testadas

### Ambiente de Teste
```
Arduino IDE: 2.2.1
ESP32 Core: 2.0.11
Windows 10: 64-bit
Hardware: ESP32 DevKit v1
```

### Bibliotecas Testadas
```
ESP Mail Client: 3.4.17
ArduinoJson: 6.21.3
LiquidCrystal I2C: 1.1.2
```

## Resolução de Problemas

### Erro: "Library not found"
```
Solução:
1. Verificar se a biblioteca está instalada
2. Reiniciar Arduino IDE
3. Verificar nome exato da biblioteca
4. Instalar versão mais recente
```

### Erro: "Compilation error"
```
Solução:
1. Verificar versão do ESP32 Core
2. Atualizar todas as bibliotecas
3. Verificar configurações do board
4. Limpar cache de compilação
```

### Erro: "Upload failed"
```
Solução:
1. Verificar porta serial
2. Verificar velocidade de upload
3. Pressionar botão BOOT durante upload
4. Verificar cabo USB
```

## Comandos de Instalação

### Via Arduino IDE
```
1. Sketch → Include Library → Manage Libraries
2. Buscar e instalar uma por uma
3. Verificar versões instaladas
```

### Via PlatformIO (Alternativa)
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    mobizt/ESP Mail Client@^3.4.0
    bblanchon/ArduinoJson@^6.21.0
    johnrickman/LiquidCrystal_I2C@^1.1.2
```

Com essas bibliotecas e configurações, o sistema minimal estará pronto para compilação e upload no ESP32.
