// PC tests for the pump logic. Run with:  ./run_tests.sh
#include <cstdio>
#include <cstdlib>
#include "../src/pump_logic.h"

static int failures = 0;
#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
      failures++;                                                     \
    }                                                                 \
  } while (0)

static const uint32_t SEC = 1000, MIN = 60000;

// Small simulator: holds inputs, steps time in 100 ms ticks
struct Sim {
  PumpController c;
  Inputs in;
  Outputs out;
  uint32_t t;
  Settings s;
  Sim(const Settings& st, uint32_t start = 0) : c(st), t(start), s(st) { kpa(300); }
  void kpa(float k) { in.mA = 4.0f + k / s.rangeKpa * 16.0f; }
  void step(uint32_t ms) {
    uint32_t end = t + ms;
    do {
      out = c.update(in, t);
      t += 100;
    } while ((int32_t)(end - t) > 0);
    out = c.update(in, t);
  }
  void boot() { step(s.powerUpDelayMs + SEC); }
  void reset() { in.reset = true; step(100); in.reset = false; }
  void feedback() { in.running[0] = out.pump[0]; in.running[1] = out.pump[1]; }
};

// ---------------- Mode 1 ----------------
static void test_power_up_delay() {
  puts("power-up delay");
  Sim m(defaultsFor(Mode::PressurePump));
  m.kpa(100);
  m.step(5 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::PowerUp);
  m.step(6 * SEC);
  CHECK(m.out.pump[0]);
}

static void test_start_stop_min_run_restart() {
  puts("start / stop / min run / restart delay");
  Sim m(defaultsFor(Mode::PressurePump));
  m.boot();
  CHECK(!m.out.pump[0]);
  m.kpa(190);
  m.step(200);
  CHECK(m.out.pump[0]);
  m.kpa(360);
  m.step(2 * SEC);
  CHECK(m.out.pump[0]);  // min run 10 s
  m.step(9 * SEC);
  CHECK(!m.out.pump[0]);
  m.kpa(150);
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);  // restart delay 5 s
  m.step(4 * SEC);
  CHECK(m.out.pump[0]);
  CHECK(m.c.starts(0) == 2);
}

static void test_sensor_fault() {
  puts("sensor fault (frost-blown transducer)");
  Sim m(defaultsFor(Mode::PressurePump));
  m.boot();
  m.kpa(150);
  m.step(SEC);
  CHECK(m.out.pump[0]);
  m.in.mA = 0.0f;
  m.step(1 * SEC);
  CHECK(m.out.pump[0]);  // short glitch tolerated
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.out.alarm);
  CHECK(m.c.fault() == Fault::Sensor);
  m.kpa(300);
  m.step(SEC);
  CHECK(m.c.fault() == Fault::None);
  CHECK(!m.out.alarm);
}

static void test_startup_timer_and_stepped_retry() {
  puts("start-up timer trip + stepped retry");
  Sim m(defaultsFor(Mode::PressurePump));  // trip 50, start-up 5 s
  m.boot();
  m.kpa(20);
  m.step(4 * SEC);
  CHECK(m.out.pump[0]);  // inside start-up timer
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::DryRunWait);
  CHECK(m.c.fault() == Fault::DryRun);
  CHECK(m.out.alarm);
  m.step(9 * MIN);
  CHECK(!m.out.pump[0]);
  m.step(MIN + SEC);  // 10 min retry
  CHECK(m.out.pump[0]);
  m.step(6 * SEC);  // still dry
  CHECK(!m.out.pump[0]);
  CHECK(m.c.retries() == 2);
  m.step(29 * MIN);
  CHECK(!m.out.pump[0]);  // second wait is 30 min
  m.step(MIN + SEC);
  CHECK(m.out.pump[0]);
  m.kpa(250);  // water is back
  m.step(SEC);
  m.kpa(360);
  m.step(10 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.retries() == 0);
  CHECK(!m.out.alarm);
}

static void test_run_on_timer() {
  puts("run-on (dip) timer");
  Sim m(defaultsFor(Mode::PressurePump));  // run-on 5 s
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.kpa(250);  // above start: start-up done
  m.step(SEC);
  m.kpa(30);
  m.step(3 * SEC);
  CHECK(m.out.pump[0]);  // short dip tolerated
  m.kpa(250);
  m.step(SEC);
  m.kpa(30);
  m.step(6 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::DryRun);
}

