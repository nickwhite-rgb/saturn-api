// Saturn Pump Controller: bench prototype firmware for Eletechsup ES32D26
// Serial monitor 115200 baud. Commands:
//   r = reset lockout    m = manual run on/off    t = self-test (pins/wiring)
#include <Arduino.h>
#include <Preferences.h>

#include "board_es32d26.h"
#include "config.h"
#include "pump_logic.h"

static Settings settings = makeSettings();
static PumpController ctrl(settings);
static Preferences prefs;

static bool remoteLatch = true;  // for momentary start/stop buttons
static bool serialManual = false;
static bool serialReset = false;
static bool manualArmed = false;  // manual input must be seen OFF once after boot
static float vFiltered = -1;

static float readPinVolts(int pin) {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(pin);
  return (mv / 16.0f) / 1000.0f;
}

// Pin volts -> loop mA, using the two-point calibration in config.h
static float voltsToMilliAmps(float v) {
  float kpaPerVolt = CAL_KNOWN_KPA / (CAL_V_KNOWN - CAL_V_ZERO);
  float kpa = (v - CAL_V_ZERO) * kpaPerVolt;
  return 4.0f + kpa / settings.rangeKpa * 16.0f;
}

static float readPressureVolts() {
  float v = readPinVolts(PIN_PRESSURE);
  vFiltered = vFiltered < 0 ? v : vFiltered * 0.8f + v * 0.2f;  // smooth noise
  return vFiltered;
}

// ---------- Self-test: confirms the board's pins on the bench ----------
// Run it with the 240 V supply to the contactor coils switched OFF.
static void selfTest() {
  bool relay[8] = {false}, closed[8];
  Serial.println("\n=== SELF-TEST (type any key to stop) ===");
  Serial.println("Step 1: each relay clicks on for 1 s, in order 1 to 8.");
  Serial.println("        Check the relay LED/click matches the number.");
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) relay[j] = j == i;
    board::transfer(relay, closed);
    Serial.printf("  Relay %d ON\n", i + 1);
    delay(1000);
  }
  for (int j = 0; j < 8; j++) relay[j] = false;
  board::transfer(relay, closed);

  Serial.println("Step 2: live readings. Close each input to GND in turn, then");
  Serial.println("        put the transducer on CH1 and watch which pin's volts move.");
  while (Serial.available()) Serial.read();
  while (!Serial.available()) {
    board::transfer(relay, closed);
    Serial.print("  IN1-8: ");
    for (int i = 0; i < 8; i++) Serial.print(closed[i] ? "X" : ".");
    Serial.print("  | pin V:");
    for (int i = 0; i < board::ADC_CANDIDATE_COUNT; i++)
      Serial.printf(" %d=%.2f", board::ADC_CANDIDATES[i], readPinVolts(board::ADC_CANDIDATES[i]));
    Serial.println();
    delay(500);
  }
  while (Serial.available()) Serial.read();
  Serial.println("=== SELF-TEST done, back to normal ===\n");
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
  board::begin();
  bool off[8] = {false}, closed[8];
  board::transfer(off, closed);  // all relays off straight away

  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Saturn Pump Controller - Mode 1 (bench, ES32D26) ===");
  Serial.println("Commands: r = reset, m = manual run, t = self-test");
  analogSetAttenuation(ADC_11db);

  for (int i = 0; i < 8; i++) {
    if (INPUT_MAP[i].fn == InFn::Pump1Running) settings.pumpFeedback[0] = true;
    if (INPUT_MAP[i].fn == InFn::Pump2Running) settings.pumpFeedback[1] = true;
  }
  ctrl = PumpController(settings);
  loadCounters();

  // Refuse to run on settings that clash (relays stay off)
  Issue issue = validate(settings);
  if (issue != Issue::None) {
    while (true) {
      Serial.printf("!! CHECK SETTINGS in config.h: %s\n", issueText(issue));
      delay(5000);
    }
  }
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
    if (c == 't') {
      bool off[8] = {false}, closed[8];
      board::transfer(off, closed);
      selfTest();
      now = millis();
    }
  }

  // ---- Read inputs ----
  static bool relay[8] = {false};
  bool closed[8];
  board::transfer(relay, closed);

  Inputs in;
  float volts = readPressureVolts();
  in.mA = voltsToMilliAmps(volts);
  bool haveSwitch = false, switchOn = false, manualInput = false;
  for (int i = 0; i < 8; i++) {
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
      case InFn::Manual: manualInput |= active; break;
      case InFn::None: break;
    }
  }
  // A manual input stuck on at power-up (wiring fault) is ignored until released
  if (!manualInput) manualArmed = true;
  in.manual = (manualArmed && manualInput) || serialManual;
  in.enable = remoteLatch && (!haveSwitch || switchOn);
  in.reset |= serialReset;
  serialReset = false;

  Outputs out = ctrl.update(in, now);

  relay[0] = out.pump[0];
  relay[1] = out.pump[1];
  relay[2] = out.alarm;
  relay[3] = out.aux;
  board::transfer(relay, closed);

  saveCounters(now);

  // ---- Status line once a second ----
  static uint32_t lastPrint = 0;
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    Serial.printf("%4.0f kPa (%5.2f mA, pin %4.2f V) | %-16s | P1 %s P2 %s | lead P%d | %s",
                  ctrl.pressure(), in.mA, volts, stateName(ctrl.state()),
                  out.pump[0] ? "ON " : "off", out.pump[1] ? "ON " : "off", ctrl.lead() + 1,
                  faultName(ctrl.fault()));
    uint32_t wait = ctrl.retryInMs(now);
    if (wait) Serial.printf(" | retry in %lu s", (unsigned long)(wait / 1000));
    Serial.println();
  }

  delay(50);
}
