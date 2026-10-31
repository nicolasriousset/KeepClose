// Tests de la logique partagée — node --test firmware/common/test/
const test = require("node:test");
const assert = require("node:assert/strict");
const K = require("../kccommon.js");
const C = K.C;

const BRACELET = "11:22:33:44:55:66";
const OTHER_BRACELET = "66:55:44:33:22:11";
const TAG_A = "aa:bb:cc:dd:ee:01";
const TAG_B = "aa:bb:cc:dd:ee:02";
const TAG_C = "aa:bb:cc:dd:ee:03";
const OWN = K.crc16(BRACELET);

// RSSI donnant une distance donnée (inverse de K.distance)
const rssiAt = (d) => C.P1M - 10 * C.PATH_LOSS_N * Math.log10(d);
const tagFrame = (owner) => ({ type: C.TYPE_TAG, owner: owner || 0 });

// Bracelet avec balises déjà appairées, mesures stabilisées à la distance donnée
function bracelet(paired, opts) {
  const events = [];
  const b = K.createBracelet(OWN, Object.assign({ paired }, opts || {}), (e, d) => events.push([e, d]));
  b.events = events;
  return b;
}
// Envoie n trames à distance constante (stabilise l'EMA), toutes les 100 ms à partir de t
function feed(b, addr, d, t, n) {
  for (let i = 0; i < (n || 1); i++) b.onTagFrame(addr, rssiAt(d), tagFrame(OWN), t + i * 100);
  return t + (n || 1) * 100;
}

// ── 2.1 Identifiant compact ──────────────────────────────────────────────────

test("CRC16-CCITT : vecteur de référence '123456789' = 0x29B1", () => {
  assert.equal(K.crc16("31:32:33:34:35:36:37:38:39"), 0x29B1);
});

test("CRC indépendant de la casse et du suffixe de type d'adresse", () => {
  assert.equal(K.crc16("AA:BB:CC:DD:EE:FF"), K.crc16("aa:bb:cc:dd:ee:ff random"));
});

// ── 2.2 Trames ───────────────────────────────────────────────────────────────

test("trame bracelet : aller-retour et petit-boutiste", () => {
  const data = K.encodeBracelet(0x1A2B, 0x3C4D);
  assert.deepEqual(data, [0x4B, 0x43, 0x01, 0x2B, 0x1A, 0x4D, 0x3C]);
  assert.deepEqual(K.decode(0xFFFF, data), { type: 1, alert: 0x1A2B, ring: 0x3C4D });
});

test("trame balise : aller-retour, propriétaire 0x1A2B → octets 2B 1A", () => {
  const data = K.encodeTag(0x1A2B);
  assert.deepEqual(data.slice(3), [0x2B, 0x1A]);
  assert.deepEqual(K.decode(0xFFFF, new Uint8Array(data).buffer), { type: 2, owner: 0x1A2B });
  assert.deepEqual(K.decode(0xFFFF, K.encodeTag(0)), { type: 2, owner: 0 });
});

test("trames étrangères rejetées", () => {
  assert.equal(K.decode(0x0059, K.encodeTag(1)), null);            // autre company ID
  assert.equal(K.decode(0xFFFF, [0x41, 0x42, 0x02, 0, 0]), null);    // mauvais préfixe
  assert.equal(K.decode(0xFFFF, [0x4B, 0x43, 0x07, 0, 0]), null);    // type inconnu
  assert.equal(K.decode(0xFFFF, [0x4B, 0x43, 0x01, 0]), null);       // tronquée
});

// ── 2.3 Registre : EMA, distance, vivacité, hystérésis ───────────────────────

test("première mesure à -61 dBm → 1,0 m", () => {
  const b = bracelet([TAG_A]);
  b.onTagFrame(TAG_A, -61, tagFrame(OWN), 0);
  assert.equal(b.list(0)[0].dist, 1.0);
});

test("pic isolé de +20 dB → RSSI lissé +2 dB", () => {
  const b = bracelet([TAG_A]);
  b.onTagFrame(TAG_A, -70, tagFrame(OWN), 0);
  b.onTagFrame(TAG_A, -50, tagFrame(OWN), 100);
  assert.ok(Math.abs(b.list(100)[0].ema - -68) < 1e-9);
});

test("balise non reçue depuis 6 s → hors de portée", () => {
  const b = bracelet([TAG_A]);
  feed(b, TAG_A, 0.2, 0, 30);
  assert.equal(b.status(3000), "ok");
  assert.equal(b.status(2900 + C.ALIVE_MS), "out");
});

