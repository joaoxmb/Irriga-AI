import aiRequest from "../../scripts/ai-request.js";
import { delay } from "../../scripts/delay.js";

let requestCount = 0;
const requestCounter = () => {
  if (requestCount > 5) {
    location.reload();
    return;
  }

  requestCount++
}

const promptPrimary = class{
  constructor(plant, city){
    return [
      {
        "role": "system",
        "content": "Você receberá o nome de uma muda em português e receberá o nome de uma cidade."
      },
      {
        "role": "user",
        "content": `planta=${plant};cidade=${city}, Brasil`
      },
      {
        "role": "system",
        "content": "Agora defina os seguintes parametros: STATUS: valide as informacoes passadas pelo usuario, a planta deve ser um nome de Planta existente e cidade deve corresponder a uma cidade do Brasil, retorne OK se estiver correto e ERROR se estiver alguma informacao incorreta. MENSAGEM: descreva o porque de OK ou ERROR. PLANTA: escreva o nome da planta com o primeiro caractere maiúsculo. SOBRE: escreva um texto de 10 linhas sobre a planta e suas características. MAX-UMIDADE: escreva um valor de 0 a 100 que represente a umidade maxima do solo suportada pela planta levando em consideração sua necessidade hídrica, esse valor não pode ser superior doque 90. MIN-UMIDADE: escreva um valor de 0 a 100 que represente a umidade minima do solo suportada pela planta levando em consideração sua necessidade hídrica, esse valor não pode ser abaixo de 10. SEMANA: crie um plano semanal de rega para a planta com base em sua necessidade hidrica. Insira ele dentro de um array de boleanos, o array começa pelo domingo, coloque true se for para regar e false caso contrario, repita para os 7 dias da semana, o ultimo valor deverá ser sempre true. MODO-SEGURANCA: se a planta sofrer algum tipo de dano caso a umidade do solo fique abaixo de 5% coloque true, caso contrario false. Geralmente espécies de cactos não sofrem com essa baixa umidade. MODO-SEGURANCA-MOTIVO: descreva o motivo pelo qual você tomou a decisao ao definir MODO-SEGURANCA. CIDADE: escreva o nome da cidade formatado e a latitude e longitude da cidade em um array, a cidade será passada pelo usuario. TEMPERATURA: escreva a temperatura minima e temperatura maxima em um array que seja ideal para a planta passada pelo usuario. TEMPERATURA-DICAS: escreva uma mensagem para quando a temperatura estiver abaixo do limite para a planta, quando a temperatura estiver adequado para a planta e quando a temperatura estiver a mais para a planta em um array crescente."
      }
    ]
  }
}
const promptSecondary = class{
  constructor(params){
    return [
      {
        "role": "user",
        "content": params
      },
      {
        "role": "system",
        "content": "retorne um json com a estrutura especificada abaixo com os dados passados pelo usuario."
      },
      {
        "role": "system",
        "content": "{ \"umidade\": { \"min\": MIN-UMIDADE, \"max\": MAX-UMIDADE }, \"planta\": { \"nome\": PLANTA, \"sobre\": SOBRE }, \"cidade\": { \"nome\": CIDADE>NOME, \"coordenadas\": [CIDADE>LATITUDE, CIDADE>LONGITUDE] }, \"seguranca\": [ MODO-SEGURANCA, MODO-SEGURANCA-MOTIVO ], \"semana\": [SEMANA], \"temperatura\": { \"graus\": [TEMPERATURA>MINIMA, TEMPERATURA>MAXIMA], \"dicas\": [TEMPERATURA-DICAS] }, \"status\": STATUS, \"mensagem\": MENSAGEM }"
      }
    ]
  }
}

const requestJson = async (params) => {
  console.log("requestJson");
  return new Promise(async (resolve, reject) => {
    let isValid = true;
    console.log("secundary promise");
    while (isValid) {
      requestCounter()
      console.log("secundary while loop");
      await aiRequest(new promptSecondary(params))
        .then((response) => {
          try{
            const json = JSON.parse(response)
  
            if (json.status !== "OK") {
              reject(json)
            } else {
              delete json.mensagem
              delete json.status
              json["semana"][6] = true // sabado sempre ativo.
  
              resolve(json)
            }

            isValid = false;
          }
          catch(err){
            console.log(err);
          }
        })
        .catch(async (err) => {
          console.log(err);
        })
        await delay(1000);
      }
    })
  }
  
  export default async function defineParams(plant, city) {
    console.log("defineParams");
    return new Promise(async (resolve, reject) => {
      let isValid = true;
      console.log("primary promise");
      while (isValid) {
        console.log("primary while loop");
        requestCounter()
        await aiRequest(new promptPrimary(plant, city))
          .then(async (params) => {
            isValid = false;
            
            await requestJson(params)
            .then((json) => {
              resolve(json)
            })
            .catch((err) => {
              reject(err)
            })
          })
          .catch((err) => {
            console.log(err);
          })
        await delay(1000);
    }
  })
}