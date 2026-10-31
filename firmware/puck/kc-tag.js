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
var KCC = require("kccommon");
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
// Espruino exécute le code au fil de l'eau (pas de hoisting global) : `tag` est créé dans start(),
// une fois toutes les fonctions déclarées.
var storage = require("Storage");
var ownCrc  = KCC.crc16(NRF.getAddress());
var tag, beepTimer, piezoOn = false, cycles = 0;

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
  tag = KCC.createTag(ownCrc, (storage.readJSON(STORE, 1) || {}).owner || 0, onTagEvent);
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
