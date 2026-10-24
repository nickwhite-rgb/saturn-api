# Saturn Pump Controller — Spec v0.1 (Draft)

**Goal:** A cheaper E30-style pressure pump controller with the pressure transducer **outside** the box, so frost only kills a cheap, plug-in sensor that a farmer can swap themselves.

**Stage:** Prove it first. Trial it on Saturn Electrical installs, then decide whether to sell it.

---

## 1. Hardware (inputs and outputs)

| Channel | Use (Mode 1) | Spec |
|---|---|---|
| **AI1** | Pressure transducer | 4–20 mA, 2-wire, powered by the controller (24 VDC loop). Range set in the app. |
| AI2 | Spare (tank level, later) | 4–20 mA / 0–10 V |
| AO1–AO2 | Spare (VSD speed, later) | 0–10 V |

### Digital inputs: 6 × volt-free contacts, each one assignable in the app
| Input | Default use |
|---|---|
| DI1 | Pump 1 running (contactor auxiliary contact) |
| DI2 | Pump 1 fault (overload / MPCB auxiliary contact) |
| DI3 | Pump 2 running |
| DI4 | Pump 2 fault |
| DI5 | Remote start / stop (switch or button) |
| DI6 | Low tank / float switch |

Any input can be reassigned to: running feedback, fault, remote start, remote stop, remote enable, float, flow switch, or not used. Each one can be set as normally open (N/O) or normally closed (N/C).

### Digital outputs: 4 × relays
| Output | Use | Rating |
|---|---|---|
| **DO1** | Pump 1 contactor coil | **240 VAC 5 A**, inductive-rated (10 A relay fitted), with an RC snubber |
| **DO2** | Pump 2 contactor coil | **240 VAC 5 A**, as above |
| DO3 | Fault alarm (strobe, buzzer, BMS) | Volt-free, 1–2 A signal relay |
| DO4 | Assignable (second alarm, "system healthy", low tank) | Volt-free, 1–2 A signal relay |

- **Supply:** 240 VAC in, internally fused.
- **Pump size is not the controller's problem.** It only drives the contactor coils, so the pumps can be any size.

### Farmer-replaceable transducer
- **Thread:** 1/4" BSP male, stainless body. Fit it to a tee with an isolation valve so it can be swapped without draining the system.
- **Plug:** a weatherproof screw-on plug (M12 or DIN 43650 style). Pick one and standardise on it.
- **Lead:** pre-wired with a moulded plug so there's no terminating on site.
- **Replacing it:** close the valve → unplug → unscrew → screw in the new one → plug in. Done.
- Keep spare sensors in stock as a sale item (repeat revenue).

## 2. Mode 1: Pressure pump (E30-style)

### Normal operation
- Pump **starts** when pressure falls below **Cut-in**.
- Pump **stops** when pressure reaches **Cut-out** and the **Minimum run time** has passed.
- **Restart delay** stops instant re-starts after a stop.
- **Power-up delay** after a power cut, so pumps don't all start at once when power comes back on.

### Pumps: 1 or 2 (set in the app)
With 2 pumps set:
- **Alternate duty.** The lead pump swaps each start, so the pumps wear evenly. Option: swap by run hours instead.
- **Assist.** If pressure keeps falling below Cut-in minus *Assist margin* for *Assist delay*, the second pump starts too.
- **Failover.** If the lead pump trips (fault input) or fails to start (no running feedback within *Start confirm time*), it is locked out and the other pump takes over. An alarm is raised.
- **Run hours and starts** are logged separately for each pump.

### Run feedback and fault inputs
- **Fail to start:** the relay is on but there's no running feedback within *Start confirm time* (default 3 s). Pump fault: "contactor / overload".
- **Pump fault input:** that pump is off and locked out, and the alarm output turns on.
- **Remote start / stop:** a stop always wins. Settable as a maintained switch or as momentary start/stop buttons.

### Protections (everything the E30 does, plus a few extras)
| Protection | Trigger | Action |
|---|---|---|
| **Dry run** | Pump running and pressure stays below *Dry-run pressure* for *Dry-run time* (catches both "never built pressure" and "lost water mid-run") | Pump off → auto retry |
| **Auto retry** | After a dry-run trip | Retry after 10 min → 30 min → 60 min → every 60 min. Lock out after *Max retries* (0 = retry forever). |
| **Rapid cycling** (leak / waterlogged tank) | More than *Max starts* within *Cycle window* | Alarm only, or lock out (setting) |
| **Max run time** (burst pipe) | Pump runs continuously longer than *Max run time* | Pump off, lock out |
| **High pressure** | Pressure above *High pressure cutout* | Pump off immediately |
| **Sensor fault** (frost-blown / broken / unplugged sensor) | Current < 3.6 mA or > 21 mA | Pump off, fault **"SENSOR – replace transducer"** |
| **Low tank** (if DI1 used) | Float open | Pump off, auto-resume when the float closes |

