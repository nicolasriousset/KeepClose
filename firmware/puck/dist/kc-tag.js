// FICHIER GÉNÉRÉ par firmware/tools/build.js — ne pas modifier à la main
/*
 * KeepClose — Firmware balise
 * Hardware : Puck.js (Espruino), pile CR2032, buzzer piézo externe
 * Role     : balise posée sur un objet (canne, repère de chemin…)
 *   • émet en continu sa trame KC (type 0x02 + bracelet propriétaire)
 *   • écoute le bracelet par fenêtres courtes (autonomie) : alerte, sonnerie
 *   • LEDs en flash, bip-bip sur le piézo, bouton d'acquittement
 *
 * Ne pas envoyer ce fichier tel quel : utiliser firmware/puck/dist/kc-tag.js,
 * généré par `node firmware/tools/build.js` (kccommon inliné).
 */
var KCC = (function () {
var exports = {};
/*
 * KeepClose — logique partagée bracelet (Bangle.js 2) / balise (Puck.js)
 *
 * Module pur : aucune API matérielle, le temps (`now`, en ms) est toujours injecté.
 * Compatible Espruino (require("kccommon")) et Node (tests : node --test firmware/common/test/).
 * Écrit en JavaScript prudent pour Espruino : pas de Map/Set, pas de for...of, pas de classes.
 */

// ── Constantes ───────────────────────────────────────────────────────────────
var C = {
  MFR_ID:          0xFFFF, // company ID non assigné (usage prototype)
  MAGIC0:          0x4B,   // 'K'
  MAGIC1:          0x43,   // 'C'
  TYPE_BRACELET:   0x01,
  TYPE_TAG:        0x02,

  // Calibration RSSI — valeurs XIAO, à recalibrer sur Bangle.js 2 / Puck.js (tâche 5.1)
  PAIRING_RSSI:    -38,    // dBm, seuil "au contact"
  PAIRING_MS:      3000,   // durée de contact requise
  P1M:             -61,    // RSSI à 1 m
  PATH_LOSS_N:     2.0,
  EMA_ALPHA:       0.1,

  ALIVE_MS:        6000,   // balise vue récemment
  NEAR_M:          0.45,   // retour à portée
  FAR_M:           0.55,   // sortie de portée
  REACHED_M:       1.0,    // Ti Poucet : balise atteinte
  RING_MS:         10000,  // N.I.C.O. : durée max d'une sonnerie
  TAG_STALE_MS:    6000,   // balise : trame bracelet périmée
  LOW_BATTERY_PCT: 15
};
exports.C = C;

// ── Identifiant compact ──────────────────────────────────────────────────────

// "aa:bb:cc:dd:ee:ff" (éventuellement suivi de " public"/" random") → [0xAA, …, 0xFF]
function addrBytes(addr) {
  var parts = String(addr).split(" ")[0].split(":");
  var out = [];
  for (var i = 0; i < parts.length; i++) out.push(parseInt(parts[i], 16) & 0xFF);
  return out;
}

// CRC16-CCITT (poly 0x1021, init 0xFFFF) des 6 octets de l'adresse, ordre d'affichage
function crc16(addr) {
  var data = addrBytes(addr);
  var crc = 0xFFFF;
  for (var i = 0; i < data.length; i++) {
    crc ^= data[i] << 8;
    for (var j = 0; j < 8; j++)
      crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) & 0xFFFF : (crc << 1) & 0xFFFF;
  }
  return crc;
}
exports.crc16 = crc16;

// ── Trames KC (manufacturer data, company ID 0xFFFF) ─────────────────────────

function u16le(v) { return [v & 0xFF, (v >> 8) & 0xFF]; }

// Bracelet : [K C 0x01 | alertCrc(2 LE) | ringCrc(2 LE)]
exports.encodeBracelet = function (alertCrc, ringCrc) {
  return [C.MAGIC0, C.MAGIC1, C.TYPE_BRACELET].concat(u16le(alertCrc || 0), u16le(ringCrc || 0));
};

// Balise : [K C 0x02 | ownerCrc(2 LE)]
exports.encodeTag = function (ownerCrc) {
  return [C.MAGIC0, C.MAGIC1, C.TYPE_TAG].concat(u16le(ownerCrc || 0));
};

// Retourne {type, alert, ring} | {type, owner} | null si la trame n'est pas KeepClose
exports.decode = function (manufacturer, data) {
  if (manufacturer !== C.MFR_ID || !data) return null;
  if (typeof ArrayBuffer !== "undefined" && data instanceof ArrayBuffer) data = new Uint8Array(data);
  if (data.length < 3 || data[0] !== C.MAGIC0 || data[1] !== C.MAGIC1) return null;
  var type = data[2];
  if (type === C.TYPE_BRACELET && data.length >= 7)
    return { type: type, alert: data[3] | (data[4] << 8), ring: data[5] | (data[6] << 8) };
  if (type === C.TYPE_TAG && data.length >= 5)
    return { type: type, owner: data[3] | (data[4] << 8) };
  return null;
};

// ── Mesures ──────────────────────────────────────────────────────────────────

exports.ema = function (prev, rssi) {
  return prev === null || prev === undefined ? rssi : C.EMA_ALPHA * rssi + (1 - C.EMA_ALPHA) * prev;
};

exports.distance = function (rssi) {
  return Math.pow(10, (C.P1M - rssi) / (10 * C.PATH_LOSS_N));
};

// ── Bracelet ─────────────────────────────────────────────────────────────────

function listToSet(list) {
  var s = {};
  if (list) for (var i = 0; i < list.length; i++) s[list[i]] = true;
  return s;
}

/*
 * Machine d'état du bracelet.
 *   ownCrc : identifiant compact du bracelet
 *   saved  : état persistant (résultat de toJSON()) ou undefined
 *   emit   : function(event, data) — "paired", "unpaired", "persist", "reached", "tripEnd"
 */
exports.createBracelet = function (ownCrc, saved, emit) {
  saved = saved || {};
  emit = emit || function () {};

  var s = {
    mode:      saved.mode === "trail" ? "trail" : "nico",
    enabled:   saved.enabled !== false,
    paused:    !!saved.paused,
    paired:    listToSet(saved.paired),
    snoozed:   listToSet(saved.snoozed),
    reached:   listToSet(saved.reached),
    beacons:   {},     // addr → mesure (volatile)
    ringCrc:   0,      // N.I.C.O. : sonnerie demandée
    ringUntil: 0,
    trailRing: 0,      // Ti Poucet : balise désignée au dernier tick
    tripEndSignalled: false
  };

  function persist() { emit("persist", self.toJSON()); }

  function beacon(addr) {
    var b = s.beacons[addr];
    if (!b) b = s.beacons[addr] = { addr: addr, crc: crc16(addr), rssi: null, ema: null,
                                    dist: null, lastSeen: null, pairStart: null, out: true };
    return b;
  }

  function alive(b, now) { return b.lastSeen !== null && now - b.lastSeen < C.ALIVE_MS; }
  function measured(b, now) { return alive(b, now) && b.ema !== null; }

  // Hystérésis par balise : FAR pour sortir, NEAR pour revenir
  function updateOut(b, now) {
    if (!measured(b, now)) b.out = true;
    else b.out = b.out ? b.dist > C.NEAR_M : b.dist > C.FAR_M;
    return b.out;
  }

  function pairedBeacons() {
    var out = [];
    for (var addr in s.paired) out.push(beacon(addr));
    return out;
  }

  function sortKey(b, now) { return measured(b, now) ? b.dist : Infinity; }

  function nearest(list, now) {
    var best = null;
    for (var i = 0; i < list.length; i++)
      if (!best || sortKey(list[i], now) < sortKey(best, now)) best = list[i];
    return best;
  }

  function outOfRange(now, includeSnoozed) {
    return pairedBeacons().filter(function (b) {
      return updateOut(b, now) && (includeSnoozed || !s.snoozed[b.addr]);
    });
  }

  function ringActive(now) { return s.ringCrc !== 0 && now < s.ringUntil; }

  var self = {
    state: s,

    toJSON: function () {
      return { mode: s.mode, enabled: s.enabled, paused: s.paused,
               paired: Object.keys(s.paired), snoozed: Object.keys(s.snoozed),
               reached: Object.keys(s.reached) };
    },

    isPaired: function (addr) { return !!s.paired[addr]; },

    // Trame de balise reçue
    onTagFrame: function (addr, rssi, frame, now) {
      if (!s.enabled) return;
      var b = beacon(addr);
      b.lastSeen = now;
      b.rssi = rssi;

      if (s.paired[addr]) {
        if (frame.owner !== 0 && frame.owner !== ownCrc) {
          delete s.paired[addr]; delete s.snoozed[addr]; delete s.reached[addr];
          b.pairStart = null; b.ema = null; b.dist = null; b.out = true;
          emit("unpaired", addr);
          persist();
          return;
        }
      } else {
        if (rssi >= C.PAIRING_RSSI) {
          if (b.pairStart === null) b.pairStart = now;
          else if (now - b.pairStart >= C.PAIRING_MS) {
            s.paired[addr] = true;
            b.pairStart = null;
            emit("paired", addr);
            persist();
          }
        } else b.pairStart = null;
      }

      if (s.paired[addr]) {
        b.ema = exports.ema(b.ema, rssi);
        b.dist = exports.distance(b.ema);
      }
    },

    /*
     * À appeler périodiquement. Retourne :
     *   { alertCrc, ringCrc, vibrate }  — vibrate : alerte vibrante N.I.C.O. active
     */
    tick: function (now) {
      var res = { alertCrc: 0, ringCrc: 0, vibrate: false };

      // Oubli des balises non appairées disparues (mémoire limitée)
      for (var a in s.beacons)
        if (!s.paired[a] && !alive(s.beacons[a], now)) delete s.beacons[a];

      if (!s.enabled) return res;

      if (s.mode === "nico") {
        // Levée automatique du snooze
        var changed = false;
        for (var addr in s.snoozed) {
          var sb = beacon(addr);
          if (measured(sb, now) && sb.dist <= C.NEAR_M) { delete s.snoozed[addr]; changed = true; }
        }
        if (changed) persist();

        var oor = outOfRange(now, true);
        var alertBeacon = nearest(oor, now);
        res.alertCrc = alertBeacon ? alertBeacon.crc : 0;
        res.vibrate = oor.some(function (b) { return !s.snoozed[b.addr]; });

        if (s.ringCrc && now >= s.ringUntil) s.ringCrc = 0;
        res.ringCrc = s.ringCrc;
      } else {
        // Ti Poucet
        var candidates = [], anyAlive = false;
        pairedBeacons().forEach(function (b) {
          if (!measured(b, now)) return;
          anyAlive = true;
          if (s.reached[b.addr]) return;
          if (!s.paused && b.dist < C.REACHED_M) {
            s.reached[b.addr] = true;
            emit("reached", b.addr);
            persist();
            return;
          }
          candidates.push(b);
        });
        var target = nearest(candidates, now);
        if (target) s.tripEndSignalled = false;
        else if (anyAlive && !s.tripEndSignalled) {
          s.tripEndSignalled = true;
          emit("tripEnd");
        }
        res.ringCrc = s.trailRing = (target && !s.paused) ? target.crc : 0;
      }
      return res;
    },

    /*
     * Action utilisateur unique. Retourne ce qui a été fait :
     *   "ringCancelled" | "snoozed" | "ring" | "noBeacon" | "paused" | "resumed" | "disabled"
     */
    action: function (now) {
      if (!s.enabled) return "disabled";
      if (s.mode === "trail") {
        s.paused = !s.paused;
        persist();
        return s.paused ? "paused" : "resumed";
      }
      if (ringActive(now)) { s.ringCrc = 0; return "ringCancelled"; }

      var oor = outOfRange(now, false);
      if (oor.length) {
        oor.forEach(function (b) { s.snoozed[b.addr] = true; });
        persist();
        return "snoozed";
      }

      var target = nearest(pairedBeacons(), now);
      if (!target) return "noBeacon";
      s.ringCrc = target.crc;
      s.ringUntil = now + C.RING_MS;
      return "ring";
    },

    setMode: function (mode) {
      s.mode = mode === "trail" ? "trail" : "nico";
      s.reached = {}; s.snoozed = {}; s.paused = false;
      s.ringCrc = 0; s.tripEndSignalled = false;
      persist();
    },

    setEnabled: function (on) { s.enabled = !!on; s.ringCrc = 0; persist(); },

    newTrip: function () {
      s.reached = {}; s.paused = false; s.tripEndSignalled = false;
      persist();
    },

    forget: function () {
      s.paired = {}; s.snoozed = {}; s.reached = {};
      s.beacons = {}; s.ringCrc = 0; s.tripEndSignalled = false;
      persist();
    },

    // Balises appairées triées (distance croissante, non vivantes en dernier) — pour l'affichage
    list: function (now) {
      var crcRing = s.mode === "nico" ? s.ringCrc : s.trailRing;
      return pairedBeacons().sort(function (a, b) { return sortKey(a, now) - sortKey(b, now); })
        .map(function (b) {
          return { addr: b.addr, crc: b.crc, dist: measured(b, now) ? b.dist : null,
                   rssi: b.rssi, ema: b.ema, out: updateOut(b, now),
                   snoozed: !!s.snoozed[b.addr], reached: !!s.reached[b.addr],
                   ringing: crcRing !== 0 && crcRing === b.crc };
        });
    },

    // État global pour le widget : "none" | "ok" | "out"
    status: function (now) {
      var list = pairedBeacons();
      if (!list.length) return "none";
      return list.some(function (b) { return updateOut(b, now); }) ? "out" : "ok";
    }
  };
  return self;
};

// ── Balise ───────────────────────────────────────────────────────────────────

/*
 * Machine d'état de la balise.
 *   ownCrc   : identifiant compact de la balise
 *   ownerCrc : bracelet propriétaire persistant (0 = non appairée)
 *   emit     : function(event, data) — "adopted" (persister + LED verte 1 s)
 */
exports.createTag = function (ownCrc, ownerCrc, emit) {
  emit = emit || function () {};
  var t = { owner: ownerCrc || 0, lastSeen: null, alert: false, ringReq: false, acked: false };

  function fresh(now) { return t.lastSeen !== null && now - t.lastSeen < C.TAG_STALE_MS; }

  var self = {
    state: t,

    onBraceletFrame: function (addr, rssi, frame, now) {
      var crc = crc16(addr);
      if (rssi >= C.PAIRING_RSSI && crc !== t.owner) {
        t.owner = crc;
        t.alert = false; t.ringReq = false; t.acked = false;
        emit("adopted", crc);
      }
      if (crc !== t.owner) return;
      t.lastSeen = now;
      t.alert = frame.alert !== 0 && frame.alert === ownCrc;
      var req = frame.ring !== 0 && frame.ring === ownCrc;
      if (!req) t.acked = false; // le bracelet ne nous désigne plus → réarmer
      t.ringReq = req;
    },

    isRinging: function (now) { return t.ringReq && !t.acked && fresh(now); },

    // Appui bouton : "acked" (sonnerie acquittée) ou "flash" (afficher l'état)
    button: function (now) {
      if (self.isRinging(now)) { t.acked = true; return "acked"; }
      return "flash";
    },

    // "blue" | "green" | "red"
    color: function (now) {
      if (!t.owner) return "blue";
      return fresh(now) && !t.alert ? "green" : "red";
    }
  };
  return self;
};

// Motif bip-bip, période 1 s : 100 ms ON, 100 OFF, 100 ON, 700 OFF
exports.beepOn = function (now) {
  var t = now % 1000;
  return t < 100 || (t >= 200 && t < 300);
};

exports.isLowBattery = function (pct) { return pct >= 0 && pct < C.LOW_BATTERY_PCT; };
return exports;
})();
var C = KCC.C;

