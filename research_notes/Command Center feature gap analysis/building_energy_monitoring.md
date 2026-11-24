# Building Management, Facility Monitoring and Solar/Energy Monitoring Platforms: Features Useful to Saturn Command Center

Research date: 24 Nov 2026. Method: web search across vendor sites, docs, AU solar media (SolarQuotes, Energy Matters, Solar Choice), Whirlpool, QCA/AER/Ergon. Caveat: direct page fetches to solaranalytics.com.au and service.wattwatchers.com.au failed because this session's network does not allow those hosts, so some figures come from search-result extracts of those pages and are not verified against the full page. Most feature claims are vendor marketing, not independent tests.

## Energy features (consumption/generation dashboards, AU tariffs/cost, self-consumption, load shifting, demand alerts, carbon, bill comparison)

### Takeaway
Solar platforms all offer production/consumption/self-sufficiency dashboards. The features AU homeowners actually value are tariff-aware cost tracking, retail plan comparison (Solar Analytics claims >$400/yr savings), and solar-surplus load switching (Wattwatchers 6M+3SW, CATCH). Home Assistant's built-in Energy dashboard already does flows, per-device breakdown and self-sufficiency, but handles TOU/demand tariffs and per-device cost poorly. Warwick is on the Ergon network (regional QLD, QCA-regulated notified prices) with a 5-8pm weekday demand window for small business. That gives Saturn a concrete gap to fill: tariff-aware cost, demand alerts and surplus-based load control.

