/*
  Abaixo estão as bibliotecas utilizadas no sistema, são as seguintes:

    1. Wifi - conectar o esp ao wifi
    2. WebServer - cria um servidor local - disponibilizar de forma online um site
    3. DNS - altera o link do servidor que são numeros para "irrigaai.local"
    4. WifiManager - cria uma rede wifi própria para o usuario se conectar e conseguir conectar o esp no wifi diretamente do celular
    5. ArduinoJson - responsavel por deixar com que conseguimos trabalhar com json na linguagem do esp - c++
    6. HTTPClient - identifica o que o usuario está acessando do seu servidor e redireciona-lo - exemplo: o usuario acessa o <http://irriga.local/>, 
    ele identifica que o usuario está pedindo/GET para acessar a página "/". Com isso podemos redirecionar para a pagina inicial
    7. HTTPClientSecure - permite o sistema/esp pedir informacoes da internet - acessar
    8. NTPClient - responsavel por pegar o horario atual da internet
    9. WifiUdp - responsavel por pegar o horario atual da internet
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

/*
  Define a porta de saida do rele e de entrada de informacoes do sensor
*/
#define relePorta D0
#define sensorPorta A0

/*
  Criamos variaveis json e dentro de <> colocamos o tamanho de json - para calcular o tamanho do json usamos o site disponibilizado pelo criador da biblioteca ArduinoJson: 
  
  <https://arduinojson.org/v6/assistant/#/step2>

  @CONFIG guarda todas as informacoes do sistema, como a planta, a umidade minima e maxima etc.
  @INFO guarda por enquanto apenas a informacao da % de umidade do solo captada pelo sensor.
*/
StaticJsonDocument<1536> CONFIG;
StaticJsonDocument<60> INFO;

/*
  Muito complexo de escrever, despois explico. 
  De forma rapida, é responsavel por criar delay no código sem atrapalhar na velocidade de resposta do servidor ao acessar o site do sistema.

  @previusMillis é os segundos atuais do esp
  @interval é o intervalo em milisegundos que o código esperara antes de executar de novo
*/
unsigned long previousMillis = 0;
const long interval = 1000; 

/*
  Define as variaveis necessarias para pegar o horario do brasil da internet, pois o esp não possui relógio proprio - esse código está disponivel no site da própria biblioteca:

  <https://github.com/arduino-libraries/NTPClient>

*/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

/*
  Define as variaveis para o uso da biblioteca WifiManager e inicia o servidor do esp na porta (90)
*/
WiFiManager wm;
ESP8266WebServer server(80);

/*
  FUNCAO PRINCIPAL

  Funcao setup, é o primeiro código a rodar quando ligamos o esp. Ele executa apenas uma vez. É importante para executar partes importante que precisam ser executados apenas uma vez
*/
void setup() {
  /*
    1. Incia o monitor serial - imprime os valores que quisermos em uma area visivel no aplicativo do arduino ide no pc
    2. Define o modo wifi utilizado pelo esp - Foi pego do site oficial a biblioteca Wifi
    3. Adiciona um delay de 10 milisegundos
  */
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  delay(10);
  
  /*
    Define que a porta do rele será do tipo saida - apenas será enviado dados para o rele e não podera receber dados do rele.
    Escreve que o rele estará desligado
  */
  pinMode(relePorta, OUTPUT);
  digitalWrite(relePorta, LOW);

  /*
    Ao iniciar o esp escrevemos na nossa variavel json de CONFIG que o sistema nAo está configurado - configurado = false - de falso
    Escrevemos tambem que a umidade começa em 0
  */
  CONFIG["configurado"] = false;
  INFO["umidade"] = 0;

  // Autodescritivo
  iniciarWifi();

  /*
    Essa é a parte que criamos a nossa prorpia API, é onde falamos para o servidor o que ele deve responder para quem estiver acessando - usuario. Ou seja, quando acessarmos <irrigaai.local> ele identificara que você está pedindo/HTTP_GET a pagina do site, e quando ele detecta isso ele aexecuta outra funcão

    Para isso usamos a funcaoc server.on() - devemos passar para essa funcao o link de acesso, o método e a funcao que ele executara quando for acessado.

    Em server.on("/api/config", HTTP_GET, getConfigAPI) estamos dizendo que:
      - "/api/config" - quando o servidor for acessado pelo link /api/config, ou seja, irrigaai.local/api/config
      - HTTP_GET - estamos dizendo que só será executado a funcão se o cliente pediu e não postou nada - não enviou nenhuma informacao, apenas pediu informacao.
      - getConfigAPI - funcao que será executada se todos os outros forem satisfeitos

    Seguindo a ordem temos:
      1. pagina de configuracao ou pagina de inicio
      2. mostra em json as configuracoes do sistema
      3. link para enviarmos as cofiguracoes criadas pela IA em formato de json
      4. mostra em json a informacao de umidade do solo
      5. quando enviamos qualquer informacao mesmo que nada para essse link ele executa a funcao de desconectar o wifi do esp/sistema
      6. onNotFound - quando acessar um link que não definimos para responder alguma coisa - quando acessamos por exemplo: irrigaai.local/camila - será mostrado uma mensagem de erro
  */
  server.on("/", HTTP_GET, getPage);  
  server.on("/api/config", HTTP_GET, getConfigAPI);  
  server.on("/api/config", HTTP_POST, postConfigAPI);
  server.on("/api/info", HTTP_GET, getInfoAPI);
  server.on("/api/reset", HTTP_POST, postResetAPI);
  server.onNotFound(paginaNaoEncontrada);

  /*
    funcao para iniciar o servidor do sistema
    funcao para iniciar a captura do horario
  */
  server.begin();
  timeClient.begin();
  timeClient.setTimeOffset(0);
}

