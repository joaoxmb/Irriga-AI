export default function importHTML(src, type) {
  return fetch(src)
    .then((response) => response.text())
    .then((data) => {
      document.querySelector(type).innerHTML += data;
    })
    .catch((err) => {
      console.log(err);
    })
}