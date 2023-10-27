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

unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiManager wm;
ESP8266WebServer server(80); // Inicia servidor na porta 80

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
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  CONFIG["configurado"] = true;
  mesclarJson(CONFIG, RECEPTOR);
  server.send(200, "text/json", CONFIG.as<String>());
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
  double porcentagem = (((ler - 1024) / (200 - 1024)) * 100);
  porcentagem = min(porcentagem, 100.0);
  porcentagem = max(porcentagem, 0.0);

  INFO["umidade"] = porcentagem;
}
void lidaComRele() {
  if (INFO["umidade"] <= CONFIG["umidade"]["min"]) {
    digitalWrite(relePorta, HIGH); // liga sistema
    return;
  }
  if(INFO["umidade"] >= CONFIG["umidade"]["max"]) {
    digitalWrite(relePorta, LOW); // Desliga sistema
  }
}
void regar() {
  if (CONFIG["configurado"] && INFO["umidade"] > 0) { // Se o sistema estiver configurado e o sensor estiver instalado corretamente
    int hoje = timeClient.getDay();

    if (CONFIG["semana"][hoje]) { // Se o dia da semana estiver true para regar
      lidaComRele();
    } else if (CONFIG["seguranca"]) { // Se o dia não estiver para regar e o modo de seguranca estiver ativado
      lidaComRele();
    }
  }
}

void iniciarWifi() {
  if (!wm.autoConnect("Irriga AI", "12345678")) {
    Serial.println("Failed to connect");
    ESP.restart();
  } 
  Serial.println("connected...yeey :)");
  
  MDNS.begin("irrigaai");
  MDNS.addService("http", "tcp", 80);
}
void desconectarWifi() {
  WiFi.disconnect();
  delay(2000);
  ESP.reset();
}

void mesclarJson(JsonVariant dst, JsonVariantConst src) {
  if (src.is<JsonObjectConst>()) {
    for (JsonPairConst kvp : src.as<JsonObjectConst>()) {
      if (dst[kvp.key()]) {
        mesclarJson(dst[kvp.key()], kvp.value());
      } else {
        dst[kvp.key()] = kvp.value();
      }
    }
  } else {
    dst.set(src);
  }
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
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void paginaConfig() { // Funcao que envia o site de configuracao para o usuario.
  requerirPaginaWeb("https://raw.githack.com/joaoxmb/irriga-ai/main/web/app/config/index.html", "<script>const _OPENAI_KEY = \"\";</script>");
}
void paginaDash() { // Funcao que envia o site de inicio para o usuario.
  requerirPaginaWeb("https://raw.githack.com/joaoxmb/irriga-ai/main/web/app/dashboard/index.html", "");
}
