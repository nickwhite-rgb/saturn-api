#include "pump_logic.h"

const char* faultName(Fault f) {
  switch (f) {
    case Fault::None: return "OK";
    case Fault::Sensor: return "SENSOR FAULT - replace transducer";
    case Fault::HighPressure: return "HIGH PRESSURE";
    case Fault::LowPressure: return "LOW PRESSURE";
    case Fault::LowTank: return "LOW TANK";
    case Fault::DryRun: return "DRY RUN - will retry";
    case Fault::DryRunLockout: return "DRY RUN - locked out";
    case Fault::AntiCycle: return "TOO MANY STARTS - check tank / leaks";
    case Fault::MaxRun: return "MAX RUN TIME - check for burst pipe";
    case Fault::Pump1: return "PUMP 1 FAULT - overload / contactor";
    case Fault::Pump2: return "PUMP 2 FAULT - overload / contactor";
    case Fault::NoPumps: return "NO PUMPS AVAILABLE";
  }
  return "?";
}

const char* stateName(State s) {
  switch (s) {
    case State::PowerUp: return "POWER UP";
    case State::Stopped: return "STOPPED (remote)";
    case State::Idle: return "STANDBY";
    case State::PreStart: return "PRE-START";
    case State::Running: return "RUNNING";
    case State::DryRunWait: return "RETRY WAIT";
    case State::Lockout: return "LOCKOUT";
    case State::Fault: return "FAULT";
    case State::Manual: return "MANUAL";
    case State::BatchDone: return "BATCH DONE";
  }
  return "?";
}

Settings defaultsFor(Mode m) {
  Settings s;
  s.mode = m;
  switch (m) {
    case Mode::PressurePump:
      break;
    case Mode::Guard:
      s.tripKpa = 100;
      s.highLimitKpa = 600;
      s.restart = Restart::Off;
      s.maxRunMs = 0;
      break;
    case Mode::Snap:
      s.tripKpa = 0;
      s.highLimitKpa = 0;
      s.restart = Restart::Off;
      s.maxRunMs = 0;
      break;
    case Mode::Twin:
      s.start2Kpa = 180;  // lag pump assists if pressure keeps falling
      s.stop2Kpa = 350;
      break;
  }
  return s;
}

Issue validate(const Settings& s) {
  switch (s.mode) {
    case Mode::PressurePump:
    case Mode::Twin:
      if (s.startKpa <= 0) return Issue::StartZero;
      if (s.stopKpa <= s.startKpa) return Issue::StopNotAboveStart;
      if (s.tripKpa > 0 && s.startKpa <= s.tripKpa) return Issue::StartNotAboveTrip;
      if (s.mode == Mode::Twin) {
        if (s.stop2Kpa <= s.start2Kpa) return Issue::Stop2NotAboveStart2;
        if (s.tripKpa > 0 && s.start2Kpa <= s.tripKpa) return Issue::Start2NotAboveTrip;
        if (s.alternateEvery < 2 || s.alternateEvery > 240) return Issue::AlternateRange;
      }
      {
        float topStop = s.stopKpa;
        if (s.mode == Mode::Twin && s.stop2Kpa > topStop) topStop = s.stop2Kpa;
        if (s.highLimitKpa > 0 && s.highLimitKpa <= topStop) return Issue::HighNotAboveStop;
      }
      if (s.batchMs && s.restart != Restart::Off) return Issue::BatchWithRestart;
      break;
    case Mode::Guard:
      if (s.highLimitKpa > 0 && s.highLimitKpa <= s.tripKpa) return Issue::GuardHighNotAboveLow;
      break;
    case Mode::Snap:
      if (s.snapDeltaKpa <= 0) return Issue::SnapDeltaZero;
      break;
  }
  return Issue::None;
}

const char* issueText(Issue i) {
  switch (i) {
    case Issue::None: return "OK";
    case Issue::StartZero: return "Start pressure can't be zero.";
    case Issue::StopNotAboveStart: return "Stop must be higher than Start, or the pump would never switch off.";
    case Issue::StartNotAboveTrip: return "Start must be higher than Trip, or it would trip as soon as it starts.";
    case Issue::HighNotAboveStop: return "High limit must be higher than Stop.";
    case Issue::Stop2NotAboveStart2: return "Pump 2 Stop must be higher than Pump 2 Start.";
    case Issue::Start2NotAboveTrip: return "Pump 2 Start must be higher than Trip.";
    case Issue::GuardHighNotAboveLow: return "High trip must be higher than Low trip.";
    case Issue::SnapDeltaZero: return "Differential must be more than zero.";
    case Issue::BatchWithRestart: return "Timed run can't be used with auto restart. Turn auto restart off first.";
    case Issue::AlternateRange: return "Alternate every must be 2 to 240 starts.";
  }
  return "?";
}

