# H-Bridge Wiring Reference

This document captures the H-bridge connection between the ESP8266 and the DC motors, matching the wiring scheme used by the firmware in [src/RcCarFs.ino](../src/RcCarFs.ino).

## 1. Wiring layout

The controller uses a dual H-bridge arrangement (for example an L298N-style module) with one bridge driving the right motor and the second bridge driving the left motor.

```text
ESP8266                         H-Bridge module                         Motors
-------------------------------------------------------------------------------------------
D1  -----------------------> IN1 (right motor direction)          OUT1 ----> Right motor terminal A
D3  -----------------------> IN2 (right motor direction)          OUT2 ----> Right motor terminal B
D2  -----------------------> ENA (right motor PWM)               

D5  -----------------------> IN3 (left motor direction)           OUT3 ----> Left motor terminal A
D7  -----------------------> IN4 (left motor direction)           OUT4 ----> Left motor terminal B
D6  -----------------------> ENB (left motor PWM)

GND -----------------------> GND
5V ------------------------> H-bridge VCC / motor supply (as required by the module)
```

## 2. Functional mapping

### Right motor

- `pwmMotorA = D2`
- `in1MotorA = D1`
- `in2MotorA = D3`

Direction logic:

- forward: `in1MotorA = HIGH`, `in2MotorA = LOW`, PWM > 0
- reverse: `in1MotorA = LOW`, `in2MotorA = HIGH`, PWM > 0
- stop: `PWM = 0`, both direction pins low

### Left motor

- `pwmMotorB = D6`
- `in1MotorB = D5`
- `in2MotorB = D7`

Direction logic:

- forward: `in1MotorB = HIGH`, `in2MotorB = LOW`, PWM > 0
- reverse: `in1MotorB = LOW`, `in2MotorB = HIGH`, PWM > 0
- stop: `PWM = 0`, both direction pins low

## 3. Notes

- The firmware uses `analogWrite` on the ENA/ENB pins to control speed.
- The direction pins determine rotation direction.
- The H-bridge must share a common ground with the ESP8266.
- The motor supply voltage must match the H-bridge and motor requirements; it is not always the same as the ESP8266 3.3V rail.
- The `D8` output is used for the LED/light control and is separate from the motor drive path.

## 4. Reference in the project

The design used by this project is aligned with the code in [src/RcCarFs.ino](../src/RcCarFs.ino), where the following signal assignments are used:

| Motor | PWM | IN1 | IN2 |
| --- | --- | --- | --- |
| Right | D2 | D1 | D3 |
| Left | D6 | D5 | D7 |

This wiring is the operational basis for the motor actuation logic in the firmware.
