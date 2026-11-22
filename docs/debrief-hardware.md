# Debrief: Saturn site control hardware

Handover for a new chat about the hardware only. Written 22 Nov 2026.

## Who and what

- **Nick White**, licensed electrician, owner of **Saturn Electrical** (Warwick, QLD). He does all the install work himself on his own sites.
- Wants plain language, multiple-choice questions, and practical answers.
- **Goal:** a standard control platform for his own sites first (house, shed, depot), later sold to customers. **Hard requirement:** Claude must be able to program and change everything 100%, remotely, from a prompt. Nick only adds passwords and does the physical install.

## Related documents

- Design pack (architecture, network, I/O lists, safety, parts): https://claude.ai/artifact/6meshbiNoNZkqhmu7TZTuK
- Home Assistant Stage 1 setup guide (HA Green, SMA, C-Bus): https://claude.ai/artifact/U1dzfFL7Zs7uXNgmPM1zkW
- Repo `nickwhite-rgb/saturn-api`, branch `ccr-d1b25b67-bdlpxj`, folder `sites/`:
  - `sites/shed/shed_plc.st`: shed PLC program (OpenPLC Structured Text)
  - `sites/shed/ha_shed_package.yaml`: shed Home Assistant package
  - `sites/depot/ha_depot_package.yaml`: depot Home Assistant package (no PLC)
- Website debrief: `docs/debrief-website.md`

## The sites

| Site | Network | What's there |
|---|---|---|
| **House** | UniFi; house and big shed cabled together | **Clipsal C-Bus** (Nick programmed it; has no network interface yet), panels, phone |
| **Big shed** (same property) | Cabled to the house | **SMA Sunny Tripower** solar inverter, **2 piston air compressors**, condensate drains, water tank + pump |
| **Office / storage yard (depot)** | Separate site, own internet, UniFi | Yard lights, roller doors, gate, alarm status, office A/C, energy meter. **No compressors** |

## Architecture

- **Home Assistant per site:** HA Green at the house; a second HA at the depot.
- **Link:** UniFi Site Magic VPN between the sites. Modbus never goes on the internet.
- **Networks:** each site gets a Controls VLAN.
  - House: 192.168.10.0/24 (HA .5, PLC .10, couplers .20+, meters .30+, panels .50–.69)
  - Depot: 10.20.10.0/24
- **Screens and access:**
  - IP65 15" panel PCs in the shed and yard (an AliExpress i5-6200U panel was looked at; fine for personal use).
  - PoE Android tablets with Fully Kiosk indoors.
  - HA Companion app on phones.
- **Users:** Nick is admin; 2–3 senior staff get depot-only access.

## PLC decision (how we got here)

1. Schneider **M221** (TM221CE24R) was considered. **Rejected**: it's programmed in Schneider's Windows software, so Claude can't do it 100%. Also, the "M" models have no Ethernet.
2. ESPHome / Arduino (CONTROLLINO) controllers were considered. **Rejected**: Nick wants Schneider I/O and a real PLC, and updates must be remote (no USB).
3. **Chosen:** **Schneider TM3 remote I/O** (TM3BCEIP Modbus TCP coupler + modules) **+ OpenPLC v3** running on an industrial Linux DIN-rail box. Claude writes 100% of the program in Structured Text.
   - Use OpenPLC **v3**, which has the web upload page. v4 drops the web UI.
   - OpenPLC isn't officially supported on these boxes, but they're Debian Linux and the I/O comes from the Schneider coupler over Modbus. Expected to work; untested.
4. **The box:**