void PumpController::restoreCounters(const uint32_t starts[2], const uint32_t runSec[2]) {
  for (int p = 0; p < 2; p++) {
    starts_[p] = starts[p];
    runMs_[p] = (uint64_t)runSec[p] * 1000;
  }
}

uint32_t PumpController::retryDelayMs() const {
  if (s_.restart == Restart::Stepped) {
    if (retries_ <= 1) return 10UL * 60000;
    if (retries_ == 2) return 30UL * 60000;
    return 60UL * 60000;
  }
  return s_.restartAfterMs;
}

uint32_t PumpController::retryInMs(uint32_t now) const {
  if (!dryWait_) return 0;
  uint32_t gone = now - dryRunAt_;
  uint32_t d = retryDelayMs();
  return gone >= d ? 0 : d - gone;
}

uint8_t PumpController::startsToAlternate() const {
  if (s_.mode != Mode::Twin || !s_.alternate) return 0;
  if (lead_ == 1) return 1;
  int left = (int)s_.alternateEvery - 1 - altCount_;
  return left < 1 ? 1 : (uint8_t)left;
}

bool PumpController::restartReady(uint32_t now) const {
  return !everStopped_ || now - lastStop_ >= s_.restartDelayMs;
}

void PumpController::startPump(uint8_t p, uint32_t now) {
  if (on_[p]) return;
  on_[p] = true;
  onSince_[p] = now;
  pending_[p] = false;
  starts_[p]++;
}

void PumpController::stopPump(uint8_t p, uint32_t now) {
  if (!on_[p]) return;
  on_[p] = false;
  if (!anyOn()) {
    lastStop_ = now;
    everStopped_ = true;
    lowTiming_ = false;
    hpTiming_ = false;
  }
}

void PumpController::stopAll(uint32_t now) {
  stopPump(0, now);
  stopPump(1, now);
  pending_[0] = pending_[1] = false;
}

bool PumpController::requestStart(uint8_t p, uint32_t now) {
  if (s_.preStartMs == 0) {
    startPump(p, now);
    return true;
  }
  if (!pending_[p]) {
    pending_[p] = true;
    pendingSince_[p] = now;
    return false;
  }
  if (now - pendingSince_[p] >= s_.preStartMs) {
    startPump(p, now);
    return true;
  }
  return false;
}

void PumpController::lockout(Fault f, uint32_t now) {
  stopAll(now);
  lockedOut_ = true;
  lockFault_ = f;
  dryWait_ = false;
  state_ = State::Lockout;
  fault_ = f;
}

void PumpController::lockPump(uint8_t p, uint32_t now) {
  if (pumpLocked_[p]) return;
  pumpLocked_[p] = true;
  pumpLockFault_ = p == 0 ? Fault::Pump1 : Fault::Pump2;
  bool wasRunning = on_[p];
  stopPump(p, now);
  if (wasRunning && !anyOn()) {
    // Failover: hand the demand to the other pump if it can run
    uint8_t other = 1 - p;
    if (available(other)) {
      startPump(other, now);
      lead_ = other;
    }
  }
  bool any = false;
  for (uint8_t i = 0; i < pumpCount(); i++) any |= !pumpLocked_[i];
  if (!any) lockout(pumpCount() == 1 ? pumpLockFault_ : Fault::NoPumps, now);
}

bool PumpController::recordStart(uint32_t now) {
  if (s_.maxStartsPerHour == 0) return true;
  uint32_t minGap = 3600000UL / s_.maxStartsPerHour;
  if (anyStart_ && now - lastStartAt_ < minGap) streak_++;
  else streak_ = 1;
  anyStart_ = true;
  lastStartAt_ = now;
  if (streak_ >= s_.startsInSequence) {
    if (s_.antiCycleLockout) {
      lockout(Fault::AntiCycle, now);
      return false;
    }
    antiCycleAlarm_ = true;
  } else if (streak_ == 1) {
    antiCycleAlarm_ = false;
  }
  return true;
}

