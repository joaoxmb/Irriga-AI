export const loading = {
  enable(){
    $(`
        <div id="loading">
        <div class="dot-wave">
          <div class="dot-wave__dot"></div>
          <div class="dot-wave__dot"></div>
          <div class="dot-wave__dot"></div>
          <div class="dot-wave__dot"></div>
        </div>
      </div>
    `).appendTo("body")
    $("html").css("overflow", "hidden");
  },
  disable(){
    const element = $("#loading")

    $("html").attr("style", "");

    element.animate({
      opacity: 0,
    }, 500, () => {
      element.remove();
    })
  }
}