test("hystérésis : oscillation 0,48–0,52 m reste hors de portée, retour à 0,40 m", () => {
  const b = bracelet([TAG_A]);
  let t = feed(b, TAG_A, 0.3, 0, 50);
  assert.equal(b.status(t), "ok");
  t = feed(b, TAG_A, 0.7, t, 80);
  assert.equal(b.status(t), "out");
  for (let i = 0; i < 100; i++) {
    t = feed(b, TAG_A, i % 2 ? 0.48 : 0.52, t);
    assert.equal(b.status(t), "out");
  }
  t = feed(b, TAG_A, 0.40, t, 80);
  assert.equal(b.status(t), "ok");
});

test("hystérésis propre à chaque balise", () => {
  const b = bracelet([TAG_A, TAG_B]);
  let t = feed(b, TAG_A, 0.5, 0, 50);   // A arrive à 0,5 m depuis "hors portée" → reste out
  feed(b, TAG_B, 0.3, 0, 50);
  const l = b.list(t);
  assert.equal(l.find((x) => x.addr === TAG_A).out, true);
  assert.equal(l.find((x) => x.addr === TAG_B).out, false);
});

// ── 2.4 Appairage ────────────────────────────────────────────────────────────

test("contact maintenu 3 s → appairée et persistée", () => {
  const b = bracelet([]);
  for (let t = 0; t <= 3000; t += 100) b.onTagFrame(TAG_A, -30, tagFrame(0), t);
  assert.equal(b.isPaired(TAG_A), true);
  assert.ok(b.events.some(([e]) => e === "paired"));
  assert.deepEqual(b.events.filter(([e]) => e === "persist").pop()[1].paired, [TAG_A]);
});

test("contact rompu à 2 s → délai remis à zéro", () => {
  const b = bracelet([]);
  for (let t = 0; t <= 2000; t += 100) b.onTagFrame(TAG_A, -30, tagFrame(0), t);
  b.onTagFrame(TAG_A, -60, tagFrame(0), 2100);
  for (let t = 2200; t <= 5000; t += 100) b.onTagFrame(TAG_A, -30, tagFrame(0), t);
  assert.equal(b.isPaired(TAG_A), false);
  b.onTagFrame(TAG_A, -30, tagFrame(0), 5200);
  assert.equal(b.isPaired(TAG_A), true);
});

test("état persistant rechargé : balise toujours appairée", () => {
  const b1 = bracelet([TAG_A], { mode: "trail", snoozed: [], reached: [TAG_A] });
  const b2 = K.createBracelet(OWN, JSON.parse(JSON.stringify(b1.toJSON())));
  assert.equal(b2.isPaired(TAG_A), true);
  assert.equal(b2.state.mode, "trail");
  assert.equal(b2.state.reached[TAG_A], true);
});

test("désappairage automatique quand la balise publie un autre bracelet", () => {
  const b = bracelet([TAG_A]);
  feed(b, TAG_A, 2, 0, 10);
  b.onTagFrame(TAG_A, -70, tagFrame(K.crc16(OTHER_BRACELET)), 2000);
  assert.equal(b.isPaired(TAG_A), false);
  assert.ok(b.events.some(([e]) => e === "unpaired"));
  assert.deepEqual(b.tick(2000), { alertCrc: 0, ringCrc: 0, vibrate: false });
});

test("oublier les balises", () => {
  const b = bracelet([TAG_A, TAG_B]);
  b.forget();
  assert.equal(b.status(0), "none");
  assert.deepEqual(b.tick(0), { alertCrc: 0, ringCrc: 0, vibrate: false });
});

// ── 2.5 N.I.C.O. ─────────────────────────────────────────────────────────────

test("canne hors périmètre → vibration et balise publiée en alerte", () => {
  const b = bracelet([TAG_A]);
  const t = feed(b, TAG_A, 2, 0, 30);
  const r = b.tick(t);
  assert.equal(r.vibrate, true);
  assert.equal(r.alertCrc, K.crc16(TAG_A));
});

test("deux balises hors de portée → la plus proche publiée, non vivante classée la plus loin", () => {
  const b = bracelet([TAG_A, TAG_B, TAG_C]);
  let t = feed(b, TAG_B, 3, 0, 30);
  feed(b, TAG_A, 1.2, 0, 30);
  assert.equal(b.tick(t).alertCrc, K.crc16(TAG_A));
});

test("action pendant l'alerte → snooze, la vibration s'arrête mais l'alerte reste publiée", () => {
  const b = bracelet([TAG_A]);
  const t = feed(b, TAG_A, 2, 0, 30);
  assert.equal(b.action(t), "snoozed");
  const r = b.tick(t);
  assert.equal(r.vibrate, false);
  assert.equal(r.alertCrc, K.crc16(TAG_A));
});

