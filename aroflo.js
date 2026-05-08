// aroflo.js — AroFlo API client
//
// AroFlo's API lives at https://api.aroflo.com/ and uses a "zone" query system.
// Pass ?zone=purchaseorders, optionally ?join=lineitems, plus credentials.
// Multiple ?where=and|field|op|value clauses can be combined.
//
// All responses are XML. We parse them with fast-xml-parser and return the
// imsapi root as a plain JS object.

const { XMLParser } = require('fast-xml-parser');

const BASE_URL = 'https://api.aroflo.com/';

const parser = new XMLParser({
  ignoreAttributes: false,
  attributeNamePrefix: '@_',
  parseAttributeValue: true,
  trimValues: true,
});

function getCredentials() {
  const u = process.env.AROFLO_U_ENCODED;
  const p = process.env.AROFLO_P_ENCODED;
  const org = process.env.AROFLO_ORG_ENCODED;
  if (!u || !p || !org) {
    throw new Error(
      'AroFlo credentials missing. Set AROFLO_U_ENCODED, AROFLO_P_ENCODED, AROFLO_ORG_ENCODED in env.'
    );
  }
  return { u, p, org };
}

/**
 * Fetch one page of data from AroFlo.
 * Returns the parsed `imsapi` root object.
 */
async function fetchZone({ zone, join, page = 1, where = [] } = {}) {
  if (!zone) throw new Error('fetchZone: zone is required');
  const { u, p, org } = getCredentials();

  const params = new URLSearchParams();
  params.append('zone', zone);
  params.append('page', String(page));
  if (join) params.append('join', join);
  for (const clause of where) params.append('where', clause);
  params.append('uencoded', u);
  params.append('pencoded', p);
  params.append('orgencoded', org);

  const url = `${BASE_URL}?${params.toString()}`;
  const response = await fetch(url, { method: 'GET' });
  const text = await response.text();

  if (response.status !== 200) {
    throw new Error(`AroFlo HTTP ${response.status}: ${text.slice(0, 200)}`);
  }

  const parsed = parser.parse(text);
  const imsapi = parsed.imsapi;
  if (!imsapi) {
    throw new Error(`AroFlo response missing <imsapi> root. Body starts: ${text.slice(0, 200)}`);
  }
  // status 0 = OK; any other value = error
  if (imsapi.status !== 0 && imsapi.status !== '0') {
    throw new Error(`AroFlo error status ${imsapi.status}: ${imsapi.statusmessage}`);
  }
  return imsapi;
}

/**
 * Walk every page of a zone until we've fetched them all.
 * Returns a flat array of items (e.g. one entry per <purchaseorder>).
 */
async function fetchAllPages({ zone, join, where = [] } = {}) {
  const dataKey = singularize(zone);
  const allItems = [];
  let page = 1;
  let totalPages = 1;
  const safetyLimit = 200; // worst-case 200 pages * 500/page = 100k records

  while (page <= totalPages && page <= safetyLimit) {
    const imsapi = await fetchZone({ zone, join, page, where });
    const zoneResponse = imsapi.zoneresponse || {};
    const maxPerPage = parseInt(zoneResponse.maxpageresults, 10) || 500;
    const totalCount = parseInt(zoneResponse[zone], 10) || 0;
    totalPages = Math.max(1, Math.ceil(totalCount / maxPerPage));

    // The data array sits at imsapi[singular_zone], e.g. imsapi.purchaseorder
    let items = imsapi[dataKey];
    if (items === undefined || items === null) items = [];
    if (!Array.isArray(items)) items = [items];
    allItems.push(...items);

    page++;
  }

  return allItems;
}

function singularize(zone) {
  // purchaseorders -> purchaseorder, tasks -> task, clients -> client
  if (typeof zone !== 'string') return zone;
  if (zone.endsWith('s')) return zone.slice(0, -1);
  return zone;
}

module.exports = { fetchZone, fetchAllPages };