**Fail-safe rule:** any sensor or controller fault means **pump OFF**.

### Default settings (all adjustable in the app)
| Setting | Default |
|---|---|
| Transducer range | Settable in the app; stock sensors 0–10 bar and 0–16 bar |
| Pumps | 1 |
| Start confirm time | 3 s |
| Assist margin / delay | 0.5 bar / 10 s |
| Cut-in | 2.0 bar |
| Cut-out | 3.5 bar |
| Minimum run time | 10 s |
| Restart delay | 5 s |
| Power-up delay | 10 s |
| Dry-run pressure | 0.5 bar |
| Dry-run time | 30 s |
| Max retries | 0 (retry forever) |
| Max starts / Cycle window | 6 starts / 10 min |
| Rapid-cycle action | Lock out (alarm only is selectable) |
| Max run time | 60 min |
| High pressure cutout | 6.0 bar |

### On-unit controls
- Status LEDs: **Power / Pump running / Fault / Bluetooth**
- **Reset** button (clears a lockout)
- **Manual run** (hold the button, with high-pressure cutout still active)

## 3. Enclosure and environment (production unit)
**Priority: functional, long-lasting and hard-wearing, not pretty.** Design for the worst case: full Queensland sun, heat, frost, dust and rain.

| Item | Spec |
|---|---|
| Enclosure | Off-the-shelf **UV-stabilised polycarbonate**, **IP66**. Custom moulding only once volumes justify it. |
| Colour | **Light grey RAL 7035** (reflects heat, standard industrial look) |
| Sun hood | Over the top and the screen |
| Cable entry | **Bottom only.** 3 × M20 glands (power, pump 1, pump 2), 1–2 × M16 (signals) |
| Transducer | **M12 socket on the bottom of the box**, so the sensor is swapped without opening the enclosure |
| Breather | IP-rated membrane vent to stop condensation pumping |
| Electronics | Industrial temp-rated parts (−20 to +70 °C minimum), **conformal-coated** board |
| Screen | **Monochrome transflective LCD**, wide-temp (−20 to +70 °C operating is the common standard; the sun hood keeps it inside that). Readable in direct sun, no fragile backlight dependency. Behind a hard-coated UV-resistant window. Shows pressure, pump state and fault code. |
| Buttons | Sealed membrane or IP67 pushbuttons: Reset, Manual run, Setup (Bluetooth) |

## 4. Setup and monitoring
- **Bluetooth setup:** a QR sticker on each unit. Scan it in the app to connect, then pick the mode and adjust settings. No Wi-Fi needed on site.
- **Wi-Fi (Stage 3):** live pressure, pump state, faults, start count and run hours in the app, with push alerts for faults.
- **Logged on the unit:** last 50 faults, total starts, total run hours.

## 5. Build stages
1. **Bench prototype.** Buy an off-the-shelf industrial ESP32 board with a 4–20 mA input and relays, and write Mode 1. Test it on a bench rig (pump, tank, transducer, contactor). The Waveshare 7" screen can be the test display.
2. **Field trial.** 3–5 units on friendly customers' sites.
3. **App and cloud.** Bluetooth setup first, then remote monitoring.
4. **Custom board and enclosure.** Cost-down for volume, and start the compliance (RCM) path if selling.

## 6. Not in Stage 1
- Other modes (tank fill, bore, timer, frost heater, VSD constant pressure). The board has the spare inputs and outputs for them.
- Phone app (settings live in the code for the bench test)
- Wi-Fi and cloud

## 7. Decisions
- [x] Phones: **iPhone and Android**
- [x] Transducer range: **settable in the app**, two stock sensors (0–10 and 0–16 bar)
- [x] Sensor plug: **M12 screw-on**
- [x] Run/fault feedback inputs: **yes**, 6 digital inputs
- [x] Outputs: **4 relays**, 2 for pump contactors (5 A) and 2 for alarms
- [x] Twin pump alternate / assist / failover: **yes**
- [x] Rapid cycling default: **lock out**
- [x] Screen: **monochrome transflective LCD** (hard-wearing over pretty)
- [x] Enclosure: off-the-shelf UV-stable polycarbonate IP66, light grey, bottom-only glands (assumed, change if needed)