### Cited Findings
**Home Assistant Energy dashboard (the data source Saturn already has)**
- Tracks individual devices, and each one can be nested under an upstream device so usage isn't double-counted. — [HA docs: individual devices](https://www.home-assistant.io/docs/energy/individual-devices/)
- Energy cards include a self-sufficiency gauge, which falls as the home relies on grid imports. — [HA Energy cards](https://home-assistant.io/lovelace/energy)
- The official docs don't cover self-consumption as a metric. Users compute it manually. — [dev.to write-up](https://dev.to/cloudapp_dev/self-consumption-autarky-pv-metrics-in-home-assistant-4a9); [HA community](https://community.home-assistant.io/t/energy-dashboard-self-sufficiency-gauge-card/341692)
- TOU pricing works by pairing each utility_meter tariff (peak/off-peak) with a matching price sensor. A template sensor can serve as the price entity, and HA then builds a cost sensor. — [HA community](https://community.home-assistant.io/t/quick-tldr-on-how-to-create-multi-tariff-energy-meters/585379); [HA community](https://community.home-assistant.io/t/wth-cant-i-set-a-price-entity-for-my-power-consumption/809214/2)
- The native dashboard lacks per-device cost under dynamic prices. People use the "Dynamic Energy Cost" custom integration to get daily, monthly and yearly cost sensors per device. — [HA community](https://community.home-assistant.io/t/custom-integration-dynamic-energy-cost-track-real-time-and-interval-electricity-costs-per-device/726931); [GitHub](https://github.com/martinarva/dynamic_energy_cost)
- There is an open user request for a tariff entity so the dashboard can group consumption and cost by Peak/Off-peak. I found no evidence it has been implemented. — [HA community WTH](https://community.home-assistant.io/t/wth-allow-configuring-tariffs-for-the-energy-dashboard-and-displaying-consumption-for-each-one/802280)
- I found nothing on native demand (kW) charge handling.

**Australian / Warwick tariff context**
- Ergon Energy Retail must charge regional QLD customers the notified prices set by the Queensland Competition Authority (QCA). — [Ergon annual report 2025-26](https://www.parliament.qld.gov.au/Work-of-the-Assembly/Tabled-Papers/docs/5826T1808/5826t1808.pdf)
- QCA notified-price tables include TOU and TOU-demand tariffs for residential, small business and large business. Examples are Tariff 22E, 24A and 65A (2025-26 draft). — [QCA draft determination appendices 2025-26](https://www.qca.org.au/wp-content/uploads/2024/12/notified-prices-draft-determination-appendices20408006.pdf); [QCA draft main report](https://www.qca.org.au/wp-content/uploads/2024/12/notified-prices-draft-determination-main-report20407949.pdf)
- On Ergon's small business TOU demand network tariff (BTDEM), the peak demand window is 5-8pm weekdays. The large business tariff (LTOUD) has time-varying demand and energy charges, with demand billed in kVA by default. — [Ergon Network Tariff Guide 2025-26](https://www.ergon.com.au/__data/assets/pdf_file/0009/1651554/Ergon-Energy-Network-Tariff-Guide-2025-26.pdf)
- Notified prices are below the cost of supply. The QLD Government paid Ergon Retail about $603M of community service obligation (CSO) subsidy in 2024-25. — [QCA info booklet 2025-26](https://www.qca.org.au/wp-content/uploads/2024/12/information-booklet-notified-prices.pdf)
- A final 2026-27 regional determination exists. I did not review its figures. — [QCA 2026-27 final report](https://www.qca.org.au/wp-content/uploads/2025/12/notified-prices-final-determination-report.pdf)

**Plan comparison / bill features**
- Solar Analytics Plan Optimiser: an ARENA-funded project reported it "can save solar consumers over $400pa" through retail plan analysis. This is a project claim. — [ARENA](https://arena.gov.au/projects/solar-analytics-monitoring-better-energy-outcomes); [Plan Optimiser PDF Sep 2021](https://info.solaranalytics.com/kb-assets/5070749919/Solar%20Analytics%20-%20Plan%20Optimiser%20general%20-Sep21.pdf)
- With CATCH Control hardware, the plan optimiser and apps carry no ongoing subscription. CATCH's portal also notifies when the network has activated dynamic export limiting or the emergency backstop. — [SolarQuotes (Solar Analytics Integrated / CATCH)](https://www.solarquotes.com.au/blog/solar-analytics-integrated/); [SolarQuotes](https://www.solarquotes.com.au/blog/?p=86442)
- Fronius Solar.web Premium adds a 7-day weather forecast, a 2-day PV yield forecast, and "PV profitability" and yield/cost overviews. — [Fronius Solar.web](https://www.fronius.com/en/solar-energy/installers-partners/products-solutions/monitoring-digital-tools/pv-system-monitoring-solarweb); [Fronius FAQ](https://www.fronius.com/faq-online-tools/faq-solar-energy/solarweb)

**Load shifting / control**
- The Wattwatchers Auditor 6M+3SW adds switching for up to three contactors (single- or three-phase), controlled from Wattwatchers apps or third-party apps via API. Listed uses include scheduling EV charging on solar generation or the lowest-cost period, and inverter disconnection or DRM0 control for export limiting (SA Power Networks). — [Wattwatchers 6M+3SW overview](https://service.wattwatchers.com.au/auditor-6m3sw-overview); [product overview PDF](https://service.wattwatchers.com.au/hubfs/Knowledge%20base/Wattwatchers%20Hardware/A6M+3SW/Auditor%206M+3SW%20Product%20Overview%20v.02.pdf)
- Wiser Home (Clipsal) has an Energy Management section that appears only when energy devices are installed. PowerTag circuit monitors pair in order: grid, then solar, then large loads (HWS, aircon). A Wiser AI feature enrols high-energy devices such as a Schneider EV charger to cut costs. — [Schneider Wiser Home System User Guide](https://www.productinfo.schneider-electric.com/wiser_home_pacific/wiser-system-user-guide-clipsal-home/English/System%20User%20Guide_Wiser_Home_Clipsal%20(bookmap).pdf)
- Emporia Vue tracks net metering/solar and supports app setup around TOU, peak demand and excess solar. It does circuit-level monitoring (e.g. EV charger, aircon). — [Emporia blog](https://www.emporiaenergy.com/blog/welcome-to-vue); [Walmart.ca listing](https://www.walmart.ca/en/ip/Emporia-Gen-3-Smart-Home-Energy-Monitor-16-50A-Circuit-Level-Sensors-Automation-Control-Real-Time-Electricity-Monitor-Meter-Solar-Net-Metering/2KIGHI96G68S)
- Reposit Power battery VPP: 1 GridCredit (A$1) per kWh exported from the battery during a grid event. That flat rate doesn't track wholesale spikes. A 2026 review estimates $400-$1,000+/yr earnings with a 10kWh battery. — [SolarQuotes on GridCredits](https://www.solarquotes.com.au/blog/powershop-reposit-gridcredits); [SolarQuotes review](https://www.solarquotes.com.au/blog/will-reposit-really-halve-the-payback-time-of-your-battery/); [TechGuide](https://www.techguide.com.au/?p=105416)

**Carbon / benchmarking (commercial)**
- Schneider EcoStruxure Building Activate (formerly Energy Hub) "automatically connects, stores, visualizes, reports, and alarms" on energy data. It offers benchmarking and compliance support (US codes ASHRAE/IECC/Title 24). Vendor claims 15-30% savings. — [Schneider small buildings energy management](https://www.se.com/uk/en/work/software/ecostruxure-building/small-buildings-energy-management); [Schneider Building Activate](https://www.se.com/uk/en/work/products/product-launch/ecostruxure-building-activate); [Engineering News, Sep 2025](https://www.engineeringnews.co.za/article/simplicity-scalability-and-sustainability-saas-energy-management-for-small-business-2025-09-04)

### Inferences
- The highest-value, lowest-cost addition for Saturn is a tariff engine. It would store per-site tariff definitions (QCA notified tariffs such as residential TOU and small business TOU-demand with the 5-8pm weekday window) and compute cost from HA energy statistics. HA only does this partially, so the portal can add value on top of HA rather than duplicating it.
- Demand alerts follow naturally. At the depot or shed, warn when rolling 30-minute kW in the 5-8pm weekday window approaches that month's peak, and inhibit the compressors via the OpenPLC/TM3 outputs. That mirrors what 6M+3SW-style contactor switching does, using hardware Saturn already has.
- Solar-surplus load shifting (run the shed compressors or tank pump when the SMA export exceeds X kW for Y minutes) matches the Wattwatchers and CATCH use cases. It is easy to implement as a PLC/HA rule with a portal UI for thresholds.
- Show a "dynamic export limit / backstop active" indicator. CATCH users value it, and QLD has emergency backstop requirements for new solar. That requirement comes from general knowledge and is not sourced here; verify it.
- Carbon reporting would mainly matter to light-commercial customers. A simple kWh × NGA emissions factor would suffice. The factor is not sourced here.

### Gaps
- Final 2025-26 and 2026-27 QCA notified c/kWh and demand $/kW rates were not extracted. The PDFs are linked above.
- Whether HA's 2025-2026 releases added native tariff or demand-charge support is unconfirmed.
- No source found on bill-comparison features (actual bill vs. modelled) other than the Solar Analytics Plan Optimiser.

## Installer/fleet features (fleet view, fault/underperformance alerts, customer vs installer portal, white-labelling, sharing with homeowners)

### Takeaway
Every solar OEM portal (SMA, Fronius, Enphase) plus Solar Analytics and Wattwatchers provides an installer fleet view with fault alerts sent to the installer and customer, role-based access (owner/installer/guest), and invites for homeowners. The common patterns are a New/Active alert triage list with acknowledgement, performance-ratio and inverter-comparison underperformance alerts, comms-loss alerts, and an audit log of who changed what. Explicit white-labelling showed up mainly for retailer partners (Solar Analytics) and was not confirmed for OEM portals.

### Cited Findings
- **SMA Sunny Portal powered by ennexOS:** shows all systems in one account with grouping by custom criteria, and has Administrator/Installer/User roles. It alerts on portal-to-system communication faults, on automatic inverter comparison, and on performance ratio threshold breaches. — [SMA America Sunny Portal](https://www.sma-america.com/products/apps-software/sunny-portal); [SMA manual Data Manager](https://manuals.sma.de/Business-Systeme-PL/en-US/15213608971.html); [SMA manual STP X](https://manuals.sma.de/Business-Systeme-PL/en-US/14400672779.html)
- **SMA Smart Connected:** free automatic inverter fault monitoring. Errors go to the SMA Contact Center or the installer, and SMA emails the owner and installer once a fault is diagnosed. — [SMA Smart Connected](https://www.sma.de/en/products/monitoring-control/sma-smart-connected.html)
- SMA notifications include an event report covering all events and a regular yield "info report". The monitoring API is an optional add-on. Classic Sunny Portal is migrating to ennexOS, with users, reports and Smart Connected settings carried over. — [SMA notification types](https://manuals.sma.de/SPortal/en-US/16916601355.html); [SMA manual](https://manuals.sma.de/SPortal/en-US/16908584971.html)
- **Fronius Solar.web:** automatic service notifications let installers handle cases remotely, and installers get real-time fleet monitoring and remote fault monitoring. Installers see the same key data Fronius Tech Support uses on a service case. Roles are owner (full), supervisor (most settings) and guest (limited), and the installer usually holds owner. Every change is logged in a message center, and owners can revoke installer remote-config rights. — [Fronius AU installer page](https://www.fronius.com/en-au/australia/solar-energy/installers-partners/products-solutions/monitoring-digital-tools/pv-system-monitoring-solarweb); [Fronius FAQ](https://www.fronius.com/en/faq-online-tools/faq-solar-energy/solarweb); [Fronius AU press](https://www.fronius.com/en-au/australia/solar-energy/about-us/press/intelligent-energy-management-and-servicing-support-300720)
- **Enphase Installer Portal:** a real-time fleet view, plus a Maintenance Alerts widget to manage the whole fleet's maintenance needs. A third-party source says Production Issue and Microinverter Not Reporting alerts carry sub-status reasons, including RMA eligibility. A Reports tab emails yearly production and consumption reports. The portal sends site-access emails to customers and has a Service Manager link. — [Enphase AU documentation](https://www.enphase.com/en-au/installers/resources/documentation/apps); [Enphase installer newsletter 2023-12](https://enphase.com/installers/news/2023-12); [ION Solar training (3rd party)](https://sites.google.com/ionsolar.com/field-support/trainings/monitoring-platforms/enphase/troubleshooting); [Enlighten feature sheet](https://www.solarelectricsupply.com/media/sparsh/product_attachment/custom/upload/Enlighten_feature_sheet.pdf)
- **Solar Analytics fleet dashboard:** the alerts page sorts and searches the fleet's alerts, split into "New" and "Active", with an acknowledge button. Analytics auto-diagnose faults, and alerts go to both the installer and the customer. — [Solar Analytics Alerts Page](https://info.solaranalytics.com/solar-analytics-installers-resellers/fleet-dashboard-features/alerts-page/); [Monitor your fleet](https://info.solaranalytics.com/solar-analytics-installers-resellers/installer-getting-started/monitor-your-fleet/)
- Solar Analytics also runs a retailer partner programme. A dedicated retailer page suggests branded/partner offerings, but white-label details are unconfirmed. — [Solar Analytics retailer](https://solaranalytics.com.au/retailer)
- Reliability matters: SolarQuotes reported Solar Analytics users "plagued by dropouts". — [SolarQuotes](https://www.solarquotes.com.au/blog/solar-analytics-users-plagued-by-dropouts/)
- **Wattwatchers:** the Fleet Manager and the Onboarding installer app are included with hardware purchase. Installers request Fleet access, or access for a business customer, via a form. — [Wattwatchers get Fleet Manager](https://service.wattwatchers.com.au/getting-started/get-fleet-manager); [Wattwatchers toolkit](https://service.wattwatchers.com.au/getting-started/toolkit); [Fleet Manager user guide](https://service.wattwatchers.com.au/software/fleet-manager-user-guide)

### Inferences
- Saturn's per-site permissions already resemble these models. The gaps are a role split (installer/admin vs. owner vs. guest/view-only), a homeowner invite flow, and an owner-visible audit log of settings changes and I/O forces. Fronius's "owner can see who changed what and revoke installer rights" is a good trust model for customer jobs where Saturn can force outputs.
- An alert inbox with New → Acknowledged → Resolved states, plus cause sub-statuses, would make the existing site log actionable.
- Underperformance alerts can be built from SMA data Saturn already has: expected yield vs. actual (performance ratio, or a clear-sky/peer comparison when one site has several inverters), comms-loss, and "zero production in daylight".
- Note that SMA Smart Connected already emails the installer on inverter faults. Saturn should ingest or deduplicate rather than replicate it.

### Gaps
- White-labelling for SMA, Fronius and Enphase installer portals is unconfirmed. I found no source.
- Wattwatchers Fleet Manager feature details couldn't be read because the host is blocked. Also unverified: a search-result claim that Solar Analytics stops commissioning new Wattwatchers-integrated sites from 1 July 2026, and an unverified review comment that Wattwatchers had gone out of business. The 1 July 2026 claim comes from a search summary of the [Solar Analytics pricing page](https://www.solaranalytics.com.au/subscription-prices) and needs confirming.

## BMS features (schedules/holiday calendars, setpoints, equipment templates, FDD rules, analytics, work-order hand-off)

### Takeaway
Commercial BMS/analytics platforms (Niagara, Switch Automation, Schneider Building Activate) differ from solar portals by offering configurable fault rules (severity + duration), portfolio KPIs/benchmarking, role-specific dashboards with scheduled reports, CMMS work-order integration, cloud historian, remote access and backup. For a small contractor, the useful subset is configurable rules with duration/severity, schedules with holiday exceptions, equipment templates (a "compressor" or "pump" template with standard points and rules), and pushing an alarm to a job/work-order.

### Cited Findings
- **Switch Automation:** FDD with real-time monitoring and alarm management, and a pre-built analytics library covering equipment health, energy and indoor environmental quality. — [Switch smart building solutions](https://www.switchautomation.com/solutions/smart-building-solutions); [Switch DX3](https://www.switchautomation.com/platform/switch-dx3)
- Switch fault rules are configurable with conditions such as severity and duration (not hard-coded). Tasks or work orders can be assigned to users with priority and due date. Dashboards are role-specific with scheduled reports. — [Switch Performance Optimization overview PDF](https://info.switchautomation.com/hubfs/Performance%20Optimization%20Overview_US.pdf)
- Switch auto-triggers work orders in Fiix CMMS based on equipment condition, occupancy and custom alerts. "Events" serve as work requests to track issue resolution. — [Fiix App Exchange](https://fiixsoftware.com/app-exchange/switch-automation); [Switch roadmap: alerts analysis](https://switchautomation.com/roadmap-releases/alerts-analysis)
- Switch offers portfolio dashboards, benchmarking and KPI management. A profile claims 6,500+ buildings. — [Switch financial services](https://switchautomation.com/solutions-industry/financial-services); [YesPress profile](https://yespress.io/switch-automation)
- **Tridium Niagara Cloud Suite:** Niagara Data Service is SaaS cloud storage of Niagara data on Azure. Niagara Remote gives browser access to a station. Niagara Recover does cloud backups. It also has live read/write APIs and a management portal. — [Tridium NDS](https://www.tridium.com/us/en/Products/niagara-cloud-suite/niagara-data-service); [NDS features overview 2025-05](https://www.tridium.com/content/dam/tridium/en/documents/document-lists/cloud/nds/tri-niagara-data-service-features-overview-en-2025-05.pdf); [SDM mag](https://www.sdmmag.com/articles/105333-tridium-introduces-cloud-capabilities-that-simplify-and-modernize-connected-building-software-platform)
- **Schneider Building Activate (ex-Energy Hub):** the Essential tier targets single buildings with "simple monitoring and remote access". Emergency lighting monitoring, alarming and commands is a paid add-on. — [Schneider Building Activate](https://www.se.com/us/en/work/products/product-launch/ecostruxure-building-activate/)
- **Wiser Home:** has separate Schedules, Moments (scenes) and Automations sections. — [Wiser Home user guide](https://www.productinfo.schneider-electric.com/wiser_home_pacific/wiser-system-user-guide-clipsal-home/English/System%20User%20Guide_Wiser_Home_Clipsal%20(bookmap).pdf)
- **C-Bus apps:** C-Bus Manager covers lighting, HVAC, AV, blinds, energy and security for residential users. The older Wiser 2 app supports Wiser MK II and C-Bus Home Controller. A user review says it can't add new schedules, which is unofficial. — [C-Bus Manager privacy notice](https://www.se.com/ww/en/about-us/legal/c-bus-manager-privacy-notice); [Wiser 2 App Store](https://apps.apple.com/gb/app/wiser-2/id804416380)
- **FDD research context:** NREL describes an open cloud FDD workflow (modeling, detection, diagnosis, prioritization, fault model library) aimed at cost-effective FDD for small commercial buildings. LBNL notes it is hard to judge FDD tool capability from vendor marketing. — [NREL](https://nrel.gov/docs/fy19osti/71860.pdf); [LBNL eScholarship](https://escholarship.org/content/qt4j72k57p/qt4j72k57p.pdf)

### Inferences
- Possible FDD rules for Saturn's assets:
  - Compressor: excessive run-time or short-cycling (starts per hour), or running with no downstream demand (air leak).
  - Pump: running with no tank level rise (dry-run or leak), or high starts/hour.
  - Yard lights: on in daylight.
  - Depot: door open after hours with the alarm unarmed.
  - Energy meter: comms loss.
  Each needs severity and duration fields, as in Switch.
- Equipment templates (point list + default rules + dashboard tile) would let Saturn roll out customer farm pumps and compressors quickly.
- Schedules with an AU/QLD public holiday calendar matter for depot lights and alarm arming. This is from general BMS practice; Niagara's holiday calendars weren't sourced here.
- Work-order hand-off could be as simple as "create job" from an alarm into Saturn's job system (or ServiceM8/simPRO if used; unverified), similar to the Switch→Fiix pattern.
- Niagara's Recover/Remote suggest two more small but valued features: PLC/HA config backups per site and secure remote access.

### Gaps
- Buildings IOT: no useful results. Search returned only academic FDD papers, so I can't characterise its features or pricing.
- Niagara schedule/holiday and template (tagging/Haystack) features weren't directly sourced in this session.

## Customer-facing reporting (monthly PDF/email reports, savings reports, what homeowners/small businesses value)

### Takeaway
Scheduled email reports are standard: SMA daily/monthly reports and yield info reports, Enphase yearly production/consumption emails, and Switch scheduled reports. Plus automatic fault emails to the owner. AU homeowners respond most to dollar value (plan optimiser savings, VPP earnings), fault notification (systems silently failing), and network export-limit notices. A monthly site report covering kWh, $, self-sufficiency, faults/actions taken and recommendations would be a differentiator for a contractor.

### Cited Findings
- SMA offers daily and monthly standard reports, preconfigured reports via email, an event report and a yield info report. — [SMA Sunny Portal](https://www.sma-america.com/products/apps-software/sunny-portal); [SMA notification types](https://manuals.sma.de/SPortal/en-US/16916601355.html)
- Enphase's Reports tab generates yearly production/consumption reports and emails them. The feature sheet mentions automatic emails on events affecting performance. — [Enphase newsletter](https://www.enphase.com/installers/news/2023-04); [Enlighten feature sheet](https://www.solarelectricsupply.com/media/sparsh/product_attachment/custom/upload/Enlighten_feature_sheet.pdf)
- Fronius Premium offers a "presentation mode" and PV profitability and yield/cost overviews for installers. — [Fronius installer page](https://www.fronius.com/en/solar-energy/installers-partners/products-solutions/monitoring-digital-tools/pv-system-monitoring-solarweb)
- A SolarQuotes case shows a monitoring failure going unnoticed and costing thousands. — [SolarQuotes "When Solar Monitoring Fails"](https://www.solarquotes.com.au/blog/?p=86442)
- Solar Analytics' plan optimiser is pitched at >$400/yr savings, and its fault alerts go to the customer as well as the installer. — [ARENA](https://arena.gov.au/projects/solar-analytics-monitoring-better-energy-outcomes); [Solar Analytics fleet](https://info.solaranalytics.com/solar-analytics-installers-resellers/installer-getting-started/monitor-your-fleet/)
- Switch provides scheduled reports for different user roles. — [Switch PDF](https://info.switchautomation.com/hubfs/Performance%20Optimization%20Overview_US.pdf)
- Homeowner sentiment: Whirlpool and ProductReview threads show frustration with subscription changes and with lifetime subscriptions being cut. — [Whirlpool](https://forums.whirlpool.net.au/archive/36yzv678); [ProductReview](https://www.productreview.com.au/listings/solar-analytics); [SolarQuotes lifetime subs](https://www.solarquotes.com.au/blog/solar-analytics-lifetime-subscriptions/)

### Inferences
- A monthly PDF/email per site could include:
  - generation, consumption, import/export and self-sufficiency %
  - estimated bill on the customer's QCA tariff vs. last month and last year
  - peak demand and when it occurred
  - faults detected and what Saturn did
  - equipment runtime (compressor/pump hours, useful for maintenance)
  - one or two recommendations (e.g. "shift pump to 10am-2pm to use $X more solar")
- An annual "system health" report doubles as a lead-generation tool: service call, battery upsell.
- Subscription backlash means pricing should be transparent and avoid "lifetime" promises tied to hardware that will age out (3G/4G).

### Gaps
- No survey data found quantifying which report items homeowners value. The above is inferred from product focus and forum sentiment.

## Pricing (what installers/customers pay, AUD where possible)

### Takeaway
Solar/residential monitoring in AU runs about A$60-100/yr per device (Solar Analytics), or is free with OEM hardware (Fronius Basic, SMA Smart Connected, Wattwatchers Fleet Manager bundled). Commercial BMS/analytics (Switch, Niagara Cloud, Schneider Building Activate, Buildings IOT) is quote-only, priced per instance/device/data volume. That points Saturn to roughly A$5-10/month per residential site, with higher per-site fees for commercial monitoring plus service.

### Cited Findings
- **Solar Analytics** (AUD incl. GST, per device): search extracts of the official page show $5.99/month or $59.99/year, and separately $10/month or $95/year. I could not confirm which applies to which device class, possibly 4G vs. OEM-integrated. A 2023 brochure listed $59.99/yr for small sites (<15kW) and $99.99/yr for large sites (>15kW). A 2017 rate was $6/month or $60/yr. Payment can be monthly, yearly or 5-yearly. It is free with CATCH Control, and a two-for-one offer exists for multi-device sites. — [Solar Analytics subscription prices](https://www.solaranalytics.com.au/subscription-prices); [Solar Analytics pricing](https://www.solaranalytics.com.au/pricing); [Solar Analytics KB](https://info.solaranalytics.com/solar-system-owners/subscription/solar-analytics-subscription-price-classic-or-integrated/); [2023 brochure via SolarLab](https://solarlab.net.au/wp-content/uploads/2023/10/Solar-Analytics-brochure.pdf); [2017 reseller note](https://www.solaranalytics.com.au/community-news/solar-analytics-announcements-for-resellers-july-2017). The figures conflict, so flag them as uncertain.
- 3G-era lifetime subscriptions were converted to a free five-year subscription when 3G closed (2023-24). — [SolarQuotes](https://www.solarquotes.com.au/blog/solar-analytics-lifetime-subscriptions/)
- **Wattwatchers:** Fleet Manager and the Onboarding app are included with hardware. Device subscriptions are pre-paid yearly by direct debit and start on delivery, so stock in inventory also incurs charges. No amount was found. — [Wattwatchers subscription info](https://service.wattwatchers.com.au/quick-answers/subscriptions); [toolkit](https://service.wattwatchers.com.au/getting-started/toolkit)
- **Fronius Solar.web:** Basic is free. Premium is sold for 3 months, 1 year or 3 years, with the price shown only at checkout. No AUD figure found. — [Fronius FAQ](https://www.fronius.com/faq-online-tools/faq-solar-energy/solarweb)
- **SMA:** Smart Connected is free. The monitoring API is an optional extra, with no price found. — [SMA Smart Connected](https://www.sma.de/en/products/monitoring-control/sma-smart-connected.html); [SMA manual](https://manuals.sma.de/SPortal/en-US/16908584971.html)
- **Reposit Power:** hardware costs A$700-1,800 installed (older guide) or about A$600-800 (2026 review), and the estimates conflict. Earnings are paid at A$1/kWh GridCredits. — [Solar Choice](https://www.solarchoice.net.au/?p=20288); [TechGuide](https://www.techguide.com.au/?p=105416)
- **Tridium Niagara Cloud Suite:** tiered by data stored and queries, subscribed per Niagara instance, and requires an active Software Maintenance Agreement (SMA). Overages are charged per GB/month. Prices are quote-only via distributors (e.g. Innon Silver tier). — [Tridium NDS](https://www.tridium.com/us/en/Products/niagara-cloud-suite/niagara-data-service); [Cochrane Supply](https://www.cochranesupply.com/news/niagara-cloud-suite-by-tridium); [Innon](https://innon.com/products/ncs-nds-silver-o)
- **Schneider Building Activate:** price depends on feature plan, number of IoT devices (device credits) and term (1 or 3 years), regardless of the number of sites. Contact sales. — [Schneider](https://www.se.com/us/en/work/software/ecostruxure-building/small-buildings-energy-management)
- **Switch Automation:** no published price. Enterprise quote only. — [TrustRadius](https://www.trustradius.com/products/switch/pricing); [SourceForge](https://sourceforge.net/software/product/Switch-Automation)
- **Emporia Vue 3:** about C$199.99 (16 sensors) and C$229.99 (8-sensor listing), Canadian retail. No AUD price or AU compliance info found. — [Emporia shop](https://shop.emporiaenergy.com/products/emporia-vue-energy-monitor-3rd-generation-with-16-sensors); [Staples.ca](https://www.staples.ca/products/3164596-en-emporia-vue-3-home-energy-monitor-with-8-50a-circuit-level-sensors)
- A CHOICE article cites a home energy management subscription of A$6/month or A$72/year. The product is not identified. — [CHOICE](https://www.choice.com.au/home-improvement/energy-saving/reducing-your-carbon-footprint/articles/home-energy-management-systems)

### Inferences
- The market anchors around A$60-100 per site per year for solar monitoring and alerts. Saturn could bundle monitoring into a service agreement, for example A$10-20/month residential, which includes monthly reports and fault response. Farm and light-commercial sites with PLC control and FDD would justify more. These figures are my suggestion, not sourced.
- Because OEM portals are free, Saturn's paid value has to come from cross-vendor aggregation (SMA + C-Bus + PLC + meters in one view), control/automation, tariff/cost insight, and the contractor's human response.

### Gaps
- No AUD pricing for Wattwatchers subscriptions or hardware, Fronius Premium, Enphase, Sense or Emporia in AU, Switch, Niagara Cloud, Schneider Building Activate, or Buildings IOT.
- Sense: no AU-relevant results found. I believe it is not officially sold in AU, but this is unverified.
- Wiser/C-Bus app subscription costs: none found. The apps appear free with hardware, but this is unconfirmed.
