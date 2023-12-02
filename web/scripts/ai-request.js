// import {_OPENAI_KEY} from "../private.js"; // Dev Dep

export default function aiRequest(prompt) {
  const url = "https://api.openai.com/v1/chat/completions";
  const params = {
    "method": "POST",
    "headers": {
      "Content-Type": "application/json",
      "Authorization": `Bearer ${_OPENAI_KEY}`
    },
    "body": JSON.stringify({
      messages: [...prompt],
      temperature: 0.7,
      model: "gpt-3.5-turbo"
    })
  }

  return new Promise(async (resolve, reject) => {
    try{
      const request = await fetch(url, params)
      const response = await request.json();
      const data = await JSON.parse(response.choices[0].message.content);

      if (data.status !== "OK") {
        reject(data)
      } else {
        delete data.mensagem
        delete data.status
        resolve(data);
      }
  
    } catch {
      reject({status: "ERROR", mensagem: "Erro ao se comunicar com o servidor!"})
    }
  })
}
