# Security, reliability and Australian compliance features for Saturn Command Center (small-site OT remote monitoring and control)

Research date: 24 Nov 2026. Method: web search only. Direct page fetches of cyber.gov.au and cisa.gov failed because this session's network blocks those hosts, so ACSC/CISA content below comes from search-result extracts of those pages, not full reads. Items marked "(unverified)" come from general knowledge and were not confirmed by a source in this session.

## 1. Authentication and access control (MFA, SSO, PIN risk, sessions, re-auth, comparable platforms)

### Takeaway
Every comparable OT remote-access platform offers MFA as a minimum: TOTP for IXON, SMS 2FA for Talk2M Pro/Light, a physical key for Tosibox, and Google 2FA plus SAML SSO for Niagara. Ignition hands MFA off to an external IdP. Australian guidance has moved toward phishing-resistant MFA (passkeys/FIDO2). A PIN-only login with a 30-day session falls well short of these norms for a system that can force PLC outputs. The highest-priority gap is MFA, ideally passkeys/WebAuthn with TOTP as a fallback, plus a much shorter idle timeout and step-up re-authentication before forcing outputs.

### Cited Findings
- The ASD Essential Eight maturity model update (27 Nov 2023) "bolstered" MFA requirements so that phishing-resistant MFA is needed at a lower maturity level, affecting Maturity Levels Two and Three — [ASD Essential Eight maturity model changes](https://www.cyber.gov.au/resources-business-and-government/essential-cybersecurity/essential-eight/essential-eight-maturity-model-changes)
- For customer-facing online services, the Nov 2023 model offers phishing-resistant MFA to customers as an option at lower maturity levels and requires it at higher levels — [ASD (via search summary)](https://www.cyber.gov.au/resources-business-and-government/essential-cyber-security/essential-eight/essential-eight-maturity-model-changes); [ITBrief](https://itbrief.com.au/story/asd-updates-essential-eight-strengthens-phishing-resistant-mfa-requirements)
- Maturity Level One previously didn't specify which kinds of authentication factor were acceptable, which allowed weaker forms. The Nov 2023 update tightened this. Examples of phishing-resistant MFA: security keys, smart cards, passkeys/FIDO2 — [OneSpan](https://www.onespan.com/blog/essential-eight-authentication-measures); [Corbado](https://corbado.com/blog/essential-eight-passkeys-mfa)
- One secondary source says the Nov 2023 model was still the version in force in 2026, and that ASD consulted on an "Essentials" series in June–July 2026 without yet releasing a replacement (unconfirmed against ASD) — [iTWire](https://itwire.com/business-it-news/security/essential-eight-maturity-model-what-applies-now-and-what-asd-is-changing)
- The ACSC small business guidance lists turning on MFA as a top-three starting measure, "starting with your most important accounts" — [ACSC Small business cyber security guide, Jan 2025](https://cyber.gov.au/sites/default/files/2025-01/ACSC_Small_business_cyber_security_guide_January_2025.pdf); [ACSC small business page](https://www.cyber.gov.au/learn-basics/explore-basics/small-business)
- CISA's OT primary mitigations (May 2025) say that where remote access to OT is essential, it should go over a private connection/VPN with strong passwords and **phishing-resistant MFA**, and be least-privilege per asset and per user role — [CISA Primary Mitigations fact sheet](https://www.cisa.gov/sites/default/files/2025-05/fact-sheet-primary-mitigations-to-reduce-cyber-threats-to-operational-technology-508c.pdf)
- NIST SP 800-63B-4 (final, July 2025), AAL2: overall re-authentication timeout SHOULD be no more than 24 hours, and inactivity timeout SHOULD be no more than 1 hour. AAL3: 12 h overall and 15 min inactivity (the AAL3 figures come from a secondary summary). The older 800-63B-3 AAL2 figures were 12 h overall and 30 min idle — [NIST SP 800-63B-4 session/AAL](https://pages.nist.gov/800-63-4/sp800-63b/aal); [Okta whitepaper (older rev)](https://okta.com/sites/default/files/2024-08/Okta_and_NIST_800-63B.pdf); [MojoAuth summary](https://mojoauth.com/glossary/user-authentication-lifecycle.md)
- **Ignition (Inductive Automation):** 2FA comes through external IdPs (OpenID Connect/SAML) rather than natively. IdP "User Inactivity Timeout" defaults to 10 minutes. The Gateway web UI has its own inactivity timeout, and sessions never time out if it is set to 0 or less. Perspective has "Security Levels", a hierarchical model for authorisation. A Security Hardening Guide (8.1, also applicable to 8.3) exists and is being revised — [Ignition IdP docs](https://docs.inductiveautomation.com/docs/8.1/platform/security/identity-provider-authentication-strategy); [Gateway general security settings](https://www.docs.inductiveautomation.com/docs/8.1/platform/security/gateway-general-security-settings); [IA forum on MFA](https://forum.inductiveautomation.com/t/most-secure-authentication/90742.md); [Hardening guide](https://inductiveautomation.com:443/resources/article/ignition-security-hardening-guide); [Hardening guide update thread](https://forum.inductiveautomation.com/t/security-hardening-guide-2027-update-and-feedback-request/117605)
- **Ewon Talk2M (HMS):** per-user 2FA (password plus SMS code) on Talk2M Light/Pro, with an admin default for new users. "Remember this PC" is controlled by account security policy. Password policy, user groups and device pools restrict which users can reach which devices. Connection audit trail. ISO 27001 scope covers Talk2M (docs are older, 2016–2024) — [eCatcher Users help](https://help.ewon.biz/ecatcher/help/en/users.html); [Talk2M Pro security features AUG-0057](https://foxon.s63.cdn-upgates.com/r/r68c16b3db3e0a-aug-0057-00-en-ecatcher-security-features-with-a-talk2m-pro-account.pdf); [Why upgrade from Talk2M Free](https://becolve.com/wp-content/uploads/2024/10/ssh-0085-00-why-to-upgrade-from-talk2m-free-to-talk2m-light-or-pro.pdf); [eWON security paper 2017](https://www.quantumautomation.com/uploads/7/3/8/8/7388264/ewon_security-paper_08-2017.pdf)
- **IXON Cloud:** TOTP 2FA can be enabled per user or mandated company-wide. Device-level and company-wide audit trails. Centralised server logs analysed against security rules. IEC 62443-4-1 certified development process. SecureEdge Pro is claimed IEC 62443-4-2 certified (resellers word the scope differently, so confirm) — [IXON Technical & Organisational Measures](https://www.ixon.cloud/technical-and-organizational-measures); [IXON 62443-4-2](https://www.ixon.cloud/knowledge-hub/iec-62443-4-2-certification); [IXON 62443-4-1](https://www.ixon.cloud/knowledge-hub/iec-62443-4-1-certification); [IXON Security](https://ixon.cloud/security)
- **Tosibox:** two-factor by design. The physical Key or Mobile Client device is "something you have" and the password is "something you know". Lock/Key pair by physical certificate exchange. Access rights are device-specific and non-transferable. Vendor remote support is off by default — [Tosibox security](https://www.tosibox.com/security); [LAPP: What makes Tosibox so secure](https://t3.lappcdn.com/fileadmin/DAM/Miltronic_Denmark/4_servicecenter/LAPP_What-makes-Tosibox-so-secure.pdf)
- **Niagara 4 (Tridium):** Google 2-factor auth, client-certificate auth, SAML SSO (from 4.9), TLS 1.2/1.3, AES-256 at rest. Security requirements "derived from ISA 62443 SL4" (a vendor design claim, not a certification) — [Tridium IT cyber brochure](https://www.tridium.com/content/dam/tridium/en/documents/document-lists/2022-0004-IT-Cyber-Brochure.pdf)
- IEC 60204-1 (adopted as AS/NZS 4024.1204) allows a locked mode selector to be replaced by "another selection method which restricts the use of certain functions... to certain categories of operator", e.g. access codes. Access control on functions is therefore an accepted safety-related control pattern — [IEC 60204-1:2018 text](https://eclass.uniwa.gr/modules/document/file.php/IDPE391/%CE%A0%CE%A1%CE%9F%CE%A4%CE%A5%CE%A0%CE%91/EN%2060204-1-2018.pdf)

### Inferences
- **PIN-only risk:** a 6-digit PIN has 10^6 combinations. Without strict lockout or rate limiting, online brute force is feasible. With a 30-day signed cookie, one stolen cookie or shoulder-surfed PIN gives a month of remote control over pumps and doors. No comparable platform relies on a short PIN alone for remote control. Keeping PIN as a "quick unlock" is reasonable only *after* a strong login (passkey or password plus TOTP) on a trusted device, similar to a phone lock screen.
- **Practical priority for 1–2 devs (Node/Express):**
  1. **P0:** WebAuthn/passkeys (e.g. `@simplewebauthn/server`) plus TOTP fallback (`otplib`) with recovery codes. Mandatory for super admin and anyone with "force output" rights.
  2. **P0:** per-account and per-IP rate limiting and progressive lockout on PIN/password (e.g. 5 failures leads to backoff, 10 leads to lockout plus email alert). Store PINs with a slow hash.
  3. **P0:** session changes. Absolute lifetime of about 12–24 h for control-capable sessions (NIST AAL2 ≤24 h). Idle timeout of about 15–30 min that locks the UI and requires the PIN (or a passkey for writes). A view-only "kiosk/wallboard" mode can keep longer sessions.
  4. **P0:** server-side session store (Postgres table) so sessions can be listed and revoked ("log out all devices"). A signed cookie alone can't be revoked before it expires.
  5. **P1:** step-up re-auth (passkey/TOTP within the last ~5 min) before forcing outputs, changing permissions, adding HA tokens or deploying PLC programs.
  6. **P1:** new-device/new-location login email alerts and a "sessions & devices" page.
  7. **P1:** per-user scoped API tokens (hashed at rest, expiry, read-only vs write scopes) for integrations, instead of sharing accounts.
  8. **P2:** SSO (Google/Microsoft OIDC) for business customers. IP allow-lists per customer/org as an option, since farm and residential users on mobile IPs won't use them.
- Cookie hygiene: `HttpOnly`, `Secure`, `SameSite=Lax/Strict`, CSRF tokens on state-changing endpoints, session ID rotation on login and privilege change.

### Gaps
- Couldn't fetch cyber.gov.au to quote the exact ML1/ML2 MFA wording for "online customer services".
- No current source on whether IXON or Talk2M offer SAML/OIDC SSO. Talk2M documents are dated.
- Couldn't verify Ignition 8.3 native MFA features (2025+).

## 2. Remote access architecture for OT (outbound-only, VPN, zero trust, IEC 62443, Modbus TCP risk, internet-exposed HMIs)

### Takeaway
The government and vendor consensus is that OT should never be directly exposed to the internet. Remote access should be brokered, monitored and controlled: devices make outbound connections to a broker, with MFA and least privilege, and the vendor can't see traffic where possible. Modbus TCP has no authentication or encryption, so it must stay confined to an isolated local zone. Saturn's existing model (cloud app → HA API over UniFi site-to-site VPN) is broadly aligned. The gaps are: inbound exposure of HA, broad HA long-lived tokens, and no segmentation between HA/IT and the Modbus I/O LAN.

### Cited Findings
- *Principles of Operational Technology Cybersecurity* (1 Oct 2024) was authored by ASD's ACSC with CISA, FBI, NSA and international partners. It sets six principles, including "protect OT networks and information by separating networks, controlling connections, and limiting access" between OT and lower-trust networks — [ASD/ACSC](https://www.cyber.gov.au/business-government/secure-design/operational-technology-environments/principles-operational-technology-cyber-security); [CISA alert](https://cisa.gov/news-events/alerts/2024/10/01/asds-acsc-cisa-fbi-nsa-and-international-partners-release-guidance-principles-ot-cybersecurity); [PDF](https://media.defense.gov/2024/Oct/01/2003556960/-1/-1/0/PRINCIPLES_OF_OPERATIONAL_TECHNOLOGY_CYBER_SECURITY.PDF)
- *Secure connectivity principles for operational technology* was led by UK NCSC with ASD's ACSC and others. ASD page first published 15 Jan 2026. Key message: where access to OT systems is necessary, "ensure the access is mediated, monitored and controlled" — [ASD/ACSC](https://www.cyber.gov.au/business-government/secure-design/operational-technology-environments/secure-connectivity-principles-for-operational-technology); [NCSC NZ joint guidance](https://www.ncsc.govt.nz/protect-your-organisation/cyber-security-for-operational-technology/)
- CISA Primary Mitigations (May 2025): remove OT connections from the public internet; if remote access is essential, use private IP/VPN plus strong passwords plus phishing-resistant MFA; least privilege per asset and role. The fact sheet also covers changing default passwords, segmenting networks and being able to operate manually (from the search summary; full text not read) — [CISA fact sheet](https://www.cisa.gov/sites/default/files/2025-05/fact-sheet-primary-mitigations-to-reduce-cyber-threats-to-operational-technology-508c.pdf); [CISA page](https://www.cisa.gov/resources-tools/resources/primary-mitigations-reduce-cyber-threats-operational-technology)
- **Modbus TCP:** no encryption, authentication or integrity checks. Anyone with network access can issue control commands using publicly documented function codes. The practical mitigation is network isolation rather than retrofitting encryption. Modbus/TCP Security (TLS on port 802 with X.509 mutual auth) exists but device support is limited — [Trout Software](https://www.trout.software/blog/how-to-secure-modbus-tcp-best-practices-for-modern-ics-networks); [Opto 22](https://www.opto22.com/articles/segmented-networking-protects-your-legacy-plcs)
- **IEC 62443 basics:** a *zone* is a grouping of assets with common security requirements. A *conduit* is the grouping of communication channels between zones. Each zone gets a target Security Level (SL 0–4) from a risk assessment (62443-3-2), and its conduits must enforce it. 62443-3-3 lists system security requirements per SL — [Cisco 62443-3-3 whitepaper](https://cisco.com/c/en/us/products/collateral/security/isaiec-62443-3-3-wp.html); [Trout Software](https://www.trout.software/resources/tech-blog/how-to-secure-modbus-tcp-best-practices-for-modern-ics-networks)
- **Vendor architectures:** Talk2M devices authenticate *to* the Talk2M cloud over TLS/HTTPS (outbound), with configurable firewall levels on the gateway — [Talk2M white paper](https://www.klinkmann.com/hubfs/e-Catalogue/HMS%20Networks/Talk2M_Secure_Cloud_Service.pdf?hsLang=en). Tosibox builds direct end-to-end VPN between its devices, uses a "MatchMaker" broker only for connection setup, and says the vendor can't decrypt traffic (AES-CBC, RSA 2048–4096 PKI) — [Tosibox security](https://www.tosibox.com/security). IXON SecureEdge has an integrated firewall isolating the machine network from the internet — [IXON](https://ixon.cloud/security)

### Inferences
- **Suggested zone model for a Saturn site (SL1–SL2 target is realistic for small sites):**
  - Zone A, *Control LAN* (OpenPLC Pi ↔ TM3 Modbus TCP): a dedicated VLAN/physical switch with no route to the internet. Only the PLC and the HA/edge agent can reach port 502. UniFi firewall rule: deny everything else.
  - Zone B, *Site edge* (Home Assistant, Saturn edge agent): can talk to Zone A on specific ports only, and outbound HTTPS/VPN only to the cloud.
  - Zone C, *Site IT/guest* (phones, cameras, customer Wi-Fi): no access to A, and to B only by HA UI if needed.
  - Conduit to cloud: site-to-site VPN or, better for customer sites, an **outbound-only agent connection** (WebSocket/MQTT over TLS from the edge to the Command Center). This removes inbound port-forwards and doesn't depend on every customer having a UniFi gateway. It mirrors the Talk2M/IXON model.
- **HA long-lived tokens are full-admin credentials.** Mitigations: a dedicated HA user per site with non-admin rights where possible, encryption with a key outside the DB (env var/KMS), quarterly rotation, and logging token use. Better still, have the edge agent expose a narrow, allow-listed command API (e.g. `force(io_id, value, ttl)`) instead of the cloud holding a general HA token.
- **Never expose HA, OpenPLC web UI (default port 8080) or Modbus 502 to the internet.** Add a periodic external port scan of each site's public IP to the health checks.
- Change OpenPLC default credentials, and disable OpenPLC's web upload in production if the GitHub deploy agent is the only route for programs.
- **PLC auto-deploy from GitHub** is a supply-chain risk. Use signed commits/tags, a protected branch, and staged deploy with local confirmation or a scheduled window. Keep the previous program for one-click rollback. The edge agent should pull (outbound) rather than accept pushes.

### Gaps
- Couldn't read the full text of the 2024 OT Principles or the 2026 Secure Connectivity Principles (host blocked). The exact principle list and remote-access clauses need to be checked on cyber.gov.au.
- No source on the security posture of OpenPLC or Home Assistant specifically. Also no ACSC advisory on internet-exposed HMIs found in this session (CISA has published several, but they weren't retrieved).

## 3. Australian legal and regulatory requirements

### Takeaway
For a small Queensland SaaS the binding obligations are mainly: the Australian Consumer Law (unfair contract terms now carry penalties), possibly the Privacy Act (only if turnover is above $3M or an exception applies, though good practice and customer expectations apply anyway), the new statutory privacy tort (no small-business exemption), and the Electrical Safety Act/WHS duties that apply when equipment is controlled remotely. SOCI almost certainly doesn't apply. Hosting in Australia isn't legally required for private-sector data, but Railway has no Australian region (Singapore is the closest), so residency becomes a sales and latency question rather than a compliance one.

### Cited Findings
**Essential Eight**
- The ACSC recommends that small businesses aim for Essential Eight Maturity Level One once they have done the basics in the Small Business Cyber Security Guide — [ACSC Small business cyber security guide Jan 2025](https://cyber.gov.au/sites/default/files/2025-01/ACSC_Small_business_cyber_security_guide_January_2025.pdf)
- The ACSC also publishes small business cloud security guides (from Dec 2022) — [ACSC cloud guides](https://www.cyber.gov.au/resources-business-and-government/essential-cybersecurity/small-business-cybersecurity/small-business-cloud-security-guides)

**Privacy Act 1988 / APPs / NDB**
- The small business exemption (annual turnover ≤ $3M) was **retained** by the Privacy and Other Legislation Amendment Act 2024. Removing it was not included in Tranche 1 — [APH Bills Digest](https://www.aph.gov.au/Parliamentary_Business/Bills_Legislation/bd/bd2425/25bd016)
- The Government in Sept 2023 "agreed in principle" to removing the small business exemption (future risk) — [IDCARE](https://www.idcare.org/learning-centre/fact-sheets/what-to-do-if-your-business-has-been-breached)
- The exemption doesn't apply to certain business types listed in s 6D(4)–(9), which were seen as higher privacy risk — [OAIC submission, exemptions](https://www.oaic.gov.au/engage-with-us/submissions/privacy-act-review-issues-paper-submission/part-4-exemptions). (Unverified detail: these include health service providers, businesses that trade in personal information, contracted service providers to the Commonwealth, and businesses that opt in.)
- NDB scheme applicability follows Privacy Act coverage (>$3M turnover businesses plus the exceptions) — [Sprintlaw](https://sprintlaw.com.au/articles/data-breach-notification-obligations-for-australian-businesses/); [OAIC NDB Part 4](https://www.oaic.gov.au/privacy/privacy-guidance-for-organisations-and-government-agencies/preventing-preparing-for-and-responding-to-data-breaches/data-breach-preparation-and-response/part-4-notifiable-data-breach-ndb-scheme)
- **Statutory tort for serious invasions of privacy** (Royal Assent 10 Dec 2024, in force 10 June 2025) has **no small-business exemption**. It is standalone from the APPs — [Coleman Greig](https://colemangreig.com.au/insights/blogs/concerns-for-business-under-the-privacy-and-other-legislation-amendment-act-2024); [Digital Rights Watch](https://digitalrightswatch.org.au/2025/06/09/2025-privacy-reform-explainer/); [Mondaq](https://www.mondaq.com/australia/privacy-protection/1588538/serious-invasion-of-privacy-and-small-business)
- **Cross-border/data residency:** APP 8 doesn't mandate Australian hosting. It is an accountability regime. Pure overseas cloud *storage*, where the business keeps effective control, is generally treated as a "use", not a "disclosure", so it falls outside APP 8. APP 6 and APP 11 (security) still apply. OAIC APP Guidelines Ch 8 v1.3 (Oct 2025) is the authoritative text — [OAIC APP 8 guidelines v1.3](https://www.oaic.gov.au/__data/assets/pdf_file/0036/256959/APP-Guidelines-Chapter-8-Cross-border-disclosure-of-personal-information-October-2025-v1.3.PDF); [Baker McKenzie](https://resourcehub.bakermckenzie.com/en/resources/cloud-compliance-center/apac/australia/topics/overseas-hosting)

**Railway regions (data residency in practice)**
- Railway's only Asia-Pacific region is Southeast Asia (Singapore), `asia-southeast1-eqsg3a`. The default region is US West (Oregon). Staff said Sydney is not supported, users report poor AU latency, and a Melbourne CDN PoP exists but isn't a compute region — [Railway regions docs](https://docs.railway.com/reference/deployment-regions); [Railway Station: Sydney](https://station.railway.com/questions/need-be-service-to-be-in-sydney-austral-429ed381); [Railway Station: Australia region](https://station.railway.com/feedback/australia-region-d336e8b1)
- Railway volume backups are stored in the service's deploy region — [Railway volume backups](https://docs.railway.com/volumes/backups)

**SOCI Act 2018**
- SOCI covers 11 sectors and 22 asset classes, each with specific definitions and a defined "responsible entity" (s 12L), e.g. the licensed water utility for a critical water asset — [CISC SOCI Act](https://www.cisc.gov.au/legislation-regulation-and-compliance/soci-act-2018); [CISC register guidance](https://www.cisc.gov.au/resources-subsite/Documents/register-critical-infrastructure-assets.pdf); [HSF](https://www.hsfkramer.com/insights/2023-03/demystifying-australias-recent-security-of-critical-infrastructure-act-reforms)

**Queensland electrical safety / WHS**
- The Electrical Safety Act 2002 (Qld) imposes a primary duty of care (s 30) and a duty on the "person in control of electrical equipment" (s 38). Where both the WHS Act 2011 and the ESA apply, the ESA takes priority — [Electrical Safety Act 2002 (Qld)](https://www.legislation.qld.gov.au/view/whole/html/current/act-2002-042); [WorkSafe Qld electrical safety laws](https://worksafe.qld.gov.au/laws-and-compliance/electrical-safety-laws)
- Machinery electrical equipment: AS/NZS 4024.1204:2019 = IEC 60204-1:2016. Prevention of unexpected start-up: AS/NZS 4024.1603:2019 — [Intertek Inform 4024.1204](https://www.intertekinform.com/en-au/standards/as-nzs-4024-1204-2019-1158514_saig_as_as_2752148/); [Intertek Inform 4024.1603](https://www.intertekinform.com/en-us/standards/as-nzs-4024-1603-2019-1158104_saig_as_as_2751128/)

**Consumer law for subscriptions**
- Since Nov 2023, unfair terms in standard-form consumer and small-business contracts can attract civil penalties, not just be declared void. The ACCC has prioritised cancellation, auto-renewal and early-exit clauses. Its first court case under the new regime is against Amazon (Prime subscription variation terms). Earlier cases (JJ Richards) involved automatic renewal terms. The $50M maximum per term figure is from a secondary source (the actual maximum is the greater of $50M, 3× benefit, or 30% of turnover; unverified) — [ACCC: new penalties](https://accc.gov.au/media-release/accc-welcomes-new-penalties-and-expansion-of-the-unfair-contract-terms-laws); [ACCC: types of terms that may be unfair](https://www.accc.gov.au/business/selling-products-and-services/small-business-toolkit/unfair-contract-terms/types-of-terms-that-may-be-unfair); [Corrs](https://corrs.com.au/insights/b2b-focus-accc-tests-small-business-protections-in-new-unfair-contracts-regime)

### Inferences
- **Essential Eight for a small SaaS:** the eight controls are designed for Windows enterprise endpoints, so many map loosely. The relevant ones for Saturn are:
  - (1) **MFA**: staff accounts on GitHub, Railway, Anthropic console, domain registrar, email, UniFi cloud, plus the app itself.
  - (2) **Restrict admin privileges**: separate super admin accounts, least-privilege per site.
  - (3) **Patch applications**: npm dependencies (Dependabot), HA, OpenPLC, UniFi firmware, Pi OS.
  - (4) **Regular backups**: DB plus PLC programs plus HA configs, with restore tests.
  - (5) **Patch OS**: Raspberry Pi OS unattended-upgrades.
  - Application control, Office macros and user application hardening apply mainly to the staff laptops.
  - Claiming "aligned to Essential Eight ML1 for our own operations" is a credible marketing statement once these are done.
- **Privacy:** Saturn is likely under the $3M threshold now, so it is probably exempt from the APPs and the NDB scheme. But:
  - (a) B2B customers above $3M may contractually require APP-equivalent handling.
  - (b) The statutory tort applies regardless. Door/presence logs and camera data from residential customers are sensitive.
  - (c) The exemption may be removed in Tranche 2.
  - Recommended: publish a privacy policy, follow APP 11 security and NDB-style breach response voluntarily (assess within 30 days, notify affected users; unverified detail of the s 26WH timeframe), and consider the voluntary opt-in to the Privacy Act (s 6EA, unverified) as a trust signal.
- **AI Q&A via the Claude API** sends site data (and possibly personal info) to a US provider. This is likely an APP 8 "disclosure" if the provider processes it, not just storage. Disclose it in the privacy policy, minimise PII in prompts, and check the Anthropic commercial terms on data retention and training.
- **Residency:** not legally required. Singapore is the best Railway option for latency (it is likely far better than US West). If Australian-hosted data becomes a customer requirement (councils, agriculture co-ops, government-adjacent work), that would mean moving the DB to an AU-region provider (e.g. AWS/GCP/Azure Sydney, or AU-hosted managed Postgres) while keeping the app on Railway, at the cost of added latency.
- **SOCI:** residential, farm and light-commercial sites are very unlikely to meet the asset-class definitions or thresholds. Watch out for future customers that are themselves water utilities or energy operators, where Saturn could become a supplier within their risk management programme (CIRMP).
- **Electrical safety / WHS:** Saturn, as designer/installer and remote operator, plausibly carries "person in control" and PCBU duties for remotely started equipment (inference; not confirmed in a source). Remote control features must be designed so a person working on equipment can't be endangered by a remote start. This means isolation (AS/NZS 4836 safe work on LV installations, with lockout/tagging) must defeat remote commands *in hardware*, not just in software. Get advice from Electrical Safety Office Qld or a consultant for customer contracts.
- **Consumer law:** subscription terms should avoid unilateral variation without notice and an exit right, avoid auto-renewal without clear notice, avoid excessive early-exit fees, and avoid blanket liability exclusions. Consumer guarantees (services rendered with due care and skill) can't be excluded for consumers.

### Gaps
- Couldn't retrieve the Electrical Safety Regulation 2013 (Qld) text or ESO guidance on remote or automatic start of equipment. I found no Qld-specific rule on remote-start warnings.
- AS/NZS 3000 and AS/NZS 4836 clause-level content was not retrieved (paywalled standards).
- Couldn't confirm whether Railway has launched an Australian region since the forum posts (check the region picker).
- Details of the POLA Act 2024 (new penalty tiers, APP 11 "technical and organisational measures" clarification, automated decision-making transparency from Dec 2026) were not verified in this session.

## 4. Reliability (edge health, store-and-forward, failover, backups, DR, status page, SLAs)

### Takeaway
Comparable platforms treat the edge device as a managed asset: heartbeat/online status, connectivity, firmware and config version, and alerts when offline. On the cloud side, Railway gives scheduled volume backups and PITR, but its own guidance says to add independent logical dumps for production. For Saturn, the cheapest high-value items are: an edge agent heartbeat with Pi metrics, offline alerts, daily off-platform `pg_dump`, PLC/HA config in git with rollback, and a simple public status page.

### Cited Findings
- **Railway volume backups:** Daily schedule (every 24 h, kept 6 days), Weekly (every 7 days, kept 1 month), Monthly (every 30 days, kept 3 months), and any combination. Manual backups are limited to 50% of volume size. Backups are deleted at expiry unless locked, and **wiping or deleting the volume deletes all its backups**. Backups are stored in the service's region — [Railway volume backups](https://docs.railway.com/volumes/backups)
- **Railway PITR:** archives every WAL segment to a private Railway bucket via pgBackRest, with weekly full and daily incremental base backups. Railway's guide recommends using volume backups, PITR **and** logical `pg_dump` dumps for production ("the layer you control end to end, and the one you can test cheaply") — [Railway PITR](https://docs.railway.com/volumes/point-in-time-recovery); [Railway Postgres backups guide](https://docs.railway.com/guides/postgres-backups-restores)
- A user post claims a PITR window of about 4 weeks (unofficial) — [Railway Station](https://station.railway.com/questions/backup-retention-pitr-window-and-two-c-4cf10292)
- The ACSC small business guidance says backups need a written plan covering what, how often, where and restore testing, and warns that without reliable backups recovery "may be difficult or impossible" — [ACSC small business handbook](https://www.cyber.gov.au/publication/small-business-cyber-security-handbook/protecting-devices-systems-and-networks); [ACSC guide Jan 2025](https://cyber.gov.au/sites/default/files/2025-01/ACSC_Small_business_cyber_security_guide_January_2025.pdf)
- CISA OT mitigations include keeping the ability to operate manually if remote systems fail (from the search summary) — [CISA Primary Mitigations](https://www.cisa.gov/resources-tools/resources/primary-mitigations-reduce-cyber-threats-operational-technology)

### Inferences
- **Edge health (Saturn edge agent on the Pi, outbound):**
  - heartbeat every 30–60 s
  - CPU, RAM, disk %, SD/eMMC wear (if available), SoC temperature, uptime
  - OpenPLC runtime state (running/stopped, scan time), Modbus poll success rate per TM3 module
  - HA reachability and version
  - WAN IP, latency, VPN tunnel up/down, active WAN (fibre/NBN vs 4G)
  - UPS state via NUT (on battery, % charge, runtime)
  - Cloud alert when heartbeat is missed for more than 2–3 intervals, with escalation by email/SMS/push.
- **Watchdogs:** enable the Pi hardware watchdog (systemd `RuntimeWatchdogSec`), use `systemd Restart=always` for the agent and OpenPLC, and use read-only root/overlayfs or industrial eMMC to avoid SD corruption. The PLC program must keep safe local control with no cloud (cloud is supervisory only).
- **Store-and-forward:** buffer telemetry and events locally (SQLite) during WAN loss and backfill on reconnect with original timestamps. Queue *no* control commands while offline: commands should carry a short expiry (TTL) so stale "start pump" commands never run late.
- **4G failover:** UniFi gateways support WAN failover with a UniFi LTE backup or a third-party 4G router. Report the active WAN in the heartbeat, and alert on extended failover because of data costs.
- **Backups:**
  - Enable Railway daily plus weekly backups and PITR.
  - Add a nightly `pg_dump` to an independent store (e.g. S3/Backblaze/R2 in an AU or Singapore region, encrypted, with versioning or object lock for ransomware/accidental-delete resistance).
  - Test a restore quarterly.
  - Back up the HA config (HA built-in backups pushed off-site).
  - Keep PLC programs in GitHub with tags per deployed version, and have the edge agent report the running program hash so drift is detectable.
- **DR targets for a small SaaS:** RPO ≤ 24 h (≤ 5 min with PITR) and RTO a few hours. Document a runbook covering Railway outage, accidental delete and compromised credentials. Keep secrets (HA token encryption key) backed up separately; without the key, encrypted tokens can't be restored.
- **Status page and SLA:** use a hosted status page (e.g. Better Stack/Instatus/UptimeRobot) with external uptime checks on the app and API. 99.5–99.9% monthly is a typical small-SaaS target. Offer service credits rather than damages, and state clearly that cloud control isn't a safety function and that local control continues during outages.

### Gaps
- No vendor-sourced specifics were found for Talk2M/IXON device health metrics or SLAs. IXON and HMS uptime commitments were not retrieved.
- No industry benchmark source found for SLA norms in small OT SaaS. The 99.5–99.9% figure is a general SaaS convention, not a sourced fact.

## 5. Safety for remote control (interlocks, confirmations, warnings, local/remote selection, rate limiting)

### Takeaway
Machinery safety standards (IEC 60204-1 / AS/NZS 4024.1204, AS/NZS 4024.1603 unexpected start-up) expect a risk-assessed consideration of audible/visual warnings before hazardous starts, a lockable mode selector (or access-code equivalent) for local versus remote control, and a separate deliberate start action. Safety functions such as e-stop, interlocks and isolation must stay in hardware or the PLC, independent of the cloud. Software confirmations, step-up auth, command TTLs and rate limits add defence in depth.

### Cited Findings
- IEC 60204-1: "The provision of acoustic and/or visual warning signals before the starting of hazardous machine operation shall be considered during the risk assessment." — [EN 60204-1:2018](https://eclass.uniwa.gr/modules/document/file.php/IDPE391/%CE%A0%CE%A1%CE%9F%CE%A4%CE%A5%CE%A0%CE%91/EN%2060204-1-2018.pdf)
- IEC 60204-1: where operating modes need different protective measures, a mode selector lockable in each position (e.g. key switch) is required. It may be replaced by a selection method restricting functions to categories of operator (e.g. access codes). Mode selection alone must not start the machine, and the selected mode must be indicated — [EN 60204-1:2018](https://eclass.uniwa.gr/modules/document/file.php/IDPE391/%CE%A0%CE%A1%CE%9F%CE%A4%CE%A5%CE%A0%CE%91/EN%2060204-1-2018.pdf)
- IEC 60204-1: a separate actuation of the start control is required. Where more than one control station is used, the standard sets requirements for starting from each, and a stop from any station must be effective where safety requires — [EN 60204-1:2018](https://eclass.uniwa.gr/modules/document/file.php/IDPE391/%CE%A0%CE%A1%CE%9F%CE%A4%CE%A5%CE%A0%CE%91/EN%2060204-1-2018.pdf); [PLCtalk discussion](https://plctalk.net/threads/remote-start-does-it-need-to-have-a-remote-stop.115792)
- Commentary: a station able to start remotely should also be able to stop, and stop takes priority over start (secondary source) — [Industrial Monitor Direct](https://industrialmonitordirect.com/blogs/knowledgebase/remote-start-requires-remote-stop-safety-standards-analysis)
- "Local control" as a safety function: when in local, remote commands must be inhibited — [Machinery Safety 101](https://machinerysafety101.com/2022/06/29/local-control/)
- AS/NZS 4024.1603:2019 covers prevention of unexpected start-up — [Intertek Inform](https://www.intertekinform.com/en-us/standards/as-nzs-4024-1603-2019-1158104_saig_as_as_2751128/)

### Inferences
- **Hardware/PLC layer (Saturn's electrical design, applies before any software):**
  - Physical **Local/Off/Remote selector** on each controlled load. In Local or Off, the PLC ignores cloud/HA commands, and the state is reported to the app (shown as "Local – remote disabled").
  - Isolators and lockout must physically prevent energisation regardless of software.
  - **Pre-start warning** (beacon/siren for N seconds) on hazardous loads such as compressors, pumps with exposed shafts, roller doors and gates. It is implemented in the PLC so it can't be bypassed by the cloud.
  - Hardwired interlocks (pressure, level, door photo-eyes and safety edges per the gate/door standards).
  - Remote stop always permitted. Remote start only where the risk assessment allows.
- **Software layer (Command Center):**
  - Classify each I/O point (info / low-risk e.g. lights / hazardous e.g. compressor, gate). Hazardous points require step-up auth plus typed or hold-to-confirm, with the current live state and site camera snapshot shown before confirming.
  - Optional two-person approval for customer-defined critical points (low priority for small sites).
  - **"Force" mode with automatic expiry** (e.g. a force reverts after X minutes unless renewed), visible banners for all active forces, and a "release all forces" button.
  - Command TTL (reject if not executed within ~10 s) and read-back confirmation from the PLC.
  - Per-user and per-point **rate limits** (e.g. ≤1 start per 30 s per motor, to protect against short-cycling and scripted abuse).
  - Disallow remote forcing of points the PLC marks as safety-related.
  - Schedules and automations are subject to the same interlocks.
- **AI Q&A must be read-only:** don't let LLM tool calls issue writes to I/O. If that is ever added, require explicit human confirmation and step-up auth.
- Document each site's remote-control risk assessment (a simple hazard/risk register per controlled load). This supports the ESA/WHS "reasonably practicable" defence.

### Gaps
- No Australian regulator guidance (ESO Qld, Safe Work Australia) found specifically on remote or app-based starting of equipment. AS/NZS 4024 clause text is paywalled.
- Gate/door automation standards (e.g. AS/NZS 60335.2.103 for gates and doors) weren't researched. They may impose specific requirements for remote operation without line of sight (unverified).

## 6. Logging and audit (immutable audit trail, retention, alerting)

### Takeaway
Comparable platforms provide device-level and company-wide audit trails (IXON), connection audit trails (Talk2M) and audit databases (Ignition), with centralised log analysis (IXON). Saturn needs an append-only audit table for logins, permission changes, token changes, every output force/command (who, what, old→new value, result, source IP/device) and deployments, plus alerts on anomalies.

### Cited Findings
- IXON Cloud provides device-specific and company-wide audit trails. All server logs go to a centralised log system and are analysed against community and custom security rules — [IXON TOMs](https://www.ixon.cloud/technical-and-organizational-measures)
- Talk2M accounts keep an audit trail of connections — [Talk2M brochure](https://coevagi.com/wp-content/uploads/2024/01/Ewon-Talk2M.pdf); [Talk2M upgrade guide](https://becolve.com/wp-content/uploads/2024/10/ssh-0085-00-why-to-upgrade-from-talk2m-free-to-talk2m-light-or-pro.pdf)
- Ignition's hardening guidance covers an audit DB (third-party summary) — [Industrial Monitor Direct](https://industrialmonitordirect.com/blogs/knowledgebase/ignition-security-hardening-guide-audit-db-openscap-compliance)
- The secure connectivity principles require OT access to be "mediated, monitored and controlled" — [ASD/ACSC](https://www.cyber.gov.au/publication/secure-connectivity-principles-for-operational-technology)

### Inferences
- **Implementation (Postgres):**
  - `audit_events` table: id, ts (server), actor_user_id, actor_session_id, site_id, action, target, before, after, result, ip, user_agent, request_id, and `prev_hash`/`hash` (SHA-256 chain) for tamper evidence.
  - Revoke UPDATE/DELETE on the table from the app DB role (grant INSERT/SELECT only). Optionally ship nightly to object storage with object lock (WORM).
- **Retention suggestion:** security and control audit for at least 12 months online and 7 years archived. Seven years matches common Australian record-keeping norms, e.g. tax records; this is not a specific OT rule, so treat as a judgement call. High-rate telemetry can be shorter or downsampled. Publish retention in the privacy policy, since APP 11.2 requires destroying or de-identifying personal info no longer needed (unverified wording).
- **Alerting rules (email/push to super admin):**
  - N failed logins or lockout
  - login from a new device/country
  - super admin permission change or new user
  - HA token added, changed or viewed
  - forces outside business hours or bursts of commands
  - PLC program deploy, or a mismatch between the running hash and the repo
  - an edge device going offline
- Show per-site activity logs to customers (a selling point, matching IXON/Talk2M) and support CSV export.
- Log the AI Q&A prompts and responses that touch customer data, for accountability.

### Gaps
- No sourced Australian legal retention period specific to OT command logs. No regulator-mandated retention found for small private operators.
- Couldn't retrieve Ignition's official audit-log documentation directly.

---
**Prioritised roadmap summary (for a 1–2 person team):**
- **Now (weeks):**
  - MFA (passkey plus TOTP) mandatory for admins and control users
  - lockout and rate limiting
  - server-side sessions with idle lock and 24 h max for control sessions
  - step-up auth before forcing outputs
  - append-only audit log
  - nightly off-platform `pg_dump` plus Railway backups/PITR
  - edge heartbeat plus offline alerts
  - verify that nothing at the sites is internet-exposed
  - Local/Remote selectors and pre-start warnings on hazardous loads
- **Next (1–3 months):**
  - outbound-only edge agent with a narrow command API (replace broad cloud-held HA tokens)
  - force expiry/TTL and rate limits
  - login alerts and a sessions page
  - status page
  - privacy policy and terms reviewed for unfair contract terms
  - Essential Eight ML1 for internal accounts
  - PLC program signing and rollback
- **Later:**
  - SSO (OIDC)
  - scoped API tokens
  - two-person approval
  - per-site risk assessment templates
  - AU-region DB if customers demand it
  - external pen test
  - formal IEC 62443-aligned zone documentation for commercial customers
