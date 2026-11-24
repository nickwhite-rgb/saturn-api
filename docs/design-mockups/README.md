# Saturn Command Center: approved design mock-ups

**Approved by Nick on 24 Nov 2026.** This is the look to build the real app to.

Live version (with comments and tweaks): https://claude.ai/artifact/Fq6icUzQXM3vonxnjjwKN4

| File | Screen |
|---|---|
| `Clean-Overview.dc.html` | Light mode – Dashboard (the layout to use for both modes) |
| `Clean-IO.dc.html` | Light mode – Site I/O page with forcing |
| `Main.dc.html` | Dark mode – Dashboard (dark colours, glow, numbers style) |
| `ControlRoom-IO.dc.html` | Dark mode – Site I/O page |
| `Settings.dc.html` | Settings (gear button) |
| `Users.dc.html` | Users & permissions list (super admin only) |
| `UserAccess.dc.html` | One user's tick-and-flick access page |

## What's decided

- One layout (light mode's) in **light and dark**, with a sun/moon toggle and a settings gear in the top bar of every screen.
- **Saturn logo top left:** the full logo on the dashboard, the planet mark on inner pages. Files and colours are in `/brand`.
- **Purple** for active tabs, buttons, ticks and the line under the top bar. **Silver** for lines and secondary text. **Navy** as the base. Montserrat for headings, Open Sans for text.
- Status colours keep their meaning: green = OK, orange = alarm, amber = forced.
- **Roles:** Super admin (Nick only for now) and User. New users start with no sites and no adjust rights. Each site has See and Adjust boxes, plus ability ticks: Add sites, Force I/O, Ask AI, Phone alerts.

These files are the design canvas's source; the logos inside them point at the copies uploaded to the live mock-up. Readings and names in them are sample data.
