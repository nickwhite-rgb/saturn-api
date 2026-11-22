# Saturn Command

One mobile web app to see and look after every Saturn Electrical site: Nick's own sites, the depot, and customer jobs.

- **PIN login** (6 digits) for each employee
- **Site list** with search, filters and a status dot per site
- **Site page**: overview, live readings from the site's Home Assistant, equipment and I/O list, log, settings
- **Ask AI**: Claude answers questions about a site using its record, log and live readings (read-only in this version)
- **Team**: a profile per employee with None / View / Edit access per site, "can add new sites", admin and active switches, PIN reset

Node.js + Express + Postgres, hosted on Railway. Formerly "Saturn OS". The old AroFlo purchase-history tables are removed automatically on first start; user logins are kept.

## Railway variables

| Variable | Needed | What it's for |
|---|---|---|
| `DATABASE_URL` | Yes | Linked from the Railway Postgres service |
| `SESSION_SECRET` | Yes | Long random string. Keeps people logged in across restarts and encrypts saved Home Assistant tokens. Don't change it once set, or saved tokens must be re-entered |
| `ANTHROPIC_API_KEY` | For Ask AI | Anthropic API key (console.anthropic.com) |
| `ADMIN_INITIAL_PIN` | Only on an empty database | Creates the first admin; remove after first login |
| `CLAUDE_MODEL` | No | Override the AI model (default `claude-opus-5-5`) |
| `DATA_KEY` | No | Separate key for encrypting Home Assistant tokens (defaults to `SESSION_SECRET`) |

The old `AROFLO_*` variables are no longer used and can be deleted.

## Adding live data to a site

1. In the site's Home Assistant: profile → Security → create a long-lived access token. A dedicated non-admin HA user is best.
2. In Saturn Command: site → Settings → Live data. Enter the HA address (e.g. the Nabu Casa URL), paste the token, and list the entity IDs to show, one per line.

## Files

- `index.js`: server and API routes
- `auth.js`: PIN login and sessions
- `migrate.js`: creates and updates the database tables on every start
- `ha.js`: Home Assistant live data and token encryption
- `ai.js`: Ask AI (Claude API)
- `ui.html`: the whole mobile web interface
- `db.js`: Postgres connection
