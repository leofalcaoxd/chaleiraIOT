#include "arduino_secrets.h"

#include "thingProperties.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h> // Inclui a biblioteca MQTT para comunicaÃ§Ã£o

// VariÃ¡veis globais
unsigned long timer = 0; // Armazena o tempo em que o relÃ© foi ativado
bool timerActive = false; // Controle de estado do temporizador
const int relayPin = 14; // Pino GPIO onde o relÃ© estÃ¡ conectado
unsigned long delayTime = 600000; // Tempo de ativaÃ§Ã£o do relÃ© em milissegundos (10 minutos)
AsyncWebServer server(80); // Servidor web configurado para operar na porta 80

unsigned long wifiCheckTimer = 0; // Timer para checar a conexÃ£o Wi-Fi
const unsigned long wifiCheckInterval = 60000; // Intervalo para checar a conexÃ£o Wi-Fi (1 minuto)
unsigned long apDisableTimer = 0; // Timer para desativar o Access Point (AP) apÃ³s a conexÃ£o Wi-Fi
bool apDisableTimerActive = false; // Controle do estado do timer para desativar o AP

// VariÃ¡veis para controle de log
bool printRemainingTime = false; // Flag para imprimir o tempo restante apenas uma vez

// ConfiguraÃ§Ãµes do MQTT
const char* mqttServer = "b37.mqtt.one"; // EndereÃ§o do servidor MQTT
const int mqttPort = 1883; // Porta do servidor MQTT
const char* mqttUser = "2aceiu7365"; // Nome de usuÃ¡rio do MQTT
const char* mqttPassword = "16egknoswy"; // Senha do MQTT
const char* mqttTopic = "2aceiu7365/"; // TÃ³pico do MQTT

const char* defaultSSID = "CapitalNet Fibra (99224-9595)"; // Nome do Wi-Fi padrÃ£o
const char* defaultPassword = "C@130215!!"; // Senha do Wi-Fi padrÃ£o

WiFiClient espClient; // Cliente Wi-Fi
PubSubClient client(espClient); // Cliente MQTT

void setup() {
  Serial.begin(9600); // Inicializa a comunicaÃ§Ã£o serial
  while (!Serial) {
    ; // Aguarda a conexÃ£o serial
  }

  Serial.print("Conectado Ã  porta serial: ");
  Serial.println(Serial);

  initProperties(); // Inicializa as propriedades da nuvem

  pinMode(relayPin, OUTPUT); // Configura o pino do relÃ© como saÃ­da
  digitalWrite(relayPin, LOW); // Garante que o relÃ© esteja desligado inicialmente

  ArduinoCloud.begin(ArduinoIoTPreferredConnection); // Inicia a conexÃ£o com a nuvem Arduino

  setDebugMessageLevel(2); // Define o nÃ­vel de mensagens de depuraÃ§Ã£o
  ArduinoCloud.printDebugInfo(); // Imprime informaÃ§Ãµes de depuraÃ§Ã£o da nuvem

  // Tenta conectar-se Ã  rede Wi-Fi padrÃ£o
  WiFi.begin(defaultSSID, defaultPassword);
  int attempts = 0;
  const int maxAttempts = 10;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(1000); // Aguarda 1 segundo entre tentativas
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao Wi-Fi!");
    mostrarInformacoesWiFi(); // Mostra informaÃ§Ãµes da rede Wi-Fi
    apDisableTimer = millis(); // Inicia o timer para desativar o AP
    apDisableTimerActive = true;
  } else {
    Serial.println("\nNÃ£o foi possÃ­vel conectar ao Wi-Fi. Iniciando modo AP...");
    configurarAP(); // Inicia o modo Access Point (AP) se nÃ£o conseguir conectar
  }

  wifiCheckTimer = millis(); // Inicializa o timer para checar a conexÃ£o Wi-Fi
  client.setServer(mqttServer, mqttPort); // Define o servidor MQTT
  client.setCallback(mqttCallback); // Define a funÃ§Ã£o callback para mensagens recebidas
}

void loop() {
  ArduinoCloud.update(); // Atualiza o status da nuvem

  if (!client.connected()) {
    reconnectMQTT(); // Tenta reconectar ao servidor MQTT se nÃ£o estiver conectado
  }
  client.loop(); // MantÃ©m a conexÃ£o MQTT ativa

  if (timerActive && (millis() - timer >= delayTime)) {
    chaleira = false; // Desliga a chaleira apÃ³s o tempo de delay
    onChaleiraChange(); // Atualiza o estado do relÃ©
  }

  if (millis() - wifiCheckTimer >= wifiCheckInterval) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi desconectado. Iniciando modo AP...");
      configurarAP(); // Reconfigura o AP se o Wi-Fi estiver desconectado
    } else {
      Serial.println("Wi-Fi ainda conectado, checagem OK.");
      mostrarInformacoesWiFi(); // Mostra informaÃ§Ãµes da rede Wi-Fi
      desativarAP(); // Desativa o AP se o Wi-Fi estiver conectado
    }
    wifiCheckTimer = millis(); // Reinicia o timer de checagem Wi-Fi
  }

  // Verifica se o timer de 10 segundos para desativar o AP estÃ¡ ativo
  if (apDisableTimerActive && (millis() - apDisableTimer >= 10000)) { // 10 segundos
    desativarAP(); // Desativa o AP
    apDisableTimerActive = false; // Desativa o timer
  }
}

