# Instruções de Instalação e Teste - Versão Minimal

## Preparação do Ambiente

### 1. Arduino IDE
```
- Instale Arduino IDE 2.0+
- Adicione ESP32 board package
- Selecione "ESP32 Dev Module"
```

### 2. Bibliotecas Necessárias
Instale via Library Manager:
```
- ESP Mail Client (v3.4.0+)
- ArduinoJson (v6.21.0+)
- LiquidCrystal I2C (v1.1.2+)
```

### 3. Configuração do ESP32
```
Board: ESP32 Dev Module
CPU Frequency: 240MHz
Flash Frequency: 80MHz
Flash Size: 4MB
Partition Scheme: Default 4MB with spiffs
```

## Instalação Passo a Passo

### Passo 1: Preparar Arquivos
1. Abra o projeto `exame_minimal.ino` no Arduino IDE
2. Edite o arquivo `data/config.json` com suas configurações reais
3. Verifique as conexões de hardware

### Passo 2: Configurar Email Gmail
1. Acesse Google Account Settings
2. Ative "2-Step Verification"
3. Vá em "Security" → "App passwords"
4. Gere uma nova senha de app
5. Use esta senha no campo `emailPassword`

### Passo 3: Upload do Código
1. Conecte o ESP32 ao computador
2. Selecione a porta correta
3. Compile e envie o código
4. Aguarde a conclusão

### Passo 4: Upload da Configuração
1. Instale plugin "ESP32 LittleFS Data Upload"
2. Edite o arquivo `data/config.json`
3. Use "Tools" → "ESP32 LittleFS Data Upload"

## Teste do Sistema

### Teste 1: Inicialização
**Objetivo**: Verificar se o sistema inicializa corretamente

**Passos**:
1. Abra Serial Monitor (115200 baud)
2. Pressione reset no ESP32
3. Observe as mensagens de inicialização

**Resultado Esperado**:
```
=== ESP32 Email Alert System - Minimal Version ===
Conectando WiFi...
WiFi conectado
IP: 192.168.x.x
Sincronizando horário NTP...
Horário sincronizado
Sistema pronto!
```

### Teste 2: Alarme Único
**Objetivo**: Testar detecção de alarme individual

**Passos**:
1. Pressione o botão uma vez
2. Observe LCD e Serial Monitor
3. Escute o buzzer (2 segundos)

**Resultado Esperado**:
```
LCD: "ALARME #1" / "HH:MM:SS"
Serial: "ALARME #1 DISPARADO! Hora: DD/MM/YYYY HH:MM:SS"
Buzzer: 2 segundos de som
```

### Teste 3: Contador de Alarmes
**Objetivo**: Verificar incremento correto

**Passos**:
1. Pressione o botão 4 vezes seguidas
2. Observe contadores no LCD
3. Verifique mensagens "Faltam X"

**Resultado Esperado**:
```
Alarme #1: "Faltam 4 para próximo email"
Alarme #2: "Faltam 3 para próximo email"
Alarme #3: "Faltam 2 para próximo email"
Alarme #4: "Faltam 1 para próximo email"
```

### Teste 4: Envio de Email
**Objetivo**: Testar envio automático de email

**Passos**:
1. Pressione o botão uma 5ª vez
2. Observe mensagem "Enviando email"
3. Aguarde confirmação no LCD
4. Verifique email na caixa de entrada

**Resultado Esperado**:
```
LCD: "Enviando email" / "Alarme #5"
Depois: "Email enviado!" / "Total: 1"
Email recebido com conteúdo correto
```

### Teste 5: Segundo Ciclo
**Objetivo**: Confirmar funcionamento contínuo

**Passos**:
1. Pressione o botão mais 5 vezes
2. Verifique se segundo email é enviado
3. Confirme total de emails = 2

**Resultado Esperado**:
```
Alarme #10: Segundo email enviado
LCD: "Email enviado!" / "Total: 2"
```

## Validação de Logs

### Conteúdo Esperado dos Logs
```
DD/MM/YYYY HH:MM:SS - Sistema iniciado
DD/MM/YYYY HH:MM:SS - Alarme #1 - Botão pressionado
DD/MM/YYYY HH:MM:SS - Alarme #2 - Botão pressionado
DD/MM/YYYY HH:MM:SS - Alarme #3 - Botão pressionado
DD/MM/YYYY HH:MM:SS - Alarme #4 - Botão pressionado
DD/MM/YYYY HH:MM:SS - Alarme #5 - Botão pressionado
DD/MM/YYYY HH:MM:SS - Email enviado - Alarme #5
```

## Problemas Comuns e Soluções

### Problema: WiFi não conecta
**Sintomas**: LCD mostra "Conectando WiFi" indefinidamente
**Solução**: 
- Verifique SSID e senha no config.json
- Teste com hotspot do celular
- Confirme se WiFi está em 2.4GHz

### Problema: Email não envia
**Sintomas**: LCD mostra "Erro SMTP" ou "Erro envio"
**Solução**:
- Confirme senha de app Gmail
- Verifique servidor SMTP e porta
- Teste com outro provedor (Outlook, Yahoo)

### Problema: Botão não responde
**Sintomas**: Nenhuma reação ao pressionar
**Solução**:
- Confirme conexão no pino 18
- Teste com multímetro
- Verifique se pull-up interno está funcionando

### Problema: LCD em branco
**Sintomas**: Backlight liga mas sem texto
**Solução**:
- Confirme endereço I2C (0x27)
- Teste com I2C scanner
- Verifique conexões SDA/SCL

### Problema: Reinicialização constante
**Sintomas**: ESP32 reseta sozinho
**Solução**:
- Monitore uso de memória
- Verifique fonte de alimentação
- Adicione delays em loops

## Checklist de Validação

### ✅ Pré-instalação
- [ ] Arduino IDE configurado
- [ ] Bibliotecas instaladas
- [ ] Arquivo config.json editado
- [ ] Hardware conectado corretamente

### ✅ Pós-instalação
- [ ] Sistema inicializa sem erros
- [ ] WiFi conecta corretamente
- [ ] NTP sincroniza horário
- [ ] LCD mostra mensagens
- [ ] Botão responde aos toques

### ✅ Funcionalidade
- [ ] Alarme individual funciona
- [ ] Contador incrementa corretamente
- [ ] Buzzer toca por 2 segundos
- [ ] Email enviado no 5º alarme
- [ ] Logs são gravados
- [ ] Sistema continua funcionando

### ✅ Qualidade
- [ ] Sem travamentos
- [ ] Sem vazamentos de memória
- [ ] Emails com conteúdo correto
- [ ] Horários precisos
- [ ] Interface responsiva

## Métricas de Sucesso

### Performance
- **Tempo de inicialização**: < 30 segundos
- **Resposta do botão**: < 100ms
- **Envio de email**: < 15 segundos
- **Uso de memória**: < 80% do heap

### Confiabilidade
- **Uptime**: > 24 horas sem reinicialização
- **Taxa de sucesso de email**: > 95%
- **Precisão do contador**: 100%
- **Sincronização de horário**: ± 1 segundo

Esta versão minimal garante que o sistema funcione de forma confiável mesmo em condições adversas de memória e processamento.
