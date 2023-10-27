export default function espRequest({body, method, type}) {
  // const url = `http://192.168.0.17:5500/web/mock/api-${type}-response.json`;
  const url = `http://irrigaai.local/api/${type}`;
  let params = {method: method};

  if (body !== undefined) {
    params = {
      ...params,
      headers: {"Content-Type": "application/json"},
      body: JSON.stringify({...body})
    }
  }

  return fetch(url, params);
}