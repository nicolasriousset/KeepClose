#!/usr/bin/env node
/*
 * Simulation du code boot + widget Bangle.js 2 sous Node, avec des API Espruino factices.
 * Ne remplace pas les tests sur montre : vérifie seulement le câblage (erreurs d'exécution,
 * trame publiée, vibrations, persistance) sur un scénario N.I.C.O. complet.
 *
 *   node firmware/tools/simulate-bangle.js
 */
const vm = require("vm"), fs = require("fs"), path = require("path"), assert = require("assert/strict");
const DIR = path.resolve(__dirname, "../bangle/keepclose");
const K = require("../common/kccommon.js");

let timers = [], now = 0, buzz = [], adv = null, scanCb = null, store = {};
const g = new Proxy({}, { get: (t, k) => (k === "theme" ? {} : () => g) });
const ctx = {
  console, Promise,
  Date: { now: () => now },
  NRF: { getAddress: () => "11:22:33:44:55:66 random", setAdvertising: (d, o) => { adv = o; }, setScan: (cb) => { scanCb = cb; } },
  Bangle: { buzz: (ms) => { buzz.push(ms); return Promise.resolve(); }, on: (e, f) => { ctx["_" + e] = f; } },
  require: (m) => (m === "kccommon" ? K : { readJSON: (f) => store[f], writeJSON: (f, d) => { store[f] = d; } }),
  setInterval: (f, ms) => timers.push({ f, ms, next: now + ms }),
  setTimeout: (f, ms) => timers.push({ f, ms, next: now + ms, once: 1 }),
  clearInterval: (i) => { if (timers[i - 1]) timers[i - 1].dead = 1; },
  clearTimeout: (i) => { if (timers[i - 1]) timers[i - 1].dead = 1; },
  WIDGETS: {}, g
};
ctx.global = ctx;
vm.createContext(ctx);
// Espruino : tout objet porte on()/emit()
vm.runInContext(`
  Object.defineProperty(Object.prototype, "on", { writable: true, value: function (e, f) {
    var l = this.__l || Object.defineProperty(this, "__l", { value: {} }).__l; (l[e] = l[e] || []).push(f); } });
  Object.defineProperty(Object.prototype, "emit", { writable: true, value: function (e) {
    ((this.__l || {})[e] || []).forEach(function (f) { f(); }); } });`, ctx);
for (const f of ["keepclose.boot.js", "keepclose.wid.js"])
  vm.runInContext(fs.readFileSync(path.join(DIR, f), "utf8"), ctx, { filename: f });
Object.assign(ctx.WIDGETS.keepclose, { x: 0, y: 0 });

const flush = () => new Promise((r) => setImmediate(r));  // laisse s'exécuter les promesses Espruino
async function run(ms) {
  const end = now + ms;
  for (;;) {
    await flush();
    const t = timers.filter((t) => !t.dead).sort((a, b) => a.next - b.next)[0];
    if (!t || t.next > end) break;
    now = t.next;
    if (t.once) t.dead = 1; else t.next += t.ms;
    t.f();
  }
  now = end;
}
const TAG = "aa:bb:cc:dd:ee:01 random";
const rssiAt = (d) => K.C.P1M - 10 * K.C.PATH_LOSS_N * Math.log10(d);
async function at(d, ms) {
  for (let t = 0; t < ms; t += 100) {
    scanCb({ id: TAG, rssi: d === "contact" ? -30 : rssiAt(d), manufacturer: 0xFFFF, manufacturerData: K.encodeTag(0) });
    await run(100);
  }
}
const KC = ctx.KC;

(async () => {
  assert.equal(KC.status(), "none");
  await at("contact", 4000);
  assert.equal(KC.status(), "pairing");
  assert.deepEqual(buzz.slice(0, 3), [100, 100, 100], "3 vibrations d'appairage");
  await run(6000);
  await at(0.3, 6000);
  assert.equal(KC.status(), "ok");
  assert.deepEqual(adv.manufacturerData, K.encodeBracelet(0, 0));
  assert.equal(adv.whenConnected, true);

  buzz = [];
  await at(2, 6000);
  assert.equal(KC.status(), "out");
  assert.deepEqual(adv.manufacturerData, K.encodeBracelet(K.crc16(TAG), 0));
  assert.ok(buzz.filter((ms) => ms === 200).length >= 4, "alerte 200 ms / s");

  ctx._tap({ double: true });
  await run(1500); buzz = []; await run(3000);
  assert.equal(buzz.length, 0, "silence après snooze");
  await run(1000);
  assert.deepEqual(JSON.parse(JSON.stringify(store["keepclose.json"])).snoozed, [TAG]);

  await at(0.3, 8000);
  ctx._tap({ double: true });
  await run(600);
  assert.deepEqual(adv.manufacturerData, K.encodeBracelet(0, K.crc16(TAG)), "sonnerie publiée");
  await at(0.3, 10000);
  assert.deepEqual(adv.manufacturerData, K.encodeBracelet(0, 0), "sonnerie arrêtée à 10 s");

  KC.setEnabled(false);
  assert.equal(KC.status(), "off");
  assert.equal(adv.manufacturerData, undefined, "plus de trame KC");
  console.log("simulation Bangle : OK");
})().catch((e) => { console.error(e); process.exit(1); });