// ── Configuration ────────────────────────────────────────────────────────────
// Piézo entre PIEZO_A (PWM) et PIEZO_B (maintenue à 0) — à confirmer selon la révision (tâche 1.4)
var PIEZO_A = D1;
var PIEZO_B = D2;
var PIEZO_FREQ = 4000;

var ADV_INTERVAL_MS = 500;
var CYCLE_MS        = 2500;  // période du cycle scan + flash
var SCAN_WINDOW_MS  = 600;   // fenêtre d'écoute du bracelet dans chaque cycle
var FLASH_MS        = 40;
var HB_EVERY        = 4;     // journal console tous les N cycles (si console connectée)
var STORE           = "kctag.json";

var LEDS = { red: LED1, green: LED2, blue: LED3 };  // actives à l'état haut

// ── État ─────────────────────────────────────────────────────────────────────
var storage = require("Storage");
var ownCrc  = KCC.crc16(NRF.getAddress());
var saved   = storage.readJSON(STORE, 1) || {};
var tag     = KCC.createTag(ownCrc, saved.owner || 0, onTagEvent);
var beepTimer, piezoOn = false, cycles = 0;

function log() {
  if (NRF.getSecurityStatus().connected) console.log.apply(console, arguments);
}

function onTagEvent(ev, crc) {
  if (ev !== "adopted") return;
  storage.writeJSON(STORE, { owner: crc });
  setAdvertising();
  digitalPulse(LEDS.green, 1, 1000);
  log("[appairage] nouveau bracelet, CRC=0x" + crc.toString(16));
}

