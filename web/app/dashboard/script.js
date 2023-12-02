import espRequest from "../../scripts/esp-request.js";
import chat from "../../scripts/chat.js";
import { delay } from "../../scripts/delay.js";
import { loading } from "../../scripts/loading.js";

import * as insert from "./insert.js";

let INFO,
  CONFIG;

function getConfig() {
  return espRequest({ method: "GET", type: "config" })
    .then((response) => response.json())
    .then((data) => {
      CONFIG = data
    })
    .catch((error) => {
      console.log("Houve um erro ao pedir configuracoes, tentando novamente", error);
    })
}
function getInfo() {
  return espRequest({ method: "GET", type: "info" })
    .then((response) => response.json())
    .then((data) => {
      const { umidade } = data;
      const rounded = Math.floor(umidade);
      const warning = $("#warning");
      INFO = data;
      $("#system-umidty").text(rounded);


      if (rounded === 0) {
        warning.slideDown().attr("active", true);
        return;
      }
      if (warning.is("[active=true]")) {
        warning.attr("active", false).slideUp();
      }
    })
    .catch((err) => {
      console.error(err);
    });
}

function salutation() {
  const now = moment();
  const hours = Number(now.format("H"));
  if (hours >= 0 && hours < 12) {
    return "Bom dia";
  }
  if (hours > 11 && hours < 18) {
    return "Boa tarde";
  }
  return "Boa noite";
}
function getTemperature(coordenadas) {
  const url = "https://api.open-meteo.com/v1/forecast";
  const params = `?latitude=${coordenadas[0]}&longitude=${coordenadas[1]}&hourly=temperature_2m`;

  return fetch(url + params)
    .then((response) => response.json())
    .then((data) => {
      const { temperature_2m, time } = data.hourly;

      const temperatureIndex = time.indexOf(moment().format("yyyy-MM-DDTHH:00"));
      const currentTemperatureIndex = temperatureIndex != -1 ? temperatureIndex : 0;
      const temperature = temperature_2m[currentTemperatureIndex];

      INFO.temperatura = temperature;
    })
    .catch((err) => {
      console.error("aconteceu um erro ao buscar a temperatura local." + err);
      console.log("Fazendo uma nova requisição para API de temperatura.");

      getTemperature();
    });
}

const getEssentialParams = async () => {
  try {
    console.log("getEssentialParams executado!!!!!!!");
    await Promise.all([
      getConfig(),
      getInfo(),
    ])
    await getTemperature([...CONFIG.cidade.coordenadas])
  }
  catch {
    throw "Erro ao requisitar os parametros essenciais"
  }
}

const humidityStatus = () => {
  if (INFO.umidade < CONFIG.umidade.min) {
    return "A umidade do solo está abaixo do ideal, estarei ligando o sistema de rega!"
  }
  if (INFO.umidade > CONFIG.umidade.max) {
    return "Analisei os dados do sistema e constatei que a umidade do solo está acima do ideal. O sistema de rega está desabilitado!"
  }
  return `O solo de sua(seu) ${CONFIG.planta.nome} está com a umidade adequada!`
};
const temperatureStatus = () => {
  if (INFO.temperatura < CONFIG.temperatura.graus[0]) {
    return `A temperatura de ${CONFIG.cidade.nome} está abaixo do recomendado para ${CONFIG.planta.nome}! ` + CONFIG.temperatura.dicas[0];
  }
  if (INFO.temperatura > CONFIG.temperatura.graus[1]) {
    return `A temperatura atual está acima do recomendado para ${CONFIG.planta.nome}! ` + CONFIG.temperatura.dicas[3];
  }
  return `A temperatura agora em ${CONFIG.cidade.nome} está adequada para ${CONFIG.planta.nome}! ` + CONFIG.temperatura.dicas[1];
};

const app = async () => {
  getEssentialParams()
    .then(async() => {
      insert.date();
      insert.about({...CONFIG.planta});
      insert.week([...CONFIG.semana]);
      insert.temperature(INFO.temperatura);
      insert.security(CONFIG.seguranca);
      loading.disable();
  
      setInterval(async () => {
        await getInfo();
      }, 2000)
  
      await delay(1000);
      await chat("ia", { text: `${salutation()}, ${CONFIG.userNome}. Tudo bem?`, delay: 1000 })
      await chat("ia", { text: `${humidityStatus()}` })
      await chat("ia", { text: `${temperatureStatus()}` })
    })
    .catch(async() => {
      console.log("deu erro,tentanto novamente");
      await delay(1000)
      app();
    })
}

$(document).ready(() => {
  app();
})