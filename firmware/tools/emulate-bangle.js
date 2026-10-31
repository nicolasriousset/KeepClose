#!/usr/bin/env node
/*
 * Exécute l'app Bangle.js 2 dans le VRAI interpréteur Espruino (émulateur emscripten du Web IDE),
 * sous Node. Vérifie la compatibilité Espruino du code (syntaxe, API) et rejoue un scénario N.I.C.O.
 * Le BLE n'existe pas dans l'émulateur : NRF.setScan / setAdvertising et Bangle.buzz sont interceptés.
 *
 * Prérequis : le dossier emu/ du dépôt EspruinoWebIDE, par exemple
 *   git clone --depth 1 --filter=blob:none --sparse https://github.com/espruino/EspruinoWebIDE.git
 *   (cd EspruinoWebIDE && git sparse-checkout set emu)
 *
 *   node firmware/tools/emulate-bangle.js <chemin/vers/EspruinoWebIDE/emu>
 */
const fs = require("fs"), path = require("path");

const APP = path.resolve(__dirname, "../bangle/keepclose");
const meta = JSON.parse(fs.readFileSync(path.join(APP, "metadata.json"), "utf8"));

require("./lib/espruino-emu").start(process.argv[2], "emulate-bangle.js").then(async (emu) => {
  const { send, evalJSON, check, noErrors, sleep } = emu, screenshot = emu.screenshot;
  try {
    // ── Installation des fichiers comme le ferait l'App Loader ──────────────
    for (const s of meta.storage) {
      const file = path.join(APP, s.url);
      if (s.evaluate) await send("require('Storage').write(" + JSON.stringify(s.name) + "," + fs.readFileSync(file, "utf8").trim() + ")");
      else await emu.upload(s.name, file);
    }
    const files = await evalJSON('require("Storage").list(/keepclose|kccommon/)');
    check("fichiers installés : " + files.length + "/" + meta.storage.length, files.length === meta.storage.length, files);

    // ── Interception du matériel absent de l'émulateur ──────────────────────
    await noErrors("interception BLE / vibreur",
      'global.T={buzz:[],adv:null,scan:null};' +
      'NRF.setScan=function(cb){T.scan=cb;};' +
      'NRF.setAdvertising=function(d,o){T.adv=o;};' +
      'NRF.getAddress=function(){return "11:22:33:44:55:66 random";};' +
      'Bangle.buzz=function(ms){T.buzz.push(ms);return new Promise(function(r){setTimeout(r,ms||200);});};');

    // ── Module partagé, code boot ───────────────────────────────────────────
    await noErrors("require('kccommon') dans Espruino", 'var KCC=require("kccommon");print(KCC.crc16("31:32:33:34:35:36:37:38:39"))');
    check("CRC de référence dans Espruino = 0x29B1", (await evalJSON('KCC.crc16("31:32:33:34:35:36:37:38:39")')) === 0x29B1);
    await noErrors("keepclose.boot.js s'exécute", 'eval(require("Storage").read("keepclose.boot.js"))', 600);
    check("objet global KC créé, état 'none'", (await evalJSON("KC.status()")) === "none");
    check("scan démarré", await evalJSON("typeof T.scan") === "function");

    // ── Scénario : appairage par contact ────────────────────────────────────
    const TAG = "aa:bb:cc:dd:ee:01 random";
    const frame = (rssi) => 'T.scan({id:"' + TAG + '",rssi:' + rssi + ',manufacturer:0xFFFF,manufacturerData:KCC.encodeTag(0)})';
    await noErrors("trames au contact", "var iv=setInterval(function(){" + frame(-30) + "},100)", 3800);
    await send("clearInterval(iv)");
    check("balise appairée après 3 s de contact", (await evalJSON('KC.core.isPaired("' + TAG + '")')) === true);
    check("état 'pairing' (vert 5 s)", (await evalJSON("KC.status()")) === "pairing");
    await sleep(1200);
    check("3 vibrations de confirmation", JSON.stringify(await evalJSON("T.buzz.slice(0,3)")) === "[100,100,100]");
    check("keepclose.json persisté", (await evalJSON('require("Storage").readJSON("keepclose.json").paired')).length === 1);

    // ── Alerte : balise à 2 m ───────────────────────────────────────────────
    const at = (d) => Math.round(-61 - 20 * Math.log10(d));
    await send("T.buzz=[];var iv=setInterval(function(){" + frame(at(2)) + "},100)");
    await sleep(4000);
    check("état 'out' à 2 m", (await evalJSON("KC.status()")) === "out");
    check("vibrations d'alerte 200 ms", (await evalJSON("T.buzz.filter(function(m){return m==200}).length")) >= 2);
    check("trame bracelet : balise en alerte publiée",
      JSON.stringify(await evalJSON("T.adv.manufacturerData")) === JSON.stringify([0x4B, 0x43, 1].concat(await evalJSON('(function(c){return [c&255,c>>8,0,0]})(KCC.crc16("' + TAG + '"))'))));
    check("advertising whenConnected", (await evalJSON("T.adv.whenConnected")) === true);

    // ── Double tap → snooze ─────────────────────────────────────────────────
    await noErrors("double tap", 'Bangle.emit("tap",{double:true,dir:"front"})', 300);
    await send("T.buzz=[]");
    await sleep(2500);
    check("silence après snooze", (await evalJSON("T.buzz.length")) === 0);
    await send("clearInterval(iv)");

    // ── Widget, app, réglages ───────────────────────────────────────────────
    await noErrors("widget chargé et dessiné", "Bangle.loadWidgets();Bangle.drawWidgets();WIDGETS.keepclose.draw()", 400);
    await noErrors("app plein écran", 'eval(require("Storage").read("keepclose.app.js"))', 1200);
    screenshot("1-app-alerte-snoozee");
    await noErrors("app : bouton d'action tactile", 'Bangle.emit("touch",1,{x:88,y:160,type:0})', 400);
    await noErrors("app : bouton d'action → sonnerie", "0", 300);
    screenshot("2-app-apres-action");
    await noErrors("app : vue débogage", 'Bangle.emit("swipe",1,0)', 400);
    screenshot("3-app-debug");
    await noErrors("réglages ouverts depuis l'app (haut de l'écran)", 'Bangle.emit("touch",1,{x:88,y:30,type:0})', 1500);
    screenshot("4-reglages");
    await noErrors("réglages : mode Ti Poucet", 'KC.setMode("trail")', 300);
    check("mode Ti Poucet persisté", (await evalJSON("KC.core.toJSON().mode")) === "trail");
    await noErrors("réglages : désactivation", "KC.setEnabled(false)", 300);
    check("état 'off', plus de trame KC", (await evalJSON("[KC.status(), T.adv.manufacturerData===undefined]")).join() === "off,true");

    const mem = await evalJSON("process.memory()");
    console.log("Mémoire : " + mem.usage + "/" + mem.total + " blocs utilisés");
    check("mémoire < 50 % après app + widget", mem.usage < mem.total / 2, mem);
  } catch (e) {
    emu.fail(e);
  }
  emu.finish("émulation Bangle");
});
