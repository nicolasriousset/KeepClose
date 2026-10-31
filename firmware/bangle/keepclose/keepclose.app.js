/*
 * KeepClose — app plein écran
 *   Haut     : mode, batterie de la montre, balises triées par distance
 *   Bas      : grand bouton d'action (identique au double tap et à BTN1)
 *   Glisser ← → : bascule vue normale / vue débogage RSSI (calibration)
 *   Toucher le haut de l'écran : réglages
 */
Bangle.loadWidgets();
Bangle.drawWidgets();

if (!global.KC) { E.showMessage("KeepClose inactif\nRedemarrer la montre"); throw "KC absent"; }

var debug = false, inMenu = false;
var R = Bangle.appRect;
var ACTION_Y = R.y + R.h - 50;

function fmtDist(b) { return b.dist === null ? "hors portee" : b.dist.toFixed(1) + " m"; }

function flags(b) {
  return (b.ringing ? " SON" : "") + (b.snoozed ? " zZ" : "") + (b.reached ? " OK" : "");
}

function actionLabel() {
  var s = KC.core.state;
  if (!s.enabled) return "Desactive";
  if (s.mode === "trail") return s.paused ? "Reprendre" : "Pause";
  if (s.ringCrc) return "Arreter son";
  return KC.status() === "out" ? "Silence" : "Sonner";
}

function draw() {
  if (inMenu) return;
  var s = KC.core.state, list = KC.list();
  g.reset().clearRect(R.x, R.y, R.x2, R.y2);

  g.setFont("Vector", 18).setFontAlign(-1, -1);
  g.drawString((s.mode === "trail" ? "Ti Poucet" : "N.I.C.O.") + (debug ? " dbg" : ""), R.x + 4, R.y + 2);
  g.setFontAlign(1, -1).drawString(E.getBattery() + "%", R.x2 - 4, R.y + 2);

  g.setFontAlign(-1, -1);
  var y = R.y + 26;
  if (!list.length) g.setFont("Vector", 20).drawString("Aucune balise\nColler 3 s", R.x + 4, y);
  list.slice(0, 4).forEach(function (b) {
    var line = debug
      ? b.addr.substr(12, 5) + " " + (b.rssi === null ? "--" : b.rssi) + "/" + (b.ema === null ? "--" : Math.round(b.ema))
      : fmtDist(b) + flags(b);
    g.setColor(b.out ? "#f00" : "#0a0").setFont("Vector", debug ? 18 : 22).drawString(line, R.x + 4, y);
    y += 24;
  });

  g.setColor(g.theme.fg).fillRect(R.x + 4, ACTION_Y, R.x2 - 4, R.y2 - 4);
  g.setColor(g.theme.bg).setFont("Vector", 26).setFontAlign(0, 0)
    .drawString(actionLabel(), (R.x + R.x2) / 2, (ACTION_Y + R.y2) / 2);
}

function act() { KC.action(); draw(); }

function openSettings() {
  var settings = eval(require("Storage").read("keepclose.settings.js"));
  inMenu = true;
  settings(function () { inMenu = false; setupUI(); draw(); });
}

function setupUI() {
  Bangle.setUI({
    mode: "custom",
    btn: act,
    touch: function (_, e) {
      if (e.y >= ACTION_Y) act();
      else if (e.y < R.y + 24) openSettings();
    },
    swipe: function (dir) { if (dir) { debug = !debug; draw(); } }
  });
}

setupUI();
setInterval(draw, 1000);
KC.on("change", draw);
draw();