| Box | Price seen | Verdict |
|---|---|---|
| Kunbus RevPi Connect 4 (100378) | A$858–1,494 | Too expensive |
| OpenEmbed / Seeed EdgeLogix-1145 (CM5 not included) | A$445 + GST + CM5 | Not cheaper; built-in I/O not needed |
| **Seeed reComputer R1025-10** (Seeed part 113991274, CM4, 4 GB RAM, 32 GB eMMC, DIN rail, 2× Ethernet) | Pakronics A$575 inc GST (out of stock); RS AU 285-6821 ≈ A$354 inc GST (unverified listing); Seeed-vendor store ~$389 (check it's the R1025 not the R1013, the currency, and GST/shipping) | **Chosen** |

   - **4 GB vs 8 GB:** no real difference for this job. Buy whichever is cheaper. 32 GB eMMC is the bit that matters.
   - **Add the SuperCAP UPS module (LTC3350) for the R1000 series**, for a clean shutdown on power loss.
   - **Confirm it runs straight off the 24 VDC panel supply.**
   - Fallback: a fanless Intel N100 mini PC (~A$300, estimate).
   - **One box can run the house and shed together** (OpenPLC polls both couplers). Nick was leaning that way to save money; it's not confirmed.
5. The **house PLC** is wanted, but what it controls isn't decided yet (options offered: hot water on solar, pumps, gates, generator changeover).

## Shed (2 compressors lead/lag + tank)

**Hardware:** reComputer + TM3BCEIP + TM3DI16 + TM3DQ16R + TM3AI4 + 24 V PSU + HOA switches + interposing relays.

| Point | Signal |
|---|---|
| DI0–2 | C1 Auto / C1 running / C1 overload healthy |
| DI3–5 | C2 Auto / C2 running / C2 overload healthy |
| DI6–9 | Pump Auto / pump running / tank above low float / shed door |
| DQ0 / DQ1 | C1 run / C1 drain |
| DQ2 / DQ3 | C2 run / C2 drain |
| DQ4 | Pump enable |
| DQ8–15 | Direct from HA (DQ8 = shed lights) |
| AI0 | Air main pressure 4–20 mA (0–16 bar) |
| AI1 | Tank level 4–20 mA |

**Logic** (`shed_plc.st`):
- Lead/lag with lag offset 0.5 bar and lead rotation every 24 h (or fixed / swap now).
- Changeover on fault, 10 s staggered starts.
- Minimum off time 60 s; max 10 starts per hour; max run 20 min then 10 min rest.
- Timed condensate drains while running, plus one at each stop.
- Faults: overload, fail to start, pressure not rising, transmitter fault.
- Tank pump permissive with level hysteresis and an overrun fault.
- If HA is lost: compressors keep running for 4 h, then stop.
- **I/O force registers with a 60-minute auto-clear** (for the Saturn Command Center I/O page).

**Assumption to confirm:** both compressors feed **one air main with one pressure transmitter**.

## Depot (no PLC)

Depot HA talks straight to TM3BCEIP + TM3DI8 + TM3DQ8R.

| Point | Signal |
|---|---|
| DI0–1 | Roller doors |
| DI2 | Gate |
| DI3 | Alarm armed |
| DI4 | Yard PIR |
| DQ0–2 | Yard lights (comms-loss fallback ON) |
| DQ3 | Office A/C (comms-loss fallback OFF) |

Automations: sunset lights; 10 pm security mode with motion; A/C off after close; alerts for doors/gate open after hours, alarm not armed, I/O offline.

## House: existing systems

- **SMA Sunny Tripower:** try HA's built-in SMA integration first; fall back to Modbus TCP (registers 30775 / 30529, unit 3; check against SMA's register list).
- **C-Bus:** needs an Ethernet CNI (5500CN2; confirm part and power) plus a C-Bus-to-MQTT bridge (cmqttd or C-Gate + cgateweb; check which is maintained). Nick has his own Toolkit project file.

## Safety rules (non-negotiable)

- HA and the PLC give permission; **hardwired gear protects**:
  - each compressor's own pressure switch in series (high limit, about 0.5 bar above cut-out)
  - overloads
  - Hand/Off/Auto switches
  - hardwired tank low-float cut-out
  - E-stops never through software
- Schneider couplers set to **outputs OFF on comms loss** (except depot yard lights ON).
- Forces auto-clear after 60 min. **OpenPLC persistent storage OFF.**
- All work to AS/NZS 3000.

## Still needed / open

- Screenshots of the **TM3 coupler register maps** (shed and depot). The PLC slave-device addresses and the depot HA addresses are placeholders until then.
- Compressor make, size, voltage, max starts per hour and duty rating (setpoints are placeholders).
- Separate receivers or one air main? Tank capacity and height. Depot energy meter model.
- reComputer: confirm the exact model, price and 24 V power. One box or two?
- What the house PLC should control.
- Remote-change workflow: a private GitHub repo plus an auto-deploy agent on the box. Approval step (tap Approve / auto / setpoints-only auto) **not yet decided**. Nick wants Claude to do everything from a prompt; he'll add passwords only.
- The PLC code **has not been compiled yet** (the compiler build was blocked in the last session). The first upload to OpenPLC may show errors for Claude to fix.
