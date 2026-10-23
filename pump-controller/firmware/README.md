# Saturn Pump Controller: Bench Firmware (Mode 1)

This firmware is for the **KinCony KC868-A6** test board. It does pressure pump control (E30-style) with optional twin pump. All the settings live in `src/config.h`.

## 1. Flashing it (one-off setup, ~20 min)
1. Install **VS Code** (free) and then the **PlatformIO** extension from inside VS Code.
2. In VS Code: *File → Open Folder* → pick this `firmware` folder.
3. Plug the KC868-A6 into your PC with USB.
4. Click the **→ (Upload)** arrow on the blue bar at the bottom.
5. Click the **🔌 (Serial Monitor)** icon. You'll see a status line every second:
   ```
    2.35 bar (10.24 mA, 2.56 V) | RUNNING | P1 ON  P2 off | lead P1 | OK
   ```

**Serial commands:** type `r` + Enter to **reset** a lockout, and `m` + Enter to toggle **manual run**.

## 2. Bench wiring
| Terminal | Connect |
|---|---|
| 12 V / GND | 12 VDC supply |
| **A1** | Transducer loop: 24 V+ → transducer → A1, with a **250 Ω resistor from A1 to GND** (24 V supply 0 V joined to the board GND) |
| DI1 | Pump 1 contactor aux (N/O) |
| DI2 | Pump 1 overload aux, or a toggle switch to fake one |
| DI3 / DI4 | Pump 2 running / fault (leave open if using 1 pump) |
| DI5 | Remote run switch (**closed = run**). Link it out if not used. |
| DI6 | Float switch (**closed = tank OK**). Link it out if not used. |
| Relay 1 | Pump 1 contactor coil (240 V switched through the relay) |
| Relay 2 | Pump 2 contactor coil |
| Relay 3 | Fault alarm |
| Relay 4 | System healthy |

> ⚠️ DI5 and DI6 must be **linked out** if nothing's connected, otherwise it shows STOPPED / LOW TANK. Or set them to `InFn::None` in `config.h`.

> ⚠️ First power-up: check the serial monitor shows `I2C scan: 0x22 0x24`. If it doesn't, the board revision uses different chips. Tell Claude what it shows.

## 3. Calibrate the pressure reading
1. With the transducer at 0 bar (vented), check the **mA** reading is about **4.0**.
2. Put a gauge on the same tee and compare at 3 bar.
3. If it's off, measure the volts across the 250 Ω resistor and adjust `A1_SCALE` in `config.h` until the **V** reading matches your meter.

## 4. Bench test checklist
| # | Test | How | Expected |
|---|---|---|---|
| 1 | Power-up delay | Power on with low pressure | Waits 10 s, then pump starts |
| 2 | Cut-in / cut-out | Open a tap, then close it | Starts below 2.0 bar, stops at 3.5 bar (after 10 s min run) |
| 3 | **Sensor fault** | **Unplug the M12 transducer** | Pump off within 2 s, `SENSOR - replace transducer`, alarm relay on. Plug back in and it clears. |
| 4 | Dry run | Close the suction valve | Trips after 30 s, `DRY RUN - will retry`, retries in 10 min |
| 5 | Rapid cycling | Crack a tap so it short-cycles | Locks out on the 7th start in 10 min. `r` resets it. |
| 6 | Max run | Leave a tap fully open (or set `maxRunMs` to 2 min to test quickly) | Locks out with `MAX RUN TIME` |
| 7 | High pressure | Set `highCutoutBar` just above cut-out | Pump stops immediately |
| 8 | Float | Open DI6 | Pump off, `LOW TANK`, resumes when closed |
| 9 | Remote stop | Open DI5 | Pump off, `STOPPED`, no alarm |
| 10 | Overload | Close DI2 | Pump off, `PUMP 1 FAULT`. With 2 pumps, pump 2 takes over. |
| 11 | Contactor fail | Pull the contactor coil wire | No feedback in 3 s, so pump locked out (pump 2 takes over) |
| 12 | Twin alternate | Set `pumps = 2` | Lead swaps each start |
| 13 | Twin assist | Big flow, pressure drops below 1.5 bar | Second pump joins after 10 s |
| 14 | Manual run | Type `m` | Pump runs even in lockout, still stops on high pressure |
| 15 | Power cut | Kill power mid-run | Restarts after 10 s, start count kept |

## 5. Logic tests (for developers)
`./run_tests.sh` runs all the pump logic scenarios on a PC (needs `g++`). No board needed.

## Known limits (bench version)
- No Bluetooth, Wi-Fi or app yet. Settings come from `config.h`.
- Start and run-hour counters are saved every 15 minutes, so a power cut can lose up to 15 min.
- The KC868-A6 isn't the production board. It's fine for the bench, but not for a pump shed.
