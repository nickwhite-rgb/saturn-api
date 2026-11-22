# Debrief: Saturn Command (website)

Handover for a new chat about the website only. Written 22 Nov 2026.

## Who and what

- **Nick White**, licensed electrician, owner of **Saturn Electrical** (Warwick, QLD, 10+ staff, uses AroFlo).
- Not a developer. Wants plain language, bullet points, multiple-choice questions, and things that make the business less reliant on him.
- **Saturn Command** is a mobile-first, password-protected web app to oversee **all sites**: Nick's own (house, shed), the Saturn depot, and customer jobs. It has AI built in.

## Where the code lives

- Repo: `nickwhite-rgb/saturn-api` (to be renamed `saturn-command`; Nick does that in GitHub settings).
- Hosting: **Railway** (Node.js + Express + Postgres). Deploys from `main`.
- Work branch: **`ccr-d1b25b67-bdlpxj`**, **not merged to `main` yet, so not live**. Railway still runs the old Saturn OS.
  - `0c54c39`: Saturn Command v1 (working, tested)
  - `e9e91a4`: I/O forcing groundwork (not wired into the API/UI yet)
- It was repurposed from **Saturn OS** (an AroFlo purchase-history tool). Lachlan built a different system (Orbit), so Saturn OS isn't needed. All AroFlo/purchase code is removed. Old tables `purchase_lines` and `sync_runs` are **dropped automatically on first start** (Nick approved). The `users` table and existing PINs are kept.

## Files

| File | What |
|---|---|
| `index.js` | Server and all API routes |
| `auth.js` | 6-digit PIN login, signed-cookie sessions (30 days), rate limiting, first-admin bootstrap |
| `migrate.js` | Creates/updates tables on every start |
| `ha.js` | Home Assistant: live readings, `fetchStates`, `callService`, AES-256-GCM token encryption |
| `ai.js` | "Ask AI" via the Claude API |
| `io.js` | **New, not wired up yet:** I/O point list parser, live I/O read, force / release / clear-all |
| `ui.html` | The whole mobile UI (single page, hash routes) |
| `sites/shed/*`, `sites/depot/*` | PLC program and Home Assistant packages (see the hardware debrief) |

## Done and tested (v1)

- **Login** by PIN.
- **Site list:** search, filters (All / Not OK / Customer / Saturn / Personal), status dot, "+ New site".
- **Site page tabs:** Overview, Live (HA readings, refresh every 15 s), Ask AI, Equipment (equipment / I/O list / network / notes as text), Log, Settings (edit; HA URL + token + entity list).
- **Team** (admin): add employee (PIN shown once), profile page with name / role / mobile / email, ticks for **Can add new sites**, **Admin** and **Active**, per-site **None / View / Edit** buttons (save instantly), Reset PIN.
- **Permissions:**
  - Admins see and edit everything.
  - Staff see only ticked sites; view-only users can't edit or log.
  - Whoever creates a site gets Edit on it.
  - An admin can't demote or deactivate themselves.
