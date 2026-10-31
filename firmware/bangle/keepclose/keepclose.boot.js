/*
 * KeepClose — surveillance en arrière-plan (Bangle.js 2)
 * Relancé à chaque chargement d'app : l'état durable vit dans keepclose.json.
 * Expose l'objet global KC (état + actions) au widget, à l'app et aux réglages.
 * Événement : KC.emit("change") à chaque changement visible.
 */
(function () {
  var S = require("Storage");
  var KCC = require("kccommon");
  var C = KCC.C;
  var FILE = "keepclose.json";

  var TICK_MS     = 500;
  var WATCHDOG_MS = 10000;  // relance scan/advertising si écrasés par une autre app
  var ADV_MS      = 200;

  var KC = global.KC = {};
  var saveTimer, alertTimer, lastFrameAt = 0, lastStatus, advData;

  // ── Persistance (écriture différée et seulement sur changement) ────────────
  function persist(data) {
    if (saveTimer) clearTimeout(saveTimer);
    saveTimer = setTimeout(function () { saveTimer = undefined; S.writeJSON(FILE, data); }, 1000);
  }

  // ── Retours haptiques non bloquants ────────────────────────────────────────
  // seq : [on, off, on, off, …] en ms
  function buzzSeq(seq) {
    var p = Promise.resolve();
    seq.forEach(function (ms, i) {
      p = p.then(function () {
        return i % 2 ? new Promise(function (r) { setTimeout(r, ms); }) : Bangle.buzz(ms);
      });
    });
    return p;
  }
  KC.buzzSeq = buzzSeq;

  function onEvent(ev, data) {
    if (ev === "persist") return persist(data);
    if (ev === "paired") { KC.pairedUntil = Date.now() + 5000; buzzSeq([100, 150, 100, 150, 100]); }
    if (ev === "tripEnd") buzzSeq([500, 300, 500]);
    KC.emit("change");
  }

  var core = KC.core = KCC.createBracelet(KCC.crc16(NRF.getAddress()), S.readJSON(FILE, 1), onEvent);
  KC.pairedUntil = 0;

  // ── BLE ────────────────────────────────────────────────────────────────────
  function onDevice(d) {
    var f = KCC.decode(d.manufacturer, d.manufacturerData);
    if (!f || f.type !== C.TYPE_TAG) return;
    lastFrameAt = Date.now();
    core.onTagFrame(d.id, d.rssi, f, lastFrameAt);
  }

  function startScan() {
    NRF.setScan(onDevice, { filters: [{ manufacturerData: { 0xFFFF: {} } }] });
  }

  // Les données de service partagées des autres apps (Bangle.bleAdvert) sont conservées,
  // la trame KC passe par les options manufacturer ; le nom système est conservé.
  function applyAdvertising() {
    var opts = { interval: ADV_MS, whenConnected: true };
    if (advData) { opts.manufacturer = C.MFR_ID; opts.manufacturerData = advData; }
    NRF.setAdvertising(Bangle.bleAdvert || {}, opts);
  }

  // ── Boucle ─────────────────────────────────────────────────────────────────
  function setAlert(on) {
    if (on && !alertTimer) {
      Bangle.buzz(200);
      alertTimer = setInterval(function () { Bangle.buzz(200); }, 1000);
    } else if (!on && alertTimer) {
      clearInterval(alertTimer);
      alertTimer = undefined;
    }
  }

  function tick() {
    var now = Date.now();
    var r = core.tick(now);
    var data = core.state.enabled ? KCC.encodeBracelet(r.alertCrc, r.ringCrc) : undefined;
    if (String(data) !== String(advData)) { advData = data; applyAdvertising(); }
    setAlert(r.vibrate);
    var st = KC.status();
    if (st !== lastStatus) { lastStatus = st; KC.emit("change"); }
  }

  function watchdog() {
    if (!core.state.enabled) return;
    if (Date.now() - lastFrameAt > WATCHDOG_MS) startScan();
    applyAdvertising();
  }

  // ── API publique ───────────────────────────────────────────────────────────
  // "off" | "pairing" | "none" | "ok" | "out"
  KC.status = function () {
    if (!core.state.enabled) return "off";
    var now = Date.now();
    if (now < KC.pairedUntil) return "pairing";
    return core.status(now);
  };

  KC.action = function () {
    var res = core.action(Date.now());
    buzzSeq(res === "noBeacon" ? [50, 300, 800] : [50]);
    tick();
    KC.emit("change");
    return res;
  };

  KC.setEnabled = function (on) {
    core.setEnabled(on);
    if (on) startScan(); else { NRF.setScan(); setAlert(false); }
    tick();
    KC.emit("change");
  };
  KC.setMode = function (m) { core.setMode(m); tick(); KC.emit("change"); };
  KC.newTrip = function () { core.newTrip(); tick(); KC.emit("change"); };
  KC.forget  = function () { core.forget(); tick(); KC.emit("change"); };
  KC.list    = function () { return core.list(Date.now()); };

  // ── Démarrage ──────────────────────────────────────────────────────────────
  if (core.state.enabled) startScan();
  setInterval(tick, TICK_MS);
  setInterval(watchdog, WATCHDOG_MS);
  Bangle.on("tap", function (e) { if (e.double) KC.action(); });
  tick();
})();
