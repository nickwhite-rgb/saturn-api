// Saturn Pump Controller: bench prototype firmware for KinCony KC868-A6
// Serial monitor 115200 baud. Commands:  r = reset   m = manual run on/off
#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>

#include "config.h"
#include "pump_logic.h"

static Settings settings = makeSettings();
static PumpController ctrl(settings);
static Preferences prefs;

static bool remoteLatch = true;  // for momentary start/stop buttons
static bool serialManual = false;
static bool serialReset = false;
static float mAFiltered = 0;
static bool i2cOk = true;

// ---------- KC868-A6 I/O (PCF8574 expanders, active low) ----------
static void writeRelays(const bool relay[6]) {
  uint8_t b = 0xFF;  // 1 = off
  for (int i = 0; i < 6; i++)
    if (relay[i]) b &= ~(1 << i);
  Wire.beginTransmission(ADDR_RELAYS);
  Wire.write(b);
  i2cOk = Wire.endTransmission() == 0;
}

// Returns true when each input contact is closed
static bool readInputs(bool closed[6]) {
  if (Wire.requestFrom(ADDR_INPUTS, (uint8_t)1) != 1) {
    for (int i = 0; i < 6; i++) closed[i] = false;
    return false;
  }
  uint8_t b = Wire.read();
  for (int i = 0; i < 6; i++) closed[i] = !(b & (1 << i));
  return true;
}

static float readMilliAmps() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_PRESSURE);
  float volts = (mv / 16.0f) / 1000.0f * A1_SCALE;
  float mA = volts / SHUNT_OHMS * 1000.0f;
  mAFiltered = mAFiltered == 0 ? mA : mAFiltered * 0.8f + mA * 0.2f;  // smooth noise
  return mAFiltered;
}

static void i2cScan() {
  Serial.print("I2C scan:");
  bool in = false, out = false;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" 0x%02X", a);
      in |= a == ADDR_INPUTS;
      out |= a == ADDR_RELAYS;
    }
  }
  Serial.println();
  if (!in) Serial.println("!! Input chip not found - check ADDR_INPUTS / pins in config.h");
  if (!out) Serial.println("!! Relay chip not found - check ADDR_RELAYS / pins in config.h");
}

// ---------- Counters saved to flash (every 15 min if changed) ----------
static uint32_t lastSave = 0;
static uint32_t savedStarts = 0;

static void loadCounters() {
  prefs.begin("pump", false);
  uint32_t st[2] = {prefs.getUInt("s0", 0), prefs.getUInt("s1", 0)};
  uint32_t rs[2] = {prefs.getUInt("r0", 0), prefs.getUInt("r1", 0)};
  ctrl.restoreCounters(st, rs);
  savedStarts = st[0] + st[1];
}

static void saveCounters(uint32_t now) {
  if (now - lastSave < 15UL * 60000) return;
  lastSave = now;
  if (ctrl.starts(0) + ctrl.starts(1) == savedStarts) return;
  savedStarts = ctrl.starts(0) + ctrl.starts(1);
  prefs.putUInt("s0", ctrl.starts(0));
  prefs.putUInt("s1", ctrl.starts(1));
  prefs.putUInt("r0", ctrl.runSeconds(0));
  prefs.putUInt("r1", ctrl.runSeconds(1));
}

void setup() {
  Wire.begin(PIN_SDA, PIN_SCL);
  bool off[6] = {false};
  writeRelays(off);  // all relays off straight away

  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Saturn Pump Controller - Mode 1 (bench) ===");
  i2cScan();
  analogSetPinAttenuation(PIN_PRESSURE, ADC_11db);

  for (int i = 0; i < 6; i++) {
    if (INPUT_MAP[i].fn == InFn::Pump1Running) settings.pumpFeedback[0] = true;
    if (INPUT_MAP[i].fn == InFn::Pump2Running) settings.pumpFeedback[1] = true;
  }
  ctrl = PumpController(settings);
  loadCounters();
}

void loop() {
  uint32_t now = millis();

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') serialReset = true;
    if (c == 'm') {
      serialManual = !serialManual;
      Serial.println(serialManual ? "Manual ON" : "Manual OFF");
    }
  }

  // ---- Build inputs from the input map ----
  bool closed[6];
  bool inputsOk = readInputs(closed);
  Inputs in;
  in.mA = readMilliAmps();
  bool haveSwitch = false, switchOn = false;
  for (int i = 0; i < 6; i++) {
    bool active = closed[i] != INPUT_MAP[i].normallyClosed;
    switch (INPUT_MAP[i].fn) {
      case InFn::Pump1Running: in.running[0] = active; break;
      case InFn::Pump1Fault: in.pumpFault[0] |= active; break;
      case InFn::Pump2Running: in.running[1] = active; break;
      case InFn::Pump2Fault: in.pumpFault[1] |= active; break;
      case InFn::RemoteSwitch: haveSwitch = true; switchOn = active; break;
      case InFn::RemoteStart: if (active) remoteLatch = true; break;
      case InFn::RemoteStop: if (active) remoteLatch = false; break;
      case InFn::Float: in.tankOk = active; break;
      case InFn::Reset: in.reset |= active; break;
      case InFn::Manual: in.manual |= active; break;
      case InFn::None: break;
    }
  }
  // Stop always wins: a stop button press clears the latch even if the switch is on
  in.enable = remoteLatch && (!haveSwitch || switchOn);
  in.reset |= serialReset;
  in.manual |= serialManual;
  serialReset = false;

  // Can't read the input chip = treat as tank/pump fault, fail safe
  if (!inputsOk) {
    in.tankOk = false;
  }

  Outputs out = ctrl.update(in, now);

  bool relay[6] = {false};
  relay[0] = out.pump[0];
  relay[1] = out.pump[1];
  relay[2] = out.alarm;
  relay[3] = !out.alarm && ctrl.state() != State::Lockout && i2cOk;  // healthy
  writeRelays(relay);

  saveCounters(now);

  // ---- Status line once a second ----
  static uint32_t lastPrint = 0;
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    Serial.printf("%5.2f bar (%5.2f mA, %4.2f V) | %-16s | P1 %s P2 %s | lead P%d | %s",
                  ctrl.pressure(), in.mA, in.mA * SHUNT_OHMS / 1000.0f,
                  stateName(ctrl.state()), out.pump[0] ? "ON " : "off",
                  out.pump[1] ? "ON " : "off", ctrl.lead() + 1, faultName(ctrl.fault()));
    uint32_t wait = ctrl.retryInMs(now);
    if (wait) Serial.printf(" | retry in %lu s", (unsigned long)(wait / 1000));
    if (!inputsOk) Serial.print(" | !! INPUT CHIP NOT RESPONDING");
    Serial.println();
  }

  delay(50);
}