/*
  FUNCAO PRINCIPAL

  loop é a funcao que será executada milhares de vezes enquanto o esp estiver ligado, nela colocamos as funcao que precisam ser executadas mais de uma vez, e sempre - por exemplo a funcao de ler a umidade da terra, devemos fazer isso constantemente para sabermos se está secando ao longo do tempo ou não.
*/
void loop() {
  /*
    1. Atualiza o DNS
    2. Atualiza o horario
    3. Detecta a API / Servidor do sistema foi acessado pelos links - irrigaai.local, irrigaai.local/api/config...
    4. Adiciona delay de 1 milisegundos. Um cara comprovou que 1 milessegundo consegue fazer o esp ficar mais rapido, já que executar a funcao varias vezes por segundos pode deixar ele lento.
  */
  MDNS.update();
  timeClient.update();
  server.handleClient();
  delay(1);
  
  /*
    Complexo, depois te explico melhor.
    Criamos um delay sem afetar a velocidade de resposta do site, fazemos isso pegando os segundos atuais do esp - esses segundos é os segundos que o esp começa a contar ao ligar ele.

    Depois do delay será executado:

    @lerUmidade - executa a funcao que irá ler a umidade vinda do sensor e converte para porcentagem
    @ergar - funcao responsavel por identificar o momento de rega e ligar e desligar o rele para regar a planta
  */
  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis) >= interval) {
    previousMillis = currentMillis;
    lerUmidade();
    regar();
  }
}