static void test_restart_off_single_repeat() {
  puts("auto restart: off / single / repeat");
  {
    Settings s = defaultsFor(Mode::PressurePump);
    s.restart = Restart::Off;
    Sim m(s);
    m.boot();
    m.kpa(20);
    m.step(6 * SEC);
    CHECK(m.c.state() == State::Lockout);
    CHECK(m.c.fault() == Fault::DryRunLockout);
    m.step(5 * 60 * MIN);
    CHECK(!m.out.pump[0]);
    m.reset();
    m.step(6 * SEC);  // restart delay after the trip has long passed
    CHECK(m.c.starts(0) == 2);
  }
  {
    Settings s = defaultsFor(Mode::PressurePump);
    s.restart = Restart::Single;
    s.restartAfterMs = 2 * MIN;
    Sim m(s);
    m.boot();
    m.kpa(20);
    m.step(6 * SEC);
    CHECK(m.c.state() == State::DryRunWait);
    m.step(2 * MIN + SEC);
    CHECK(m.out.pump[0]);  // the single retry
    m.step(6 * SEC);
    CHECK(m.c.state() == State::Lockout);
  }
  {
    Settings s = defaultsFor(Mode::PressurePump);
    s.restart = Restart::Repeat;
    s.restartAfterMs = 2 * MIN;
    Sim m(s);
    m.boot();
    m.kpa(20);
    for (int i = 0; i < 5; i++) {
      m.step(6 * SEC);
      CHECK(m.c.state() == State::DryRunWait);
      m.step(2 * MIN);
    }
    CHECK(m.c.retries() >= 5);  // never locks out
  }
}

static void test_delayed_restart() {
  puts("delayed restart after stop");
  Settings s = defaultsFor(Mode::PressurePump);
  s.restartDelayMs = 30 * MIN;
  Sim m(s);
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.kpa(360);
  m.step(11 * SEC);
  CHECK(!m.out.pump[0]);
  m.kpa(150);
  m.step(29 * MIN);
  CHECK(!m.out.pump[0]);
  m.step(MIN + SEC);
  CHECK(m.out.pump[0]);
}

static void test_batch() {
  puts("batch (timed run)");
  Settings s = defaultsFor(Mode::PressurePump);
  s.restart = Restart::Off;
  s.batchMs = 5 * MIN;
  s.alarmMask = Alarm::BatchDone;
  Sim m(s);
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.kpa(300);
  m.step(4 * MIN);
  CHECK(m.out.pump[0]);
  m.step(MIN + SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::BatchDone);
  CHECK(m.out.alarm);
  m.kpa(100);
  m.step(MIN);
  CHECK(!m.out.pump[0]);
  m.reset();
  m.step(SEC);
  CHECK(m.out.pump[0]);
}

static void test_high_pressure_run_on() {
  puts("high-pressure run-on band + high limit");
  Settings s = defaultsFor(Mode::PressurePump);
  s.hpRunOnMs = 60 * SEC;
  Sim m(s);
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.kpa(250);
  m.step(10 * SEC);
  m.kpa(400);  // above stop, below high limit
  m.step(50 * SEC);
  CHECK(m.out.pump[0]);
  m.kpa(300);  // back under stop: band timer resets
  m.step(SEC);
  m.kpa(400);
  m.step(50 * SEC);
  CHECK(m.out.pump[0]);
  m.step(11 * SEC);
  CHECK(!m.out.pump[0]);
  m.kpa(150);
  m.step(6 * SEC);
  CHECK(m.out.pump[0]);
  m.kpa(620);  // over the high limit: instant stop
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::HighPressure);
}

