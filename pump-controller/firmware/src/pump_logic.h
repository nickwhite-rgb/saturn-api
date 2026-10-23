// Mode 1: Pressure pump logic (E30-style), with optional twin pump.
// Pure C++ with no Arduino calls, so it can be tested on a PC.
// Time is passed in as milliseconds (wrap-safe unsigned maths).
#pragma once
#include <stdint.h>

enum class Fault : uint8_t {
  None,
  Sensor,        // transducer broken / unplugged / frost-blown (<3.6 mA or >21 mA)
  HighPressure,  // above high pressure cutout (auto clears below cut-out)
  LowTank,       // float open (auto clears)
  DryRun,        // waiting to retry
  DryRunLockout, // max retries used up
  RapidCycle,    // too many starts in the cycle window
  MaxRun,        // ran too long continuously (burst pipe)
  Pump1,         // pump 1 overload input or failed to start
  Pump2,         // pump 2 overload input or failed to start
  NoPumps,       // every configured pump is locked out
};

const char* faultName(Fault f);

enum class State : uint8_t { PowerUp, Stopped, Idle, Running, DryRunWait, Lockout, Fault, Manual };

const char* stateName(State s);

struct Settings {
  float rangeBar = 10.0f;          // transducer full scale (20 mA)
  float cutInBar = 2.0f;
  float cutOutBar = 3.5f;
  float highCutoutBar = 6.0f;
  float dryRunBar = 0.5f;
  uint32_t dryRunMs = 30000;
  uint32_t minRunMs = 10000;
  uint32_t restartDelayMs = 5000;
  uint32_t powerUpDelayMs = 10000;
  uint32_t maxRunMs = 60UL * 60000;  // 0 = off
  uint8_t maxRetries = 0;            // 0 = retry forever
  uint8_t maxStarts = 6;             // 0 = off
  uint32_t cycleWindowMs = 10UL * 60000;
  bool rapidCycleLockout = true;     // false = alarm only
  uint32_t sensorFaultMs = 2000;     // signal must be bad this long before tripping

  // Twin pump
  uint8_t pumps = 1;                 // 1 or 2
  bool alternate = true;             // swap lead pump each start
  float assistMarginBar = 0.5f;
  uint32_t assistDelayMs = 10000;

  // What is wired up (set from the input map)
  bool pumpFeedback[2] = {false, false};  // running feedback fitted
  uint32_t startConfirmMs = 3000;
};

struct Inputs {
  float mA = 0;                  // transducer current
  bool running[2] = {false, false};  // contactor aux feedback
  bool pumpFault[2] = {false, false};
  bool enable = true;            // remote start/stop result (true = allowed to run)
  bool tankOk = true;            // float closed (true if not fitted)
  bool reset = false;            // reset pressed
  bool manual = false;           // manual run held
};

struct Outputs {
  bool pump[2] = {false, false};
  bool alarm = false;
};

class PumpController {
 public:
  explicit PumpController(const Settings& s) : s_(s) {}

  // Call often (every 50-100 ms). Returns what the relays should do.
  Outputs update(const Inputs& in, uint32_t now);

  // Status for display / app / logging
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

  // Restore counters (from flash) on boot
  void restoreCounters(const uint32_t starts[2], const uint32_t runSec[2]);

 private:
  bool available(uint8_t p) const { return p < s_.pumps && !pumpLocked_[p]; }
  void startPump(uint8_t p, uint32_t now);
  void stopAll(uint32_t now);
  void lockout(Fault f, uint32_t now);
  void lockPump(uint8_t p, uint32_t now);
  bool anyOn() const { return on_[0] || on_[1]; }
  bool rapidCycling(uint32_t now) const;
  uint32_t retryDelayMs() const;

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

  bool lockedOut_ = false;      // latched until reset
  Fault lockFault_ = Fault::None;
  bool dryWait_ = false;        // waiting to retry after dry run
  uint32_t dryRunAt_ = 0;
  uint8_t retries_ = 0;
  bool rapidAlarm_ = false;     // rapid cycling, alarm-only mode

  bool on_[2] = {false, false};
  uint32_t onSince_[2] = {0, 0};
  bool pumpLocked_[2] = {false, false};
  Fault pumpLockFault_ = Fault::None;
  uint8_t lead_ = 0;
  uint32_t runStart_ = 0;       // when this demand cycle started
  uint32_t lastStop_ = 0;
  bool everStopped_ = false;
  bool lowTiming_ = false;
  uint32_t lowSince_ = 0;
  bool assistTiming_ = false;
  uint32_t assistSince_ = 0;

  static const uint8_t kHist = 32;
  uint32_t startHist_[kHist] = {0};
  uint8_t histCount_ = 0;
  uint8_t histHead_ = 0;

  uint32_t starts_[2] = {0, 0};
  uint64_t runMs_[2] = {0, 0};
  uint32_t lastTick_ = 0;
};
