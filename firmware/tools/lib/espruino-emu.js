/*
 * Pilote Node de l'interpréteur Espruino réel (émulateur emscripten Bangle.js 2 du Web IDE).
 * Utilisé par emulate-bangle.js et emulate-puck.js.
 *
 *   const emu = await require("./lib/espruino-emu").start(dossierEmu);
 *   await emu.noErrors("libellé", "code Espruino");
 *   emu.check("libellé", (await emu.evalJSON("expr")) === 42);
 *   emu.finish("nom du scénario");
 */
const fs = require("fs"), path = require("path"), vm = require("vm"), zlib = require("zlib");

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function usage(script) {
  console.error("usage : node firmware/tools/" + script + " <EspruinoWebIDE/emu>\n" +
    "  git clone --depth 1 --filter=blob:none --sparse https://github.com/espruino/EspruinoWebIDE.git\n" +
    "  (cd EspruinoWebIDE && git sparse-checkout set emu)");
  process.exit(2);
}

function start(EMU, script) {
  if (!EMU || !fs.existsSync(path.join(EMU, "emulator_banglejs2.js"))) usage(script);

  let output = [];
  const ctx = vm.createContext({
    console, setTimeout, clearTimeout, setInterval, clearInterval, process, require, Buffer,
    TextDecoder, TextEncoder, URL, WebAssembly, performance,
    jsUpdateGfx: () => {},
    onConsoleOutput: (l) => { output.push(l); if (process.env.EMU_VERBOSE) console.log("  emu> " + l); },
    __dirname: EMU, __filename: path.join(EMU, "emulator_banglejs2.js")
  });
  for (const f of ["emu_banglejs2.js", "emulator_banglejs2.js", "common.js"])
    vm.runInContext(fs.readFileSync(path.join(EMU, f), "utf8"), ctx, { filename: f });
  ctx.jsRXCallback = undefined; // pas de fenêtre hôte

  let failures = 0;

  const emu = {
    sleep,

    // Envoie une ligne au REPL et retourne la sortie produite
    async send(code, waitMs) {
      output = [];
      ctx.jsTransmitString("\x10" + code + "\n"); // \x10 : pas d'écho
      await sleep(waitMs || 150);
      return output.join("\n");
    },

    // Évalue une expression et retourne sa valeur JSON
    async evalJSON(expr) {
      const out = await emu.send('print("@@"+JSON.stringify(' + expr + '))');
      const line = out.split("\n").find((l) => l.startsWith("@@"));
      if (!line) throw new Error("pas de résultat pour " + expr + "\n" + out);
      return JSON.parse(line.slice(2));
    },

    check(label, ok, detail) {
      console.log((ok ? "  ok    " : "  ÉCHEC ") + label + (ok || detail === undefined ? "" : " → " + JSON.stringify(detail)));
      if (!ok) failures++;
    },

    async noErrors(label, code, waitMs) {
      const out = await emu.send(code, waitMs);
      const bad = /Uncaught|Error|Not a function|not implemented|undefined is not/i.test(out);
      emu.check(label, !bad, out.trim());
      return out;
    },

    // Écrit un fichier local dans le Storage de l'émulateur
    async upload(name, file) {
      const b64 = fs.readFileSync(file).toString("base64");
      await emu.send("require('Storage').write(" + JSON.stringify(name) + ",atob(" + JSON.stringify(b64) + "))", 300);
    },

    // Capture de l'écran 176×176 en PNG, si EMU_SHOTS=<dossier>
    screenshot(name) {
      const dir = process.env.EMU_SHOTS;
      if (!dir) return;
      const W = 176, rgba = new Uint8Array(W * W * 4);
      ctx.jsGetGfxContents(rgba);
      const raw = Buffer.alloc((W * 3 + 1) * W);
      for (let y = 0; y < W; y++) for (let x = 0; x < W; x++)
        for (let c = 0; c < 3; c++) raw[y * (W * 3 + 1) + 1 + x * 3 + c] = rgba[(y * W + x) * 4 + c];
      const table = [];
      for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; table[n] = c >>> 0; }
      const crc = (b) => { let c = 0xFFFFFFFF; for (const x of b) c = table[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; };
      const chunk = (t, d) => {
        const l = Buffer.alloc(4); l.writeUInt32BE(d.length);
        const td = Buffer.concat([Buffer.from(t), d]);
        const c = Buffer.alloc(4); c.writeUInt32BE(crc(td));
        return Buffer.concat([l, td, c]);
      };
      const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(W, 0); ihdr.writeUInt32BE(W, 4); ihdr[8] = 8; ihdr[9] = 2;
      fs.mkdirSync(dir, { recursive: true });
      fs.writeFileSync(path.join(dir, name + ".png"), Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]),
        chunk("IHDR", ihdr), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]));
    },

    fail(e) { console.error(e); failures++; },

    finish(name) {
      console.log(failures ? failures + " échec(s)" : name + " : OK");
      process.exit(failures ? 1 : 0);
    }
  };

  return new Promise((resolve) => {
    ctx.Module.onRuntimeInitialized = async () => {
      ctx.jsInit();
      ctx.jsIdle();
      await sleep(500);
      const probe = await emu.send("fonctionInexistante()");
      if (!/Uncaught|Error/i.test(probe)) throw new Error("le détecteur d'erreurs ne voit pas les exceptions : " + probe);
      console.log("Espruino " + (await emu.evalJSON("process.env.VERSION")) + " (interpréteur de l'émulateur Bangle.js 2)");
      resolve(emu);
    };
  });
}

module.exports = { start };
