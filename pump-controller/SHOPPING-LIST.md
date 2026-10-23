# Bench Prototype: Shopping List

This list is for **Stage 1 only**: a bench rig to prove the Mode 1 code. Hold off on custom circuit boards for now.

## Controller board: KinCony KC868-A6 (ESP32)
Why this board: it already has close to the I/O in the spec, and it's cheap enough to try.

| Spec needs | KC868-A6 has | Notes |
|---|---|---|
| 6 DI | 6 optically isolated DI | Dry contact or up to 12 V |
| 4 relay DO | 6 relays (10 A) | 2 spare |
| 2 AI (4–20 mA) | 4 AI, **0–5 V** | 4–20 mA works via a 250 Ω resistor (gives 1–5 V) |
| 2 AO 0–10 V | 2 DAC outputs | Some board revisions have a reported output fault. Not needed for Mode 1. |
| Wi-Fi + Bluetooth | ESP32 | Also has RS485 (for the Waveshare screen later) |
| Power | **12 VDC** | Needs a 240 VAC → 12 VDC supply |

**Before buying:** confirm the current revision and download the schematic from kincony.com. It sells on AliExpress and kincony.com.

## Everything else
| Item | Qty | Notes |
|---|---|---|
| KinCony KC868-A6 | 1 | As above |
| DIN-rail PSU 240 VAC → 12 VDC (e.g. Mean Well HDR-15-12) | 1 | Powers the board |
| DIN-rail PSU 240 VAC → 24 VDC (e.g. Mean Well HDR-15-24) | 1 | **Powers the transducer loop.** 12 V isn't enough for an 8–32 V transducer once the 250 Ω resistor drops 5 V. |
| 250 Ω 0.1% resistor (or a 4–20 mA to 1–5 V signal module) | 2 | One per 4–20 mA input |
| Pressure transducer 0–10 bar, 4–20 mA, G1/4 BSP, M12, stainless | 1–2 | e.g. RS PRO IPS-M12 (RS stock no. 175-4985, check its datasheet says 4–20 mA) or Balluff BSP (Pacific Automation) |
| M12 4-pin female lead, 2–5 m, moulded | 2 | Matches the transducer |
| 1/4" BSP tee + mini ball valve | 1 | Lets you swap the sensor without draining the system |
| Modular contactor, 240 V coil, 20 A (any brand from the wholesaler) | 2 | Pump 1 and Pump 2. Must have an **aux contact** for running feedback. |
| Auxiliary contact block (if the contactor doesn't include one) | 2 | Wires to DI1 / DI3 |
| Toggle switches or pushbuttons | 4 | Simulate overload faults, remote start/stop and the float |
| Small pressure pump + pressure tank (an old E30 install off a job is ideal) | 1 | Real-world testing |
| IP65 DIN-rail enclosure, ~12 module | 1 | |
| USB cable (to suit the board) | 1 | For flashing the code |
| Optional: Waveshare 7" ESP32-S3 screen | 1 | Bench test display over RS485 |

## Rough budget
Roughly **$300–500 AUD** all up, mostly the transducer and power supplies. **Prices aren't checked.** Get quotes from your wholesaler for the electrical gear.
