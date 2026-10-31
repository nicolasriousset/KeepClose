#!/usr/bin/env node
/*
 * KeepClose — génération des fichiers installables (Node, sans dépendance)
 *
 *   node firmware/tools/build.js
 *
 *   1. firmware/bangle/keepclose/kccommon.js   ← copie de firmware/common/kccommon.js
 *   2. firmware/bangle/keepclose/app.png       ← icône App Loader (48×48)
 *      firmware/bangle/keepclose/app-icon.js   ← même icône, format image Espruino
 *   3. firmware/puck/dist/kc-tag.js            ← firmware balise, kccommon inliné
 *
 * Les sorties sont commitées : le build est déterministe (relancer = aucun diff).
 */
const fs = require("fs");
const path = require("path");
const zlib = require("zlib");

const ROOT = path.resolve(__dirname, "..");
const COMMON = path.join(ROOT, "common", "kccommon.js");
const BANGLE = path.join(ROOT, "bangle", "keepclose");
const PUCK_SRC = path.join(ROOT, "puck", "kc-tag.js");
const PUCK_DIST = path.join(ROOT, "puck", "dist", "kc-tag.js");
const HEADER = "// FICHIER GÉNÉRÉ par firmware/tools/build.js — ne pas modifier à la main\n";

function write(file, content) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  const prev = fs.existsSync(file) ? fs.readFileSync(file) : null;
  const buf = Buffer.isBuffer(content) ? content : Buffer.from(content, "utf8");
  if (prev && prev.equals(buf)) return console.log("  inchangé  " + path.relative(ROOT, file));
  fs.writeFileSync(file, buf);
  console.log("  écrit     " + path.relative(ROOT, file));
}

// ── 1. Module partagé pour la Bangle ─────────────────────────────────────────
const common = fs.readFileSync(COMMON, "utf8").replace(/\r\n/g, "\n");
write(path.join(BANGLE, "kccommon.js"), HEADER + common);

// ── 2. Icône : anneau (bracelet) + point central (balise) ────────────────────
const W = 48;
const on = (x, y) => {
  const r = Math.hypot(x - 23.5, y - 23.5);
  return (r >= 16 && r <= 21) || r <= 6;
};
// Image Espruino : [largeur, hauteur, bpp] + bits ligne par ligne, MSB en premier
const bits = [];
for (let y = 0; y < W; y++)
  for (let x = 0; x < W; x += 8) {
    let byte = 0;
    for (let b = 0; b < 8; b++) if (on(x + b, y)) byte |= 0x80 >> b;
    bits.push(byte);
  }
const img = Buffer.from([W, W, 1].concat(bits));
write(path.join(BANGLE, "app-icon.js"), 'atob("' + img.toString("base64") + '")\n');

// PNG 48×48 niveaux de gris 1 bit (pixel allumé = noir)
function crc32(buf) {
  let c, crc = 0xFFFFFFFF;
  for (let n = 0; n < buf.length; n++) {
    c = (crc ^ buf[n]) & 0xFF;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
    crc = (crc >>> 8) ^ c;
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
}
function chunk(type, data) {
  const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type, "ascii"), data]);
  const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}
const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(W, 0); ihdr.writeUInt32BE(W, 4);
ihdr[8] = 1; ihdr[9] = 0; // 1 bit, niveaux de gris
const raw = [];
for (let y = 0; y < W; y++) {
  raw.push(0); // filtre "none"
  for (let i = 0; i < W / 8; i++) raw.push(~bits[y * (W / 8) + i] & 0xFF);
}
const png = Buffer.concat([
  Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]),
  chunk("IHDR", ihdr),
  chunk("IDAT", zlib.deflateSync(Buffer.from(raw), { level: 9 })),
  chunk("IEND", Buffer.alloc(0))
]);
write(path.join(BANGLE, "app.png"), png);

// ── 3. Firmware Puck.js avec kccommon inliné ─────────────────────────────────
if (!fs.existsSync(PUCK_SRC)) {
  console.log("  ignoré    " + path.relative(ROOT, PUCK_SRC) + " absent");
} else {
  const src = fs.readFileSync(PUCK_SRC, "utf8").replace(/\r\n/g, "\n");
  const marker = 'require("kccommon")';
  if (!src.includes(marker)) throw new Error(PUCK_SRC + " : " + marker + " introuvable");
  const inlined = "(function () {\nvar exports = {};\n" + common + "return exports;\n})()";
  write(PUCK_DIST, HEADER + src.replace(marker, () => inlined));
}
