const path = require('path');
const express = require('express');
const app = express();
const server = require('http').createServer(app);
const WebSocket = require('ws');

const PORT = 3000;
const wss = new WebSocket.WebSocketServer({ server, host: '0.0.0.0' });

const state = {
  isBlueLedOn: false,
  leftMotor: 0,
  rightMotor: 0
};

const isMotorValue = (value) =>
  Number.isInteger(value) && value >= -255 && value <= 255;

const updateValidators = {
  isBlueLedOn: (value) => typeof value === 'boolean',
  leftMotor: isMotorValue,
  rightMotor: isMotorValue
};

const send = (client, message) => {
  if (client.readyState === WebSocket.OPEN) {
    client.send(JSON.stringify(message));
  }
};

const broadcast = (message) => {
  wss.clients.forEach((client) => send(client, message));
};

const applyUpdate = (value) => {
  if (value === null || typeof value !== 'object' || Array.isArray(value)) {
    console.error('WebSocket update requires an object value');
    return;
  }

  for (const [key, isValid] of Object.entries(updateValidators)) {
    if (Object.prototype.hasOwnProperty.call(value, key) && !isValid(value[key])) {
      console.error(`WebSocket update has an invalid ${key} value`);
      return;
    }
  }

  const changed = {};
  for (const key of Object.keys(updateValidators)) {
    if (Object.prototype.hasOwnProperty.call(value, key) && state[key] !== value[key]) {
      state[key] = value[key];
      changed[key] = value[key];
    }
  }

  if (Object.keys(changed).length > 0) {
    broadcast({ action: 'update', value: changed });
  }
};

const handleMessage = (client, data, isBinary) => {
  if (isBinary) {
    console.error('Ignoring binary WebSocket message');
    return;
  }

  const rawMessage = data.toString();
  let message;
  try {
    message = JSON.parse(rawMessage);
  } catch (error) {
    console.error('Invalid WebSocket JSON:', error.message);
    return;
  }

  if (message === null || typeof message !== 'object' || Array.isArray(message) ||
      typeof message.action !== 'string') {
    console.error('WebSocket message is missing a valid action');
    return;
  }

  switch (message.action) {
    case 'message':
      if (typeof message.value !== 'string') {
        console.error('WebSocket message action requires a string value');
        return;
      }
      console.log('WebSocketClient message', message.value);
      break;
    case 'update':
      applyUpdate(message.value);
      break;
    default:
      console.log('Unknown action', message.action, 'message', rawMessage);
  }
};

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html'));
});

app.use('/vendor/nipplejs', express.static(path.join(__dirname, 'node_modules/nipplejs/dist')));

wss.on('connection', (client) => {
  send(client, { action: 'update', value: { ...state } });
  client.on('message', (data, isBinary) => handleMessage(client, data, isBinary));
  client.on('error', (error) => console.error('WebSocket client error:', error));
  client.on('close', () => console.log('WebSocket client disconnected'));
});

server.listen(PORT, '0.0.0.0', () => {
  console.log(`Server started on port ${PORT}`);
});