/*
  Aqui definimos as funçoes que iram retornar o site para o usuario que estiver acessando o serevidor do ESP. São elas que serao executadas quando o usuario acessar por exemplo: <http://irrigaai.local/> ou <http://irrigaai.local/api/info> etc.
  Se você reparar, estamos chamando as funçoes abaixo la na funcao principal SETUP

  @paginaNaoEncontrada - envia para o usuario que acessou um link inexistente, um site que contem apenas um aviso - "Nada por aqui...";

  @getInfoAPI - envia para o usuario que acessou o link irrigaai.local/api/info as informacoes contida no json da nossa variavel principal INFO, declarada nas primeiras linhas como: StaticJsonDocument<60> INFO;

  @postConfigAPI - quando o site envia os dados para o sistema, ele envia no metodo POST, essa funcao tem como objetivo pegar o json que o site enviou para o sistema e armazenar no ESP. O código usado nessa funcao foi criado com base no código disponibilizado pela própria biblioteca que permite usar JSON dentro do ESP. <https://arduinojson.org/>;

  @getConfigAPI - funcao que será executado quando queremos/requisitamos o link <http://irrigaai.local/api/config>, ao pedir, o sistema irá enviar um JSON contendo todas as informacoes do sistema: planta, umidade min e max etc;

  @getPage - essa funcao verifica se o sistema já esta configurado ou não, se não estiver configurado o sistema irá enviar para o usuario a pagina de configuracao, caso contrario será enviado a pagina de informacoes/dashboard.

  @postResetAPI - quando o site enviar uma solicitacao no método POST para o link <http://irrigaai.local/api/reset> o sistema irá executar a funcao de resetar o sistema, isso inclui remover as configuracoes do wifi.
*/
void paginaNaoEncontrada() {
  // envia o site com aviso
  server.send(200, "text/html", F("<h1>Nada por aqui...</h1>"));
}
void getInfoAPI() {
  // envia JSON contendo a umidade captada pelo sensor
  server.send(200, "text/json", INFO.as<String>());
}
void postConfigAPI() {
  // cria uma variavel para receber os dados temporariamente
  StaticJsonDocument<1536> RECEPTOR;
  // capta os dados enviado pelo site atraves da funcao server.arg("plain"), converte e insere os dados dentro da variavel RECPTOR
  DeserializationError error = deserializeJson(RECEPTOR, server.arg("plain"));

  // detecta se aconteceu algum erro, se acontecer a funcao para e envia uma mensagem de erro no console
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  // se não acontecer nenhum erro
  // define a variavel "configurado" como verdadeiro - para que o sistema identifique que já está configurado
  CONFIG["configurado"] = true;
  // a funcao mesclarJson pega o JSON com as configuracoes que já tem no sistema e mescla com as novas configuracoes enviadas pelo usuario. Quando o sistema não está configurado ele irá mesclar com o nada, mas quando o sistema está configurado e alteramos o plano semanal de rega, ele mescla o novo plano semanal enviado pelo site com as configuracoes que já existe dentro do sistema.
  mesclarJson(CONFIG, RECEPTOR);
  // envia um JSON com as configuracoes atualizada para finalizar a funcao
  server.send(200, "text/json", CONFIG.as<String>());
}
void getConfigAPI() {
  // envia um JSON contendo todas as configuracoes no sistema
  server.send(200, "text/json", CONFIG.as<String>());
}
void getPage() {
  // se o sistema não estiver configurado, mostre a pagina de configuracao
  if (!CONFIG["configurado"]) {
    paginaConfig();
    return;
  }
  // se o sistema estiver configurado, mostre a pagina de informacoes/dashboard
  paginaDash();
}
void postResetAPI() {
  // envia um JSON com uma mensagem
  server.send(200, "text/json", F("{\"message\": \"O ESP está sendo resetado!\"}"));
  // espera 1 segundo
  delay(1000);
  // executa a funcao de desconectar wifi
  desconectarWifi();
}
/*
  Funcao responsavel por ler a umidade atual da solo e converter em porcentagem, pois o sensor no envia um numero inteiro que varia entre 1024 - 0, 1024 sendo 0% e 0 sendo 100%. No entando dificilmente ele chega a 0, o maximo que chegou foi entre 250 a 200, entao consideramos 200 como o 100%.
*/
void lerUmidade() {
  // cria uma variavel que terá o valor do sensor, a funcao analogRead(sensorPorta); é responsavel por captar o valor vindo do sensor.
  double ler = analogRead(sensorPorta);
  // faz um calculo matematico para converter os numeros de 1024 a 200 para 0% a 100%
  double porcentagem = (((ler - 1024) / (200 - 1024)) * 100);
  // define que o numero da porcentagem não poderá passar de 100%
  porcentagem = min(porcentagem, 100.0);
  // define que o numero da porcentagem não poderá ser menor que 0%
  porcentagem = max(porcentagem, 0.0);

  // insere o valor calculado da porcentagem na variavel principal INFO
  INFO["umidade"] = porcentagem;
}
/*
  Funcao responsavel por ligar ou desligar o rele, essa funcao leva em consideracao a umidade atual captada pelo sensor e a umidade minima e maxima da planta.
*/
void lidaComRele() {
  // Se a umidade por menor ou igual a minima ele ligará o rele
  if (INFO["umidade"] <= CONFIG["umidade"]["min"]) {
    digitalWrite(relePorta, HIGH); // liga sistema
    return;
  }
  // Se a umidade por maior ou igual a maxima ele desligara o rele
  if(INFO["umidade"] >= CONFIG["umidade"]["max"]) {
    digitalWrite(relePorta, LOW); // Desliga sistema
  }
}
/*
  Funcao responsavel por identificar o momento que o rele deve ser ligado ou desligado com base nos dados de configuracao da planta, ele é executado continuamente na funcao principal LOOP, para que fique sempre verificando se é o momento de rega ou o momento para desligar.
*/
void regar() {
  // se o sistema estiver configurado e a umidade captada pelo sensor for maior que 0% passará para a proxima etapa
  if (CONFIG["configurado"] && INFO["umidade"] > 0) {
    // pega o dia atual com a funcao timeClient.getDay(); e armazena na variavel hoje. O valor que a funcao timeClient.getDay(); nos da é de 0 a 6, 0 correspondendo a domingo, 1 correspondendo a segunda-feira e por assim em diante.
    int hoje = timeClient.getDay();

    // Se o dia da semana estiver true/verdadeiro para regar executa a funcao @lidaComRele
    if (CONFIG["semana"][hoje]) { 
      lidaComRele();

    // caso o dia da semana não está verdadeiro para regar, verifica se o modo de seguranca está ativo, se estiver ativo será executado a funcao @lidaComRele
    } else if (CONFIG["seguranca"]) {
      lidaComRele();
    }
    return;
  }
  // se o sistema não estiver configurado ou a umidade captada pelo sensor for igual a 0 será desligado o rele.
  if (INFO["umidade"] == 0) {
    digitalWrite(relePorta, LOW);
  }
}
/*
  Funcao responsavel por fazer a conexao do wifi e a inicializacao do DNS

  WIFI - inicia o wifi do esp.
  DNS - responsavel por alterar o link do servidor, de numeros para como nos queremos. por exemplo: de <1.000.00:5000> para <http://irrigaai.local>

  Esse trecho do código foi criado com base nos códigos fornecidos pela próprias bibliotecas de wifi e dns. <https://github.com/tzapu/WiFiManager> e <https://gist.github.com/Cyclenerd/7c9cba13360ec1ec9d2ea36e50c7ff77>
*/
void iniciarWifi() {
  if (!wm.autoConnect("Irriga AI", "12345678")) {
    Serial.println("Failed to connect");
    ESP.restart();
  } 
  Serial.println("connected...yeey :)");
  // inicia o DNS com o nome irrigaai, que nos permitirá acessar <http://irrigaai.local/>
  MDNS.begin("irrigaai");
  MDNS.addService("http", "tcp", 80);
}
/*
  Funcao responsavel por desconectar o wifi do esp, excluir as configuracoes de wifi existente no esp.
*/
void desconectarWifi() {
  // desconecta o wifi
  WiFi.disconnect();
  // espera 2 segundos
  delay(2000);
  // reseta o esp
  ESP.reset();
}

