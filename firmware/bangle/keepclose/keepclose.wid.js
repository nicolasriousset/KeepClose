/*
 * KeepClose — widget d'état
 *   bleu  + cercle vide      : aucune balise appairée
 *   vert  + cercle plein + ✓ : toutes les balises à portée (et 5 s après un appairage)
 *   rouge + triangle + !     : au moins une balise hors de portée
 *   gris  + tiret            : KeepClose désactivé
 */
(function () {
  if (!global.KC) return;

  function draw() {
    if (this.x === undefined) return;  // widgets pas encore positionnés
    var x = this.x + 12, y = this.y + 12;
    var st = KC.status();
    g.reset().clearRect(this.x, this.y, this.x + 23, this.y + 23);
    if (st === "out") {
      g.setColor("#f00").fillPoly([x, y - 10, x + 11, y + 9, x - 11, y + 9]);
      g.setColor("#fff").fillRect(x - 1, y - 4, x + 1, y + 3).fillRect(x - 1, y + 5, x + 1, y + 7);
    } else if (st === "ok" || st === "pairing") {
      g.setColor("#0f0").fillCircle(x, y, 10);
      g.setColor("#000").drawLine(x - 5, y, x - 1, y + 4).drawLine(x - 1, y + 4, x + 5, y - 4)
        .drawLine(x - 5, y + 1, x - 1, y + 5).drawLine(x - 1, y + 5, x + 5, y - 3);
    } else if (st === "none") {
      g.setColor("#00f").drawCircle(x, y, 10).drawCircle(x, y, 9);
    } else {
      g.setColor("#888").fillRect(x - 8, y - 1, x + 8, y + 1);
    }
  }

  WIDGETS.keepclose = { area: "tr", width: 24, draw: draw };
  KC.on("change", function () { WIDGETS.keepclose.draw(); });
})();
