// =====================================================================
//  SATURN PUMP CONTROLLER: BENCH SETTINGS
//  Change the values below, then upload again. (The phone app replaces
//  this file later.)
// =====================================================================
#pragma once
#include "pump_logic.h"

// ---------- Pressure settings ----------
inline Settings makeSettings() {
  Settings s;
  s.rangeBar        = 10.0f;   // transducer full scale (0-10 or 0-16 bar)
  s.cutInBar        = 2.0f;    // pump starts below this
  s.cutOutBar       = 3.5f;    // pump stops at this
  s.highCutoutBar   = 6.0f;    // emergency stop above this
  s.dryRunBar       = 0.5f;    // below this while running = dry
  s.dryRunMs        = 30000;   // ...for this long (ms) = dry run trip
  s.minRunMs        = 10000;
  s.restartDelayMs  = 5000;
  s.powerUpDelayMs  = 10000;
  s.maxRunMs        = 60UL * 60000;  // 60 min, 0 = off
  s.maxRetries      = 0;       // 0 = retry forever
  s.maxStarts       = 6;       // starts allowed in the window, 0 = off
  s.cycleWindowMs   = 10UL * 60000;
  s.rapidCycleLockout = true;  // false = alarm only

  // ---------- Pumps ----------
  s.pumps           = 1;       // 1 or 2
  s.alternate       = true;
  s.assistMarginBar = 0.5f;
  s.assistDelayMs   = 10000;
  s.startConfirmMs  = 3000;
  return s;
}

// ---------- Digital inputs DI1..DI6 ----------
enum class InFn : uint8_t {
  None,
  Pump1Running,  // contactor aux
  Pump1Fault,    // overload aux
  Pump2Running,
  Pump2Fault,
  RemoteSwitch,  // closed = run allowed, open = stop
  RemoteStart,   // momentary start button
  RemoteStop,    // momentary stop button (stop always wins)
  Float,         // closed = tank OK
  Reset,         // momentary reset button
  Manual,        // hold to run
};

struct InputCfg {
  InFn fn;
  bool normallyClosed;  // true = contact opens to signal (e.g. N/C overload)
};

// Change what each input does here. Unused = InFn::None
const InputCfg INPUT_MAP[6] = {
  {InFn::Pump1Running, false},  // DI1
  {InFn::Pump1Fault,   false},  // DI2
  {InFn::Pump2Running, false},  // DI3
  {InFn::Pump2Fault,   false},  // DI4
  {InFn::RemoteSwitch, false},  // DI5
  {InFn::Float,        false},  // DI6
};

// ---------- Outputs (relays) ----------
// Relay 1 = Pump 1, Relay 2 = Pump 2, Relay 3 = Fault alarm,
// Relay 4 = System healthy (on when everything is OK). Relays 5-6 spare.

// ---------- KinCony KC868-A6 hardware ----------
// Confirm these against your board revision. The I2C scan at start-up
// prints what it finds.
const int PIN_SDA = 4;
const int PIN_SCL = 15;
const uint8_t ADDR_INPUTS = 0x22;
const uint8_t ADDR_RELAYS = 0x24;
const int PIN_PRESSURE = 36;    // analogue input A1

// 4-20 mA through a 250 ohm resistor = 1-5 V on A1.
// CALIBRATE: put a known signal in (or measure volts at the A1 terminal)
// and adjust until the serial monitor's "V" reading matches your meter.
const float A1_SCALE = 5000.0f / 3300.0f;  // terminal volts per ADC volt
const float SHUNT_OHMS = 250.0f;
