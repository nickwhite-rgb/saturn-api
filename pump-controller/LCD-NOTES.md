# Bench LCD: EQV LCD12864-06D (Gray), from the seller's datasheet

- Type: **FSTN transflective, positive** (dark on light, sun-readable). The Gray option only. Blue and Black are negative types.
- Controller: **ST7565R**, SPI. 128×64 dots. 1/65 duty, 1/9 bias.
- Supply: **3.0 V** (run from the ESP32's 3V3 pin).
- Backlight: white LED (LEDA/LEDK). Check whether there's a series resistor on the board before powering it. If not, add ~100 Ω from 3V3.
- Operating temp −20 to +70 °C, storage −30 to +80 °C.
- Size: 61.8 × 45.8 × 6.8 mm, viewing area 58 × 28 mm.

| Pin | Symbol | Use |
|---|---|---|
| 1 | CS | Chip select |
| 2 | RES | Reset |
| 3 | RS | Command/data (a.k.a. A0/DC) |
| 4 | SCK | SPI clock |
| 5 | SDA | SPI data (MOSI) |
| 6 | VDD | 3.0–3.3 V |
| 7 | GND (VSS) | Ground |
| 8 | LEDA | Backlight + |
| 9 | LEDK | Backlight − |
| 10–13 | IC_SCK / IC_CS / IC_SDO / IC_SDI | Chinese font chip, not used, leave unconnected |

Pin numbers are as read from the listing's small pin table. Confirm against the silkscreen on the board.
