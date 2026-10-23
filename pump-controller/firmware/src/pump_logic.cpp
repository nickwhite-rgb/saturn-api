#include "pump_logic.h"

const char* faultName(Fault f) {
  switch (f) {
    case Fault::None: return "OK";
    case Fault::Sensor: return "SENSOR - replace transducer";
    case Fault::HighPressure: return "HIGH PRESSURE";
    case Fault::LowTank: return "LOW TANK";
    case Fault::DryRun: return "DRY RUN - will retry";
    case Fault::DryRunLockout: return "DRY RUN LOCKOUT";
    case Fault::RapidCycle: return "RAPID CYCLING - check for leaks / tank";
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
    case State::Running: return "RUNNING";
    case State::DryRunWait: return "RETRY WAIT";
    case State::Lockout: return "LOCKOUT";
    case State::Fault: return "FAULT";
    case State::Manual: return "MANUAL";
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
  // 10 min, 30 min, then every 60 min
  if (retries_ <= 1) return 10UL * 60000;
  if (retries_ == 2) return 30UL * 60000;
  return 60UL * 60000;
}

uint32_t PumpController::retryInMs(uint32_t now) const {
  if (!dryWait_) return 0;
  uint32_t gone = now - dryRunAt_;
  uint32_t d = retryDelayMs();
  return gone >= d ? 0 : d - gone;
}

void PumpController::startPump(uint8_t p, uint32_t now) {
  if (on_[p]) return;
  on_[p] = true;
  onSince_[p] = now;
  starts_[p]++;
}

void PumpController::stopAll(uint32_t now) {
  if (anyOn()) {
    lastStop_ = now;
    everStopped_ = true;
  }
  on_[0] = on_[1] = false;
  lowTiming_ = false;
  assistTiming_ = false;
}

void PumpController::lockout(Fault f, uint32_t now) {
  stopAll(now);
  lockedOut_ = true;
  lockFault_ = f;
  dryWait_ = false;
}

void PumpController::lockPump(uint8_t p, uint32_t now) {
  if (pumpLocked_[p]) return;
  pumpLocked_[p] = true;
  pumpLockFault_ = p == 0 ? Fault::Pump1 : Fault::Pump2;
  bool wasRunning = on_[p];
  on_[p] = false;
  if (wasRunning && !anyOn()) {
    // Failover: hand the demand to the other pump if it can run
    uint8_t other = 1 - p;
    if (available(other)) {
      startPump(other, now);
      lead_ = other;
    } else {
      lastStop_ = now;
      everStopped_ = true;
    }
  }
  bool any = false;
  for (uint8_t i = 0; i < s_.pumps; i++) any |= !pumpLocked_[i];
  if (!any) lockout(s_.pumps == 1 ? pumpLockFault_ : Fault::NoPumps, now);
}

bool PumpController::rapidCycling(uint32_t now) const {
  if (s_.maxStarts == 0) return false;
  uint8_t n = 0;
  for (uint8_t i = 0; i < histCount_; i++) {
    if (now - startHist_[i] < s_.cycleWindowMs) n++;
  }
  return n > s_.maxStarts;
}

Outputs PumpController::update(const Inputs& in, uint32_t now) {
  Outputs out;

  // --- Run hour counters ---
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
  bool valid = in.mA >= 3.6f && in.mA <= 21.0f;
  if (valid) {
    sensorOk_ = true;
    sensorBadTiming_ = false;
    float bar = (in.mA - 4.0f) / 16.0f * s_.rangeBar;
    pressure_ = bar < 0 ? 0 : bar;
  } else {
    if (!sensorBadTiming_) {
      sensorBadTiming_ = true;
      sensorBadSince_ = now;
    }
    if (now - sensorBadSince_ >= s_.sensorFaultMs) sensorOk_ = false;
  }

  // --- Reset button clears latched faults ---
  if (in.reset) {
    lockedOut_ = false;
    lockFault_ = Fault::None;
    pumpLocked_[0] = pumpLocked_[1] = false;
    pumpLockFault_ = Fault::None;
    dryWait_ = false;
    retries_ = 0;
    rapidAlarm_ = false;
    histCount_ = 0;
    histHead_ = 0;
  }

  // --- Pump overload inputs and fail-to-start ---
  for (uint8_t p = 0; p < s_.pumps; p++) {
    if (in.pumpFault[p]) lockPump(p, now);
    if (on_[p] && s_.pumpFeedback[p] && !in.running[p] &&
        now - onSince_[p] >= s_.startConfirmMs)
      lockPump(p, now);
  }

  // --- Manual run (held button). Still protected by sensor + high pressure ---
  if (in.manual && sensorOk_ && pressure_ < s_.highCutoutBar) {
    uint8_t p = lead_;
    if (in.pumpFault[p] && s_.pumps == 2) p = 1 - p;
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
      return out;
    }
  }
  if (state_ == State::Manual) stopAll(now);

  // --- Power-up delay ---
  if (!poweredUp_) {
    if (now - bootTime_ < s_.powerUpDelayMs) {
      state_ = State::PowerUp;
      fault_ = Fault::None;
      return out;
    }
    poweredUp_ = true;
  }

  // --- Blocking conditions: pump off, priority order ---
  Fault block = Fault::None;
  State blockState = State::Fault;
  if (!sensorOk_) block = Fault::Sensor;
  else if (lockedOut_) { block = lockFault_; blockState = State::Lockout; }
  else if (pressure_ >= s_.highCutoutBar) block = Fault::HighPressure;
  else if (!in.tankOk) block = Fault::LowTank;
  else if (!in.enable) { blockState = State::Stopped; }

  if (block != Fault::None || blockState == State::Stopped) {
    stopAll(now);
    state_ = blockState;
    fault_ = block;
    out.alarm = block != Fault::None;
    // High pressure auto clears only once pressure drops back below cut-out
    return out;
  }

  // --- Dry run retry wait ---
  if (dryWait_) {
    if (retryInMs(now) > 0) {
      state_ = State::DryRunWait;
      fault_ = Fault::DryRun;
      out.alarm = true;
      return out;
    }
    dryWait_ = false;  // time to retry
  }

  if (rapidAlarm_ && !rapidCycling(now)) rapidAlarm_ = false;

  if (!anyOn()) {
    // --- Standby: start on low pressure ---
    state_ = State::Idle;
    bool delayDone = !everStopped_ || now - lastStop_ >= s_.restartDelayMs;
    if (pressure_ < s_.cutInBar && delayDone) {
      uint8_t p = available(lead_) ? lead_ : 1 - lead_;
      if (available(p)) {
        lead_ = p;
        startPump(p, now);
        runStart_ = now;
        state_ = State::Running;
        startHist_[histHead_] = now;
        histHead_ = (histHead_ + 1) % kHist;
        if (histCount_ < kHist) histCount_++;
        if (rapidCycling(now)) {
          if (s_.rapidCycleLockout) {
            lockout(Fault::RapidCycle, now);
            state_ = State::Lockout;
            fault_ = Fault::RapidCycle;
            out.alarm = true;
            return out;
          }
          rapidAlarm_ = true;
        }
      }
    }
  } else {
    // --- Running ---
    state_ = State::Running;
    uint32_t ran = now - runStart_;

    // Dry run: pressure stays below threshold for dry-run time
    if (pressure_ < s_.dryRunBar) {
      if (!lowTiming_) {
        lowTiming_ = true;
        lowSince_ = now;
      } else if (now - lowSince_ >= s_.dryRunMs) {
        stopAll(now);
        retries_++;
        if (s_.maxRetries && retries_ > s_.maxRetries) {
          lockout(Fault::DryRunLockout, now);
          state_ = State::Lockout;
          fault_ = Fault::DryRunLockout;
        } else {
          dryWait_ = true;
          dryRunAt_ = now;
          state_ = State::DryRunWait;
          fault_ = Fault::DryRun;
        }
        out.alarm = true;
        return out;
      }
    } else {
      lowTiming_ = false;
    }

    // Max run time (burst pipe)
    if (s_.maxRunMs && ran >= s_.maxRunMs) {
      lockout(Fault::MaxRun, now);
      state_ = State::Lockout;
      fault_ = Fault::MaxRun;
      out.alarm = true;
      return out;
    }

    // Normal stop at cut-out
    if (pressure_ >= s_.cutOutBar && ran >= s_.minRunMs) {
      stopAll(now);
      state_ = State::Idle;
      retries_ = 0;  // good run, reset dry run retries
      if (s_.pumps == 2 && s_.alternate && available(1 - lead_)) lead_ = 1 - lead_;
    } else if (s_.pumps == 2 && !(on_[0] && on_[1])) {
      // Assist: second pump joins if pressure keeps falling
      uint8_t other = on_[0] ? 1 : 0;
      if (available(other) && pressure_ < s_.cutInBar - s_.assistMarginBar) {
        if (!assistTiming_) {
          assistTiming_ = true;
          assistSince_ = now;
        } else if (now - assistSince_ >= s_.assistDelayMs) {
          startPump(other, now);
        }
      } else {
        assistTiming_ = false;
      }
    }
  }

  // Non-blocking faults still raise the alarm
  if (pumpLocked_[0] || pumpLocked_[1]) fault_ = pumpLockFault_;
  else if (rapidAlarm_) fault_ = Fault::RapidCycle;
  else fault_ = Fault::None;

  out.pump[0] = on_[0];
  out.pump[1] = on_[1];
  out.alarm = fault_ != Fault::None;
  return out;
}
