import espRequest from "../../scripts/esp-request.js";
import chat from "../../scripts/chat.js";
import defineParams from "./define-params-ai-request.js";

let PARAMS = {};
let SYSTEM = {};

async function definirParametros() {
  return defineParams(SYSTEM.plant, SYSTEM.city)
    .then(async (params) => {
      PARAMS = params;
      console.log(PARAMS);
    
      await chat("ia", { text: "Tudo certo! Já defini todos os parâmetros." })
      await chat("ia", { text: `Com base na necessidade hídrica da ${PARAMS.planta.nome}, cheguei à conclusão que a umidade do solo ideal é de ${PARAMS.umidade.min}% a ${PARAMS.umidade.max}%`, delay: 2000 })
      await chat("ia", { text: `Aqui está os dias da semana que será feito a rega:` })
      await chat("ia", {
        text: `${PARAMS.semana
          .reduce((p, c, index) => {
            const week = ["Domingo", "Segunda-Feira", "Terça-Feira", "Quarta-Feira", "Quinta-Feira", "Sexta-Feira", "Sábado"];
    
            p += `${c ? "✔️" : "❌"} ${week[index]}\n`
            return p
          }, "")
          }`,
        delay: 2000
      })
      await chat("ia", {text: PARAMS.planta.sobre})
    })
    .catch(async (error) => {
      await chat("ia", { text: error.mensagem || "Error" })
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
  await chat("ia", { text: `${SYSTEM.userName.split(" ")[0]}, qual a sua cidade?` })
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
