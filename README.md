# ESP32 Email Alert System - Versão Minimal

## Descrição
Esta é uma versão simplificada do sistema de alarme com email automático para ESP32. O foco está apenas na funcionalidade essencial: **detectar alarmes e enviar emails a cada 5 ocorrências**.

## Funcionalidades Incluídas
- ✅ Detecção de alarme via botão
- ✅ Envio de email automático a cada 5 alarmes
- ✅ Display LCD com feedback
- ✅ Buzzer para sinalização
- ✅ Configuração via JSON
- ✅ Log de eventos
- ✅ Sincronização de horário NTP
- ✅ Proteção thread-safe para LCD

## Funcionalidades Removidas
- ❌ Servidor web
- ❌ Página HTML
- ❌ LED externo
- ❌ Interface web completa
- ❌ APIs REST
- ❌ Funcionalidades extras

## Hardware Necessário
- ESP32
- Display LCD I2C (endereço 0x27)
- Botão pushbutton (pino 18)
- Buzzer (pino 12)
- Resistor pull-up interno (usado no botão)

## Conexões
```
ESP32          Componente
GPIO 18   -->  Botão (pull-up interno)
GPIO 12   -->  Buzzer
GPIO 21   -->  SDA (LCD I2C)
GPIO 22   -->  SCL (LCD I2C)
VCC       -->  3.3V (LCD e componentes)
GND       -->  GND (comum)
```

## Configuração

### 1. Arquivo de Configuração
Edite o arquivo `data/config.json` com suas configurações:

```json
{
  "wifiSsid": "SeuWiFi",
  "wifiPassword": "SuaSenhaWiFi",
  "ntpServer": "time.google.com",
  "timezoneOffset": -3,
  "emailAlarm": "destinatario@exemplo.com",
  "smtpServer": "smtp.gmail.com",
  "smtpPort": 587,
  "emailSender": "seuemail@gmail.com",
  "emailPassword": "suasenhaapp",
  "emailSubject": "Alerta ESP32 - Sistema de Alarme"
}
```

### 2. Configuração de Email Gmail
Para usar Gmail, você precisa:
1. Ativar autenticação de 2 fatores
2. Gerar senha de app específica
3. Usar a senha de app no campo `emailPassword`

### 3. Upload dos Arquivos
1. Edite o arquivo `data/config.json` com suas configurações
2. Faça upload do arquivo `config.json` para o LittleFS do ESP32
3. Compile e envie o código `exame_minimal.ino`

## Funcionamento

### Sequência de Operação
1. **Inicialização**: Sistema conecta ao WiFi e sincroniza horário
2. **Espera**: Sistema fica aguardando pressionar o botão
3. **Alarme**: A cada pressão do botão:
   - Contador de alarmes é incrementado
   - Buzzer é acionado por 2 segundos
   - LCD mostra informações do alarme
   - Evento é registrado no log
4. **Email**: A cada 5 alarmes, um email é enviado automaticamente

### Mensagens LCD
- `"Sistema pronto!"` / `"Pressione botão"` - Sistema inicializado
- `"ALARME #X"` / `"HH:MM:SS"` - Alarme disparado
- `"Faltam X"` / `"para próximo email"` - Contador para próximo email
- `"Enviando email"` / `"Alarme #X"` - Enviando email
- `"Email enviado!"` / `"Total: X"` - Email enviado com sucesso

### Conteúdo do Email
```
🚨 ALERTA DO SISTEMA ESP32 🚨

RESUMO DO ALERTA:
- Total de alarmes: X
- Última ocorrência: DD/MM/YYYY HH:MM:SS
- IP do ESP32: X.X.X.X
- MAC Address: XX:XX:XX:XX:XX:XX

PRÓXIMO EMAIL:
- Será enviado no alarme #X

Sistema ESP32 - Gerado automaticamente
Hora: DD/MM/YYYY HH:MM:SS
```

## Logs
O sistema gera logs automáticos em `/alarm_log.txt` com:
- Inicialização do sistema
- Cada alarme disparado
- Emails enviados
- Erros de conexão/envio

## Bibliotecas Necessárias
Instale via Library Manager do Arduino IDE:
- `ESP32` (board package)
- `LittleFS` (incluído no ESP32)
- `ArduinoJson` (by Benoit Blanchon)
- `LiquidCrystal I2C` (by Frank de Brabander)
- `ESP Mail Client` (by Mobizt)

## Resolução de Problemas

### WiFi não conecta
- Verifique SSID e senha no config.json
- Certifique-se que o WiFi está acessível

### Email não envia
- Verifique configurações SMTP
- Confirme senha de app do Gmail
- Verifique conexão com internet

### LCD não funciona
- Confirme endereço I2C (0x27)
- Verifique conexões SDA/SCL
- Teste com I2C scanner

### Botão não responde
- Verifique conexão no pino 18
- Confirme que está usando pull-up interno
- Teste debounce (200ms)

## Vantagens desta Versão
- **Menor uso de memória** - Apenas funcionalidades essenciais
- **Mais estável** - Menos pontos de falha
- **Fácil teste** - Funcionalidade isolada
- **Debugger simples** - Logs detalhados
- **Deploy rápido** - Configuração mínima

## Teste Recomendado
1. Carregue o código no ESP32
2. Pressione o botão 5 vezes rapidamente
3. Verifique se o email foi enviado
4. Pressione mais 5 vezes para confirmar segundo email
5. Monitore logs no Serial Monitor

## Memória Estimada
- **Flash**: ~1.2MB (incluindo bibliotecas)
- **RAM**: ~150KB (durante operação)
- **Heap livre**: ~180KB (após inicialização)

Esta versão minimal garante funcionamento confiável do sistema de email automático mesmo em condições de pouca memória disponível.
