import typingEffect from "./typing-effect.js";
import formatText from "./format-text.js";

export default async function chat(type, params, callback) {
  const {text, delay} = params;

  const chat = $("#chat")
  const message = $(`
    <li class="${type}" style="display: none;">
      <div>
      </div>
    </li>
  `)
  const content = $(`
    ${type == "ia" || type == "loading" ?
      `<p></p>`
      :
      `<form>
        <input type="text" placeholder="${text}" />
      </form>`}
  `)

  message
  .find("div")
  .append(content)
  message
  .appendTo(chat)
  .slideDown(200)

  // $("html").animate({
  //   scrollTop: message.offset().top - 100
  // }, 1000)

  if (type == "ia") {
    await typingEffect(text, content)

  } else if (type == "user") {
    await new Promise((resolve) => {
      const input = $(content)
      .find("input")
      .focus();

      content.on("submit", (e) => {
        e.preventDefault();
        const value = input
        .attr("disabled", true)
        .val()

        if (callback !== undefined) {
          callback(formatText(value));
        }

        content.off();
        resolve();
      })
    })
  }

  // Added delay
  await new Promise((resolve) => setTimeout(() => { resolve() }, delay | 500));
}