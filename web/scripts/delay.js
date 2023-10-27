const delay = (temp) => new Promise((resolve) => {
  setTimeout(() => {
    resolve();
  }, temp)
})

export {delay}