import espRequest from "../../scripts/esp-request.js"

function date() {
  $("#system-data").text(moment().format("DD/MM/yyyy"));
}
function about(planta) {
  const {nome, sobre} = planta;
  
  $("#about p").text(sobre);
  $("#about h2").text(nome);
}

export {
  date,
  about
}