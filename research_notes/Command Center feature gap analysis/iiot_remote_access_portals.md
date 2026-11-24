# Remote PLC access / IIoT / cloud-SCADA portals: feature and pricing comparison against Saturn Command Center

Research date: 24 Nov 2026. Method note: direct page fetches to vendor sites (inductiveautomation.com, ixon.cloud, thingsboard.io, blynk.io) were blocked by this session's network allowlist, so all findings below come from web-search results (vendor pages, docs, resellers, aggregators as indexed). Prices therefore need confirming on the live vendor pages before they go into a customer-facing document. All prices are in the original currency, and "as seen Nov 2026" means as indexed at the time of research.

Baseline (Saturn Command Center today): PIN login; super admin plus users with per-site See/Adjust permissions; site list with status; live readings from each site's Home Assistant; I/O page with remote forcing (confirm, 60-min auto-clear, logging); equipment/I/O/network notes; site log; AI Q&A per site; light/dark mode. Edge: OpenPLC on an industrial Raspberry Pi with Schneider TM3 I/O, Home Assistant, UniFi VPN.

## Q1. Which features do comparable platforms offer that Saturn Command Center lacks?

### Takeaway
The biggest gaps against every serious competitor are, in order: (1) alarm management with notification, escalation and acknowledgement; (2) a historian with trending; (3) customer-org multi-tenancy with white-labelling; (4) a proper RBAC and audit trail; (5) edge/device management (remote firmware and program handling, health, store-and-forward). Saturn's forcing workflow and AI Q&A are not standard in this class and could be differentiators.

### Cited Findings

