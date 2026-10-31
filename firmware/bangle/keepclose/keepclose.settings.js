/*
 * KeepClose — réglages (menu Réglages de la montre, ou depuis l'app)
 * Passe par l'objet global KC (code boot) ; à défaut, modifie directement keepclose.json.
 */
(function (back) {
  var FILE = "keepclose.json";
  var S = require("Storage");

  function cfg() { return global.KC ? KC.core.toJSON() : (S.readJSON(FILE, 1) || {}); }
  function patch(p) {
    var c = cfg();
    for (var k in p) c[k] = p[k];
    S.writeJSON(FILE, c);
  }

  function setEnabled(v) { if (global.KC) KC.setEnabled(v); else patch({ enabled: v }); }
  function setMode(m) {
    if (global.KC) KC.setMode(m);
    else patch({ mode: m, reached: [], snoozed: [], paused: false });
  }
  function newTrip() { if (global.KC) KC.newTrip(); else patch({ reached: [], paused: false }); }
  function forget() {
    if (global.KC) KC.forget(); else patch({ paired: [], snoozed: [], reached: [] });
  }

  function confirm(msg, fn) {
    E.showPrompt(msg, { buttons: { "Non": false, "Oui": true } }).then(function (ok) {
      if (ok) fn();
      showMenu();
    });
  }

  function showMenu() {
    var c = cfg();
    E.showMenu({
      "": { title: "KeepClose", back: back },
      "Actif": { value: c.enabled !== false, onchange: setEnabled },
      "Mode": {
        value: c.mode === "trail" ? 1 : 0, min: 0, max: 1,
        format: function (v) { return v ? "Ti Poucet" : "N.I.C.O."; },
        onchange: function (v) { setMode(v ? "trail" : "nico"); }
      },
      "Nouveau trajet": function () { newTrip(); E.showMessage("Nouveau trajet"); setTimeout(showMenu, 800); },
      "Oublier balises": function () { confirm("Oublier toutes\nles balises ?", forget); }
    });
  }

  showMenu();
})