// FunÃ§Ã£o para alterar o tempo do relÃ©
void alterarTempoRele(unsigned long novoTempo) {
  delayTime = novoTempo; // Define o novo tempo de ativaÃ§Ã£o do relÃ©
  Serial.print("Novo tempo do relÃ© ajustado para: ");
  Serial.print(delayTime / 1000 / 60); // Exibe o tempo em minutos
  Serial.println(" minutos.");
}

void onChaleiraChange() {
  if (chaleira) {
    digitalWrite(relayPin, HIGH); // Liga o relÃ©
    timer = millis(); // Armazena o tempo atual
    timerActive = true; // Ativa o temporizador
    Serial.println("RelÃ© ligado. Temporizador iniciado.");
    client.publish(mqttTopic, "RELE LIGADO"); // Publica o estado no MQTT
  } else {
    digitalWrite(relayPin, LOW); // Desliga o relÃ©
    timerActive = false; // Desativa o temporizador
    Serial.println("RelÃ© desligado.");
    client.publish(mqttTopic, "RELE DESLIGADO"); // Publica o estado no MQTT
  }
}

// FunÃ§Ã£o para configurar o Access Point (AP)
void configurarAP() {
  IPAddress local_IP(192, 168, 4, 1); // Define o IP local do AP
  IPAddress gateway(192, 168, 4, 1); // Define o gateway do AP
  IPAddress subnet(255, 255, 255, 0); // Define a mÃ¡scara de sub-rede do AP

  WiFi.softAPConfig(local_IP, gateway, subnet); // Configura o AP
  WiFi.softAP("ESP32-RECONFIGURACAO"); // Nome do AP

  // Configura a pÃ¡gina de configuraÃ§Ã£o do AP
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"(
      <!DOCTYPE html>
      <html lang='pt-BR'>
      <head>
        <meta charset='UTF-8'>
        <meta name='viewport' content='width=device-width, initial-scale=1.0'>
        <title>CONFIGURACAO DE WI-FI</title>
      </head>
      <body>
        <h2>CONFIGURACAO DE WI-FI</h2>
        <form action='/save'>
          <label for='ssid'>NOME DA REDE (SSID):</label><br>
          <input type='text' id='ssid' name='ssid' value=''><br>
          <label for='pass'>SENHA:</label><br>
          <input type='password' id='pass' name='pass' value=''><br><br>
          <input type='submit' value='SALVAR'>
        </form>
      </body>
      </html>
    )");
  });

  // Processa a solicitaÃ§Ã£o de salvamento de configuraÃ§Ãµes Wi-Fi
  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request){
    String ssid = request->getParam("ssid")->value();
    String pass = request->getParam("pass")->value();
    if (ssid.length() > 0 && pass.length() > 0) {
      Serial.print("Salvando nova rede: ");
      Serial.println(ssid);
      WiFi.begin(ssid.c_str(), pass.c_str()); // Tenta conectar-se Ã  nova rede
      int attempts = 0;
      const int maxAttempts = 10;
      while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(1000); // Aguarda 1 segundo entre tentativas
        Serial.print(".");
        attempts++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado a nova rede Wi-Fi.");
        mostrarInformacoesWiFi(); // Mostra informaÃ§Ãµes da nova rede Wi-Fi
        request->send(200, "text/html", "<h2>Conectado com sucesso! Reinicie o dispositivo.</h2>");
        desativarAP(); // Desativa o AP apÃ³s conectar-se Ã  nova rede
      } else {
        request->send(200, "text/html", "<h2>InformaÃ§Ãµes salvas, mas nÃ£o foi possÃ­vel conectar.</h2>");
      }
    } else {
      request->send(200, "text/html", "<h2>Por favor, preencha todos os campos.</h2>");
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/"); // Redireciona para a pÃ¡gina inicial em caso de erro
  });

  server.begin(); // Inicia o servidor web
}

// FunÃ§Ã£o para desativar o Access Point (AP)
void desativarAP() {
  if (WiFi.softAPgetStationNum() == 0 && WiFi.status() == WL_CONNECTED) {
    WiFi.softAPdisconnect(true); // Desconecta o AP
    Serial.println("AP desativado, dispositivo conectado a uma rede Wi-Fi.");
  }
}

// FunÃ§Ã£o para mostrar informaÃ§Ãµes da conexÃ£o Wi-Fi
void mostrarInformacoesWiFi() {
  Serial.println("Rede Wi-Fi.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP()); // Exibe o IP local
}

// FunÃ§Ã£o de callback para mensagens recebidas via MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (message == "ON") {
    chaleira = true;
    onChaleiraChange(); // Liga o relÃ©
  } else if (message == "OFF") {
    chaleira = false;
    onChaleiraChange(); // Desliga o relÃ©
  } else if (message.startsWith("SET_TIME")) {
    String timeStr = message.substring(9); // ObtÃ©m o tempo apÃ³s "SET_TIME "
    unsigned long novoTempo = timeStr.toInt() * 60000; // Converte minutos em milissegundos
    alterarTempoRele(novoTempo); // Ajusta o tempo do relÃ©
  }
}

// FunÃ§Ã£o para reconectar ao servidor MQTT
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Conectado ao MQTT.");
      client.subscribe(mqttTopic); // Inscreve-se no tÃ³pico MQTT
    } else {
      Serial.print("Falha na conexÃ£o, rc=");
      Serial.print(client.state()); // Mostra o cÃ³digo de erro
      Serial.println(" Tente novamente em 5 segundos.");
      delay(5000); // Aguarda 5 segundos antes da prÃ³xima tentativa
    }
  }
}