static void test_anti_cycling() {
  puts("anti-cycling: starts per hour + starts in a row");
  for (int mode = 0; mode < 2; mode++) {
    Settings s = defaultsFor(Mode::PressurePump);
    s.maxStartsPerHour = 360;  // one start per 10 s
    s.startsInSequence = 5;
    s.minRunMs = 1000;
    s.restartDelayMs = 1000;
    s.antiCycleLockout = mode == 0;
    Sim m(s);
    m.boot();
    for (int i = 0; i < 4; i++) {
      m.kpa(150);
      m.step(SEC);
      m.kpa(360);
      m.step(2 * SEC);
    }
    CHECK(m.c.state() != State::Lockout);  // 4 quick starts OK
    m.kpa(150);
    m.step(SEC);
    if (mode == 0) {
      CHECK(m.c.state() == State::Lockout);
      CHECK(m.c.fault() == Fault::AntiCycle);
      CHECK(!m.out.pump[0]);
    } else {
      CHECK(m.out.pump[0]);  // alarm only: still supplies water
      CHECK(m.c.fault() == Fault::AntiCycle);
      CHECK(m.out.alarm);
    }
  }
  {
    // Slow starts never trip
    Settings s = defaultsFor(Mode::PressurePump);
    s.maxStartsPerHour = 360;
    Sim m(s);
    m.boot();
    for (int i = 0; i < 10; i++) {
      m.kpa(150);
      m.step(SEC);
      m.kpa(360);
      m.step(15 * SEC);
    }
    CHECK(m.c.state() != State::Lockout);
  }
}

static void test_pre_start() {
  puts("pre-start delay");
  Settings s = defaultsFor(Mode::PressurePump);
  s.preStartMs = 20 * SEC;
  s.auxMask = Alarm::PreStart;
  Sim m(s);
  m.boot();
  m.kpa(150);
  m.step(10 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::PreStart);
  CHECK(m.out.aux);  // e.g. mixer running
  m.step(11 * SEC);
  CHECK(m.out.pump[0]);
  CHECK(!m.out.aux);
}

static void test_max_run() {
  puts("max run time");
  Sim m(defaultsFor(Mode::PressurePump));
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.kpa(250);
  m.step(59 * MIN);
  CHECK(m.out.pump[0]);
  m.step(2 * MIN);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::MaxRun);
}

static void test_tank_remote() {
  puts("low tank, remote stop");
  Sim m(defaultsFor(Mode::PressurePump));
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.in.tankOk = false;
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::LowTank);
  CHECK(m.out.alarm);
  m.in.tankOk = true;
  m.step(6 * SEC);
  CHECK(m.out.pump[0]);
  m.in.enable = false;
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Stopped);
  CHECK(!m.out.alarm);
  m.in.enable = true;
  m.step(6 * SEC);
  CHECK(m.out.pump[0]);
}

static void test_single_pump_overload() {
  puts("single pump overload -> lockout");
  Sim m(defaultsFor(Mode::PressurePump));
  m.boot();
  m.kpa(150);
  m.step(SEC);
  m.in.pumpFault[0] = true;
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Lockout);
  CHECK(m.c.fault() == Fault::Pump1);
}

static void test_manual() {
  puts("manual run");
  Settings s = defaultsFor(Mode::PressurePump);
  s.restart = Restart::Off;
  Sim m(s);
  m.boot();
  m.kpa(20);
  m.step(6 * SEC);
  CHECK(m.c.state() == State::Lockout);
  m.in.manual = true;
  m.step(SEC);
  CHECK(m.out.pump[0]);
  CHECK(m.c.state() == State::Manual);
  m.kpa(620);  // still protected by high limit
  m.step(200);
  CHECK(!m.out.pump[0]);
  m.kpa(100);
  m.step(200);
  m.in.manual = false;
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Lockout);
}

static void test_millis_wrap() {
  puts("millis() wrap-around (49 days)");
  Sim m(defaultsFor(Mode::PressurePump), 0xFFFFFFFFu - 20 * SEC);
  m.boot();
  m.kpa(150);
  m.step(SEC);
  CHECK(m.out.pump[0]);
  m.kpa(360);
  m.step(15 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Idle);
}

// ---------------- Mode 4 ----------------
static void cycleOnce(Sim& m) {
  m.kpa(190);
  m.step(SEC);
  m.kpa(360);
  m.step(11 * SEC);
  m.step(6 * SEC);
}

