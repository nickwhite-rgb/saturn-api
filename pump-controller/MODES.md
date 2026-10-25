# Saturn Pump Controller: Modes & Functions v0.2

Based on a study of the Kelco E30 Mk2 manual (pp. 26–39). **Goal:** match every E30 mode and function, make setup far easier, and build it so new modes can be added later without touching the old ones.

> We're matching *what it does*, not copying its manual, screens or branding. All our wording, menus and design are our own.

---

## 1. The E30 at a glance

| E30 mode | What it does | Typical job |
|---|---|---|
| **1 Pressure Switch** | Start/stop on pressure, with loss-of-prime (dry run) protection | House pressure pump, bore to pressure tank |
| **2 Pressure Guard** | Only watches high/low limits, shuts down on fault | Protecting a system controlled by something else |
| **3 Snap Action** | A single set point with a tiny on/off band | Process control, simple pressure switching |
| **4 Dual Pump** | Two pumps, each with its own start/stop, alternating or duty/standby | Duty/standby sets, line pressurising, jockey + main |

### Every E30 function, and where we stand
| Function | E30 modes | E30 range | Our current code | Action |
|---|---|---|---|---|
| Start / stop pressure | 1, 4 (per pump) | 0–2000 kPa | ✅ Cut-in/cut-out | Add **per-pump** start/stop for twin mode |
| Trip pressure (loss of prime) | 1, 2, 4 | 0–2000 kPa, 0 = off | ✅ "Dry-run pressure" | Rename to Trip; allow 0 = off |
| **Start-up timer** (ignore low pressure while pump gets going) | 1, 2, 4 | 1–900 s, default 5 s | ⚠️ Merged with run-on | **Split into its own setting** |
| **Run-on / dip timer** (tolerate brief dips) | 1, 2, 4 | 0–900 s | ⚠️ Merged with start-up | **Split into its own setting** |
| Auto restart after dry run | 1 | 1 min–99 h 59, single or repeated | ⚠️ Fixed 10/30/60 min schedule | Make the delay settable + **Off / Single / Repeat / Stepped** (ours) |
| **Delayed restart** after a high-pressure stop | 1 | 1 min–99 h 59 | ❌ | Add |
| **Batch controller** (run X time, then stop) | 1 | 1 min–99 h 59 | ❌ | Add (Saturn "Timed run") |
| **High-pressure run-on** (stop *band* for flat-curve pumps) | 1 | 0–900 s | ❌ | Add, with a **warning screen for helical / PD pumps** |
| High-pressure limit | 1, 2 | 0–2000 kPa | ✅ High cutout | Keep |
| **Anti-cycling**: max starts/hour + starts in a row | 1 | 1–1800 /h, 2–50 in a row (default 5) | ⚠️ "x starts in y min" | Change to the E30 method (better for bore motors) |
| **Pre-start delay** (stagger starts, run a mixer first) | 1, 2, 4 | 0–18 h | ❌ | Add |
| Remote 1 = start/stop (tank float) | 1, 2, 3 | Yes/No | ✅ Float / remote switch | Keep (we have 8 inputs, not 2) |
| Remote 2 = reboot | all | Yes/No | ✅ Reset input | Keep |
| **Alarm relay options** | 1, 2 | Pick **one** of 10 conditions | ⚠️ "Any fault" only | Add, and let you pick **several** (ours is better) |
| Alternate pumps, every N starts | 4 | 2–240 starts | ⚠️ Every start only | Add "every N starts" + countdown on screen |
| Duty / standby (no alternation) | 4 | Yes/No | ⚠️ | Add |
| Snap set point + delta | 3 | 7–2000 kPa, delta 7–140 kPa | ❌ | Add as Mode 3 |
| Pressure guard hi/lo trips | 2 | 0–2000 kPa | ❌ | Add as Mode 2 |
| Units kPa / psi | all | | ❌ bar only | **kPa / bar / psi** |
| Settings checks (stop > start > trip, etc.) | all | Warning screen | ❌ | Add, with a **plain-English reason** |
| "Purge to factory defaults" when changing mode | all | Manual step | ❌ | **Automatic.** Changing mode loads clean defaults |
| 22 bar hard limit (protects internal sensor) | all | Fixed | n/a | Not needed (external sensor). We have **sensor fault** detection instead, which the E30 can't do |

---

## 2. Our modes

Same four modes as the E30, plus room for more. Each mode is a self-contained "application" sitting on shared I/O and shared protections.

