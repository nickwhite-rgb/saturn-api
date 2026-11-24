# Operational features for remote monitoring platforms: alarms, history/reporting, maintenance/FSM, assets/documents, commissioning, audit

Research date: 24 Nov 2026. Method note: web *fetch* was blocked in this session (help.aroflo.com and apidocs.aroflo.com could not be reached, and the session said to stop fetching other hosts). Every finding below comes from search-result snippets and summaries, not full page reads. Verify primary docs before building against them. Costs are as published at the dates shown. Items marked "(inference)" are my reasoning or general industry knowledge, not a cited source.

## 1. Alarm management: ISA-18.2 / IEC 62682 essentials scaled down; products; AU SMS/voice gateways

### Takeaway
The useful core of ISA-18.2/EEMUA 191 for a small portal is: (1) a written alarm philosophy, (2) rationalization, where every alarm gets a priority, cause, consequence, corrective action and response time, or else becomes a logged "event", (3) an alarm state model with acknowledge, return-to-normal, shelving with auto-expiry and suppression, (4) flood and nuisance KPIs, and (5) a notification pipeline with delay, escalation and on-call rosters. Ignition's Alarm Notification Module and WIN-911 are the reference designs to copy. For delivery in Australia, about 5-7c per SMS and about 7.5c/min (Twilio) or 15c per call (ClickSend) for voice is the going rate.