static void test_twin_alternate() {
  puts("twin: alternate every start, every 4 starts, duty/standby");
  {
    Sim m(defaultsFor(Mode::Twin));
    m.boot();
    m.kpa(190);
    m.step(SEC);
    CHECK(m.out.pump[0] && !m.out.pump[1]);
    m.kpa(360);
    m.step(17 * SEC);
    m.kpa(190);
    m.step(SEC);
    CHECK(!m.out.pump[0] && m.out.pump[1]);
  }
  {
    Settings s = defaultsFor(Mode::Twin);
    s.alternateEvery = 4;  // pump 1 three times, then pump 2 once
    Sim m(s);
    m.boot();
    for (int i = 0; i < 3; i++) cycleOnce(m);
    CHECK(m.c.starts(0) == 3 && m.c.starts(1) == 0);
    CHECK(m.c.lead() == 1);
    cycleOnce(m);
    CHECK(m.c.starts(1) == 1);
    CHECK(m.c.lead() == 0);
    CHECK(m.c.startsToAlternate() == 3);
  }
  {
    Settings s = defaultsFor(Mode::Twin);
    s.alternate = false;  // duty / standby
    Sim m(s);
    m.boot();
    for (int i = 0; i < 4; i++) cycleOnce(m);
    CHECK(m.c.starts(0) == 4 && m.c.starts(1) == 0);
  }
}

static void test_twin_assist() {
  puts("twin: lag pump assists below its own start");
  Sim m(defaultsFor(Mode::Twin));  // lead 200/350, lag 180/350
  m.boot();
  m.kpa(190);
  m.step(SEC);
  CHECK(m.out.pump[0] && !m.out.pump[1]);
  m.kpa(150);
  m.step(3 * SEC);
  CHECK(!m.out.pump[1]);  // waits for lead's start-up time
  m.step(3 * SEC);
  CHECK(m.out.pump[0] && m.out.pump[1]);
  m.kpa(360);
  m.step(11 * SEC);
  CHECK(!m.out.pump[0] && !m.out.pump[1]);
}

static void test_twin_failover() {
  puts("twin: failover on fail-to-start and overload");
  Settings s = defaultsFor(Mode::Twin);
  s.pumpFeedback[0] = s.pumpFeedback[1] = true;
  Sim m(s);
  m.boot();
  m.kpa(190);
  m.step(SEC);
  CHECK(m.out.pump[0]);
  m.step(3 * SEC);  // pump 1 contactor never pulls in
  CHECK(!m.out.pump[0] && m.out.pump[1]);
  CHECK(m.c.pumpLocked(0));
  CHECK(m.c.fault() == Fault::Pump1);
  CHECK(m.out.alarm);
  m.feedback();
  m.step(SEC);
  CHECK(m.out.pump[1]);
  m.in.pumpFault[1] = true;
  m.step(200);
  CHECK(!m.out.pump[0] && !m.out.pump[1]);
  CHECK(m.c.state() == State::Lockout);
  CHECK(m.c.fault() == Fault::NoPumps);
  m.in.pumpFault[1] = false;
  m.reset();
  for (int i = 0; i < 70; i++) { m.feedback(); m.step(100); }
  CHECK(m.out.pump[0] || m.out.pump[1]);
  CHECK(!m.out.alarm);
}

// ---------------- Mode 2 ----------------
static void test_guard() {
  puts("guard: healthy relay, high trip latch, low trip after start-up");
  Settings s = defaultsFor(Mode::Guard);  // low 100, high 600, restart off
  Sim m(s);
  m.boot();
  CHECK(m.out.pump[0]);  // system enabled
  m.kpa(650);
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::HighPressure);
  m.kpa(300);
  m.step(MIN);
  CHECK(!m.out.pump[0]);  // latched until reset
  m.reset();
  m.step(SEC);
  CHECK(m.out.pump[0]);
  m.kpa(50);
  m.step(4 * SEC);
  CHECK(m.out.pump[0]);  // dip within run-on
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::LowPressure);
  CHECK(m.out.alarm);
}