void PumpController::tripFault(Fault f, uint32_t now) {
  stopAll(now);
  retries_++;
  bool lock = false;
  switch (s_.restart) {
    case Restart::Off: lock = true; break;
    case Restart::Single: lock = retries_ > 1; break;
    case Restart::Repeat: lock = s_.maxRetries && retries_ > s_.maxRetries; break;
    case Restart::Stepped: lock = false; break;
  }
  if (lock) {
    lockout(f == Fault::DryRun ? Fault::DryRunLockout : f, now);
    return;
  }
  dryWait_ = true;
  dryRunAt_ = now;
  dryFault_ = f;
  state_ = State::DryRunWait;
  fault_ = f;
}

bool PumpController::lowPressureCheck(uint32_t now) {
  if (s_.tripKpa <= 0) return false;
  Fault f = s_.mode == Mode::Guard ? Fault::LowPressure : Fault::DryRun;
  if (!startupDone_) {
    // Start-up timer: ignore low pressure until pressure comes up or time runs out
    float target = s_.mode == Mode::Guard ? s_.tripKpa : s_.startKpa;
    if (pressure_ >= target) {
      startupDone_ = true;
    } else if (now - runStart_ >= s_.startupMs) {
      startupDone_ = true;
      if (pressure_ < s_.tripKpa) {
        tripFault(f, now);
        return true;
      }
    }
    return false;
  }
  // Run-on (dip) timer: tolerate short dips below trip
  if (pressure_ < s_.tripKpa) {
    if (!lowTiming_) {
      lowTiming_ = true;
      lowSince_ = now;
    }
    if (now - lowSince_ >= s_.runOnMs) {
      tripFault(f, now);
      return true;
    }
  } else {
    lowTiming_ = false;
  }
  return false;
}

// ---------- Mode 1: pressure pump ----------
void PumpController::runPressurePump(uint32_t now) {
  if (batchDone_) {
    state_ = State::BatchDone;
    return;
  }
  if (!anyOn()) {
    state_ = State::Idle;
    if (pressure_ < s_.startKpa && restartReady(now) && available(0)) {
      if (requestStart(0, now)) {
        if (!recordStart(now)) return;
        runStart_ = now;
        startupDone_ = false;
        lowTiming_ = false;
        hpTiming_ = false;
        state_ = State::Running;
      } else {
        state_ = State::PreStart;
      }
    } else {
      pending_[0] = false;
    }
    return;
  }

  state_ = State::Running;
  uint32_t ran = now - runStart_;
  if (lowPressureCheck(now)) return;
  if (s_.batchMs && ran >= s_.batchMs) {
    stopAll(now);
    batchDone_ = true;
    state_ = State::BatchDone;
    return;
  }
  if (s_.maxRunMs && ran >= s_.maxRunMs) {
    lockout(Fault::MaxRun, now);
    return;
  }
  if (pressure_ >= s_.stopKpa) {
    if (ran >= s_.minRunMs) {
      bool stop = true;
      if (s_.hpRunOnMs) {
        // High-pressure run-on: allow running in the stop band for a while
        if (!hpTiming_) {
          hpTiming_ = true;
          hpSince_ = now;
        }
        stop = now - hpSince_ >= s_.hpRunOnMs;
      }
      if (stop) {
        stopAll(now);
        retries_ = 0;
        state_ = State::Idle;
      }
    }
  } else {
    hpTiming_ = false;
  }
}

// ---------- Mode 2: pressure guard ----------
// Relay 1 is "system enabled". Opens on high or low pressure.
void PumpController::runGuard(uint32_t now) {
  if (!on_[0]) {
    startPump(0, now);
    runStart_ = now;
    startupDone_ = false;
    lowTiming_ = false;
  }
  state_ = State::Running;
  if (s_.highLimitKpa > 0 && pressure_ >= s_.highLimitKpa) {
    tripFault(Fault::HighPressure, now);
    return;
  }
  if (lowPressureCheck(now)) return;
}

// ---------- Mode 3: snap switch ----------
void PumpController::runSnap() {
  float set = s_.snapSetKpa, d = s_.snapDeltaKpa;
  bool want = on_[0];
  if (s_.snapOnRising) {
    if (!want && pressure_ >= set) want = true;
    else if (want && pressure_ <= set - d) want = false;
  } else {
    if (!want && pressure_ <= set) want = true;
    else if (want && pressure_ >= set + d) want = false;
  }
  if (want && !on_[0]) startPump(0, lastTick_);
  if (!want && on_[0]) stopPump(0, lastTick_);
  state_ = on_[0] ? State::Running : State::Idle;
}

