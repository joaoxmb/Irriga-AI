import talk from "./talk.js";
import { delay } from "../../scripts/delay.js";

$(() => {
  const start = $("#start");
  const startBtn = $("#start button");
  const config = $("#config");

  startBtn.click(async () => {
    start.animate({ height: 'toggle', opacity: 'toggle' }, 1000);
    config.slideToggle(1000);
  
    await delay(1500);
    talk();
  })
})