import espRequest from "../../scripts/esp-request.js"

function date() {
  $("#system-data").text(moment().format("DD/MM/yyyy"));
}
function about(planta) {
  const {nome, sobre} = planta;
  
  $("#about p").text(sobre);
  $("#about h2").text(nome);
}
function temperature(temperature) {
  $("#system-temperature").text(Math.floor(temperature));
}

let send;
function sendWeekToSystem(params, callback) {
  clearTimeout(send);
  send = setTimeout(() => {
    espRequest({ body: params, method: "POST", type: "config" })
      .then((response) => response.json())
      .then((data) => {
        if (callback !== undefined) {
          return callback(data);
        }
      })
      .catch((error) => {
        console.log("Houve um erro ao enviar os parametros, tentando novamente", error);
        sendWeekToSystem(params, callback);
      })
  }, 3000);
};
  
function week(semana) {
  const elementoSemana = $("#semana").html("");
  const diaDaSemana = Number(moment().format("d"));
  const nomesDiasDaSemana = ["DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SÁB"];

  const handleSemana = (index, input) => {
    const value = $(input).is(":checked");
    CONFIG.semana[index] = value;

    const status = $("#watering .status")
      .text("Enviando cronograma atualizado para o sistema...")
      .slideDown(1000);

    sendWeekToSystem({ semana: [...CONFIG.semana] }, () => {
      status.text("Cronograma enviado com sucesso!")
    });
  };

  semana.forEach((dia, index) => {
    $(`<li ${diaDaSemana === index ? "class='today'" : ""}>
        <label>
          <span>${nomesDiasDaSemana[index]}</span>
          <input type="checkbox" ${dia ? "checked" : ""}>
          <div class="checkmark"></div>
        </label>
      </li>`)
      .appendTo(elementoSemana).click((e) => handleSemana(index, e.target))
  });
}
function security(security) {
  $("#security").text(
    `O modo de segurança está ${security[0] ? 'ativado':'desativado'}! ${security[1]}`
  )
}

export {
  date,
  about,
  week,
  temperature,
  security
}