test("snooze levé au retour (≤ NEAR), nouvelle alerte si la balise repart", () => {
  const b = bracelet([TAG_A]);
  let t = feed(b, TAG_A, 2, 0, 30);
  b.action(t);
  t = feed(b, TAG_A, 0.3, t, 80);
  b.tick(t);
  assert.equal(b.state.snoozed[TAG_A], undefined);
  t = feed(b, TAG_A, 1, t, 80);
  assert.equal(b.tick(t).vibrate, true);
});

test("action sans alerte → sonnerie de la plus proche, arrêt auto à 10 s", () => {
  const b = bracelet([TAG_A, TAG_B]);
  let t = feed(b, TAG_A, 0.3, 0, 30);
  feed(b, TAG_B, 0.2, 0, 30);
  assert.equal(b.action(t), "ring");
  assert.equal(b.tick(t).ringCrc, K.crc16(TAG_B));
  assert.equal(b.tick(t + C.RING_MS - 1).ringCrc, K.crc16(TAG_B));
  assert.equal(b.tick(t + C.RING_MS).ringCrc, 0);
});

test("action pendant la sonnerie → annulation prioritaire sur le snooze", () => {
  const b = bracelet([TAG_A]);
  let t = feed(b, TAG_A, 0.3, 0, 30);
  b.action(t);
  t = feed(b, TAG_A, 2, t, 30);           // la balise s'éloigne pendant la sonnerie
  assert.equal(b.action(t), "ringCancelled");
  assert.equal(b.tick(t).ringCrc, 0);
  assert.equal(b.tick(t).vibrate, true);  // l'alerte n'a pas été snoozée
});

test("sonnerie d'une balise appairée non vivante si aucune n'est vivante", () => {
  const b = bracelet([TAG_A]);
  assert.equal(b.action(0), "snoozed");   // jamais vue → hors de portée → alerte → snooze
  assert.equal(b.action(0), "ring");
  assert.equal(b.tick(0).ringCrc, K.crc16(TAG_A));
});

test("nextAction annonce l'action sans l'exécuter, y compris après un snooze", () => {
  const b = bracelet([TAG_A]);
  const t = feed(b, TAG_A, 2, 0, 30);
  assert.equal(b.nextAction(t), "snoozed");
  assert.equal(b.nextAction(t), "snoozed");   // aucun effet de bord
  b.action(t);
  assert.equal(b.nextAction(t), "ring");      // alerte déjà en sourdine → la prochaine action sonne
  b.action(t);
  assert.equal(b.nextAction(t), "ringCancelled");
});

test("action sans balise appairée → noBeacon", () => {
  assert.equal(bracelet([]).action(0), "noBeacon");
});

test("désactivé → aucune trame, aucune alerte", () => {
  const b = bracelet([TAG_A]);
  b.setEnabled(false);
  assert.deepEqual(b.tick(0), { alertCrc: 0, ringCrc: 0, vibrate: false });
  assert.equal(b.action(0), "disabled");
});

// ── 2.6 Ti Poucet ────────────────────────────────────────────────────────────

function trail() {
  const b = bracelet([TAG_A, TAG_B, TAG_C], { mode: "trail" });
  feed(b, TAG_A, 3, 0, 30);
  feed(b, TAG_B, 6, 0, 30);
  feed(b, TAG_C, 9, 0, 30);
  return b;
}

test("Ti Poucet : la plus proche non atteinte sonne, aucune alerte ni vibration", () => {
  const b = trail();
  const r = b.tick(3000);
  assert.deepEqual(r, { alertCrc: 0, ringCrc: K.crc16(TAG_A), vibrate: false });
});

test("Ti Poucet : balise atteinte (< 1 m) → la suivante sonne immédiatement", () => {
  const b = trail();
  const t = feed(b, TAG_A, 0.8, 3000, 80);
  feed(b, TAG_B, 6, t - 1000, 10);
  feed(b, TAG_C, 9, t - 1000, 10);
  assert.equal(b.tick(t).ringCrc, K.crc16(TAG_B));
  assert.ok(b.events.some(([e, d]) => e === "reached" && d === TAG_A));
});

test("Ti Poucet : fin de trajet signalée une seule fois, puis plus de sonnerie", () => {
  const b = trail();
  let t = 3000;
  for (const tag of [TAG_A, TAG_B, TAG_C]) { t = feed(b, tag, 0.5, t, 120); b.tick(t); }
  feed(b, TAG_A, 0.5, t - 1000, 10); feed(b, TAG_B, 0.5, t - 1000, 10);
  assert.equal(b.tick(t).ringCrc, 0);
  b.tick(t + 100);
  assert.equal(b.events.filter(([e]) => e === "tripEnd").length, 1);
});

