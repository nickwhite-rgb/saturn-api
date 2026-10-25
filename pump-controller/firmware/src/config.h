// =====================================================================
//  SATURN PUMP CONTROLLER: BENCH SETTINGS
//  Change the values below, then upload again. (The phone app replaces
//  this file later.)
// =====================================================================
#pragma once
#include "pump_logic.h"

// ---------- Mode and settings (all pressures in kPa) ----------
// Modes: PressurePump, Guard, Snap, Twin. See pump-controller/MODES.md.
inline Settings makeSettings() {
  Settings s = defaultsFor(Mode::PressurePump);
  s.mode            = Mode::PressurePump;  // or Mode::Twin / Mode::Guard / Mode::Snap
  s.rangeKpa        = 1000;    // transducer full scale (1000 = 0-10 bar, 1600 = 0-16 bar)

  // Pressures
  s.startKpa        = 200;     // pump starts below this
  s.stopKpa         = 350;     // pump stops at this
  s.tripKpa         = 50;      // loss of prime / dry run trip, 0 = off
  s.highLimitKpa    = 600;     // absolute high limit, 0 = off

  // Timers (ms)
  s.powerUpDelayMs  = 10000;
  s.startupMs       = 5000;    // time for pressure to come up after a start
  s.runOnMs         = 5000;    // tolerate dips below trip this long
  s.minRunMs        = 10000;
  s.restartDelayMs  = 5000;    // delayed restart after a normal stop
  s.preStartMs      = 0;       // delay before each start, 0 = off
  s.hpRunOnMs       = 0;       // high-pressure run-on, 0 = off. NEVER on helical/PD pumps
  s.batchMs         = 0;       // timed run, 0 = off (needs restart = Off)
  s.maxRunMs        = 60UL * 60000;

  // Dry run restart: Off, Single, Repeat, Stepped (10/30/60 min)
  s.restart         = Restart::Stepped;
  s.restartAfterMs  = 10UL * 60000;  // for Single / Repeat

  // Anti-cycling (bore motors): 0 = off
  s.maxStartsPerHour = 0;
  s.startsInSequence = 5;
  s.antiCycleLockout = true;

  // Twin pump (mode Twin only)
  s.start2Kpa       = 180;
  s.stop2Kpa        = 350;
  s.alternate       = true;
  s.alternateEvery  = 2;
  s.startConfirmMs  = 3000;

  // Relay 3 = alarm, relay 4 = aux. Combine with |, e.g. Alarm::LowFault | Alarm::SensorFault
  s.alarmMask       = Alarm::AnyFault;
  s.auxMask         = Alarm::Healthy;
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
// Relay 1 = Pump 1 (Guard: "system OK", Snap: switch output), Relay 2 = Pump 2,
// Relay 3 = alarm (alarmMask), Relay 4 = aux (auxMask). Relays 5-8 spare.

// ---------- Pressure transducer (4-20 mA into CH1) ----------
// PIN: confirm with the self-test (`t`), then set it here.
const int PIN_PRESSURE = 36;

// CALIBRATION (two points, read the "pin V" value from the serial monitor):
//  1. Transducer vented to air (0 bar)          -> CAL_V_ZERO
//  2. Pressurise, read the gauge (e.g. 300 kPa)  -> CAL_V_KNOWN at CAL_KNOWN_KPA
// The defaults are a guess until you calibrate.
const float CAL_V_ZERO = 0.40f;
const float CAL_V_KNOWN = 1.00f;
const float CAL_KNOWN_KPA = 300.0f;
