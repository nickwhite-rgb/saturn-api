# Saturn Stock Shelves (concept)

A weighing shelf for the workshop that counts gear, knows who took it and for which job, and tells the office when to reorder.

- `mockup.html` is a clickable mockup with three views: the 10" shelf touchscreen, the office dashboard, and the full project brief with the parts list. Open it in a browser. All data in it is made up.

## The idea in one screen

- **Hardware:** 50 bins (5 shelves × 10). Each bin sits on a 5 kg load cell with an HX711 amplifier, and one ESP32 controller per shelf reads its 10 scales. A Raspberry Pi with a 10.1" touchscreen, a camera and an NFC reader runs the rack.
- **Counting:** count = (reading − empty bin) ÷ unit weight. This works well for anything over about 20 g each (GPOs, switches, breakers, downlights). Small loose items like screws and lugs go in bags.
- **Sign-in:** face recognition runs locally on the Pi, with a PIN or key tag as a fallback.
- **Unlogged removals:** if weight leaves a bin while nobody is signed in, the Pi takes one snapshot and flags it on the dashboard.
- **Web app:** live stock, a reorder list grouped by supplier, an activity log and staff enrolment. It would sit alongside this Saturn API on the same Railway Postgres database.
- **AroFlo:** pulls today's jobs per tech, pushes the materials used onto the job, and turns the reorder list into draft POs. **To confirm:** that the AroFlo API supports writing materials and POs.

## Rough hardware budget

About **$2,430 inc GST**, including shelving (about $2,080 without it). The line items are in the brief tab of the mockup.

## Things to sort before building

- **Bin size:** a double GPO (about 115 × 73 mm) only fits a 100 × 100 bin on its side. Consider 150 mm bins for GPOs and switches.
- **Privacy:** face templates are biometric "sensitive information" under the Privacy Act. Get advice before turning face sign-in on. Get written consent, offer the PIN/tag opt-out, put up a sign, and delete the data when someone leaves.
- **Temperature drift:** shed temperatures move the load-cell readings. The software auto-zeroes the scales and they need a recalibration once a month.

## Build stages

1. **One-shelf prototype:** 10 bins with no camera and no AroFlo link. Prove the counting is accurate over a month (about $500).
2. **Full rack and dashboard:** all 50 bins, PIN/tag sign-in and low-stock emails.
3. **Face sign-in and AroFlo jobs:** the camera, consent forms and snapshots of unlogged removals.
4. **AroFlo materials and draft POs:** usage reports per tech and per job.