// ── BLE ──────────────────────────────────────────────────────────────────────
function setAdvertising() {
  NRF.setAdvertising({}, {
    name: "KC-Tag",
    manufacturer: C.MFR_ID,
    manufacturerData: KCC.encodeTag(tag.state.owner),
    interval: ADV_INTERVAL_MS
  });
}

function onDevice(d) {
  var f = KCC.decode(d.manufacturer, d.manufacturerData);
  if (!f || f.type !== C.TYPE_BRACELET) return;
  tag.onBraceletFrame(d.id, d.rssi, f, Date.now());
  updateBuzzer();
}

function scanWindow() {
  NRF.setScan(onDevice, { filters: [{ manufacturerData: { 0xFFFF: {} } }] });
  setTimeout(function () { NRF.setScan(); updateBuzzer(); }, SCAN_WINDOW_MS);
}

// ── Sorties ──────────────────────────────────────────────────────────────────
function piezo(on) {
  if (on === piezoOn) return;
  piezoOn = on;
  if (on) analogWrite(PIEZO_A, 0.5, { freq: PIEZO_FREQ });
  else digitalWrite(PIEZO_A, 0);
}

// Bip-bip piloté par une horloge à 50 ms, active seulement pendant la sonnerie
function updateBuzzer() {
  if (tag.isRinging(Date.now())) {
    if (beepTimer) return;
    beepTimer = setInterval(function () {
      var now = Date.now();
      if (!tag.isRinging(now)) return updateBuzzer();
      piezo(KCC.beepOn(now));
    }, 50);
  } else if (beepTimer) {
    clearInterval(beepTimer);
    beepTimer = undefined;
    piezo(false);
  }
}