**Alarm management**
- Ignition's Alarm Notification module uses drag-and-drop "pipelines". Escalation works with a counter, a delay (for example 300 s so operators have 5 minutes to respond), and then a branch or Jump block to a supervisor roster. — [Ignition docs: Pipeline Escalation 7.9](https://www.docs.inductiveautomation.com/docs/7.9/alarming/alarm-notification/alarm-notification-pipelines/pipeline-escalation); [8.3](https://docs.inductiveautomation.com/docs/8.3/ignition-modules/alarm-notification/alarm-notification-pipelines/pipeline-escalation)
- Ignition has On-Call Rosters and Schedules (for example separate "Operators" and "Managers" rosters). — [Ignition docs 8.3 escalation](https://docs.inductiveautomation.com/docs/8.3/ignition-modules/alarm-notification/alarm-notification-pipelines/pipeline-escalation)
- When an alarm is cleared, acknowledged or shelved, it drops out of the pipeline. Recipients can acknowledge via email (7.9 docs). — [IA "Design like a pro: alarm management"](https://www.slideshare.net/InductiveAutomation/design-like-a-pro-alarm-management); [7.9 Alarm Notification module](https://docs.inductiveautomation.com/docs/7.9/getting-started/ignition-modules/core-modules/alarm-notification-module.md)
- In Ignition, shelving suppresses notifications and hides events from the alarm status tables for a set period, then re-evaluates the source tag. — [Ignition alarming docs 8.3](https://docs.inductiveautomation.com/docs/8.3/platform/alarming)
- Ignition's Alarm Management suite bundles Alarm Notification, SMS, Twilio and Voice (VoIP text-to-speech) notification. — [IA Alarm Management pricing](https://inductiveautomation.com/pricing/solution-suites/alarm-management)
- Forum users report a caveat: escalation can continue even after someone has responded. — [IA forum: Voice notification shelving](https://forum.inductiveautomation.com/t/voice-notification-shelving/104867.md)
- Ewon Talk2M Pro includes 50+ wake-up/alarm SMS per month. — [HMS Talk2M Pro FAQ](https://support.hms-networks.com/hc/en-us/articles/18482049979026-Talk2m-FAQ-About-the-Talk2m-pro-plan)
- IXON alarms and notifications go out by email, push and webhook. One IXON page says alarms need the Machine Insights Cloud product. — [IXON end-to-end IIoT](https://www.ixon.cloud/end-to-end-iiot-solution); [IXON service portal cloud](https://www.ixon.cloud/service-portal-cloud)
- ThingsBoard PE docs cover the alarm lifecycle, severity, propagation (up the asset hierarchy) and notification delivery. v4.4 adds an alarm rules API. — [ThingsBoard PE](https://thingsboard.io/products/thingsboard-pe/); [ThingsBoard v4.4 release notes](https://thingsboard.io/docs/pe/releases/releases-table/v4-4-x/)
- VTScadaLIGHT (free) excludes Alarm Notification (email, text and voice). In full VTScada it is a paid option priced by tag count. — [VTScadaLIGHT FAQ (vendor PDF, 2017)](https://www.vtscada.com/wp-content/uploads/2017/02/FAQ-VTScadaLIGHT.pdf); [dev.to Ignition vs VTScada](https://dev.to/plantwired_hq/ignition-vs-vtscada-unlimited-tags-per-server-or-an-all-in-one-scada-priced-by-io-3897)
- Red Lion Crimson can email alarms with CSV data-log attachments. — [Ubidots Crimson 3.2 guide](https://help.ubidots.com/en/articles/4961587-connect-crimson-3-2-using-ubidots-native-cloud-connector) (hands-on review cited in search summary)

**Historian and trending**
- The Ignition 8.3 historian has been rebuilt. Historian Core is powered by QuestDB, there is a SQL Historian, and a Historian API lets users build custom historians. The Industrial Historian suite costs USD 3,500 (one-time, unlimited). — [Control Engineering: Ignition 8.3.0](https://www.controleng.com/ignition-8-3-0-universal-historian-solution-event-streams-new-perspective-drawing-tools-so-much-more/); [Ubidots on Ignition pricing](https://ubidots.com/blog/ignition-alternatives-ubidots/)
- Ignition Edge Panel stores tag history for up to 35 days if the connection to the central server is lost (store-and-forward). — [IA Edge pricing](https://inductiveautomation.com/pricing/edge)
- Ewon Flexy logs data and pushes it to DataMailbox, a temporary cloud buffer (max about 10 days, per a competitor comparison). Apps read it through the DMWeb API. Talk2M Pro includes "Talk2M Visualisation" for historical dashboards and 10M+ DataMailbox datapoints per month. An older page said 3M per month. — [HMS Talk2M Pro FAQ](https://support.hms-networks.com/hc/en-us/articles/18482049979026-Talk2m-FAQ-About-the-Talk2m-pro-plan); [IronFlock vs Ewon (competitor)](https://ironflock.com/en/docs/comparisons/ewon/); [old Ewon compare page](https://websupport.ewon.biz/products/talk2m/compare)
- IXON offers historical data logging. An older brochure claimed up to 7 years of storage, but current pricing pages show tiered storage limits. — [IXON security/TOMs](https://www.ixon.cloud/technical-and-organizational-measures); [smartbuildings.ixon.cloud pricing](https://smartbuildings.ixon.cloud/pricing)
- Weintek EasyAccess 2.0 includes a free Basic Dashboard (per cMT X HMI, tag-based). — [Weintek USA EasyAccess](https://www.weintekusa.com/easyaccess/)

**Reporting / scheduled reports**
- The Ignition Reporting Module (scheduled PDF/email reports) is about USD 3,900 (2026, third-party figure). — [softwareconnect review](https://softwareconnect.com/reviews/ignition-inductive-automation/); [G2 pricing](https://www.g2.com/products/inductive-automation-ignition/pricing)

**Dashboard builders, customer views, multi-tenancy, white-label**
- ThingsBoard has a multi-tenancy architecture with isolated tenant and customer hierarchies, multiple levels of sub-customers and RBAC. Customers inherit the tenant's white-labelling, and tenant or customer admins can set their own. — [ThingsBoard PE](https://thingsboard.io/products/thingsboard-pe/); [Azure Marketplace TB PE](https://marketplace.microsoft.com/en-us/product/things-board.tb-pe-byol)
- ThingsBoard Mobile PE is a customisable (brandable) mobile app for end users. — [ThingsBoard PE](https://thingsboard.io/products/thingsboard-pe/)
- IXON white-labelling covers the cloud portal, hardware and mobile app, with unlimited users and assigned roles. There is a "Customer portal" product. — [IXON white-labelling](https://www.ixon.cloud/iiot-platform/white-labelling); [IXON customer portal](https://ixon.cloud/customer-portal)
- Blynk.Console Pro limits are 50 devices, 50 users and 50 templates, with sub-organisations up to 4 levels deep (multi-tenant for reselling). Branded apps in the App Store or Google Play sit at a Business or Enterprise tier, about USD 999/month (aggregators). — [Blynk docs: Limits](https://docs.blynk.io/en/blynk.console/limits); [softwaresuggest Blynk pricing](https://www.softwaresuggest.com/blynk/pricing); [TrustRadius](https://www.trustradius.com/products/blynk/pricing)
- EcoStruxure Machine Advisor has an organisation hierarchy, team invitations, customer-specific teams, RBAC, custom machine types, drag-and-drop dashboards, and machine location, profiles, logbooks and documentation ("Track / Monitor / Fix"). — [Machine Advisor FAQ](https://ecostruxure-machine-advisor.se.app/faq/welcome-to-ecostruxure-machine-advisor/); [se.com Machine Advisor](https://www.se.com/ae/en/work/services/field-services/industrial-automation/oem/machine-advisor.jsp)
- Machine Advisor "Fix" gives technicians cloud-hosted engineering software (SaaS). It pairs with the Augmented Operator Advisor AR app. — [PRNewswire 2018](https://www.prnewswire.com/news-releases/schneider-electrics-ecostruxure-machine-advisor-maximizes-the-value-of-data-for-oems-300750380.html) (dated)

**Device management: firmware, version control, config**
- Talk2M Pro adds remote firmware management, user groups, device pools, temporary users, advanced firewall rules and stronger password policies. — [HMS Talk2M Pro](https://www.hms-networks.com/talk2m/talk2m-pro); [Talk2M Pro FAQ](https://support.hms-networks.com/hc/en-us/articles/18482049979026-Talk2m-FAQ-About-the-Talk2m-pro-plan)
- Ignition 8.3 (released 16 Sep 2025, LTS) adds Git version control and collaboration, a new gateway deployment mode (environment-specific config such as dev/test/prod), a built-in REST API, enhanced store-and-forward, simplified containerisation, Event Streams (event-driven integrations), and a Secrets Management API (8.3.1). — [automation.com: Ignition 8.3 release](https://www.automation.com/en-us/products/september-2025/inductive-automation-ignition-8-3); [IA docs: New in 8.3](https://www.docs.inductiveautomation.com/docs/8.3/new-in-this-version); [DMC blog](https://www.dmcinfo.com/blog/15670/ignition-8-3-whats-new-and-reasons-to-upgrade-your-project/)
- Ignition EAM (Enterprise Administration Module) is the fleet-management tool: central backups, remote upgrades, agent health. It is sold bundled with Edge, for example OnLogic's Edge Panel + Edge EAM at USD 1,725. — [OnLogic Edge Panel+EAM](https://www.onlogic.com/store/ignition-edge-panel-eam/)

**Integrations / APIs / rule engines**
- Red Lion Crimson 3.2 has built-in cloud connectors: Generic MQTT, AWS, Azure, Google, Sparkplug, Ubidots and Cumulocity. These need software Group 2 or higher on FlexEdge DA50A/DA70A. FlexEdge cannot do data acquisition into RLConnect24 (remote-access only). — [Ubidots Crimson 3.2 guide](https://help.ubidots.com/en/articles/4961587-connect-crimson-3-2-using-ubidots-native-cloud-connector); [HMS KB: FlexEdge to RLConnect24](https://support.hms-networks.com/hc/en-us/articles/27220926412690-Connecting-FlexEdge-to-RLConnect24-for-Remote-Access); [HMS KB: Crimson Cumulocity](https://www.hms-networks.com/support/tech-support/kb-articles/27220838698258-Crimson-Cloud-Connectors--Cumulocity)
- Red Lion is now an HMS Networks brand: FlexEdge docs live on hms-networks.com, alongside Ewon. — [HMS FlexEdge](https://www.hms-networks.com/flexedge)
- ThingsBoard has a rule engine and, in v4.4, a dedicated alarm-rules API. — [ThingsBoard v4.4](https://thingsboard.io/docs/pe/releases/releases-table/v4-4-x/)
- IXON offers webhooks and a Cloud API (sold as a licence, e.g. part "IXON M051 Cloud API"). — [ztec.dk IXON M051](https://www.ztec.dk/shop/ixon-m051-ixon-cloud-api-51285/document/930)

### Inferences
- Gap ranking for Saturn, based on what is universal across vendors:
  - **Alarms plus notification**: everyone has it, and it is the primary reason farm and residential customers pay monthly (pump fail, tank level, power loss).
  - **Historian and trends**: universal.
  - **Customer-org tenancy and branded views**: ThingsBoard, IXON, Blynk, Machine Advisor.
  - **Audit trail**: IXON, Tosibox, Ewon.
  - **Fleet/edge management**: Ewon firmware, Ignition EAM, IXON.
- Saturn's 60-minute auto-clearing remote force with logging resembles Ignition-style shelving (time-boxed suppression). The same "time-boxed, logged" pattern can carry over to alarm shelving.
- Saturn already pulls from Home Assistant, which has a recorder/history and notification services. A first-pass historian and alarms could reuse HA's recorder or an InfluxDB/TimescaleDB add-on rather than being built from scratch. This needs validating for retention and multi-site aggregation.
- An AI Q&A per site is not a standard feature in any of the platforms found. It is a potential differentiator, and none of the results showed AI or anomaly detection as a mainstream entry-tier feature.

### Gaps
- AI/anomaly detection and predictive maintenance: no specific, sourced current offerings found for these vendors in this pass. Cumulocity historically marketed analytics/ML, and ThingsBoard has "Trendz Analytics", but neither was verified here.
- Digital twin/asset hierarchy details: only ThingsBoard alarm propagation over asset relations and Machine Advisor machine types were confirmed.
- Ignition Perspective map component, mobile app/push notifications, Losant workflow engine detail, VTScada map views: not researched in this pass.
- Ignition EAM standalone price and exact features were not captured from the official price list. The site was blocked.

## Q2. How do they handle remote access securely?

### Takeaway
Market practice is a vendor-brokered VPN (outbound-only from the site device, so there are no open inbound ports), per-user and per-device access groups, TOTP 2FA, temporary or approval-based access, and audit logs of who connected when. Saturn's UniFi VPN plus a PIN is weaker on identity (no MFA, shared-style PIN) and on auditability.

### Cited Findings
- **IXON**: TOTP 2FA can be enabled per user or enforced account-wide. There are device-specific and company-wide audit trails, RBAC, user groups, access request/approval and temporary access. IXrouter connects to the cloud over a VPN tunnel. Factory Access Cloud adds SSO-based user sync. — [IXON TOMs](https://www.ixon.cloud/technical-and-organizational-measures); [IXON security](https://www.ixon.cloud/security); [IXON Factory Access Cloud](https://factories.ixon.cloud/factory-access-cloud)
- **Tosibox**: access is by "Access Groups" (Lock group plus Key group). Members can be Keys, Locks, IPs, ranges or MACs. Central Lock and Virtual Central Lock collect audit logs of who used the VPN and when. Tosibox's architecture uses physical USB Keys or soft keys as the user credential. — [Tosibox helpdesk: Access Groups](https://helpdesk.tosibox.com/support/solutions/articles/2100038576); [Virtual Central Lock datasheet](https://res.cloudinary.com/doiqshxvv/image/upload/v1692682486/documents/Tosibox_datasheet_Virtual_Central_Lock_v2.2_jxp7nz.pdf)
- **Ewon Talk2M Pro**: user groups, device pools, temporary users, advanced firewall rules, stronger password policies. — [HMS Talk2M Pro](https://www.hms-networks.com/talk2m/talk2m-pro)
- **Red Lion FlexEdge**: OpenVPN, IPsec and GRE tunnels. Remote access goes through RLConnect24/RLAccess (VPN client configured in Crimson). The web server supports TLS/HTTPS with an optional Remote Control toggle. — [HMS FlexEdge](https://www.hms-networks.com/flexedge); [HMS KB RLConnect24](https://support.hms-networks.com/hc/en-us/articles/27220926412690-Connecting-FlexEdge-to-RLConnect24-for-Remote-Access)
- **Weintek EasyAccess 2.0**: licensing is per HMI (permanently bound) via a vendor relay, with 3 GB/month of high-speed data per device and top-up cards of 5 or 10 GB. — [Weintek USA EasyAccess](https://www.weintekusa.com/easyaccess/); [Weintek FAQ](https://www.weintekusa.com/faq/how-do-i-activate-weinteks-easyaccess-2-0/)
- **Ignition 8.3**: adds a Secrets Management API. — [IA docs: New in 8.3](https://www.docs.inductiveautomation.com/docs/8.3/new-in-this-version)

### Inferences
- Minimum credible security baseline for Saturn when selling to third parties:
  - per-user accounts (not PINs)
  - TOTP MFA (mandatory for Adjust permission)
  - time-limited or temporary technician access
  - an audit log of logins, views, forces and setpoint changes
  - VPN access gated per user per site
- UniFi Site Magic/Teleport can provide the tunnel, but the per-user access governance must live in the portal.

### Gaps
- Session recording (screen/session capture of remote sessions): not found for any of these vendors in the search results. Confirm whether IXON, Tosibox or Secomea offer it.
- Tosibox MFA and time-of-day access restrictions: not confirmed.

## Q3. Pricing models and real price points

### Takeaway
There are three models:
- **One-time licences**: Ignition, VTScada, Tosibox, Weintek.
- **Annual or monthly cloud subscriptions**: Talk2M Pro, IXON, ThingsBoard Cloud, Blynk.
- **Quote-only enterprise**: Losant, Cumulocity, Machine Advisor.

For 5-50 small sites, the cheapest benchmarks are about USD 99 one-time per Weintek HMI, ThingsBoard Cloud USD 49-149/month for 50-100 devices, Blynk Pro about USD 49-99/month for 50 devices, and Talk2M Pro about EUR 900-1,128/year.

### Cited Findings

**Ignition (USD, one-time; support "TotalCare" is 20% per year)**
- Platform base: USD 1,200. — [IA Full Price List](https://inductiveautomation.com/pricing/list)
- Perspective: USD 11,225 (full). — [IA price list via search](https://inductiveautomation.com/pricing/list)
- Alarm Notification module: USD 2,245. SMS, Voice and Twilio: USD 945 each. Alarm Management suite: USD 3,200. — [IA Alarm Management pricing](https://inductiveautomation.com/pricing/solution-suites/alarm-management); [G2](https://www.g2.com/products/inductive-automation-ignition/pricing)
- Reporting: about USD 3,900. Industrial Historian suite: USD 3,500. An older Tag Historian price of USD 2,365 appears stale. — [softwareconnect](https://softwareconnect.com/reviews/ignition-inductive-automation/); [Ubidots blog](https://ubidots.com/blog/ignition-alternatives-ubidots/)
- TotalCare is 20% of retail, billed annually. — [G2](https://www.g2.com/products/inductive-automation-ignition/pricing)
- Edge IIoT: USD 945. Edge Panel: USD 1,950 (includes Edge IIoT and 35-day local history). Older figures were around USD 1,300. — [IA Edge pricing](https://inductiveautomation.com/pricing/edge); [NeoMatrix (older)](https://neomatrixinc.com/blog/demystifying-ignition-architectures-part-2-mitigating-network-server-failures-at-the-edge/)
- OnLogic sells Edge Panel + Edge EAM at USD 1,725 (reseller). — [OnLogic](https://www.onlogic.com/store/ignition-edge-panel-eam/)
- Cloud Edition (AWS container): USD 0.75/hour base, plus USD 0.05 per CPU-core unit and USD 0.05 per memory unit, plus AWS infrastructure. — [AWS Marketplace](https://aws.amazon.com/marketplace/pp/prodview-mzcjg2qammc2g)
- Maker Edition is free for personal, non-commercial use only, so it is not usable for paid customer sites. — [Licenseware wiki](https://wiki.licenseware.io/wiki/inductive-automation-licensing/)

**Ewon Talk2M (HMS)**
- Talk2M Pro:
  - USD 1,128 per 12 months (5 concurrent VPN connections, unlimited mobile, 20 GB/month, 50 SMS/month). — [industrialnetworking.com TM50041](https://www.industrialnetworking.com/tm50041-talk2m-pro-service/)
  - EUR 900/year excluding VAT, with EUR 200 top-up. — [Foxon eshop](https://eshop.foxon.eu/p/talk2m-pro)
  - Another listing shows EUR 75/month Pro vs EUR 40/month Light. — [Foxon Talk2M](https://eshop.foxon.eu/talk2m-ewon-cloud)
- Overages are deducted from account credit, not blocked. — [HMS FAQ](https://support.hms-networks.com/hc/en-us/articles/18482049979026-Talk2m-FAQ-About-the-Talk2m-pro-plan)
- Talk2M Free conditions changed: users must add at least one new Ewon device per year or face charges. This comes from a blog plus a PLCTalk thread, so there is some uncertainty about exact terms. — [PLCTalk "ewon free no longer free"](https://www.plctalk.net/forums/threads/e-w-o-n-free-no-longer-to-be-free.140424/); [IndustrialMonitorDirect](https://industrialmonitordirect.com/blogs/knowledgebase/ewon-free-alternatives-industrial-remote-access-vpn-solutions)

**IXON (EUR)**
- Plan names: Remote Access Cloud Free (EUR 0), Remote Access Cloud Plus (EUR 300/month), Machine Insights Cloud (from EUR 160/month), Service Portal Cloud (EUR 500/month). These figures come from a competitor's comparison, so treat them as uncertain. — [IronFlock vs IXON](https://ironflock.com/en/docs/comparisons/ixon/)
- Aggregators list a "from EUR 120 one-time" price, which is unclear. — [Capterra](https://www.capterra.com/p/191979/IXON-Cloud/)
- IXON runs segmented pricing sites (factories.ixon.cloud, smartbuildings.ixon.cloud). — [smartbuildings pricing](https://smartbuildings.ixon.cloud/pricing)

**Tosibox (one-time hardware and keys; recurring fees limited)**
- Key TBK2: about USD 360. SoftKey licence: USD 235, or USD 999 for 5. — [Broudy Precision](https://store.broudyprecision.com/custitem8/Tosibox); [Building Controls Group TBSKL5](https://buildingcontrolsgroup.com/products/tbskl5)
- Lock 175: GBP 469. Lock 675: GBP 1,019. — [Smart Building Products UK](https://www.smartbuildingproducts.co.uk/tosibox-locks-keys)
- Lock for Container: EUR 120/year (2024 reseller list). — [Secure Corner 2024 pricelist PDF](https://secure-corner.de/wp-content/uploads/SC_TOSIBOX-PRICELIST-2024_v2.pdf)
- Forum consensus is "no recurring fees" for the core model. Some US listings mention an annual software maintenance fee, amount not found. — [IndustrialMonitorDirect](https://industrialmonitordirect.com/blogs/knowledgebase/ewon-alternatives-industrial-vpn-remote-access-solutions-comparison)

**Weintek EasyAccess 2.0**
- About USD 99-104 one-time per HMI, lifetime, non-transferable, including a basic dashboard and 3 GB/month. — [Trimantec](https://trimantec.com/products/weintek-rzacea020-hmi-accessories); [Wolf Automation](https://www.wolfautomation.com/products/operator-interfaces-hmi-touchscreens/cmt3072xh2/); [Weintek USA](https://www.weintekusa.com/easyaccess/)
- EU price: EUR 150 excluding VAT. — [Damen CNC](https://www.damencnc.com/en/weintek-easyaccess-2-0-card/a6528)

**ThingsBoard Cloud (USD/month)**
- Prototype: 49 (50 devices). Pilot: 149 (100 devices; white-label from here). Startup: 399 (500 devices). Business: 749 (1,000 devices, then USD 0.30 per extra device per month). — [ThingsBoard subscription plans docs](https://thingsboard.io/docs/paas/reference/subscriptions/); [ThingsBoard Cloud](https://thingsboard.io/products/paas/)
- Self-managed PE: reported at USD 99/299/499 per month (Pilot/Startup/Business) and perpetual from USD 4,999. This is a secondary source and unverified. — [wz-it.com ThingsBoard pricing](https://wz-it.com/en/knowledge/iot/thingsboard-pricing/)

**Blynk (USD)**
- Pro is about USD 49/month or USD 99/month depending on source. Official docs give Pro limits of 50 devices and 50 users. — [softwaresuggest](https://www.softwaresuggest.com/blynk/pricing); [TrustRadius](https://www.trustradius.com/products/blynk/pricing); [Blynk docs limits](https://docs.blynk.io/en/blynk.console/limits)
- White-label apps: about USD 999/month and up (Business or Enterprise). — [softwaresuggest](https://www.softwaresuggest.com/blynk/pricing)

**VTScada (USD, one-time per PC; SupportPlus is 15% per year)**
- VTScadaLIGHT is free to 50 I/O, without alarm notification, and allows one thin client. — [VTScadaLIGHT FAQ](https://www.vtscada.com/wp-content/uploads/2017/02/FAQ-VTScadaLIGHT.pdf)
- Base licence: up to 1,000 tags USD 3,395; 5,000 tags USD 4,295; 50,000 tags USD 7,195 (one source). Another source cites a single-server bundle at 1,000 I/O for USD 14,795 (effective 1 Jul 2025). The sources conflict, likely because bundle vs base licence and price-list dates differ. — [dev.to Ignition vs VTScada](https://dev.to/plantwired_hq/ignition-vs-vtscada-unlimited-tags-per-server-or-an-all-in-one-scada-priced-by-io-3897); [reliamag SCADA comparison](https://reliamag.com/topics/guides/best-scada-hmi-software)

**Quote-only**
- Losant: no public pricing. A competitor reports a "Launch" plan at USD 250/month with a soft cap of 1,000 devices, billed per payload. — [toolradar Losant](https://toolradar.com/tools/losant); [Blynk vs Losant (competitor)](https://www.blynk.io/blog/losant-alternative-blynk-comparison)
- Cumulocity: consumption-based subscription with no public rate card (G-Cloud 14 doc, Apr 2024). — [G-Cloud Cumulocity pricing](https://assets.applytosupply.digitalmarketplace.service.gov.uk/g-cloud-14/documents/92771/633849279697710-pricing-document-2024-04-24-1500.pdf)
- EcoStruxure Machine Advisor: pay-per-use or annual subscription, with machine plans and add-on features on pre-paid licences. A free tier covers one machine for 3 months. No public price. — [se.com](https://www.se.com/ae/en/work/services/field-services/industrial-automation/oem/machine-advisor.jsp); [Machine Advisor FAQ](https://ecostruxure-machine-advisor.se.app/faq/welcome-to-ecostruxure-machine-advisor/)

### Inferences
- **Per-site cost of a commercial stack at 20 sites (rough, unverified)**:
  - ThingsBoard Cloud Pilot (white-label) is USD 149/month in total, about USD 7.50 per site per month.
  - Talk2M Pro is about EUR 900-1,128/year for the whole account, but concurrency is limited to 5.
  - Ignition central server plus Edge per site would be USD 945-1,950 per site, plus a central server of USD 10k+ with Perspective. That is overkill for residential or farm sites.
- These numbers bracket what Saturn could charge. A cloud monitoring subscription in the AUD 10-40 per site per month range would be consistent with ThingsBoard/Blynk economics. This is an inference; no AU price comparisons were found.

### Gaps
- AUD pricing from Australian distributors (e.g. IPD, NHP, Control Logic) was not found.
- The official Ignition price list (EAM, Perspective Mobile, SQL Bridge), the official IXON price page, and the official Blynk page could not be fetched (network blocked). Their figures above are from search snippets or resellers.

## Q4. Which features do integrators most value or charge customers for?

### Takeaway
There is little direct evidence. Forum data suggests integrators bundle remote-access hardware into project fees rather than itemising it, and value no-subscription or low-recurring options. The recurring revenue vendors push for is customer portals, white-label, and monitoring with alarms.

### Cited Findings
- On PLCTalk, users treat remote-access hardware as part of the project fee: "an extra 1000 for remote access is not a large bill on a 50-100k project". — [IndustrialMonitorDirect summarising PLCTalk](https://industrialmonitordirect.com/blogs/knowledgebase/ewon-alternatives-industrial-vpn-remote-access-solutions-comparison)
- Integrators push back on recurring costs. A PLCTalk user disputes a GBP 1,000/year quote for Ewon as too high, and Tosibox is praised for no recurring fees and near-direct connection performance. — [PLCTalk thread](https://www.plctalk.net/forums/threads/e-w-o-n-free-no-longer-to-be-free.140424/); [IndustrialMonitorDirect](https://industrialmonitordirect.com/blogs/knowledgebase/ewon-alternatives-industrial-vpn-remote-access-solutions-comparison)
- Vendors market customer portals and service models to OEMs/integrators as "new business models", for example IXON Service Portal Cloud and white-labelling "to build trust and loyalty". — [IXON service portal](https://www.ixon.cloud/service-portal-cloud); [IXON white-labelling](https://www.ixon.cloud/iiot-platform/white-labelling)
- Machine Advisor targets OEMs offering after-sales services: logbooks, documentation per machine, and remote fix. — [se.com Machine Advisor](https://www.se.com/ae/en/work/services/field-services/industrial-automation/oem/machine-advisor.jsp)

### Inferences
- For Saturn's market (residential, farm, light commercial), the most sellable recurring features are likely:
  - **Alarms to phone**: SMS/push for pump, tank or power faults, with an on-call roster and acknowledgement.
  - **A simple customer-facing branded view**: their own org, read-only or limited-adjust.
  - **Monthly or periodic reports**: runtime hours, energy, alarm history.
  - **"We see it before you do" monitoring**: device heartbeat and offline alerts.
- Remote programming and forcing is valued by the integrator (it saves truck rolls) more than by the customer.
- The low-recurring-fee preference suggests Saturn could position its own portal as an alternative to Talk2M or IXON subscriptions.

### Gaps
- No r/PLC or r/SCADA threads were reached by search, and no survey data on integrator billing practices was found. This needs targeted community research.
- No Australia-specific evidence on what farm or residential customers will pay per month for monitoring was found.
