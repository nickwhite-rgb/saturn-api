// Saturn pump controller logic: 4 modes (see pump-controller/MODES.md).
//   1 Pressure Pump  2 Pressure Guard  3 Snap Switch  4 Twin Pump
// Pure C++ with no Arduino calls, so it can be tested on a PC.
// Pressures are kPa. Time is milliseconds (wrap-safe unsigned maths).
#pragma once
#include <stdint.h>

enum class Mode : uint8_t { PressurePump = 1, Guard = 2, Snap = 3, Twin = 4 };

// What happens after a dry run / low pressure trip
enum class Restart : uint8_t {
  Off,      // stay locked out until reset
  Single,   // try once after the delay, then lock out
  Repeat,   // keep trying every delay (or up to maxRetries)
  Stepped,  // 10 min, 30 min, then every 60 min
};

// Conditions that can drive the alarm and aux relays (tick any combination)
namespace Alarm {
enum : uint16_t {
  AnyFault = 1 << 0,
  HiLoLimit = 1 << 1,   // pressure at/above high limit or at/below trip
  PumpOff = 1 << 2,
  PumpOn = 1 << 3,
  HighFault = 1 << 4,
  LowFault = 1 << 5,    // dry run / low pressure trip
  BatchDone = 1 << 6,
  AntiCycle = 1 << 7,
  PreStart = 1 << 8,    // e.g. run a mixer before the pump
  SensorFault = 1 << 9,
  PumpFault = 1 << 10,  // overload / contactor fail
  Healthy = 1 << 11,    // no fault, not locked out
};
}

enum class Fault : uint8_t {
  None,
  Sensor,        // transducer broken / unplugged / frost-blown
  HighPressure,  // above high limit (auto clears below stop)
  LowPressure,   // guard mode low trip
  LowTank,       // float open (auto clears)
  DryRun,        // waiting to retry
  DryRunLockout, // no more retries
  AntiCycle,     // too many starts in a row
  MaxRun,        // ran too long continuously (burst pipe)
  Pump1,         // pump 1 overload input or failed to start
  Pump2,
  NoPumps,       // every configured pump is locked out
};
const char* faultName(Fault f);

enum class State : uint8_t {
  PowerUp, Stopped, Idle, PreStart, Running, DryRunWait, Lockout, Fault, Manual, BatchDone,
};
const char* stateName(State s);

struct Settings {
  Mode mode = Mode::PressurePump;
  float rangeKpa = 1000;         // transducer full scale (20 mA)

  // --- Pressures (kPa) ---
  float startKpa = 200;          // pump 1 / lead pump start
  float stopKpa = 350;           // pump 1 / lead pump stop
  float start2Kpa = 200;         // twin: lag pump start
  float stop2Kpa = 350;          // twin: lag pump stop
  float tripKpa = 50;            // loss of prime / low trip, 0 = off
  float highLimitKpa = 600;      // absolute high limit / guard high trip, 0 = off
  float snapSetKpa = 300;        // snap: set point
  float snapDeltaKpa = 14;       // snap: differential
  bool snapOnRising = false;     // snap: relay on above set (true) or below set (false)

  // --- Timers (ms) ---
  uint32_t powerUpDelayMs = 10000;
  uint32_t preStartMs = 0;       // delay before each start, 0 = off
  uint32_t startupMs = 5000;     // ignore low pressure while the pump gets going
  uint32_t runOnMs = 5000;       // tolerate dips below trip this long, 0 = instant
  uint32_t minRunMs = 10000;
  uint32_t restartDelayMs = 5000;  // delayed restart after a normal stop
  uint32_t hpRunOnMs = 0;        // high-pressure run-on band, 0 = off (never for PD/helical pumps)
  uint32_t batchMs = 0;          // timed run then stop, 0 = off
  uint32_t maxRunMs = 60UL * 60000;  // 0 = off

  // --- Dry run restart ---
  Restart restart = Restart::Stepped;
  uint32_t restartAfterMs = 10UL * 60000;  // for Single / Repeat
  uint8_t maxRetries = 0;        // Repeat: 0 = forever

  // --- Anti-cycling ---
  uint16_t maxStartsPerHour = 0; // 0 = off
  uint8_t startsInSequence = 5;
  bool antiCycleLockout = true;  // false = alarm only

  // --- Twin ---
  bool alternate = true;         // false = duty/standby
  uint8_t alternateEvery = 2;    // 2 = every other start ... 240

  // --- Wiring / protection ---
  bool pumpFeedback[2] = {false, false};
  uint32_t startConfirmMs = 3000;
  uint32_t sensorFaultMs = 2000;

