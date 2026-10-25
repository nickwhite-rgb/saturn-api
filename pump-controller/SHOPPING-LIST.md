# Bench Prototype: Shopping List

This list is for **Stage 1 only**: a bench rig to prove the Mode 1 code. Hold off on custom circuit boards for now.

## Controller board: Eletechsup ES32D26, DC 24 V, "With DIN Box" (AliExpress, ~AU$46)
| Spec needs | ES32D26 has |
|---|---|
| 6 DI | 8 opto-isolated NPN inputs (volt-free contact to GND) |
| 4 relay DO | 8 relays (hobby-grade, fine for contactor coils on the bench) |
| 2 AI (4–20 mA) | 4 × 0/4–20 mA + 4 × 0–10 V, **native**, no resistor needed |
| 2 AO 0–10 V | 2 × 0–10 V |
| Wi-Fi + Bluetooth | ESP32, plus RS485 |
| Power | **24 VDC**, the same supply as the transducer |

## Everything else
| Item | Qty | Notes |
|---|---|---|
| ✅ ORDERED: Eletechsup ES32D26, DC 24 V, With DIN Box | 1 | As above. **The ESP32 is NOT included.** |
| ✅ ORDERED: **ESP32-DevKitC, WROOM-32, 38-pin** (Type-C, HW-395 ESP-32D) | 2 | ~AU$10 each. Plugs into the ES32D26 socket. Must say WROOM-32, **not** S3/C3/C6/P4. Confirm the fit with the seller. Second one is a spare. |
| DIN-rail PSU 240 VAC → 24 VDC, ~1 A (e.g. Mean Well HDR-30-24) | 1 | Powers the board **and** the transducer |
| Pressure transducer 0–10 bar, 4–20 mA, G1/4 BSP, M12, stainless | 1–2 | e.g. RS PRO IPS-M12 (RS stock no. 175-4985, check its datasheet says 4–20 mA) or Balluff BSP (Pacific Automation) |
| M12 4-pin female lead, 2–5 m, moulded | 2 | Matches the transducer |
| 1/4" BSP tee + mini ball valve | 1 | Lets you swap the sensor without draining the system |
| Modular contactor, 240 V coil, 20 A (any brand from the wholesaler) | 2 | Pump 1 and Pump 2. Must have an **aux contact** for running feedback. |
| Auxiliary contact block (if the contactor doesn't include one) | 2 | Wires to IN1 / IN3 |
| Toggle switches or pushbuttons | 6 | Simulate overloads, remote run and the float, plus Reset and Manual pushbuttons |
| Small pressure pump + pressure tank (an old E30 install off a job is ideal) | 1 | Real-world testing |
| IP65 DIN-rail enclosure, ~12 module | 1 | |
| USB cable (to suit the ESP32 DevKitC, usually Type-C) | 1 | For loading the code |
| ✅ ORDERED: **EQV LCD12864-06D Gray** (FSTN transflective, ST7565R, SPI, 3.0 V), see LCD-NOTES.md | 2 | ~AU$5–15 each. Bench version of the field screen. Avoid "ST7920" blue/yellow-green 5 V types. Production units come from Newhaven/RS/Mouser instead. |
| Optional: Waveshare 7" ESP32-S3 screen | 1 | Bench test display over RS485 |

## Rough budget
Roughly **$250–450 AUD** all up, mostly the transducer. **Prices aren't checked.** Get quotes from your wholesaler for the electrical gear.
