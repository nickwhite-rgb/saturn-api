# Saturn Pump Controller: Bench Firmware (Mode 1)

This firmware is for the **Eletechsup ES32D26** test board (DC 24 V version). It does pressure pump control (E30-style) with optional twin pump. All the settings live in `src/config.h`.

## 1. Flashing it (one-off setup, ~20 min)
1. Install **VS Code** (free) and then the **PlatformIO** extension from inside VS Code.
2. In VS Code: *File → Open Folder* → pick this `firmware` folder.
3. Plug the ES32D26 into your PC with USB.
4. Click the **→ (Upload)** arrow on the blue bar at the bottom.
5. Click the **🔌 (Serial Monitor)** icon. You'll see a status line every second:
   ```
    2.35 bar (10.24 mA, pin 0.85 V) | RUNNING | P1 ON  P2 off | lead P1 | OK
   ```

**Serial commands:** `r` + Enter = **reset** a lockout, `m` + Enter = toggle **manual run**, `t` + Enter = **self-test**.

## 2. First power-up: run the self-test ⚠️
The board's pin numbers aren't confirmed yet (no official pin map found). **Before connecting 240 V to the contactor coils:**
1. Power the board from 24 V only. Keep the contactor coil supply **OFF**.
2. Upload, open the serial monitor and type `t`.
3. **Step 1:** relays click on one at a time, 1 to 8. Check each click matches the number printed.
   - If relays come on at power-up or the order is wrong, **stop and send Claude a screenshot.**
4. **Step 2:** live readings.
   - Short each input IN1–IN8 to GND in turn. The matching `X` should appear in position 1–8.
   - Connect the transducer to CH1. One of the `pin V` values will change as the pressure changes. **Write down that pin number** and put it in `PIN_PRESSURE` in `src/config.h`.
5. Press any key to exit.

## 3. Bench wiring
| Terminal | Connect |
|---|---|
| 24 V / GND | 24 VDC supply (also powers the transducer loop) |
| **CH1** (4–20 mA) | Transducer: 24 V+ → transducer → CH1 (check the board's wiring diagram for the loop connection) |
| IN1 | Pump 1 contactor aux (N/O), between IN1 and GND |
| IN2 | Pump 1 overload aux, or a toggle switch to fake one |
| IN3 / IN4 | Pump 2 running / fault (leave open with 1 pump) |
| IN5 | Remote run switch (**closed = run**). Link it to GND if not used. |
| IN6 | Float switch (**closed = tank OK**). Link it to GND if not used. |
| IN7 | Reset pushbutton |
| IN8 | Manual run pushbutton (hold) |
| Relay 1 | Pump 1 contactor coil |
| Relay 2 | Pump 2 contactor coil |
| Relay 3 | Fault alarm |
| Relay 4 | System healthy |

> ⚠️ IN5 and IN6 must be **linked to GND** if nothing's connected, otherwise it shows STOPPED / LOW TANK. Or set them to `InFn::None` in `config.h`.

### Calibrate the pressure reading (two points)
1. Transducer vented to air (0 bar): note the **pin V** value and put it in `CAL_V_ZERO`.
2. Pressurise and read the gauge on the same tee (e.g. 3.0 bar): put the **pin V** value in `CAL_V_KNOWN` and the gauge reading in `CAL_KNOWN_BAR`.
3. Upload again. The bar reading should now match the gauge.

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
| 8 | Float | Open IN6 | Pump off, `LOW TANK`, resumes when closed |
| 9 | Remote stop | Open IN5 | Pump off, `STOPPED`, no alarm |
| 10 | Overload | Close IN2 | Pump off, `PUMP 1 FAULT`. With 2 pumps, pump 2 takes over. |
| 11 | Contactor fail | Pull the contactor coil wire | No feedback in 3 s, so pump locked out (pump 2 takes over) |
| 12 | Twin alternate | Set `pumps = 2` | Lead swaps each start |
| 13 | Twin assist | Big flow, pressure drops below 1.5 bar | Second pump joins after 10 s |
| 14 | Manual run | Type `m` (or hold IN8) | Pump runs even in lockout, still stops on high pressure |
| 15 | Power cut | Kill power mid-run | Restarts after 10 s, start count kept |

## 5. Logic tests (for developers)
`./run_tests.sh` runs all the pump logic scenarios on a PC (needs `g++`). No board needed.

## Known limits (bench version)
- No Bluetooth, Wi-Fi or app yet. Settings come from `config.h`.
- Start and run-hour counters are saved every 15 minutes, so a power cut can lose up to 15 min.
- The ES32D26 isn't the production board: hobby-grade relays, too big. It's fine for the bench, but not for a pump shed.
- Analogue outputs, RS485 and the LCD aren't used yet.