### Cited Findings
**Standards essentials**
- ISA-18.2 lifecycle: rationalization covers justifying each alarm against the alarm philosophy, then documenting, prioritizing and classifying it. These steps can be done in one combined session. The lifecycle applies to new and existing systems. — [Yokogawa / Control Engineering, Implementing Alarm Management per ISA-18.2](https://www.yokogawa.com/us/library/resources/media-publications/implementing-alarm-management-per-the-ansi-isa-182-standard-control-engineering/)
- Eliminating or greatly reducing nuisance alarms is an early step, and prioritization of the remaining alarms follows. Some alarms can be reclassified as "events" that are recorded for later review instead of demanding operator attention. — [Yokogawa / Control Engineering](https://www.yokogawa.com/us/library/resources/media-publications/implementing-alarm-management-per-the-ansi-isa-182-standard-control-engineering/); [Emerson alarm management paper](https://www.emerson.com/documents/automation/alarm-management-en-57058.pdf)
- Advanced alarm features include shelving, and surfacing rationalization data (cause, potential consequence, corrective action, time to respond) to the operator from the alarm summary. — [Emerson](https://www.emerson.com/documents/automation/alarm-management-en-57058.pdf)
- ISA-18.2 load benchmarks: about 150 alarms/day per operator is "very likely acceptable" and about 300/day is "maximum manageable" (about 6 and 12 per hour). — [Yokogawa](https://www.yokogawa.com/us/library/resources/media-publications/implementing-alarm-management-per-the-ansi-isa-182-standard-control-engineering/)
- Alarm flood: more than 10 alarms in 10 minutes. One definition says a flood starts when the 10-minute count exceeds 10 and ends when it drops below 5. EEMUA 191 targets are under 1 alarm per 10 minutes in steady state, under 10 alarms in the first 10 minutes after an upset, and floods for under 1% of the time. — [IChemE Alarm Management Practical Guide (2021)](https://www.icheme.org/media/16019/icheme-alarm-management-practical-guide-06_05_2021.pdf); [Towards Data Science, SCADA alarm standards & metrics](https://towardsdatascience.com/scada-alarm-standards-metrics-56512db128b0); [ABB SCADA alarm management white paper](https://library.e.abb.com/public/72f20c70c7b44d889d463db81df5c38d/SCADA%20Alarm%20Management%20White%20Paper.pdf)
- EEMUA 191 is now at Edition 4 (Nov 2024). — [EEMUA 191 Ed.4 contents](https://eemua.org/getattachment/9d3f8071-55c3-49bf-a74a-3bf6ad4a2e0f/Contents-EEMUA-Publication-191-Edition4-November-2024.pdf)

**Ignition (Inductive Automation): reference pipeline design**
- The Alarm Notification Module provides pipelines with delay, escalation, consolidation and selection logic in a drag-and-drop editor. Escalation "elevates alarm notification when operators are not handling alarms." — [Inductive Automation, Alarm Notification module](https://inductiveautomation.com/ignition/modules/alarm-notification); [Ignition 8.1 docs](https://docs.inductiveautomation.com/docs/8.1/ignition-modules/alarm-notification)
- Rosters are lists of users, and each user's schedule is checked so only people currently on shift are notified. Rosters are static, but a calculated roster can be built on demand in a Notification block. — [Ignition 8.3 docs](https://docs.inductiveautomation.com/docs/8.3/ignition-modules/alarm-notification)
- Notification profiles cover email, SMS and voice. A Twilio SMS profile is available. SMS includes an acknowledge link, and voice acknowledgement is a spoken instruction during the call. — [Ignition docs](https://docs.inductiveautomation.com/docs/8.3/ignition-modules/alarm-notification); [Product sheet](https://inductiveautomation.com/static/pdf/ProductSheet_Alarm_Notification_Module.pdf)
- Ignition 8.3 has a pipeline Test button, and the docs advise test mode on Notification blocks or a testing-only roster so tests don't page the real on-call person. — [Ignition 8.3 docs](https://docs.inductiveautomation.com/docs/8.3/ignition-modules/alarm-notification)

**WIN-911 (SmartSights / AVEVA)**
- Escalation is configured by alarm type, severity, worker role and schedule, and unacknowledged alarms automatically pass to the next responder. It supports remote acknowledgement by phone or app, plus alarm and report requests (you can ask the system for status). — [SmartSights WIN-911](https://smartsights.com/products/win-911/); [AVEVA WIN-911 overview PDF](https://aveva.com/content/dam/aveva/documents/brochures/WIN-911_Overview_English.pdf.coredownload.inline.pdf)
- Pricing: one reseller lists subscriptions starting at US$3,300 (date unclear). — [Motion World listing](https://motionworld.com/products/195194)
- SmartSights blog (1 Sep 2026) on the rising "hardware tax" on SMS alerts (GSM modems) points to cloud/app delivery instead. — [SmartSights blog](https://smartsights.com/the-hardware-tax-on-sms-alarm-alerts-is-getting-worse-theres-now-a-better-way/)

**PagerDuty-style on-call tools**
- Atlassian Opsgenie: end of sale 4 Jun 2025, end of support and data deletion 5 Apr 2027. Alternatives commonly named are PagerDuty, incident.io, Jira Service Management, xMatters, Rootly, Grafana IRM and Splunk On-Call. Don't build on Opsgenie. — [xurrent blog](https://www.xurrent.com/blog/opsgenie-end-of-life); [incident.io](https://incident.io/blog/what-to-do-when-ops-genie-sunsets.md); [PagerDuty](https://www.pagerduty.com/resources/digital-operations/learn/opsgenie-alternative-key-factors-2026/)

**Australian SMS/voice gateway costs**
- ClickSend (Melbourne HQ, owned by Sinch), from its Critical Information Summary: AU SMS 7.2c (<5k), 6.7c, 6.2c, 5.7c (250k+), AUD ex-GST. Dedicated number $19/month. Text-to-speech voice is $0.15/call to mobiles and $0.053 to landlines. **Conflict:** a 2026 review says ClickSend voice, email, fax and post are *closed to new customers*. — [ClickSend CIS PDF](https://clicksend-api-downloads.s3.ap-southeast-2.amazonaws.com/_public/_legal/ClickSend+-+Critical+Information+Summary.pdf); [SMSComparison ClickSend review 2026](https://www.smscomparison.com.au/reviews/clicksend-review/)
- Sinch MessageMedia: subscription plans from $45/month up to $1,799/month, about 5.8c/SMS on larger plans (2025 estimate). No published voice rate found. — [SMSComparison gateways 2026](https://www.smscomparison.com.au/sms-gateway/); [mobilemessage.com.au comparison (Nov 2026)](https://mobilemessage.com.au/best-bulk-sms-provider-australia)
- Mobile Message (AU) is among the cheapest. AU rates at 15k msgs/month range from 3.0c (Mobile Message) to 12.33c (Bulk SMS Australia). Note that this source is Mobile Message's own comparison. — [mobilemessage.com.au](https://mobilemessage.com.au/best-bulk-sms-provider-australia)
- Twilio AU: outbound SMS $0.0515/segment for the first 20k/month, tiering down to $0.0474. Voice outbound to AU is about $0.0252/min to landlines and $0.075/min to mobiles. Currency was not clearly stated (likely USD). — [Twilio SMS AU pricing](https://www.twilio.com/en-us/sms/pricing/au); [Twilio Voice AU pricing](https://www.twilio.com/de-de/voice/pricing/au)
- Plivo AU SMS $0.0473 (default network). — [Plivo](https://www.plivo.com/sms/pricing/au/)

### Inferences
- Minimum viable alarm engine for Saturn: an alarm definition table per point (setpoint, deadband, on-delay/off-delay, priority 1-3 or Critical/High/Low, cause, consequence, action, response time, notify group). States are Active-Unacked, Active-Acked, RTN-Unacked and Normal, with timestamps for raise, ack (who), RTN and clear. Shelving should be time-limited (e.g. 1h/8h/24h, auto-unshelve, reason required, logged). "Out of service" is the maintenance mode, with a visible banner. Anything without a defined operator action becomes an Activity "event", not an alarm.
- Concrete examples: Shed compressor "failed to start / lead didn't run when demanded", tank low-low and pump dry-run are Critical (voice call). Compressor high run-time or lag frequently assisting is Low (next-day digest). Depot alarm panel armed/disarmed is an event. Door open after hours is High (SMS).
- Notification pipeline: wait N minutes, then SMS/push the on-call person. If no ack in 10-15 minutes, voice call. If still no ack, escalate to the backup or owner. Quiet hours should only suppress Low priority, never Critical. Consolidation (one SMS per site per few minutes) handles floods, plus a "flood mode" banner when more than 10 alarms in 10 minutes. Ack by replying to the SMS or tapping a link in it (as Ignition does).
- Gateway choice: Twilio (global API, programmable voice with keypad ack) or ClickSend/Mobile Message for cheap AU SMS. Real volumes for 3-10 sites will be tens to hundreds of messages a month, so cost is a few dollars a month. Reliability (two providers, plus fallback to push/email) matters more than price.
- Track per-site KPIs monthly (alarms/day, top 10 "bad actors", standing alarms, shelved count, time-to-ack) as a lightweight ISA-18.2 "monitoring & assessment" stage.

### Gaps
- No primary-source text on ISA-18.2/IEC 62682 shelving time limits or suppression rules (paywalled standards).
- VTScada, Zenon and Ewon (Talk2M/Ewon Flexy alarm features) were not covered by results. Ewon Talk2M appears to be mainly remote access, which is unverified.
- PagerDuty free-plan limits and 2026 AUD pricing were not found.
- Twilio AU currency was unconfirmed. ClickSend voice availability for new accounts is conflicting.

## 2. Historian & reporting

### Takeaway
Mature platforms pair a tag historian (raw plus aggregated data, with defined retention) with a scheduled report engine (cron schedule, then PDF/CSV, then email or save). Home Assistant's built-in long-term statistics (hourly, kept forever) cover numeric trends. Binary/state history (compressor on/off, door open), which is what you need for run hours and cycle counts, is purged after 10 days by default unless it's exported to a separate store.

### Cited Findings
- HA short-term statistics are saved every 5 minutes and kept as long as state history (default 10 days, `purge_keep_days`). Long-term statistics are hourly and never auto-deleted, but only for sensors with a suitable `state_class`. Lights, switches and binary sensors don't get them. — [Home Assistant long-term statistics docs](https://www.home-assistant.io/docs/configuration/long-term-statistics/)
- InfluxDB is mainly useful for history HA doesn't put into statistics (binary sensors, textual states, attributes) and for combining data across systems. It adds write load and storage, and needs backups. A common pattern is recorder (MariaDB) for about a week plus InfluxDB long-term with a retention policy. — [raspberry.tips (2026)](https://raspberry.tips/?p=16279); [HA community: data persistence](https://community.home-assistant.io/t/data-persistence-options/605423/2); [smarthomescene InfluxDB guide](https://smarthomescene.com/guides/how-to-keep-home-assistant-history-infinitely-with-influxdb-on-proxmox)
- Ignition Reporting Module: cron-style schedules on the Gateway. Output to PDF, HTML, CSV or RTF. Actions include email, save file and run script. It pulls data from the tag historian and alarm journal, and parameters can be bound to tags. — [Ignition report schedules](https://www.docs.inductiveautomation.com/docs/8.1/ignition-modules/reporting/report-schedules); [Reporting product sheet](https://inductiveautomation.com/static/pdf/ProductSheet_Reporting_Module.pdf)

### Inferences
- Saturn should run its own central historian, not rely on each site's HA recorder. It could poll or subscribe through HA's WebSocket API and write to Postgres+TimescaleDB, or to InfluxDB. Keep raw data 90 days to 1 year, 1-min/hourly rollups for 5+ years, and daily rollups forever. This also survives a site's HA being rebuilt.
- Compute derived counters server-side: compressor run hours and starts per compressor (lead/lag balance), pump run hours and starts per hour (short-cycling), tank litres used per day, yard-light on-hours, door open counts. These feed both reports and maintenance triggers (section 3).
- Reports worth copying: weekly site summary email/PDF covering alarms raised/acked, top bad actors, run hours, starts, tank usage, uptime of the HA link and forced I/O events. Monthly customer-facing report (good for service contracts). CSV export of any trend. Use one report template per site type.
- Trend UI: multi-pen charts with alarm and force events overlaid on the trend, so you can see the setpoint, the alarm and who forced what.

### Gaps
- No sourced guidance on typical retention periods for small-site SCADA. The figures above are inference.
- VTScada/Zenon reporting features were not researched.

## 3. Maintenance: run-hour/cycle triggers, PM, work orders, FSM integration (AroFlo, simPRO, ServiceM8, CMMS IoT)

### Takeaway
The CMMS pattern is meter, then trigger rule, then work-order template, then auto-created work order (MaintainX). For Saturn, the natural target is AroFlo. AroFlo has an API (HMAC-SHA512, form-POST with `zone` and `postxml`, off by default, about 2,000 calls/day reportedly), with zones that include tasks (jobs) and assets. Saturn can therefore keep meters and rules itself and create AroFlo tasks against client assets when thresholds are crossed. simPRO (REST, Premium tier, webhooks) and ServiceM8 (REST, OAuth with `create_jobs` scope, webhooks) are well-documented alternatives for future customers.

### Cited Findings
**CMMS IoT patterns**
- MaintainX meter triggers "perform an action when a meter reading meets conditions you specify". A WO template populates automatically when the trigger is met, and follow-up WOs can be created. Integrations include MQTT, MachineMetrics (Enterprise plan required) and Flume Water. "Automated meters" replaced the older IoT meters. — [MaintainX: about meters](https://help.getmaintainx.com/about-meters); [Automate work orders with meter readings](https://getmaintainx.com/blog/how-to-automate-work-orders-with-meter-readings); [MaintainX MQTT integration](https://www.getmaintainx.com/integrations/mqtt); [MachineMetrics integration](https://www.getmaintainx.com/integrations/machine-metrics)

**AroFlo**
- The AroFlo API uses HMAC-SHA512 key authentication. Secrets and authorisation fields are under Site Administration, and a signature is generated per request. POSTs send form variables `zone` (e.g. tasks, assets) and `postxml` (XML in AroFlo's format), URL-encoded. Official docs live at apidocs.aroflo.com with a Postman collection. — [AroFlo API v3.0 documentation page](https://help.aroflo.com/en_AU/aroflo-api-v30-documentation); [apidocs.aroflo.com](https://apidocs.aroflo.com/)
- The API is off by default (ask AroFlo support to enable it) and is reportedly capped at 2,000 calls/day. This comes from a third-party (Cotlet) guide, so it's unverified. — [Cotlet AroFlo MCP server](https://mcpservers.org/servers/cotlet-dev-integrations-aroflo-api-mcp-server)
- AroFlo does not publish an OpenAPI spec. — [Jentic AroFlo](https://jentic.com/apis/aroflo.com/aroflo)
- A community MCP's docs describe an assets zone supporting list, create and update, and a TaskResources insert via POST. These are unofficial. — [glama aroflo-mcp assets doc](https://glama.ai/mcp/servers/@0x1NotMe/aroflo-mcp/blob/ef156158826fa60c2fe3ef5e8e4c63d43a24d79c/docs/api/assets.md); [TaskResources](https://glama.ai/mcp/servers/@0x1NotMe/aroflo-mcp/blob/ef156158826fa60c2fe3ef5e8e4c63d43a24d79c/docs/api/taskresources.md)
- Zapier has an "AroFlo HMAC" app with a "find clients and create tasks" template. This confirms task creation via API is supported. — [Zapier AroFlo HMAC + Google Forms](https://zapier.com/apps/aroflo-hmac/integrations/google-forms/1135430/find-clients-and-create-tasks-in-aroflo-hmac-with-new-google-forms-responses)
- In AroFlo a job is called a "task". Tasks can be created against client assets, or assets added via a task's Assets tab. — [AroFlo help: office adding assets to tasks](https://help.aroflo.com/office-adding-assets-to-tasks); [Tasks overview](https://help.aroflo.com/office-tasks-overview)
- AroFlo has Periodic Tasks for scheduled/recurring maintenance (with seasonal variation) and automatic notification when work is due. Its asset management covers lifecycles, maintenance schedules and usage patterns. Custom fields are available on tasks, clients, locations, suppliers, assets and users. — [AroFlo periodic routine maintenance](https://aroflo.com/features/periodic-routine-maintenance); [AroFlo asset management](https://aroflo.com/features/asset-management); [AroFlo customisation](https://aroflo.com/features/software-customisation); [AroFlo blog](https://aroflo.com/blog/three-features-facilities-maintenance-love-about-aroflo)
- A third-party API grader rates the AroFlo API "Grade D+" (quality and usability opinion). — [Supergood API report card](https://supergood.ai/api-report-card/aroflo)

**simPRO**
- REST API v1.0 is part of the Premium subscription, scoped per customer build (`https://{build}.simprosuite.com/api/v1.0`). Docs are at developer.simprogroup.com. It has create-job and create-site operations, Customer Assets and Plant & Equipment sections, and webhooks with events such as `site.created`, `asset.updated`, `job.asset.tested` and `quote.asset.assigned`. — [simPRO Developer Center](https://developer.simprogroup.com/apidoc/); [apis.io simPRO Jobs](https://apis.io/apis/simpro/simpro-jobs-api/); [Hookdeck simPRO webhooks guide](https://hookdeck.com/webhooks/platforms/guide-to-simpro-webhooks-features-and-best-practices)

**ServiceM8**
- Create a job with `POST https://api.servicem8.com/api_1.0/job.json`, which requires the OAuth scope `create_jobs`. Public add-ons use OAuth 2.0, and tokens expire after 3600 s. Webhooks go via `webhook_subscriptions`. Payloads don't include changed values, so the record has to be re-fetched. Conflict: one third-party guide uses an `X-Api-Key` header for job creation, while a forum post reports a 401 without OAuth. — [ServiceM8 job create reference](https://developer.servicem8.com/reference/post-job-create); [ServiceM8 webhooks overview](https://developer.servicem8.com/docs/webhooks-overview); [ElevenLabs ServiceM8 guide](https://elevenlabs.io/docs/reception-ai/integrations/servicem8)

### Inferences
- Saturn's maintenance model: an asset has meters (run hours, starts, calendar days), PM rules (e.g. compressor every 500 run-hours OR 6 months, whichever comes first: drain/filter/oil; pump every 12 months: inspect seals, check pressure switch), and generated "service due" items. On a trigger, create an AroFlo task against the client and location (and asset if mapped), with a description including the current meter value and a link back to the Saturn asset page. Store the AroFlo TaskID and poll for status, since AroFlo webhooks were not confirmed.
- Also allow "alarm to work order": a Critical alarm not cleared within X hours, or a repeat bad actor, offers a one-click "Raise AroFlo job" pre-filled with the alarm history.
- Because of the 2,000/day limit and HMAC complexity, use AroFlo only for job creation and status sync. Keep high-frequency data in Saturn.
- Design an FSM adapter interface (createJob, getJobStatus, linkAsset) so simPRO/ServiceM8 customers can be supported later.

### Gaps
- Could not read apidocs.aroflo.com (blocked in session). Exact task/asset zone fields, whether WHERE filters/joins exist, whether AroFlo has outbound webhooks, and API pricing or plan requirements are unconfirmed.
- Fiix and UpKeep IoT-trigger specifics were not researched in depth. The MaintainX pattern is representative.
- simPRO asset create schema is unconfirmed.

## 4. Asset register & QR/NFC tags

### Takeaway
Best practice is one record per physical asset (make, model, serial, install date, warranty expiry, supplier, photos, linked I/O points, documents, meters, service history). A durable QR tag with a printed human-readable ID opens the asset page, and scanning is made the fastest way to raise a request.

### Cited Findings
- A QR scan should open the full asset record (history, open WOs, PM schedule, parts, SOPs) and allow raising a WO or request already tied to the asset ID. "Guest scanning" lets non-users submit requests without a licence. — [Oxmaint QR guide](https://oxmaint.com/blog/post/blog-post-cmms-qr-code-asset-tagging-guide); [Fabrico: CMMS with QR tagging](https://www.fabrico.io/blog/best-cmms-software-with-qr-code-asset-tagging/); [MaintainX QR codes](https://www.getmaintainx.com/blog/optimizing-maintenance-with-qr-codes-and-a-cmms)
- Every WO must link to the correct asset for MTBF, cost-per-asset and PM-compliance metrics to mean anything. — [Oxmaint](https://oxmaint.com/blog/post/blog-post-cmms-qr-code-asset-tagging-guide)
- Field practices: print a human-readable asset number beside the QR as a fallback, photograph each tag at install, audit about 5% of tags monthly, and use labels rated for the environment (anodised aluminium or polyester; UV, heat, washdown). Make scanning the fastest path to a WO, or techs will bypass it. — [Oxmaint](https://oxmaint.com/blog/post/blog-post-cmms-qr-code-asset-tagging-guide); [eWorkOrders asset tagging guide](https://eworkorders.com/asset-management/asset-tagging/)
- QR suits stationary equipment and scan-on-demand use. RFID/barcodes suit mobile assets or bulk inventory. — [eWorkOrders](https://eworkorders.com/asset-management/asset-tagging/)
- No source confirmed native QR tagging in AroFlo. — (search returned only other vendors; [AroFlo asset management](https://aroflo.com/features/asset-management))

### Inferences
- Use QR URLs like `https://command.saturn…/a/{shortId}`. If the user isn't logged in, show a public "report a fault / call Saturn" page with no data exposed. If logged in, show the full asset page. Optionally write NFC tags with the same URL (cheap NTAG215 stickers).
- The Saturn asset is the master for monitoring, and it maps to the AroFlo asset ID for jobs. Warranty-expiry reminders can reuse the PM engine.

### Gaps
- No Australian-specific asset-labelling standard was researched.

## 5. Document storage per site (drawings, SLDs, manuals, test certificates, commissioning sheets)

### Takeaway
Store documents per site and per asset with type, revision and date. For electrical work in Queensland, certificates of testing and compliance must be kept for 5 years. The governing law changed on 1 Sep 2026 to the Electrical Safety Regulation 2026, which WorkSafe says introduces no new obligations.

### Cited Findings
- QLD electrical contractors must keep a copy of the certificate of testing and safety (electrical equipment) and the certificate of testing and compliance (installation work) for 5 years. — [WorkSafe QLD: issuing certificates of compliance](https://worksafe.qld.gov.au/laws-and-compliance/electrical-safety-laws/issuing-certificates-of-compliance); [Business Queensland: contractor safety duties](https://www.business.qld.gov.au/industries/building-property-development/building-construction/electrical-contracting/safety-duties)
- The Electrical Safety Regulation 2026 commenced 1 Sep 2026 and replaced the 2013 Regulation, which expired 31 Aug 2026. WorkSafe says it is a remake with reorganised sections and no new policy obligations. A 2013-vs-2026 section comparison is published, so section numbers for certificate provisions have likely changed. — [WorkSafe QLD ES Regulation 2026](https://www.worksafe.qld.gov.au/laws-and-compliance/electrical-safety-laws/electrical-safety-regulation-2026); [Overview of section changes 2026 vs 2013](https://www.worksafe.qld.gov.au/_media/documents/linking/Overview-of-section-changes-2026-vs-2013.pdf); [Legislation: sl-2026-0113](https://www.legislation.qld.gov.au/view/html/inforce/current/sl-2026-0113/lh)

### Inferences
- Document types: SLD/schematic, switchboard layout, PLC/HA config backup, equipment manual/datasheet, Certificate of Testing & Compliance, RCD test record, commissioning sheet, photos, warranty, compliance (e.g. pressure vessel docs for the compressor receiver).
- Each document has a revision, an "issued/superseded" status and a retention date. Certificates get a 5-year minimum retention lock (no delete before expiry), and ideally the 5-year clock is tracked from the date issued to the customer.
- Make documents searchable and available to the existing AI Q&A ("what's the breaker for pump 1?").

### Gaps
- Exact retention wording and section number under the 2026 Regulation were not confirmed from the legislation text.
- RCD testing record requirements (AS/NZS 3760 for in-service testing; QLD workplace RCD testing under WHS/ES rules) were not researched.
- No Master Electricians Australia or NECA guidance retrieved.

## 6. Commissioning tools (I/O check sheets, sign-off, as-built, handover packs)

### Takeaway
Commissioning practice is a per-point record (tag, description, I/O type, address, signal and engineering range, expected vs. actual, tester, date, signature), proved end to end with a loop check (field device, wiring, channel, logic, HMI, alarm or final element). It is signed off by category and bundled into an acceptance/handover package. Saturn already has forced I/O and notes, so a guided "commissioning mode" that walks each point and records pass/fail is a natural extension.

### Cited Findings
- Per-point I/O check sheet fields: tag name, description, I/O type, module address, signal range, engineering-unit range, test date, tester signature, operator sign-off, comments/deficiencies. — [Industrial Monitor Direct: PLC I/O testing best practices](https://industrialmonitordirect.com/blogs/knowledgebase/plc-io-testing-and-commissioning-best-practices-for-first-time-engineers)
- A loop check proves a field signal travels through the correct wiring, I/O channel, PLC logic, HMI display and alarm or final element. Point-to-point (wire endpoints) and I/O checkout are separate steps. Prerequisites are the I/O list, loop/wiring diagrams and a loop-check form with a unique tag ID. — [plcprogramming.io loop check procedure](https://plcprogramming.io/blog/loop-check-procedure)
- An acceptance test package includes I/O check sheets with sign-off columns for all parties, an alarm/interlock verification matrix, control loop test records, communication test results, a P&ID cross-reference and a non-conformance report template. — [Industrial Monitor Direct: FAT/SAT guidelines](https://industrialmonitordirect.com/blogs/knowledgebase/fat-and-sat-procedure-guidelines-for-industrial-control-systems)
- Commissioning sign-off tables by category (safety, electrical, PLC I/O) with "Verified By" and date. — [Industrial Monitor Direct: commissioning template](https://industrialmonitordirect.com/blogs/knowledgebase/industrial-machine-commissioning-procedure-standard-template)
- Building-automation point-to-point verification templates include the signature of the inspector or commissioning authority. — [MangoApps BAS point-to-point template](https://www.mangoapps.com/templates/inspections/building-automation-point-to-point-verification)

### Inferences
- Commissioning mode for Saturn: generate the checklist from the site's I/O list. For each point, the tech forces an output or simulates an input at the field device, Saturn auto-captures the observed value and timestamp, and the tech marks pass/fail with a photo. Alarm tests confirm the notification actually arrived on the phone. Close with sign-off (tech, then supervisor or customer, typed name plus signature plus timestamp).
- Freeze the result as a PDF handover pack: site summary, as-built I/O list, alarm list with setpoints, check sheets, certificates, SLD, HA config backup hash, manuals and contact/on-call details. Attach it to the AroFlo task as well.
- Re-commissioning on change: editing a point's mapping marks it "untested" until rechecked.

### Gaps
- No concrete handover-pack or as-built standard found (e.g. AS/NZS or NATSPEC handover requirements). Sources are vendor blogs and templates.

## 7. Audit trail (who/what/when/why, immutability)

### Takeaway
A defensible design is an insert-only audit table where each row records the user, a UTC timestamp, the action, the object, old and new values and a reason, hash-chained (each row includes the previous row's hash) with a verifier, plus periodic anchoring and archival to WORM storage. Append-only on its own isn't tamper-evident.

### Cited Findings
- ISPE: append-only logs alone don't meet 21 CFR Part 11 §11.10(e) because they aren't tamper-evident. Part 11 doesn't explicitly require hash chains, but they're a good way to meet the "secure" and "don't obscure prior records" requirements. Hash chain: each row stores the previous hash plus a SHA-256 of its own content. A verifier walks the chain and reports the first break. Writes must be serialized. Anchor the chain head daily (signature or RFC 3161 timestamp) to prove time. — [ISPE iSpeak: what §11.10(e) actually requires](https://ispe.org/pharmaceutical-engineering/ispeak/what-21-cfr-part-11-ss1110e-actually-requires-audit-trail-and-why)
- SCADA audit fields: NTP-synced UTC timestamp, old and new values, unique authenticated user, action code, and e-signature meaning (Approved/Reviewed). The Ignition audit DB user gets INSERT only. Keep the archive in a separate zone and use WORM storage (e.g. S3 Object Lock) for final archives. — [Industrial Monitor Direct: SCADA for 21 CFR Part 11](https://industrialmonitordirect.com/blogs/knowledgebase/selecting-scada-systems-for-fda-21-cfr-part-11-compliance)

### Inferences
- Part 11 is a US pharma rule and doesn't apply to Saturn, but it is the best-documented model. Saturn should audit login and PIN failures, permission changes, forces and unforces (including auto-clear at 60 minutes), setpoint and alarm-config changes, acks and shelves (with reason), document uploads and deletions, and commissioning sign-offs.
- Shared PINs undermine "who". Use per-user PINs (or PIN plus device) so every action is attributable. Require a reason on forces, shelving and config changes.
- The DB role for the app gets INSERT/SELECT only on the audit table, with nightly export of the chain head and rows to object-locked storage.

### Gaps
- No Australian-specific audit-log requirement found for this context. Retention could follow the 5-year certificate retention as a practical default (inference).
