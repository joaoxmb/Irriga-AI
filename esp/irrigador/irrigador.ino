#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define relePorta D0
#define sensorPorta A0

StaticJsonDocument<1536> CONFIG; // Toda configuracao do sistema, umidade, planta, temperatura etc...
StaticJsonDocument<60> INFO; // Umidade captada pelo sensor

unsigned long previousMillis = 0;
const long interval = 1000; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiManager wm;
ESP8266WebServer server(80);

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  delay(10);
  
  pinMode(relePorta, OUTPUT); // Definindo que a porta do utilizada para o rele será do tipo saida/output.
  digitalWrite(relePorta, LOW);

  CONFIG["configurado"] = false;
  INFO["umidade"] = 0;

  iniciarWifi();

  server.on("/", HTTP_GET, getPage);  
  server.on("/api/config", HTTP_GET, getConfigAPI);  
  server.on("/api/config", HTTP_POST, postConfigAPI);
  server.on("/api/info", HTTP_GET, getInfoAPI);
  server.on("/api/reset", HTTP_POST, postResetAPI);
  server.onNotFound(paginaNaoEncontrada);

  server.begin();
  timeClient.begin();
  timeClient.setTimeOffset(0);
}

void loop() {
  MDNS.update();
  timeClient.update();
  server.handleClient();
  delay(1);
  
  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis) >= interval) {
    previousMillis = currentMillis;
    lerUmidade();
    regar();
  }
}

void paginaNaoEncontrada() {
  server.send(200, "text/html", F("<h1>Nada por aqui...</h1>"));
}
void getInfoAPI() {
  server.send(200, "text/json", INFO.as<String>());
}
void postConfigAPI() {
  StaticJsonDocument<1536> RECEPTOR;
  DeserializationError error = deserializeJson(RECEPTOR, server.arg("plain"));

  if (error) {
    Serial.print(F("Erro ao converter JSON:"));
    Serial.println(error.f_str());
    return;
  }
  if (!CONFIG["configurado"]) {
    CONFIG = RECEPTOR;
    CONFIG["configurado"] = true;
    server.send(200, "text/json", F("{\"status\": \"OK\"}"));
    Serial.println(F("Configuração recebida e alocada com sucesso!"))
    return;
  }

  CONFIG["semana"][0] = RECEPTOR["semana"][0] | false;
  CONFIG["semana"][1] = RECEPTOR["semana"][1] | false;
  CONFIG["semana"][2] = RECEPTOR["semana"][2] | false;
  CONFIG["semana"][3] = RECEPTOR["semana"][3] | false;
  CONFIG["semana"][4] = RECEPTOR["semana"][4] | false;
  CONFIG["semana"][5] = RECEPTOR["semana"][5] | false;
  CONFIG["semana"][6] = RECEPTOR["semana"][6] | true;

  server.send(200, "text/json", F("{\"status\": \"OK\"}"));
  Serial.println(F("Novo cronograma de rega recebido e alocado com sucesso!"))
}
void getConfigAPI() {
  server.send(200, "text/json", CONFIG.as<String>());
}
void getPage() {
  if (!CONFIG["configurado"]) {
    paginaConfig();
    return;
  }
  paginaDash();
}
void postResetAPI() {
  server.send(200, "text/json", F("{\"message\": \"O ESP está sendo resetado!\"}"));
  delay(1000);
  desconectarWifi();
}

void lerUmidade() {
  double ler = analogRead(sensorPorta);
  double porcentagem = (((ler - 1024) / (280 - 1024)) * 100);
  porcentagem = min(porcentagem, 100.0);
  porcentagem = max(porcentagem, 0.0);

  INFO["umidade"] = porcentagem;
}
void lidaComRele() {
  int horas = timeClient.getHours(); // horas atual

  if (horas > 9 && horas < 16) { // periodo que não queremos que regue
    return;
  }

  int hoje = timeClient.getDay();
  
  if (CONFIG["semana"][hoje]) { // Se o dia da semana estiver true para regar
    if (INFO["umidade"] <= CONFIG["umidade"]["min"]) {
      digitalWrite(relePorta, HIGH); // liga sistema

      return;
    }
  } else if (CONFIG["seguranca"] && INFO["umidade"] <= 5) {
    digitalWrite(relePorta, HIGH); // liga sistema

    return;
  }

  if(INFO["umidade"] >= CONFIG["umidade"]["max"]) {
    digitalWrite(relePorta, LOW); // Desliga sistema
  }

}
void regar() {
  if (CONFIG["configurado"] && INFO["umidade"] > 0) { // Se o sistema estiver configurado e o sensor estiver instalado corretamente
    lidaComRele();
    return;
  }

  digitalWrite(relePorta, LOW);
}

void iniciarWifi() {
  if (!wm.autoConnect("Irriga AI", "12345678")) {
    Serial.println("Falha ao se conectar com a internet.");
    ESP.restart();
  } 
  Serial.println(F("Conectado com a internet!"));
  
  MDNS.begin("irrigaai");
  MDNS.addService("http", "tcp", 80);
}
void desconectarWifi() {
  WiFi.disconnect();
  delay(2000);
  ESP.reset();
}

void requerirPaginaWeb(String serverPath, String adicional) {
  if(WiFi.status() != WL_CONNECTED){
    return;
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  http.begin(client, serverPath.c_str());

  int httpResponseCode = http.GET();

  if (httpResponseCode>0) {
    String payload = http.getString();
    server.send(200, "text/html", adicional + payload);
  } else {
    Serial.print("Erro ao capturar código do site: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void paginaConfig() { // Funcao que envia o site de configuracao para o usuario.
  requerirPaginaWeb("https://raw.githack.com/joaoxmb/irriga-ai/main/web/app/config/index.html", "<script>const _OPENAI_KEY = \"sk-\";</script>");
}
void paginaDash() { // Funcao que envia o site de inicio para o usuario.
  requerirPaginaWeb("https://raw.githack.com/joaoxmb/irriga-ai/main/web/app/dashboard/index.html", "");
}