// ---------- Mode 4: twin pump ----------
void PumpController::runTwin(uint32_t now) {
  if (!available(lead_) && available(1 - lead_)) lead_ = 1 - lead_;
  uint8_t L = lead_, G = 1 - lead_;
  float startAt[2], stopAt[2];
  startAt[L] = s_.startKpa;
  stopAt[L] = s_.stopKpa;
  startAt[G] = s_.start2Kpa;
  stopAt[G] = s_.stop2Kpa;

  bool wasOn = anyOn();
  bool leadStopped = false;
  uint8_t order[2] = {L, G};
  for (uint8_t i = 0; i < 2; i++) {
    uint8_t p = order[i];
    if (!available(p)) continue;
    if (!on_[p]) {
      bool demand = pressure_ < startAt[p];
      if (p == L) demand = demand && restartReady(now);
      // Lag pump only assists once the lead has had its start-up time
      if (p == G) demand = demand && on_[L] && now - onSince_[L] >= s_.startupMs;
      if (demand) {
        if (requestStart(p, now)) {
          if (!recordStart(now)) return;
          if (!wasOn) {
            runStart_ = now;
            startupDone_ = false;
            lowTiming_ = false;
            wasOn = true;
          }
        }
      } else {
        pending_[p] = false;
      }
    } else if (pressure_ >= stopAt[p] && now - onSince_[p] >= s_.minRunMs) {
      stopPump(p, now);
      if (p == L) leadStopped = true;
    }
  }

  if (anyOn()) {
    if (lowPressureCheck(now)) return;
    if (s_.maxRunMs && now - runStart_ >= s_.maxRunMs) {
      lockout(Fault::MaxRun, now);
      return;
    }
  } else if (leadStopped) {
    retries_ = 0;
  }

  if (leadStopped && s_.alternate && available(1 - lead_)) {
    if (lead_ == 0) {
      altCount_++;
      if (altCount_ >= s_.alternateEvery - 1) {
        lead_ = 1;
        altCount_ = 0;
      }
    } else {
      lead_ = 0;
    }
  }
  state_ = anyOn() ? State::Running : (pending_[0] || pending_[1]) ? State::PreStart : State::Idle;
}

void PumpController::setAlarms(Outputs& out) const {
  uint16_t bits = 0;
  bool isFault = fault_ != Fault::None;
  if (isFault) bits |= Alarm::AnyFault;
  if (sensorOk_ && ((s_.highLimitKpa > 0 && pressure_ >= s_.highLimitKpa) ||
                    (s_.tripKpa > 0 && pressure_ <= s_.tripKpa)))
    bits |= Alarm::HiLoLimit;
  bits |= anyOn() ? Alarm::PumpOn : Alarm::PumpOff;
  if (fault_ == Fault::HighPressure) bits |= Alarm::HighFault;
  if (fault_ == Fault::DryRun || fault_ == Fault::DryRunLockout || fault_ == Fault::LowPressure)
    bits |= Alarm::LowFault;
  if (batchDone_) bits |= Alarm::BatchDone;
  if (fault_ == Fault::AntiCycle || antiCycleAlarm_) bits |= Alarm::AntiCycle;
  if (pending_[0] || pending_[1]) bits |= Alarm::PreStart;
  if (fault_ == Fault::Sensor) bits |= Alarm::SensorFault;
  if (pumpLocked_[0] || pumpLocked_[1]) bits |= Alarm::PumpFault;
  if (!isFault && !lockedOut_ && sensorOk_) bits |= Alarm::Healthy;
  out.alarm = (bits & s_.alarmMask) != 0;
  out.aux = (bits & s_.auxMask) != 0;
}

