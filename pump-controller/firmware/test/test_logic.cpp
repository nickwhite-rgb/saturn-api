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
  Sim(const Settings& st, uint32_t start = 0) : c(st), t(start), s(st) { bar(3.0f); }
  void bar(float b) { in.mA = 4.0f + b / s.rangeBar * 16.0f; }
  void step(uint32_t ms) {
    uint32_t end = t + ms;
    do {
      out = c.update(in, t);
      t += 100;
    } while ((int32_t)(end - t) > 0);
    out = c.update(in, t);
  }
  void boot() { step(s.powerUpDelayMs + SEC); }
  // Pump is "pumping": feedback follows relays
  void feedback() { in.running[0] = out.pump[0]; in.running[1] = out.pump[1]; }
};

static void test_power_up_delay() {
  puts("power-up delay");
  Settings s;
  Sim m(s);
  m.bar(1.0f);
  m.step(5 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::PowerUp);
  m.step(6 * SEC);
  CHECK(m.out.pump[0]);
}

static void test_start_stop_min_run_restart() {
  puts("start / stop / min run / restart delay");
  Settings s;
  Sim m(s);
  m.boot();
  CHECK(!m.out.pump[0]);  // 3.0 bar, above cut-in
  m.bar(1.9f);
  m.step(200);
  CHECK(m.out.pump[0]);
  m.bar(3.6f);  // over cut-out but min run not done
  m.step(2 * SEC);
  CHECK(m.out.pump[0]);
  m.step(9 * SEC);
  CHECK(!m.out.pump[0]);
  m.bar(1.5f);  // restart delay 5 s
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);
  m.step(4 * SEC);
  CHECK(m.out.pump[0]);
  CHECK(m.c.starts(0) == 2);
}

static void test_sensor_fault() {
  puts("sensor fault (frost-blown transducer)");
  Settings s;
  Sim m(s);
  m.boot();
  m.bar(1.0f);
  m.step(SEC);
  CHECK(m.out.pump[0]);
  m.in.mA = 0.0f;  // broken / unplugged
  m.step(1 * SEC);
  CHECK(m.out.pump[0]);  // short glitch tolerated
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.out.alarm);
  CHECK(m.c.fault() == Fault::Sensor);
  m.in.mA = 25.0f;  // shorted
  m.step(5 * SEC);
  CHECK(m.c.fault() == Fault::Sensor);
  m.bar(3.0f);  // new sensor plugged in
  m.step(SEC);
  CHECK(m.c.fault() == Fault::None);
  CHECK(!m.out.alarm);
}

static void test_dry_run_retry() {
  puts("dry run + retry schedule");
  Settings s;
  Sim m(s);
  m.boot();
  m.bar(0.2f);
  m.step(29 * SEC);
  CHECK(m.out.pump[0]);
  m.step(2 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::DryRunWait);
  CHECK(m.out.alarm);
  m.step(9 * MIN);
  CHECK(!m.out.pump[0]);
  m.step(MIN + SEC);  // 10 min retry
  CHECK(m.out.pump[0]);
  m.step(31 * SEC);  // still dry
  CHECK(!m.out.pump[0]);
  CHECK(m.c.retries() == 2);
  m.step(20 * MIN);
  CHECK(!m.out.pump[0]);  // second wait is 30 min
  m.step(10 * MIN + 5 * SEC);
  CHECK(m.out.pump[0]);
  m.bar(1.5f);  // water is back
  m.step(5 * SEC);
  m.bar(3.6f);
  m.step(10 * SEC);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.retries() == 0);
  CHECK(!m.out.alarm);
}

static void test_dry_run_lockout_and_reset() {
  puts("dry run lockout after max retries, reset");
  Settings s;
  s.maxRetries = 1;
  Sim m(s);
  m.boot();
  m.bar(0.2f);
  m.step(31 * SEC);
  m.step(10 * MIN + SEC);
  m.step(31 * SEC);
  CHECK(m.c.state() == State::Lockout);
  CHECK(m.c.fault() == Fault::DryRunLockout);
  m.step(2 * 60 * MIN);
  CHECK(!m.out.pump[0]);
  m.in.reset = true;
  m.step(100);
  m.in.reset = false;
  m.step(200);
  CHECK(m.out.pump[0]);
}

static void test_rapid_cycle() {
  puts("rapid cycling lockout (default) and alarm-only");
  for (int mode = 0; mode < 2; mode++) {
    Settings s;
    s.rapidCycleLockout = mode == 0;
    Sim m(s);
    m.boot();
    for (int i = 0; i < 7; i++) {
      m.bar(1.5f);
      m.step(SEC);
      m.bar(3.6f);
      m.step(11 * SEC);
      m.step(5 * SEC);
    }
    if (mode == 0) {
      CHECK(m.c.state() == State::Lockout);
      CHECK(m.c.fault() == Fault::RapidCycle);
      m.bar(1.0f);
      m.step(SEC);
      CHECK(!m.out.pump[0]);
    } else {
      CHECK(m.c.state() != State::Lockout);
      CHECK(m.c.fault() == Fault::RapidCycle);
      CHECK(m.out.alarm);
      m.bar(1.0f);
      m.step(SEC);
      CHECK(m.out.pump[0]);  // still supplies water
      m.bar(3.6f);
      m.step(15 * MIN);
      CHECK(!m.out.alarm);  // clears once cycling stops
    }
  }
}

