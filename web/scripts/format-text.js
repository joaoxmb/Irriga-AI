export default function formatText(string) {
  const formated = string
    .trim()
    .split(" ")
    .reduce((p, c) => {
      const converted = c[0].toUpperCase()
      const removed = c.substring(1, c.length).toLowerCase()
      p += " " + converted + removed;
      return p
    }, "")
  return formated.trim()
}