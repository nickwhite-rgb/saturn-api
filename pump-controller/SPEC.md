# Saturn Pump Controller — Spec v0.1 (Draft)

**Goal:** A cheaper E30-style pressure pump controller with the pressure transducer **outside** the box, so frost only kills a cheap, plug-in sensor that a farmer can swap themselves.

**Stage:** Prove it first. Trial it on Saturn Electrical installs, then decide whether to sell it.

---

## 1. Hardware (inputs and outputs)

| Channel | Use (Mode 1) | Spec |
|---|---|---|
| **AI1** | Pressure transducer | 4–20 mA, 2-wire, powered by the controller (24 VDC loop). Range set in the app (default 0–10 bar). |
| AI2 | Spare (tank level, later) | 4–20 mA / 0–10 V |
| **DI1** | Low tank / float switch (optional) | Volt-free contact |
| **DI2** | Remote enable / flow switch (optional) | Volt-free contact |
| **DO1** | **Pump contactor coil** | Relay, **240 VAC 5 A**, rated for inductive loads (10 A relay recommended), with an RC snubber across the coil |
| DO2 | Fault / alarm output | Relay, volt-free |
| AO1–AO2 | Spare (VSD speed, later) | 0–10 V |

- **Supply:** 240 VAC in, internally fused.
- **Pump size is not the controller's problem.** It only drives the contactor coil, so the pump can be any size.

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

### Protections (everything the E30 does, plus a few extras)
| Protection | Trigger | Action |
|---|---|---|
| **Dry run** | Pump running and pressure doesn't reach Cut-in + margin within *Dry-run time*, or drops below *Dry-run pressure* while running | Pump off → auto retry |
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
| Transducer range | 0–10 bar |
| Cut-in | 2.0 bar |
| Cut-out | 3.5 bar |
| Minimum run time | 10 s |
| Restart delay | 5 s |
| Power-up delay | 10 s |
| Dry-run pressure | 0.5 bar |
| Dry-run time | 30 s |
| Max retries | 0 (retry forever) |
| Max starts / Cycle window | 6 starts / 10 min |
| Rapid-cycle action | Alarm only |
| Max run time | 60 min |
| High pressure cutout | 6.0 bar |

### On-unit controls
- Status LEDs: **Power / Pump running / Fault / Bluetooth**
- **Reset** button (clears a lockout)
- **Manual run** (hold the button, with high-pressure cutout still active)

## 3. Setup and monitoring
- **Bluetooth setup:** a QR sticker on each unit. Scan it in the app to connect, then pick the mode and adjust settings. No Wi-Fi needed on site.
- **Wi-Fi (Stage 3):** live pressure, pump state, faults, start count and run hours in the app, with push alerts for faults.
- **Logged on the unit:** last 50 faults, total starts, total run hours.

## 4. Build stages
1. **Bench prototype.** Buy an off-the-shelf industrial ESP32 board with a 4–20 mA input and relays, and write Mode 1. Test it on a bench rig (pump, tank, transducer, contactor). The Waveshare 7" screen can be the test display.
2. **Field trial.** 3–5 units on friendly customers' sites.
3. **App and cloud.** Bluetooth setup first, then remote monitoring.
4. **Custom board and enclosure.** Cost-down for volume, and start the compliance (RCM) path if selling.

## 5. Not in Stage 1
- Other modes (tank fill, bore, timer, frost heater, VSD constant pressure). The board has the spare inputs and outputs for them.
- Phone app (settings live in the code for the bench test)
- Wi-Fi and cloud

## 6. Open questions
- [ ] Phones: iPhone, Android, or both?
- [ ] Transducer range: is 0–10 bar right for most jobs, or do bores need 0–16 bar?
- [ ] Plug type: M12 or DIN 43650?
- [ ] Do you want an overload / contactor-feedback input (to confirm the pump actually started)?
- [ ] Rapid cycling: alarm only or lock out by default?