- **Ask AI:** model `claude-opus-5-5`, effort `medium`, server-side refusal fallback (`fallbacks: "default"`, beta `server-side-fallback-2026-07-01`). Context is the site record + last 30 log entries + live readings. **Read-only** (it can't change anything).
- **Testing:** 34 API checks passed against local Postgres 16 with a mock Home Assistant and a mock Claude API. Phone-size UI checked with Playwright (no layout overflow). **Not yet tested against the real Claude API, a real HA, or Railway.**

## Decisions already made

- Name: **Saturn Command**.
- AI: **v1 = Q&A only**. **v2 = Claude drafts PLC/HA changes and sends them to GitHub for one-tap approval** (not started).
- Users: staff log in. Each employee has a profile page with per-site view/edit ticks.
- Master dashboard **top tabs: Overview, Alarms, Activity, Sites, Team**.
- **I/O page with forcing** for testing/commissioning:
  - Remote forcing allowed **with safeguards**: confirm step, big "FORCES ACTIVE" banner with countdown, every force logged, phone alert from HA.
  - **Auto-clear after 60 minutes** (enforced in the PLC itself, and on PLC restart).
  - Who: needs Edit access to the site **plus** a new per-employee **"Can force I/O"** tick (admins always can).

## In progress: what's left to build

1. **API routes** in `index.js` (`io.js` already does the work):
   - `GET /api/sites/:id/io`: calls `io.readIO(site)`
   - `POST /api/sites/:id/io/force` `{kind, ch, mode: auto|on|off|value, value}`: needs edit access + (admin or `can_force`); calls `io.setForce`
   - `POST /api/sites/:id/io/clear`: `io.clearAll`
   - `GET /api/dashboard`: all visible sites with status, active force count, live headline and HA reachability (parallel, short timeouts)
   - `GET /api/activity`: combined log across visible sites
   - Site PATCH/POST must accept `io_prefix`, `io_hub`, `io_slave` and an `io_points_text` field, parsed with `io.parsePoints` (return line errors) and stored as `io_points` JSONB
   - `/api/me`, the user profile and the PATCH routes must include `can_force`
2. **UI:**
   - Top tab bar: Overview (stat tiles plus a site tile grid), Alarms (sites not OK, forces active, HA unreachable), Activity, Sites, Team.
   - New site tab **I/O**:
     - Digital inputs, outputs and analogs with LED dots.
     - For each output, show "logic wants" alongside the actual state.
     - Auto/Off/On buttons; analog force by engineering value.
     - Confirm dialogs, with a stronger warning for forcing outputs ON.
     - Orange FORCES ACTIVE banner with countdown and "Clear all forces".
     - Refresh every ~3 s.
   - Settings card "I/O & forcing" (prefix, hub, slave, points text).
   - "Can force I/O" tick on the employee profile.
   - Idea floated: a dark "control room" theme to make the dashboard look great.
3. Re-run the API and UI tests, then commit and push.

## How I/O and forcing works ("Saturn PLC standard v1")

- Saturn Command → the site's **Home Assistant REST API** → HA `modbus.write_register` → PLC.
- Reads come from HA sensors named `sensor.<prefix>_...` (prefix e.g. `shed`).

| PLC | Holding reg | HA sensor | Meaning |
|---|---|---|---|
| %MW48 | 1072 | `<p>_plc_inputs` | Real inputs (bits) |
| %MW57 | 1081 | `<p>_plc_inputs_effective` | Inputs after forces |
| %MW49 | 1073 | `<p>_plc_outputs` | Real outputs (after forces) |
| %MW58 | 1082 | `<p>_plc_outputs_logic` | What the logic wants |
| %MW55 / 56 | 1079 / 1080 | `<p>_force_active` / `<p>_force_remaining` | Force status / seconds left |
| %MW70 / 71 | 1094 / 1095 | `<p>_ai0_raw` / `<p>_ai1_raw` | Real raw analog |
| %MW60–67 | 1084–1091 | (written) | DI mask, DI value, DQ mask, DQ value, AI mask, AI0 raw, AI1 raw, minutes |

- `io.js` keeps active forces in the `site_forces` table as the source of truth and writes the whole force block each time.
- If the PLC reports no forces for more than 90 s after the last change, the DB forces are cleared and logged.

**I/O point list format** (typed in Settings, one point per line):

```
DI0  C1-AUTO   Compressor 1 HOA in Auto
DQ0  C1-RUN    Compressor 1 run command
AI0  AIR-PT    Air pressure | bar | 0-16 | sensor.shed_air_pressure
AI1  TNK-LVL   Tank level | % | 0-100 | sensor.shed_tank_level | raw 0-10000
```

DI/DQ channels are 0–15. AI is 0–3; only AI0 and AI1 can be forced. The default raw range is 0–10000.

## Going live (Nick to do)

1. Merge `ccr-d1b25b67-bdlpxj` into `main` (Claude can open the PR if asked).
2. Railway variables:
   - add `ANTHROPIC_API_KEY`
   - make sure `SESSION_SECRET` is set (never change it later: it also encrypts the HA tokens)
   - delete the old `AROFLO_*` variables
3. Rename the GitHub repo and the Railway service to Saturn Command, and check Railway is still linked.
4. Per site: create an HA long-lived token (a non-admin HA user is best), then enter it in Settings with the HA URL (Nabu Casa) and the entity list.

## Open points and risks

- Newer Home Assistant may want `device_address` instead of `slave` in modbus calls and config. Check on the real HA.
- **OpenPLC "persistent storage" must stay OFF**, or forces could survive a PLC restart.
- AI cost: roughly a few cents per question (estimate).
- Customer jobs: wording on remote access and AI use belongs in customer agreements (solicitor to check).