static void test_max_run() {
  puts("max run time");
  Settings s;
  Sim m(s);
  m.boot();
  m.bar(1.5f);
  m.step(59 * MIN);
  CHECK(m.out.pump[0]);
  m.step(2 * MIN);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::MaxRun);
}

static void test_high_pressure_tank_remote() {
  puts("high pressure, low tank, remote stop");
  Settings s;
  Sim m(s);
  m.boot();
  m.bar(1.5f);
  m.step(SEC);
  m.bar(6.5f);
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.fault() == Fault::HighPressure);
  m.bar(1.5f);
  m.step(6 * SEC);
  CHECK(m.out.pump[0]);

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

static void test_twin_alternate_assist() {
  puts("twin pump: alternate + assist");
  Settings s;
  s.pumps = 2;
  Sim m(s);
  m.boot();
  m.bar(1.5f);
  m.step(SEC);
  CHECK(m.out.pump[0] && !m.out.pump[1]);
  m.bar(3.6f);
  m.step(10 * SEC);
  m.step(6 * SEC);
  m.bar(1.5f);
  m.step(SEC);
  CHECK(!m.out.pump[0] && m.out.pump[1]);  // swapped lead
  m.bar(1.2f);  // below cut-in minus assist margin
  m.step(11 * SEC);
  CHECK(m.out.pump[0] && m.out.pump[1]);
  m.bar(3.6f);
  m.step(SEC);
  CHECK(!m.out.pump[0] && !m.out.pump[1]);
}

static void test_twin_failover() {
  puts("twin pump: failover on fail-to-start and overload");
  Settings s;
  s.pumps = 2;
  s.pumpFeedback[0] = s.pumpFeedback[1] = true;
  Sim m(s);
  m.boot();
  m.bar(1.5f);
  m.step(SEC);
  CHECK(m.out.pump[0]);
  // Pump 1 contactor never pulls in (no feedback)
  m.step(3 * SEC);
  CHECK(!m.out.pump[0] && m.out.pump[1]);
  CHECK(m.c.pumpLocked(0));
  CHECK(m.c.fault() == Fault::Pump1);
  CHECK(m.out.alarm);
  m.feedback();
  m.step(SEC);
  CHECK(m.out.pump[1]);  // pump 2 keeps going
  // Pump 2 overload trips too
  m.in.pumpFault[1] = true;
  m.step(200);
  CHECK(!m.out.pump[0] && !m.out.pump[1]);
  CHECK(m.c.state() == State::Lockout);
  CHECK(m.c.fault() == Fault::NoPumps);
  // Fix it and reset
  m.in.pumpFault[1] = false;
  m.in.reset = true;
  m.step(100);
  m.in.reset = false;
  for (int i = 0; i < 70; i++) { m.feedback(); m.step(100); }  // 5 s restart delay
  CHECK(m.out.pump[0] || m.out.pump[1]);
  CHECK(!m.out.alarm);
}

static void test_single_pump_overload() {
  puts("single pump overload -> lockout");
  Settings s;
  Sim m(s);
  m.boot();
  m.bar(1.5f);
  m.step(SEC);
  m.in.pumpFault[0] = true;
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Lockout);
  CHECK(m.c.fault() == Fault::Pump1);
}

static void test_manual() {
  puts("manual run");
  Settings s;
  s.maxRetries = 1;
  Sim m(s);
  m.boot();
  m.bar(0.2f);
  m.step(31 * SEC);
  m.step(10 * MIN + SEC);
  m.step(31 * SEC);
  CHECK(m.c.state() == State::Lockout);
  m.in.manual = true;  // priming the pump
  m.step(SEC);
  CHECK(m.out.pump[0]);
  CHECK(m.c.state() == State::Manual);
  m.bar(6.5f);  // still protected by high pressure
  m.step(200);
  CHECK(!m.out.pump[0]);
  m.bar(1.0f);
  m.step(200);
  m.in.manual = false;
  m.step(200);
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Lockout);
}

static void test_millis_wrap() {
  puts("millis() wrap-around (49 days)");
  Settings s;
  Sim m(s, 0xFFFFFFFFu - 20 * SEC);
  m.boot();
  m.bar(1.5f);
  m.step(SEC);
  CHECK(m.out.pump[0]);
  m.bar(3.6f);
  m.step(15 * SEC);  // crosses the wrap
  CHECK(!m.out.pump[0]);
  CHECK(m.c.state() == State::Idle);
}

int main() {
  test_power_up_delay();
  test_start_stop_min_run_restart();
  test_sensor_fault();
  test_dry_run_retry();
  test_dry_run_lockout_and_reset();
  test_rapid_cycle();
  test_max_run();
  test_high_pressure_tank_remote();
  test_twin_alternate_assist();
  test_twin_failover();
  test_single_pump_overload();
  test_manual();
  test_millis_wrap();
  if (failures) {
    printf("\n%d check(s) FAILED\n", failures);
    return 1;
  }
  puts("\nAll tests passed");
  return 0;
}
