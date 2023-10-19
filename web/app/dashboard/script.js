import espRequest from "../../scripts/esp-request.js";
import chat from "../../scripts/chat.js";
import {delay} from "../../scripts/delay.js";
import {loading} from "../../scripts/loading.js";

import * as insert from "./insert.js";

let INFO,
    CONFIG;

function getConfig() {
  return espRequest({method: "GET", type: "config"})
    .then((response) => response.json())
    .then((data) => {CONFIG = data})
    .catch((error) => {
      console.log("Houve um erro ao pedir configuracoes, tentando novamente", error);
    })
}
function getInfo() {
  return espRequest({method: "GET", type: "info"})
    .then((response) => response.json())
    .then((data) => {
      const {umidade} = data;
      const rounded = Math.floor(umidade);
      const warning = $("#warning");
      INFO = data;
      $("#system-umidty").text(rounded);
      

      if (rounded === 0){
        warning.slideDown().attr("active", true);
        return;
      }
      if (warning.is("[active=true]")){
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
      const {temperature_2m, time} = data.hourly;

      const temperatureIndex = time.indexOf(moment().format("yyyy-MM-DDTHH:00"));
      const currentTemperatureIndex = temperatureIndex != -1 ? temperatureIndex : 0;
      const temperature = temperature_2m[currentTemperatureIndex];

      $("#system-temperature").text(Math.floor(temperature));
      INFO.temperatura = temperature;
    })
    .catch((err) => {
      console.error("aconteceu um erro ao buscar a temperatura local." + err);
      console.log("Fazendo uma nova requisição para API de temperatura.");

      insertTemperature();
    });
}

$(() => {
  loading.enable()

  getConfig()
    .then(async () => {
      insert.about({...CONFIG.planta});
      insert.week([...CONFIG.semana]);

      await getInfo();
      await getTemperature([...CONFIG.cidade.coordenadas]);
      setInterval(async () => {
        await getInfo();
      }, 5000)
    })
    .then(async () => {
      loading.disable()

      const humidityStatus = () => {
        if (INFO.umidade === 0){
          return "Parece que sensor está mal instalado, é melhor você dar uma conferida."
        }
        if (INFO.umidade < CONFIG.umidade.min) {
          return "A umidade do solo está abaixo do ideal para ela. Fique tranquilo que irei ativar o sistema de rega!"
        }
        if (INFO.umidade > CONFIG.umidade.max) {
          return "A umidade do solo está acima do ideal, fique tranquilo que não estarei regando."
        }
        return `A umidade do solo está ideal para ${CONFIG.planta.nome}!`
      };
      const temperatureStatus = () => {
        if (INFO.temperatura < CONFIG.temperatura.graus[0]) {
          return CONFIG.temperatura.dicas[0];
        }
        if (INFO.temperatura > CONFIG.temperatura.graus[1]) {
          return CONFIG.temperatura.dicas[3];
        }
        return CONFIG.temperatura.dicas[1];
      };

      await delay(1000);
      await chat("ia", {text: `${salutation()}, ${CONFIG.userNome}. Tudo bem?`, delay: 1000})
      await chat("ia", {text: `${humidityStatus()}`})
      await chat("ia", {text: `${temperatureStatus()}`})
      
    })
  insert.date()
})