// ---------------- Mode 3 ----------------
static void test_snap() {
  puts("snap switch: falling and rising");
  {
    Settings s = defaultsFor(Mode::Snap);  // set 300, delta 14, on below set
    Sim m(s);
    m.kpa(320);
    m.boot();
    m.kpa(305);
    m.step(200);
    CHECK(!m.out.pump[0]);
    m.kpa(299);
    m.step(200);
    CHECK(m.out.pump[0]);
    m.kpa(310);
    m.step(200);
    CHECK(m.out.pump[0]);  // inside differential
    m.kpa(315);
    m.step(200);
    CHECK(!m.out.pump[0]);
  }
  {
    Settings s = defaultsFor(Mode::Snap);
    s.snapOnRising = true;
    Sim m(s);
    m.kpa(290);
    m.boot();
    m.kpa(290);
    m.step(200);
    CHECK(!m.out.pump[0]);
    m.kpa(301);
    m.step(200);
    CHECK(m.out.pump[0]);
    m.kpa(290);
    m.step(200);
    CHECK(m.out.pump[0]);
    m.kpa(285);
    m.step(200);
    CHECK(!m.out.pump[0]);
  }
}

// ---------------- Settings checks ----------------
static void test_validate() {
  puts("settings checks");
  Settings s = defaultsFor(Mode::PressurePump);
  CHECK(validate(s) == Issue::None);
  s.stopKpa = 150;
  CHECK(validate(s) == Issue::StopNotAboveStart);
  s = defaultsFor(Mode::PressurePump);
  s.tripKpa = 250;
  CHECK(validate(s) == Issue::StartNotAboveTrip);
  s = defaultsFor(Mode::PressurePump);
  s.highLimitKpa = 300;
  CHECK(validate(s) == Issue::HighNotAboveStop);
  s = defaultsFor(Mode::PressurePump);
  s.batchMs = MIN;
  CHECK(validate(s) == Issue::BatchWithRestart);
  s.restart = Restart::Off;
  CHECK(validate(s) == Issue::None);
  s = defaultsFor(Mode::Twin);
  CHECK(validate(s) == Issue::None);
  s.stop2Kpa = 100;
  CHECK(validate(s) == Issue::Stop2NotAboveStart2);
  s = defaultsFor(Mode::Twin);
  s.alternateEvery = 1;
  CHECK(validate(s) == Issue::AlternateRange);
  s = defaultsFor(Mode::Guard);
  s.highLimitKpa = 80;
  CHECK(validate(s) == Issue::GuardHighNotAboveLow);
  s = defaultsFor(Mode::Snap);
  s.snapDeltaKpa = 0;
  CHECK(validate(s) == Issue::SnapDeltaZero);
}

static void test_units() {
  puts("display units kPa / psi");
  CHECK(toDisplay(1000, Units::Kpa) == 1000);
  float psi = toDisplay(1000, Units::Psi);
  CHECK(psi > 145.0f && psi < 145.1f);
  float back = fromDisplay(psi, Units::Psi);
  CHECK(back > 999.9f && back < 1000.1f);
}

static void test_relay_masks() {
  puts("alarm / aux relay options");
  Settings s = defaultsFor(Mode::PressurePump);
  s.alarmMask = Alarm::LowFault | Alarm::SensorFault;
  s.auxMask = Alarm::PumpOn;
  Sim m(s);
  m.boot();
  m.kpa(150);
  m.step(SEC);
  CHECK(m.out.aux);
  CHECK(!m.out.alarm);
  m.in.tankOk = false;  // a fault not ticked on the alarm relay
  m.step(200);
  CHECK(!m.out.alarm);
  CHECK(!m.out.aux);
  m.in.tankOk = true;
  m.in.mA = 0;
  m.step(3 * SEC);
  CHECK(m.out.alarm);
}

int main() {
  test_power_up_delay();
  test_start_stop_min_run_restart();
  test_sensor_fault();
  test_startup_timer_and_stepped_retry();
  test_run_on_timer();
  test_restart_off_single_repeat();
  test_delayed_restart();
  test_batch();
  test_high_pressure_run_on();
  test_anti_cycling();
  test_pre_start();
  test_max_run();
  test_tank_remote();
  test_single_pump_overload();
  test_manual();
  test_millis_wrap();
  test_twin_alternate();
  test_twin_assist();
  test_twin_failover();
  test_guard();
  test_snap();
  test_validate();
  test_relay_masks();
  test_units();
  if (failures) {
    printf("\n%d check(s) FAILED\n", failures);
    return 1;
  }
  puts("\nAll tests passed");
  return 0;
}