Outputs PumpController::update(const Inputs& in, uint32_t now) {
  Outputs out;

  if (!booted_) {
    booted_ = true;
    bootTime_ = now;
    lastTick_ = now;
  }
  uint32_t dt = now - lastTick_;
  lastTick_ = now;
  for (int p = 0; p < 2; p++)
    if (on_[p]) runMs_[p] += dt;

  // --- Transducer ---
  if (in.mA >= 3.6f && in.mA <= 21.0f) {
    sensorOk_ = true;
    sensorBadTiming_ = false;
    float kpa = (in.mA - 4.0f) / 16.0f * s_.rangeKpa;
    pressure_ = kpa < 0 ? 0 : kpa;
  } else {
    if (!sensorBadTiming_) {
      sensorBadTiming_ = true;
      sensorBadSince_ = now;
    }
    if (now - sensorBadSince_ >= s_.sensorFaultMs) sensorOk_ = false;
  }

  // --- Reset clears latched faults ---
  if (in.reset) {
    lockedOut_ = false;
    lockFault_ = Fault::None;
    pumpLocked_[0] = pumpLocked_[1] = false;
    pumpLockFault_ = Fault::None;
    dryWait_ = false;
    retries_ = 0;
    batchDone_ = false;
    antiCycleAlarm_ = false;
    anyStart_ = false;
    streak_ = 0;
  }

  // --- Pump overload inputs and fail-to-start ---
  for (uint8_t p = 0; p < pumpCount(); p++) {
    if (in.pumpFault[p]) lockPump(p, now);
    if (on_[p] && s_.pumpFeedback[p] && !in.running[p] && now - onSince_[p] >= s_.startConfirmMs)
      lockPump(p, now);
  }

  // --- Manual run (pump modes only). Still protected by sensor + high limit ---
  bool pumpMode = s_.mode == Mode::PressurePump || s_.mode == Mode::Twin;
  bool hpOk = s_.highLimitKpa <= 0 || pressure_ < s_.highLimitKpa;
  if (pumpMode && in.manual && sensorOk_ && hpOk) {
    uint8_t p = lead_;
    if (in.pumpFault[p] && pumpCount() == 2) p = 1 - p;
    if (!in.pumpFault[p]) {
      if (!on_[p]) {
        stopAll(now);
        startPump(p, now);
        runStart_ = now;
      }
      state_ = State::Manual;
      fault_ = Fault::None;
      out.pump[0] = on_[0];
      out.pump[1] = on_[1];
      setAlarms(out);
      return out;
    }
  }
  if (state_ == State::Manual) stopAll(now);

  // --- Power-up delay ---
  if (!poweredUp_) {
    if (now - bootTime_ < s_.powerUpDelayMs) {
      state_ = State::PowerUp;
      fault_ = Fault::None;
      setAlarms(out);
      return out;
    }
    poweredUp_ = true;
  }

  // --- Blocking conditions: outputs off, priority order ---
  Fault block = Fault::None;
  State blockState = State::Fault;
  if (!sensorOk_) block = Fault::Sensor;
  else if (lockedOut_) { block = lockFault_; blockState = State::Lockout; }
  else if (pumpMode && s_.highLimitKpa > 0 && pressure_ >= s_.highLimitKpa) block = Fault::HighPressure;
  else if (!in.tankOk) block = Fault::LowTank;
  else if (!in.enable) blockState = State::Stopped;

  if (block != Fault::None || blockState == State::Stopped) {
    stopAll(now);
    state_ = blockState;
    fault_ = block;
    setAlarms(out);
    return out;
  }

  // --- Waiting to retry after a dry run / low pressure trip ---
  if (dryWait_) {
    if (retryInMs(now) > 0) {
      state_ = State::DryRunWait;
      fault_ = dryFault_;
      setAlarms(out);
      return out;
    }
    dryWait_ = false;
  }

  if (antiCycleAlarm_ && s_.maxStartsPerHour &&
      now - lastStartAt_ >= 3600000UL / s_.maxStartsPerHour * 2)
    antiCycleAlarm_ = false;

  fault_ = Fault::None;
  switch (s_.mode) {
    case Mode::PressurePump: runPressurePump(now); break;
    case Mode::Guard: runGuard(now); break;
    case Mode::Snap: runSnap(); break;
    case Mode::Twin: runTwin(now); break;
  }

  // Non-blocking faults still show and raise the alarm
  if (state_ != State::Lockout && state_ != State::DryRunWait) {
    if (pumpLocked_[0] || pumpLocked_[1]) fault_ = pumpLockFault_;
    else if (antiCycleAlarm_) fault_ = Fault::AntiCycle;
    else fault_ = Fault::None;
  }

  out.pump[0] = on_[0];
  out.pump[1] = on_[1];
  setAlarms(out);
  return out;
}