function flashState() {
  digitalPulse(LEDS[tag.color(Date.now())], 1, FLASH_MS);
  if (KCC.isLowBattery(E.getBattery()))
    setTimeout(function () { digitalPulse(LEDS.red, 1, [FLASH_MS, 150, FLASH_MS]); }, 250);
}

function heartbeat() {
  var t = tag.state, now = Date.now();
  var s = "[hb tag] batt=" + E.getBattery() + "%  appaire=" + (t.owner ? "oui CRC=0x" + t.owner.toString(16) : "non");
  if (t.owner) {
    if (t.lastSeen === null) s += "  bracelet jamais detecte";
    else s += "  vu il y a " + Math.round((now - t.lastSeen) / 1000) + "s  alerte=" +
              (t.alert ? "oui" : "non") + "  ring=" + (tag.isRinging(now) ? "oui" : "non");
  }
  log(s);
}

// ── Démarrage ────────────────────────────────────────────────────────────────
function start() {
  digitalWrite(PIEZO_A, 0);
  digitalWrite(PIEZO_B, 0);
  setAdvertising();

  setWatch(function () {
    if (tag.button(Date.now()) === "acked") { updateBuzzer(); log("[btn tag] sonnerie acquittee"); }
    else flashState();
  }, BTN, { edge: "rising", debounce: 50, repeat: true });

  setInterval(function () {
    scanWindow();
    flashState();
    if (++cycles % HB_EVERY === 0) heartbeat();
  }, CYCLE_MS);

  log("KeepClose balise prete, CRC=0x" + ownCrc.toString(16));
}

start();