  // --- Relays ---
  uint16_t alarmMask = Alarm::AnyFault;
  uint16_t auxMask = Alarm::Healthy;
};

Settings defaultsFor(Mode m);

// Settings sanity check. Returns the first problem found (or None).
enum class Issue : uint8_t {
  None,
  StartZero,
  StopNotAboveStart,
  StartNotAboveTrip,
  HighNotAboveStop,
  Stop2NotAboveStart2,
  Start2NotAboveTrip,
  GuardHighNotAboveLow,
  SnapDeltaZero,
  BatchWithRestart,
  AlternateRange,
};
Issue validate(const Settings& s);
const char* issueText(Issue i);

struct Inputs {
  float mA = 0;
  bool running[2] = {false, false};
  bool pumpFault[2] = {false, false};
  bool enable = true;
  bool tankOk = true;
  bool reset = false;
  bool manual = false;
};

struct Outputs {
  bool pump[2] = {false, false};
  bool alarm = false;
  bool aux = false;
};

class PumpController {
 public:
  explicit PumpController(const Settings& s) : s_(s) {}

  Outputs update(const Inputs& in, uint32_t now);

  State state() const { return state_; }
  Fault fault() const { return fault_; }
  float pressure() const { return pressure_; }
  bool sensorOk() const { return sensorOk_; }
  uint8_t lead() const { return lead_; }
  uint8_t retries() const { return retries_; }
  uint32_t starts(uint8_t p) const { return starts_[p]; }
  uint32_t runSeconds(uint8_t p) const { return (uint32_t)(runMs_[p] / 1000); }
  bool pumpLocked(uint8_t p) const { return pumpLocked_[p]; }
  uint32_t retryInMs(uint32_t now) const;
  uint8_t startsToAlternate() const;  // twin: starts until the other pump leads
  const Settings& settings() const { return s_; }

  void restoreCounters(const uint32_t starts[2], const uint32_t runSec[2]);

 private:
  uint8_t pumpCount() const { return s_.mode == Mode::Twin ? 2 : 1; }
  bool available(uint8_t p) const { return p < pumpCount() && !pumpLocked_[p]; }
  bool anyOn() const { return on_[0] || on_[1]; }
  bool restartReady(uint32_t now) const;
  void startPump(uint8_t p, uint32_t now);
  void stopPump(uint8_t p, uint32_t now);
  void stopAll(uint32_t now);
  void lockout(Fault f, uint32_t now);
  void lockPump(uint8_t p, uint32_t now);
  bool recordStart(uint32_t now);  // anti-cycling, false = locked out
  void tripFault(Fault f, uint32_t now);  // stop, then restart per settings
  bool requestStart(uint8_t p, uint32_t now);  // honours pre-start delay, true = started
  bool lowPressureCheck(uint32_t now);  // shared startup + run-on logic, true = tripped
  uint32_t retryDelayMs() const;

  void runPressurePump(uint32_t now);
  void runGuard(uint32_t now);
  void runSnap();
  void runTwin(uint32_t now);
  void setAlarms(Outputs& out) const;

  Settings s_;
  State state_ = State::PowerUp;
  Fault fault_ = Fault::None;
  float pressure_ = 0;
  bool sensorOk_ = false;
  bool sensorBadTiming_ = false;
  uint32_t sensorBadSince_ = 0;
  bool booted_ = false;
  bool poweredUp_ = false;
  uint32_t bootTime_ = 0;
  uint32_t lastTick_ = 0;

  bool lockedOut_ = false;
  Fault lockFault_ = Fault::None;
  bool dryWait_ = false;
  uint32_t dryRunAt_ = 0;
  Fault dryFault_ = Fault::DryRun;
  uint8_t retries_ = 0;
  bool batchDone_ = false;
  bool antiCycleAlarm_ = false;

  bool on_[2] = {false, false};
  uint32_t onSince_[2] = {0, 0};
  bool pending_[2] = {false, false};  // pre-start delay running
  uint32_t pendingSince_[2] = {0, 0};
  bool pumpLocked_[2] = {false, false};
  Fault pumpLockFault_ = Fault::None;
  uint8_t lead_ = 0;
  uint8_t altCount_ = 0;
  uint32_t runStart_ = 0;
  bool startupDone_ = false;
  uint32_t lastStop_ = 0;
  bool everStopped_ = false;
  bool lowTiming_ = false;
  uint32_t lowSince_ = 0;
  bool hpTiming_ = false;
  uint32_t hpSince_ = 0;

  uint32_t lastStartAt_ = 0;
  bool anyStart_ = false;
  uint8_t streak_ = 0;

  uint32_t starts_[2] = {0, 0};
  uint64_t runMs_[2] = {0, 0};
};
