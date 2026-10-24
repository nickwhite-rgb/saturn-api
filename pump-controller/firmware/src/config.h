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
// Inputs are NPN opto-isolated: wire a volt-free contact between INx and GND.
const InputCfg INPUT_MAP[8] = {
  {InFn::Pump1Running, false},  // IN1
  {InFn::Pump1Fault,   false},  // IN2
  {InFn::Pump2Running, false},  // IN3
  {InFn::Pump2Fault,   false},  // IN4
  {InFn::RemoteSwitch, false},  // IN5
  {InFn::Float,        false},  // IN6
  {InFn::Reset,        false},  // IN7 (reset pushbutton)
  {InFn::Manual,       false},  // IN8 (manual run pushbutton, hold)
};

// ---------- Outputs (relays) ----------
// Relay 1 = Pump 1, Relay 2 = Pump 2, Relay 3 = Fault alarm,
// Relay 4 = System healthy (on when everything is OK). Relays 5-8 spare.

// ---------- Pressure transducer (4-20 mA into CH1) ----------
// PIN: confirm with the self-test (`t`), then set it here.
const int PIN_PRESSURE = 36;

// CALIBRATION (two points, read the "pin V" value from the serial monitor):
//  1. Transducer vented to air (0 bar)          -> CAL_V_ZERO
//  2. Pressurise, read the gauge (e.g. 3.0 bar)  -> CAL_V_KNOWN at CAL_KNOWN_BAR
// The defaults are a guess until you calibrate.
const float CAL_V_ZERO = 0.40f;
const float CAL_V_KNOWN = 1.00f;
const float CAL_KNOWN_BAR = 3.0f;
