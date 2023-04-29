const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
  <title>RC Car WebSocket Controller</title>
  <script src="/nipplejs.js"></script>
</head>
<body>
  <style>
    body {
      font-size: 16px;
      margin: 0;
      display: flex;
      flex-direction: column;
    }

    #top {
      position: relative;
      height: 5rem;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
    }

    #bottom {
      position: relative;
      display: flex;
      justify-content: space-between;
      height: calc(100vh - 5rem);
    }

    #left {
      position: relative;
      width: 200px
    }

    #right {
      position: relative;
      width: 200px
    }

    #blueLed {
      border: 0px;
      height: 4rem;
      width: 4rem;
      margin: 0;
    }
  </style>

  <div id="top">
    <input type="checkbox" id="blueLed" name="blueLed"
      onclick="state.setBlueLedOn(this.checked, true)" />
  </div>
  <div id="bottom">
    <div id="left"></div>
    <div id="right"></div>
  </div>

  <script>
    const joystickL = nipplejs.create({
      zone: document.getElementById('left'),
      mode: 'static',
      position: {
        left: '50%',
        top: '50%'
      },
      size: 200,
      color: 'black',
      lockY: true
    });
    joystickL.on('0:move', (e, { direction, distance }) => {
        const { y } = direction || {}
        let value = 0;
        if (y === 'up') {
          value = Math.round(2.55 * distance);
        }
        if (y === 'down') {
          value = Math.round(-2.55 * distance);
        }
        state.setLeftMotor(value, true);
    });
    joystickL.on('0:end', () => {
      state.setLeftMotor(0, true);
    });

    const joystickR = nipplejs.create({
      zone: document.getElementById('right'),
      mode: 'static',
      position: {
        left: '50%',
        top: '50%'
      },
      size: 200,
      color: 'black',
      lockY: true
    });
    joystickR.on('1:move', (e, { direction, distance }) => {
        const { y } = direction || {}
        let value = 0;
        if (y === 'up') {
          value = Math.round(2.55 * distance);
        }
        if (y === 'down') {
          value = Math.round(-2.55 * distance);
        }
        state.setRightMotor(value, true);
    });
    joystickR.on('1:end', () => {
      state.setRightMotor(0, true);
    });

    const socket = new WebSocket(`ws://${window.location.host}`);
    const state = {
      isBlueLedOn: false,
      leftMotor: 0,
      rightMotor: 0,
      setBlueLedOn: (value, resend = false) => {
        if (state.isBlueLedOn !== value) {
          state.isBlueLedOn = value;
          if (resend) {
            socket.send(JSON.stringify({
              action: 'update',
              value: { isBlueLedOn: value }
            }));
          } else {
            document.querySelector('#blueLed').checked = value;
          }
        }
      },
      setLeftMotor: (value, resend = false) => {
        if (state.leftMotor !== value) {
          state.leftMotor = value;
          if (resend) {
            socket.send(JSON.stringify({
              action: 'update',
              value: { leftMotor: value }
            }));
          }
        }
      },
      setRightMotor: (value, resend = false) => {
        if (state.rightMotor !== value) {
          state.rightMotor = value;
          if (resend) {
            socket.send(JSON.stringify({
              action: 'update',
              value: { rightMotor: value }
            }));
          }
        }
      }
    };

    socket.addEventListener('error', (error) => {
      console.log('WebSocketServer error', error);
    });

    socket.addEventListener('open', (event) => {
      socket.send(JSON.stringify({ action: 'message', value: 'OK' }));
    });

    socket.addEventListener('message', ({ data }) => {
      const { action, value } = JSON.parse(data);
      switch (action) {
        case 'message':
          console.log('WebSocketServer message', value);
          break;
        case 'update':
          const { isBlueLedOn, leftMotor, rightMotor } = value;
          if (isBlueLedOn !== undefined) {
            state.setBlueLedOn(isBlueLedOn);
          }
          if (leftMotor !== undefined) {
            state.setLeftMotor(leftMotor);
          }
          if (rightMotor !== undefined) {
            state.setRightMotor(rightMotor);
          }
          break;
        default:
          console.log('Unknown action', action, 'data', data);
      }
    });
  </script>
</body>
</html>
)rawliteral";