/*
  Funcao responsavel por mesclar variaveis JSON, utilizada na funcao @postConfigAPI para mesclar as configuracoes atuais com o novo plano semanal de rega, quando mudado pelo site do sistema.

  Essa funcão é inteiramente criada pelo próprio criador da biblioteca ArduinoJson, disponivel em: <https://arduinojson.org/v6/how-to/merge-json-objects/>
*/
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

/*
  Funcao responsavel pegar o código do site da internet e enviar para o usuario quando necessario acessar a pagina de configuracao ou a pagina de informacao/dashboard. Dessa forma o site do sistema fica apenas na internet, e não dentro do esp ocupando muito espaco de armazenamento.

  Assim como outras muitas funcoes, essa funcao é praticamente toda feita pelo mesmo criador da biblioteca ESP8266HTTPClient, biblioteca essa que é responsavel por permiter que o esp capture informacoes da internet.

  Na hora de executar essa funcao devemos enviar 2 parametros para ela:
  - Link que ele irá abrir
  - Código de site adicional

  Com isso a funcao irá acessar o link que passamos para ela e irá capturar tudo que estiver nesse link - todo código
*/
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
    // funcao responsavel por enviar o site para o usuario depois que conseguiu pegar o código da internet.
    server.send(200, "text/html", adicional + payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

/*
  Funcoes responsavel por enviar para o usuario o site do sistema, para isso ele executa a funcao para pegar o código do site da internet.

  @paginaConfig - Pega o código do site de configuracao, adiciona o senha secreta da Open Ai para conseguir se comunicar com o GPT e envia para o usuario
  @paginaDash - pega o código do site de informacoes/dashboard e envia para o usuario o site
*/
void paginaConfig() { 
  requerirPaginaWeb("https://raw.githack.com/joaoxmb/irriga-ai/main/web/app/config/index.html", "<script>const _OPENAI_KEY = \"\";</script>");
}
void paginaDash() {
  requerirPaginaWeb("https://raw.githack.com/joaoxmb/irriga-ai/main/web/app/dashboard/index.html", "");
}
