const path = require('path');
const app = require('express')();
const server = require('http').createServer(app);
const WebSocket = require('ws');

const wss = new WebSocket.WebSocketServer({ server, host: '0.0.0.0' });

const sendObjectToAll = (object) => {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(object), { binary: false });
    }
  });
}

const state = {
  isBlueLedOn: false,
  leftMotor: 0,
  rightMotor: 0
};

const setBlueLedOn = (value) => {
  if (state.isBlueLedOn !== value) {
    state.isBlueLedOn = value;
    sendObjectToAll({
      action: 'update',
      value: { isBlueLedOn: value }
    });
  }
}

const setLeftMotor = (value) => {
  if (state.leftMotor !== value) {
    state.leftMotor = value;
    sendObjectToAll({
      action: 'update',
      value: { leftMotor: value }
    });
  }
}

const setRightMotor = (value) => {
  if (state.rightMotor !== value) {
    state.rightMotor = value;
    sendObjectToAll({
      action: 'update',
      value: { rightMotor: value }
    });
  }
}

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.html'));
});

app.get('/nipplejs.js', (req, res) => {
  res.sendFile(path.join(__dirname, './node_modules/nipplejs/dist/nipplejs.js'));
});

wss.on('connection', (client) => {
  client.send(JSON.stringify({
    action: 'update',
    value: { isBlueLedOn: state.isBlueLedOn }
  }));

  client.on('message', (data, isBinary) => {
    const message = isBinary ? data : data.toString();
    const { action, value } = JSON.parse(message);
    switch (action) {
      case 'message':
        console.log('WebSocketClient message', value);
        break;
      case 'update':
        console.log('WebSocketClient update', value);
        const { isBlueLedOn, leftMotor, rightMotor } = value;
        if (isBlueLedOn !== undefined) {
          setBlueLedOn(isBlueLedOn);
        }
        if (leftMotor !== undefined) {
          setLeftMotor(leftMotor);
        }
        if (rightMotor !== undefined) {
          setRightMotor(rightMotor);
        }
        break;
      default:
        console.log('Unknown action', action, 'message', message);
    }
  });

  client.on('close', () => {
    console.log('on disconnect')
  });
});

server.listen(3000, '0.0.0.0', () => {
  console.log('Server started on port 3000');
});
