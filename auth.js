// auth.js — Saturn OS authentication
//
// - PINs are hashed with Node's built-in scrypt (no bcrypt dependency needed)
// - Sessions are signed cookies (HMAC-SHA256), no DB session table needed
// - Login is by PIN only (no usernames). 6-digit numeric PINs.
// - First admin is bootstrapped from ADMIN_INITIAL_PIN env var if no users exist.

const crypto = require('crypto');
const { promisify } = require('util');
const db = require('./db');

const scryptAsync = promisify(crypto.scrypt);
const COOKIE_NAME = 'saturn_session';
const SESSION_TTL_MS = 1000 * 60 * 60 * 24 * 30; // 30 days

// In-memory rate limiter for login attempts (resets on restart)
const loginAttempts = new Map(); // ip -> { count, resetAt }
const MAX_ATTEMPTS_PER_HOUR = 20;

// ---- secret management ------------------------------------------------------

let SESSION_SECRET = process.env.SESSION_SECRET;
if (!SESSION_SECRET) {
  SESSION_SECRET = crypto.randomBytes(48).toString('hex');
  console.warn('[AUTH] SESSION_SECRET not set in env — using a random one (sessions will not survive restarts). Add SESSION_SECRET to Railway env vars to fix.');
}

// ---- password hashing -------------------------------------------------------

async function hashPin(pin) {
  const salt = crypto.randomBytes(16).toString('hex');
  const buf = await scryptAsync(String(pin), salt, 64);
  return `${salt}:${buf.toString('hex')}`;
}

async function verifyPin(pin, hashStr) {
  if (!hashStr || typeof hashStr !== 'string' || !hashStr.includes(':')) return false;
  const [salt, hash] = hashStr.split(':');
  try {
    const candidate = await scryptAsync(String(pin), salt, 64);
    const stored = Buffer.from(hash, 'hex');
    if (stored.length !== candidate.length) return false;
    return crypto.timingSafeEqual(stored, candidate);
  } catch {
    return false;
  }
}

// ---- session token (signed cookie) -----------------------------------------

function signToken(payload) {
  const data = Buffer.from(JSON.stringify(payload)).toString('base64url');
  const sig = crypto.createHmac('sha256', SESSION_SECRET).update(data).digest('base64url');
  return `${data}.${sig}`;
}

function verifyToken(token) {
  if (!token || typeof token !== 'string') return null;
  const parts = token.split('.');
  if (parts.length !== 2) return null;
  const [data, sig] = parts;
  const expected = crypto.createHmac('sha256', SESSION_SECRET).update(data).digest('base64url');
  if (sig.length !== expected.length) return null;
  if (!crypto.timingSafeEqual(Buffer.from(sig), Buffer.from(expected))) return null;
  try {
    const payload = JSON.parse(Buffer.from(data, 'base64url').toString());
    if (payload.exp && payload.exp < Date.now()) return null;
    return payload;
  } catch {
    return null;
  }
}

// ---- cookie parsing (no dep) -----------------------------------------------

function parseCookies(req) {
  const out = {};
  const header = req.headers.cookie;
  if (!header) return out;
  for (const part of header.split(';')) {
    const idx = part.indexOf('=');
    if (idx < 0) continue;
    const k = part.slice(0, idx).trim();
    const v = part.slice(idx + 1).trim();
    out[k] = decodeURIComponent(v);
  }
  return out;
}

function setCookie(res, name, value, opts = {}) {
  const parts = [`${name}=${encodeURIComponent(value)}`];
  parts.push(`Path=${opts.path || '/'}`);
  if (opts.maxAge) parts.push(`Max-Age=${Math.floor(opts.maxAge / 1000)}`);
  parts.push('HttpOnly');
  parts.push('SameSite=Lax');
  // In Railway production we're on HTTPS so this is safe
  parts.push('Secure');
  res.setHeader('Set-Cookie', parts.join('; '));
}

function clearCookie(res, name) {
  res.setHeader('Set-Cookie', `${name}=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax; Secure`);
}

// ---- rate limiting ----------------------------------------------------------

function rateLimited(ip) {
  const now = Date.now();
  const rec = loginAttempts.get(ip);
  if (!rec || rec.resetAt < now) {
    loginAttempts.set(ip, { count: 1, resetAt: now + 60 * 60 * 1000 });
    return false;
  }
  rec.count++;
  return rec.count > MAX_ATTEMPTS_PER_HOUR;
}

function clearRateLimit(ip) {
  loginAttempts.delete(ip);
}

// ---- bootstrap initial admin -----------------------------------------------

async function bootstrapAdmin() {
  const result = await db.query('SELECT COUNT(*) AS n FROM users WHERE active = TRUE');
  const count = parseInt(result.rows[0].n, 10);
  if (count > 0) return; // admin already exists

  const initialPin = process.env.ADMIN_INITIAL_PIN;
  if (!initialPin) {
    console.warn('[AUTH] No active users and ADMIN_INITIAL_PIN not set. Set ADMIN_INITIAL_PIN in Railway env to create the first admin.');
    return;
  }
  if (!/^\d{6}$/.test(initialPin)) {
    console.error('[AUTH] ADMIN_INITIAL_PIN must be exactly 6 digits.');
    return;
  }

  const pinHash = await hashPin(initialPin);
  await db.query(
    `INSERT INTO users (name, pin_hash, is_admin, active) VALUES ($1, $2, TRUE, TRUE)`,
    ['Admin', pinHash]
  );
  console.log('[AUTH] Initial admin created. You can now remove ADMIN_INITIAL_PIN from Railway env vars.');
}

// ---- middleware -------------------------------------------------------------

async function loadUser(req, res, next) {
  const cookies = parseCookies(req);
  const token = cookies[COOKIE_NAME];
  const payload = verifyToken(token);
  if (!payload || !payload.uid) {
    req.user = null;
    return next();
  }
  try {
    const result = await db.query(
      'SELECT id, name, email, phone, is_admin, active FROM users WHERE id = $1',
      [payload.uid]
    );
    const user = result.rows[0];
    req.user = (user && user.active) ? user : null;
  } catch (err) {
    console.error('[AUTH] loadUser error:', err);
    req.user = null;
  }
  next();
}

function requireAuth(req, res, next) {
  if (!req.user) return res.status(401).json({ error: 'Not authenticated' });
  next();
}

function requireAdmin(req, res, next) {
  if (!req.user) return res.status(401).json({ error: 'Not authenticated' });
  if (!req.user.is_admin) return res.status(403).json({ error: 'Admin only' });
  next();
}

// ---- helpers ----------------------------------------------------------------

function generatePin() {
  // 6-digit PIN, padded with leading zeros if needed
  const n = crypto.randomInt(0, 1000000);
  return String(n).padStart(6, '0');
}

module.exports = {
  COOKIE_NAME,
  SESSION_TTL_MS,
  hashPin,
  verifyPin,
  signToken,
  verifyToken,
  setCookie,
  clearCookie,
  rateLimited,
  clearRateLimit,
  bootstrapAdmin,
  loadUser,
  requireAuth,
  requireAdmin,
  generatePin,
};
