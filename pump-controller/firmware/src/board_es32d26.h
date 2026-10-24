// Eletechsup ES32D26 (ESP32, 8 DI / 8 AI / 8 relays / 2 AO), DC 24 V version.
// Relays are driven by a 74HC595 and the inputs read by a 74HC165, sharing
// latch + clock lines.
//
// !! PIN NUMBERS NOT YET CONFIRMED FOR THIS BOARD !!
// They're taken from a sister board with the same chip layout. Run the
// self-test (type `t` in the serial monitor) on first power-up to confirm.
#pragma once
#include <Arduino.h>

namespace board {

const int PIN_DATA_OUT = 33;  // 74HC595 serial in (relays)
const int PIN_DATA_IN = 27;   // 74HC165 serial out (inputs)
const int PIN_CLOCK = 26;     // shared shift clock
const int PIN_LATCH = 25;     // 595 latch + 165 parallel load
const int PIN_ENABLE = 13;    // 595 output enable, active LOW

const bool INPUT_ACTIVE_LOW = true;  // NPN opto: input pulled to GND = 0 bit

// Analogue pins to try in the self-test (ADC1 first; ADC2 pins don't work
// while Wi-Fi is on, so the transducer should end up on an ADC1 pin).
const int ADC_CANDIDATES[] = {36, 39, 34, 35, 32, 4, 2, 15, 14, 12};
const int ADC_CANDIDATE_COUNT = sizeof(ADC_CANDIDATES) / sizeof(int);

inline void begin() {
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);  // relays disabled until we've written "all off"
  pinMode(PIN_DATA_OUT, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_DATA_IN, INPUT);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_CLOCK, LOW);
}

// Writes the 8 relays and reads the 8 inputs in one pass.
// relay[i] true = relay i+1 on. closed[i] true = input i+1 contact closed.
inline void transfer(const bool relay[8], bool closed[8]) {
  // Pulse latch low: 165 grabs the input states
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(5);
  digitalWrite(PIN_LATCH, HIGH);

  uint8_t out = 0;
  for (int i = 0; i < 8; i++)
    if (relay[i]) out |= 1 << i;

  uint8_t in = 0;
  for (int b = 7; b >= 0; b--) {
    if (digitalRead(PIN_DATA_IN)) in |= 1 << b;  // 165 shifts D7 first
    digitalWrite(PIN_DATA_OUT, (out >> b) & 1);  // 595 MSB first -> relay 8 ends at Q7
    delayMicroseconds(2);
    digitalWrite(PIN_CLOCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CLOCK, LOW);
  }

  // Rising edge of latch moves the shifted data onto the relays
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(5);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_ENABLE, LOW);

  for (int i = 0; i < 8; i++) {
    bool bit = (in >> i) & 1;
    closed[i] = INPUT_ACTIVE_LOW ? !bit : bit;
  }
}

}  // namespace board
