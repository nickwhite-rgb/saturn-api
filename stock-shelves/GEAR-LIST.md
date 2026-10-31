# Stock Shelves – Gear List

Prices are rough AUD estimates as of Oct 2026, including shipping. Paste the **search term** into AliExpress or eBay AU, then sort by orders or rating.
The **Proto** column covers the 10-bin prototype, which runs on a laptop. The **Full** column covers the 50-bin rack. Both include spares.

## A. Scales (AliExpress)

| Item | Search term | Proto | Full | Approx each |
|---|---|---|---|---|
| 5 kg load cell + HX711 kit | `5kg load cell HX711 kit aluminum bar` | 12 | 55 | $3–6 |
| ESP32 dev board, 38-pin, USB-C | `ESP32 DevKitC 38 pin type-c CP2102` | 2 | 7 | $4–7 |
| ESP32 screw-terminal breakout | `ESP32 38 pin expansion board screw terminal` | 2 | 7 | $3–5 |
| Calibration weight set | `calibration weight set 1kg 500g 200g` | 1 | 1 | $15–25 |
| M4/M5 bolt + spacer assortment | `M4 M5 stainless bolt nut assortment` | 1 | 2 | $15 |
| 4-pin aviation plugs (shelf to controller) | `GX12 4 pin aviation connector` | 12 | 60 | $1 |

## B. Rack computer (buy locally from an authorised reseller to avoid fakes)

| Item | Where | Qty | Approx |
|---|---|---|---|
| Raspberry Pi 5 8 GB + active cooler + case | Core Electronics / Pakronics / Little Bird | 1 | $190–230 |
| Official 27 W USB-C power supply | same | 1 | $20 |
| 64 GB high-endurance microSD | same or JB Hi-Fi | 1 | $20 |
| 10.1" 1280×800 capacitive HDMI touchscreen | AliExpress: `10.1 inch 1280x800 HDMI capacitive touch raspberry pi` | 1 | $100–150 |
| Camera Module 3 **Wide** + Pi 5 camera cable | Pakronics / Little Bird | 1 | $65–75 |
| USB NFC reader | AliExpress/eBay: `ACR122U NFC reader USB` | 1 | $35–50 |
| NFC key fobs | `13.56MHz Mifare key fob` (20 pk) | 1 | $8 |

*Proto doesn't need section B. Use a laptop.*

## C. From the wholesaler / van

- 4-core shielded alarm cable for each shelf run (you'll already have it)
- 5 V DIN-rail PSU, e.g. Mean Well HDR-30-5, for the ESP32s
- Small DIN enclosure, glands, ferrules, cable ties

## D. Bins and platforms (local is better since bulky freight from overseas is expensive)

- **Bins:** try eBay AU `plastic picking bin 400mm`, Bunnings, or a storage supplier. Check the internal width, since GPOs need about 120 mm or more.
- **Scale platforms:** 3 mm aluminium or 6 mm acrylic, cut 100 × 400. A local sign shop or laser cutter is cheapest.

## E. Optional

- LED strip so a bin lights up when someone searches for it: `WS2812B 5V 60 LED/m IP30`, 6 m, about $30

## Rough totals

| | Proto | Full rack |
|---|---|---|
| Electronics | ~$150 | ~$1,050 |
| Bins + platforms | ~$80 | ~$400 |
| **Total** | **~$230** | **~$1,450** (plus shelving if needed) |

## Buying tips

- Buy about 10% spare load cells and HX711s, because a few cheap ones will be duds.
- Choose kits that ship with the aluminium bar and mounting screws.
- Order the AliExpress gear first, since it takes 2–4 weeks to reach Warwick.
- Don't buy the Pi or the camera from AliExpress, where clones are common.