test("Ti Poucet : nouveau trajet → toutes redeviennent candidates", () => {
  const b = trail();
  const t = feed(b, TAG_A, 0.5, 3000, 120);
  b.tick(t);
  feed(b, TAG_A, 3, t, 200);
  b.newTrip();
  assert.equal(b.tick(t + 20000).ringCrc, K.crc16(TAG_A));
});

test("Ti Poucet : pause / reprise par l'action", () => {
  const b = trail();
  assert.equal(b.action(3000), "paused");
  assert.equal(b.tick(3000).ringCrc, 0);
  assert.equal(b.action(3000), "resumed");
  assert.equal(b.tick(3000).ringCrc, K.crc16(TAG_A));
});

test("changement de mode → atteintes effacées, alerte vibrante coupée en Ti Poucet", () => {
  const b = bracelet([TAG_A]);
  const t = feed(b, TAG_A, 2, 0, 30);
  b.setMode("trail");
  assert.equal(b.tick(t).vibrate, false);
  assert.equal(b.toJSON().mode, "trail");
});

// ── 2.7 Balise ───────────────────────────────────────────────────────────────

const TAG_OWN = K.crc16(TAG_A);
const bf = (alert, ring) => ({ type: C.TYPE_BRACELET, alert, ring });

function tag(owner) {
  const ev = [];
  const t = K.createTag(TAG_OWN, owner, (e, d) => ev.push([e, d]));
  t.events = ev;
  return t;
}

test("balise neuve : bleue ; adoption au contact → événement et verte", () => {
  const t = tag(0);
  assert.equal(t.color(0), "blue");
  t.onBraceletFrame(BRACELET, -30, bf(0, 0), 0);
  assert.deepEqual(t.events, [["adopted", OWN]]);
  assert.equal(t.color(0), "green");
});

test("changement de propriétaire au contact d'un autre bracelet", () => {
  const t = tag(OWN);
  t.onBraceletFrame(OTHER_BRACELET, -30, bf(0, 0), 0);
  assert.equal(t.state.owner, K.crc16(OTHER_BRACELET));
});

test("bracelet propriétaire loin : pas d'adoption par un autre bracelet lointain", () => {
  const t = tag(OWN);
  t.onBraceletFrame(OTHER_BRACELET, -70, bf(TAG_OWN, TAG_OWN), 0);
  assert.equal(t.state.owner, OWN);
  assert.equal(t.isRinging(0), false);   // ordre d'un autre bracelet ignoré
  assert.equal(t.color(0), "red");       // propriétaire jamais reçu → périmé
});

test("alerte → rouge ; trame périmée (6 s) → rouge", () => {
  const t = tag(OWN);
  t.onBraceletFrame(BRACELET, -70, bf(TAG_OWN, 0), 0);
  assert.equal(t.color(0), "red");
  t.onBraceletFrame(BRACELET, -70, bf(0, 0), 100);
  assert.equal(t.color(100), "green");
  assert.equal(t.color(100 + C.TAG_STALE_MS), "red");
});

test("sonnerie seulement si désignée ; s'arrête si le bracelet n'est plus reçu", () => {
  const t = tag(OWN);
  t.onBraceletFrame(BRACELET, -70, bf(0, 0x1234), 0);
  assert.equal(t.isRinging(0), false);
  t.onBraceletFrame(BRACELET, -70, bf(0, TAG_OWN), 100);
  assert.equal(t.isRinging(100), true);
  assert.equal(t.isRinging(100 + C.TAG_STALE_MS), false);
});

test("acquittement : silence tant que désignée, réarmé quand le bracelet cesse de la désigner", () => {
  const t = tag(OWN);
  t.onBraceletFrame(BRACELET, -70, bf(0, TAG_OWN), 0);
  assert.equal(t.button(0), "acked");
  t.onBraceletFrame(BRACELET, -70, bf(0, TAG_OWN), 500);
  assert.equal(t.isRinging(500), false);
  t.onBraceletFrame(BRACELET, -70, bf(0, 0), 1000);
  t.onBraceletFrame(BRACELET, -70, bf(0, TAG_OWN), 1500);
  assert.equal(t.isRinging(1500), true);
});

test("bouton hors sonnerie → flash d'état", () => {
  assert.equal(tag(OWN).button(0), "flash");
});

test("motif bip-bip 100/100/100/700 ms", () => {
  const on = [];
  for (let ms = 0; ms < 1000; ms += 50) if (K.beepOn(5000 + ms)) on.push(ms);
  assert.deepEqual(on, [0, 50, 200, 250]);
});

test("pile faible sous 15 %", () => {
  assert.equal(K.isLowBattery(14), true);
  assert.equal(K.isLowBattery(15), false);
  assert.equal(K.isLowBattery(-1), false);
});
