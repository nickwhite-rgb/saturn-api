# Stock Shelves – Gear List (v2, modular)

**Build:** 50 bins in **5 modules of 10 bins each**, plus one head unit (Pi, touchscreen, camera).
**Load cells:** 40 × 5 kg and 10 × 10 kg. Each module gets 8 × 5 kg and 2 × 10 kg, so every cabinet has a couple of heavy-duty bins.
Prices are rough AUD estimates as of Nov 2026, before GST. AliExpress adds 10% GST at checkout.

---

## Why 10 bins per module (the sweet spot)

| Limit | What it means |
|---|---|
| **ESP32 pins** | Each scale needs 1 data pin, and all scales in a module share 1 clock pin. An ESP32 has about 16 safe pins free after the clock and LED pins, so **one controller can handle up to 16 bins**. |
| **Cost per module** | The fixed cost of controller, power, box and plugs is about $45 per module. That's $4.50 a bin at 10 bins or $2.80 a bin at 16. The difference is small, so let the **cabinet size** decide. |
| **Wiring** | Keep the HX711 board right next to its load cell and run the 4-wire digital lead (5V, GND, CLK, DATA) back to the controller. Keep runs inside a cabinet, under about 2 m. |
| **Read speed** | All scales on a controller are read at the same moment on the shared clock, so 10 or 16 bins update just as fast. |
| **Size and handling** | 10 bins at 100 mm wide is about a 1.2 m cabinet. Two people can lift it, and it fits a standard shelf width. |

**Recommendation:** build **10-bin modules** using a controller that can handle 16. Later you can make a 12-, 14- or 16-bin cabinet with the same box and the same code, with no redesign.

**How modules talk:** WiFi. The Pi runs its own hotspot in the same room, so a tin shed doesn't matter. Each module only needs power. Plug it in and it joins the rack. If WiFi ever proves flaky, swap to a wired ESP32 (WT32-ETH01, about $15) later.

---

## 1. Per-module kit (× 5 modules)

| Item | AliExpress search / listing | Per module | Order (5 modules + spares) | Each | Total |
|---|---|---|---|---|---|
| 5 kg load cell + HX711 | Makeit "5kg and HX711" | 8 | **40 + 4 spare = 44** | $4.89 | $215 |
| 10 kg load cell + HX711 | Makeit "10kg and HX711" (price to confirm) | 2 | **10 + 1 spare = 11** | ~$6 | $66 |
| ESP32 DevKitC 38-pin, USB-C | `ESP32 DevKitC 38 pin type-c CP2102` | 1 | **6** | ~$5 | $30 |
| ESP32 screw-terminal breakout | `ESP32 38 pin expansion board screw terminal` | 1 | **6** | ~$4 | $24 |
| 4-pin aviation plug + socket pair | `GX12 4 pin aviation connector` | 10 | **55** | ~$1 | $55 |
| Small ABS project box | `ABS project box 100x60x25` | 1 | **6** | ~$3 | $18 |
| Nylon standoff / spacer kit (lets the load cell flex) | `M4 nylon standoff spacer kit` | — | **2** | ~$10 | $20 |
| M4/M5 stainless bolt assortment | `M4 M5 stainless bolt nut assortment` | — | **2** | ~$15 | $30 |
| 5 V 2 A USB-C plug pack | local or AliExpress | 1 | **5** | ~$12 | $60 |
| **Scale side subtotal** | | | | | **≈ $520** |

**Per module:** about **$100 in electronics**. Each extra 10-bin module after the first five costs about $100, plus its bins and cabinet.

## 2. Head unit (× 1, the touchscreen module)

| Item | Where | Qty | Approx |
|---|---|---|---|
| Raspberry Pi 5 8 GB + active cooler + case | Core Electronics / Pakronics / Little Bird (not AliExpress, which has clones) | 1 | $200 |
| Official Pi 5 27 W power supply | same | 1 | $20 |
| 64 GB high-endurance microSD | same / JB Hi-Fi | 1 | $20 |
| 10.1" 1280×800 capacitive HDMI touchscreen | AliExpress: `10.1 inch 1280x800 HDMI capacitive touch raspberry pi` | 1 | $110–150 |
| Camera Module 3 **Wide** + Pi 5 camera cable | Pakronics / Little Bird | 1 | $70 |
| USB NFC reader | `ACR122U NFC reader USB` | 1 | $40 |
| NFC key fobs, 20 pack | `13.56MHz Mifare key fob` | 1 | $8 |
| **Head unit subtotal** | | | **≈ $480** |

## 3. Bench and setup gear

| Item | Search | Qty | Approx |
|---|---|---|---|
| Calibration weights (1 kg, 500 g, 200 g, 100 g) | `calibration weight set 1kg` | 1 | $20 |
| Dupont crimp kit + header pins | `dupont crimp tool kit` | 1 | $25 |
| Soldering consumables (if not already owned) | — | — | $20 |

## 4. From the wholesaler / van

- 4-core shielded alarm cable, about 15 m per module
- Cable glands, heatshrink, ties
- A GPO or power strip per cabinet for the plug packs, or one 5 V DIN supply feeding several modules

## 5. Buy locally (bulky)

| Item | Qty | Approx |
|---|---|---|
| Bins, 100 × 100 × 400 (or 150 wide for GPOs and switches) | 50 | $5–8 ea → ~$350 |
| Top platforms: 3 mm aluminium or 6 mm acrylic, 100 × 400, cut by a sign shop | 50 | ~$150 |
| Cabinet carcass per module (ply or steel shelving) | 5 | your call |

## 6. Optional

- Light-up bins: `WS2812B 5V 60 LED/m IP30`, about 1.2 m per module, about $30 for the lot

---

## Totals (rough, inc GST)

| | Cost |
|---|---|
| Scale side, 5 modules (AliExpress) | ~$570 |
| Head unit | ~$480 |
| Bench gear | ~$65 |
| Bins + platforms | ~$500 |
| **Total** | **≈ $1,600** + cabinets |

## Buying order

1. **Now:** prototype quantity from section 1 (8 × 5 kg, 2 × 10 kg, 2 × ESP32, 2 × breakout, plugs, bolts) plus calibration weights. That's about $120.
2. **Once one module works for a month:** the rest of section 1 and the head unit.
3. **Last:** bins, platforms and cabinets, once you've confirmed bin sizes against real stock.
