import espRequest from "../../scripts/esp-request.js";
import aiRequest from "../../scripts/ai-request.js";
import chat from "../../scripts/chat.js";

let PARAMS = {};
let SYSTEM = {};

async function definirParametros() {
  const prompt = [
    {
      "role": "system",
      "content": "Você receberá o nome de uma muda em português e receberá o nome de uma cidade."
    },
    {
      "role": "user",
      "content": "planta=" + SYSTEM.plant
    },
    {
      "role": "user",
      "content": "cidade=" + SYSTEM.city
    },
    {
      "role": "system",
      "content": "Agora defina os seguintes parametros: STATUS: com base na muda que foi passada pelo usuario, defina se é algo que pode ser plantado, se sim coloque OK, caso contrario ERROR. MENSAGEM: descreva o porque de OK ou ERROR. PLANTA: escreva o nome da planta com o primeiro caractere maiúsculo. SOBRE: escreva um breve resumo sobre a planta. MAX-UMIDADE: escreva um valor de 0 a 100 que represente a umidade maxima do solo suportada pela planta levando em consideração sua necessidade hídrica. MIN-UMIDADE: escreva um valor de 0 a 100 que represente a umidade minima do solo suportada pela planta levando em consideração sua necessidade hídrica. SEMANA: crie um plano semanal de rega para a planta com base em sua necessidade hidrica. Insira ele dentro de um array de boleanos, o array começa pelo domingo, coloque true se for para regar e false caso contrario, repita para os 7 dias da semana. MODO-SEGURANCA: se a planta não for da especie de cactos coloque true, caso contrario false. CIDADE: escreva o nome da cidade formatado e a latitude e longitude da cidade em um array, a cidade será passada pelo usuario. TEMPERATURA: escreva a temperatura minima e temperatura maxima em um array que seja ideal para a planta passada pelo usuario, escreva também dicas breves e com fundamentos para quando a temperatura estiver abaixo do limite para a planta, quando a temperatura estiver adequado para a planta e quando a temperatura estiver a mais para a planta em um array crescente."
    },
    {
      "role": "system",
      "content": "Retorne o seguinte JSON com os resultados: {\"umidade\": {\"min\": MIN-UMIDADE,\"max\": MAX-UMIDADE}, \"planta\": {\"nome\": PLANTA>NOME, \"sobre\": SOBRE, \"sobre\": SOBRE>RESUMO}, \"cidade\": {\"nome\": CIDADE>NOME, \"coordenadas\"[CIDADE>LATITUDE, CIDADE>LONGITUDE]},\"seguranca\": MODO-SEGURANCA,\"semana\": SEMANA, \"temperatura\": {\"graus\": [TEMPERATURA>MINIMO, TEMPERATURA>MAXIMO], \"dicas\": TEMPERATURA>DICAS}, \"status\": STATUS, \"mensagem\": MENSAGEM}"
    }
  ]

  await aiRequest(prompt)
    .then(async (data) => {
      PARAMS = {...data};
      
      await chat("ia", { text: "Tudo certo! Já defini todos os parâmetros." })
      await chat("ia", { text: `Com base na necessidade hídrica da ${PARAMS.planta.nome}, cheguei à conclusão que a umidade do solo ideal é de ${PARAMS.umidade.min}% a ${PARAMS.umidade.max}%`, delay: 2000 })
      await chat("ia", { text: `Aqui está os dias da semana que será feito a rega:` })
      await chat("ia", {
        text: `${PARAMS.semana
          .reduce((p, c, index) => {
            const week = ["Domingo", "Segunda-Feira", "Terça-Feira", "Quarta-Feira", "Quinta-Feira", "Sexta-Feira", "Sábado"];

            p += `${week[index]}: ${c ? "✔️" : "❌"} \n`
            return p
          }, "")
          }`,
        delay: 2000
      })
    })
    .catch(async (error) => {
      await chat("ia", { text: error.mensagem })
      await chat("ia", { text: "Desculpe, terei que pedir que preencha novamente o campo de qual planta estaremos regando.", delay: 1000 })
      await perguntarPlanta()
      await definirParametros()
    })
}
async function perguntarNome() {
  await chat("ia", { text: "Vamos começar a configuração do nosso sistema!" })
  await chat("ia", { text: "Eu irei pedir algumas informações básicas sobre você e a planta na qual vamos regar 😃🌱", delay: 1000 })
  await chat("ia", { text: "Esse é um passo fundamental para que eu possa definir os parâmetros ideias para ela.", delay: 1000 })
  await chat("ia", { text: "Para começar, qual o seu nome?" })
  await chat("user", { text: "Seu nome e sobrenome..."}, (name) => {SYSTEM.userName = name;})
}
async function perguntarCidade() {
  await chat("ia", { text: `Prazer em conhecê-lo(a) ${SYSTEM.userName} 😉`, delay: 1000 })
  await chat("ia", { text: `${SYSTEM.userName.split(" ")[0]}, qual a sua idade?` })
  await chat("user", { text: "Nome da sua cidade..."}, (city) => {SYSTEM.city = city;})
  await chat("ia", { text: `Que legal! Adoro a cidade de ${SYSTEM.city} 🌇` })
}
async function perguntarPlanta() {
  await chat("user", { text: "Nome da planta ou plantas."}, (plant) => {SYSTEM.plant = plant;})
  await chat("ia", { text: `Estou pesquisando sobre a(o) ${SYSTEM.plant} para definir os parâmetros ideias.`, delay: 1000 })
  await chat("ia", { text: "Aguarde..." });
}
async function inserirNoSistema() {
  await chat("ia", { text: "Aguarde..." });
  await espRequest({
    body: {...PARAMS, userNome: SYSTEM.userName}, 
    method: "POST",
    type: "config"
  })
  .then(async () => {
    await chat("ia", { text: "Parâmetros enviado com sucesso 🥳" });
    await chat("ia", { text: "Agora o sistema está pronto para o uso!" });
  })
  .catch(async () => {
    await chat("ia", { text: "Houve algum problema ao enviar os parâmetros para o sistema 🥹" });
    await chat("ia", { text: "Estarei reenviando...", delay: 2000 });
    await inserirNoSistema()
  })
}
async function despedida() {
  await chat("ia", {
    text: `${SYSTEM.userName}, foi um prazer conversar com você 🤗`
  })
  await chat("ia", {
    text: "Agora que o sistema está configurado você já pode acessar a página de informações."
  })

  const button = $(`
    <li class="button" style="display: none;">
      <div>
        <a href="/">Clique aqui para acessar a página de informações</a>
      </div>
    </li>
  `).appendTo("#chat");
  
  button.slideDown(300)
}

export default async function talk() {
  await perguntarNome()
  await perguntarCidade()
  await chat("ia", { text: `${SYSTEM.userName.split(" ")[0]}, qual planta estaremos regando?\n` })
  await perguntarPlanta()
  await definirParametros()
  await chat("ia", { text: `${SYSTEM.userName.split(" ")[0]}, agora eu estarei enviando todos os parâmetros que defini para o nosso sistema.`, delay: 2000 })
  await inserirNoSistema()
  await despedida()
}