| # | Saturn mode | Equivalent | Notes |
|---|---|---|---|
| 1 | **Pressure Pump** | E30 Mode 1 | Single pump, all Mode 1 functions |
| 2 | **Pressure Guard** | E30 Mode 2 | Protection only: relay healthy, opens on hi/lo fault |
| 3 | **Snap Switch** | E30 Mode 3 | Set point + delta, relay follows |
| 4 | **Twin Pump** | E30 Mode 4 | Per-pump start/stop, alternate every N, duty/standby, assist |
| 5+ | *Future*, see section 5 | — | Added without changing modes 1–4 |

**Shared protections (every mode, always on):** sensor fault (<3.6 / >21 mA), pump overload inputs, contactor fail-to-start, power-up delay, event log, run hours and start counters.

---

## 3. A nicer setup: what makes ours better than the E30

The E30 manual's own top tip is *"use the least number of functions possible"*, because its long single menu lets you turn on things that clash. We fix that with design:

### 3.1 Setup wizard by application (not by mode number)
First question: **"What is this pump doing?"**
- 🏠 House pressure pump
- 🕳️ Bore pump to pressure tank
- 🛢️ Transfer pump to tank (float switch)
- 🔁 Two pumps (duty/standby or alternating)
- 🛡️ Protect a system only (pressure guard)
- ⚙️ Simple pressure switch (snap)

Each choice picks the mode, **loads sensible defaults** for that job and **only shows the settings that matter**. Everything else stays switched off and hidden. Advanced users can still open "All settings".

### 3.2 Plain-English screens
- "Pump starts below **280 kPa**", not "START PRESS 0280"
- Live pressure shown on every pressure setting screen, so you can set it while watching the gauge
- A **pressure bar graphic** showing trip, start, stop and high-limit markers, so you can see if anything's backwards

### 3.3 Settings that can't clash
- Stop must be above start, start above trip, and high limit above stop. If not, it says **why** and goes back to that setting.
- Functions that can't work together (e.g. batch and auto restart) are greyed out with a one-line reason.
- Changing mode **automatically** reloads clean defaults (the E30 needs a manual purge).

### 3.4 Smarter alarms
- The alarm relay can be ticked for **several** conditions (the E30 allows one).
- Two spare relays are assignable too (e.g. "pump running" to a light, "pre-start" to a mixer).
- An **event log** with times and pressures ("12 Nov 03:14 Dry run at 0.4 bar, retry 1 of ∞").

### 3.5 Same wizard on the phone
- The Bluetooth app runs the same wizard, with bigger screens and tooltips.
- Settings can be **saved as a template** ("Smith bore") and copied to the next job, so techs set up identical sites in 30 seconds.

---

## 4. Screen layout (128×64 LCD, 5 buttons)

```
HOME                        TWIN PUMP HOME
┌────────────────────┐      ┌────────────────────┐
│ 3.2 bar      ●RUN  │      │ 3.2 bar   P1● P2○  │
│ ▓▓▓▓▓▓▓▓░░░░░░░░   │      │ ▓▓▓▓▓▓▓▓░░░░░░░░   │
│ Start 2.0 Stop 3.5 │      │ Alt in 3 starts    │
│ OK                 │      │ OK                 │
└────────────────────┘      └────────────────────┘
► Pump detail → Faults → Counters → Info → Home
ENTER (on Home) → Menu (PIN for changes)
```
Menu: **Setup wizard · Pressures · Timers · Protection · Alarms & relays · Inputs · Units & display · Event log · Factory reset**

---

## 5. Future modes (the architecture supports these, not built yet)

| Future mode | Uses | Notes |
|---|---|---|
| Tank level fill / empty | 4–20 mA level sensor (AI2) | Start/stop on level %, high/low alarms |
| Bore with low-level cut-out | Pressure + level or probe | Protects bore from drawdown |
| Constant pressure (VSD) | AO 0–10 V → VSD | PID on pressure, sleep on no-flow |
| Frost protection | NTC temp input | Run heater / drain valve / circulate below set temp |
| Irrigation / timer | RTC + relays | Schedules, run-for-X, rain input |
| Transfer pump with float | Float inputs | Simpler version of mode 1 |
| Generic thermostat / 4–20 mA switch | Any analogue input | "If input > X then relay" |

---

## 6. Build order
1. **Restructure the logic** into shared protections + mode modules (no change to behaviour). All existing tests keep passing.
2. **Mode 1 upgrade:** split start-up / run-on timers, anti-cycling (starts/hour + in a row), settable auto restart, delayed restart, high-pressure run-on, batch, pre-start delay, alarm relay options.
3. **Mode 4 upgrade:** per-pump start/stop, alternate every N, duty/standby.
4. **Modes 2 and 3.**
5. **Units, settings validation, factory defaults per mode.**
6. **Screen + 5-button menu + setup wizard** (ready for the LCD/keypad arriving mid-November).
7. Bluetooth app (same wizard).

Each step comes with PC tests, like the current 13 scenarios.
