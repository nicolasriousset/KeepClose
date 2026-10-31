#!/usr/bin/env node
/*
 * Exécute le firmware balise généré (puck/dist/kc-tag.js) dans le VRAI interpréteur Espruino.
 * Il n'existe pas d'émulateur Puck.js : on utilise l'interpréteur de l'émulateur Bangle.js 2
 * (même moteur JavaScript) et on intercepte LEDs, piézo, bouton, BLE et pile.
 * Vérifie la compatibilité Espruino du code et rejoue les scénarios des specs côté balise.
 *
 *   node firmware/tools/emulate-puck.js <chemin/vers/EspruinoWebIDE/emu>
 *   (prérequis : voir emulate-bangle.js)
 */
const path = require("path");
const K = require("../common/kccommon.js");

const DIST = path.resolve(__dirname, "../puck/dist/kc-tag.js");
const BRACELET = "11:22:33:44:55:66 random";
const TAG_ADDR = "aa:bb:cc:dd:ee:01 random";
const OWN = K.crc16(TAG_ADDR);

require("./lib/espruino-emu").start(process.argv[2], "emulate-puck.js").then(async (emu) => {
  const { send, evalJSON, check, noErrors, sleep } = emu;
  // Trame bracelet injectée pendant les fenêtres de scan uniquement
  const inject = (rssi, alert, ring) =>
    "if(global.FEED)clearInterval(FEED);global.FEED=setInterval(function(){if(T.scanning)T.scanCb({id:" + JSON.stringify(BRACELET) +
    ",rssi:" + rssi + ",manufacturer:0xFFFF,manufacturerData:" + JSON.stringify(K.encodeBracelet(alert, ring)) + "});},100)";
  const stopFeed = "if(global.FEED)clearInterval(FEED);global.FEED=undefined";
  const pulses = async (color) => evalJSON("T.pulses.filter(function(p){return p[0]==" + JSON.stringify(color) + "})");

  try {
    // ── Matériel Puck.js simulé ─────────────────────────────────────────────
    await noErrors("interception LEDs / piézo / bouton / BLE / pile",
      'require("Storage").erase("kctag.json");' +
      'global.T={pulses:[],pwm:[],adv:null,scanCb:null,scanning:false,btn:null,batt:80};' +
      'global.LED1=D3;global.LED2=D4;global.LED3=D5;' +
      'global.digitalPulse=function(p,v,t){T.pulses.push([p==LED1?"red":p==LED2?"green":p==LED3?"blue":String(p),t]);};' +
      'global.analogWrite=function(p,v,o){T.pwm.push([String(p),v,o&&o.freq]);T.piezo=true;};' +
      'global.digitalWrite=function(p,v){if(p==D1&&!v)T.piezo=false;};' +
      'global.setWatch=function(cb,pin,o){T.btn=cb;T.btnOpts=o;};' +
      'NRF.setAdvertising=function(d,o){T.adv=o;};' +
      'NRF.setScan=function(cb,o){T.scanCb=cb||T.scanCb;T.scanning=!!cb;if(cb)T.scanOpts=o;};' +
      'NRF.getAddress=function(){return ' + JSON.stringify(TAG_ADDR) + ';};' +
      'NRF.getSecurityStatus=function(){return {connected:false};};' +
      'E.getBattery=function(){return T.batt;};');

    // ── Démarrage ───────────────────────────────────────────────────────────
    await emu.upload("kctag-dist.js", DIST);
    await noErrors("firmware balise (kccommon inliné) s'exécute", 'eval(require("Storage").read("kctag-dist.js"))', 500);
    check("trame balise neuve : KC type 2, propriétaire 0",
      JSON.stringify(await evalJSON("T.adv.manufacturerData")) === JSON.stringify(K.encodeTag(0)));
    check("advertising 500 ms", (await evalJSON("T.adv.interval")) === 500);
    check("bouton : anti-rebond 50 ms, front montant", JSON.stringify(await evalJSON("T.btnOpts")) === JSON.stringify({ edge: "rising", debounce: 50, repeat: true }));
    check("CRC de la balise identique Node / Espruino", (await evalJSON("ownCrc")) === OWN);

    await sleep(2800);
    check("flash bleu (non appairée), ≤ 50 ms", (await pulses("blue")).some((p) => p[1] <= 50));
    check("scan filtré sur manufacturer 0xFFFF", JSON.stringify(await evalJSON("T.scanOpts.filters")) === JSON.stringify([{ manufacturerData: { 65535: {} } }]));

    // ── Appairage par contact ───────────────────────────────────────────────
    await send(inject(-30, 0, 0));
    await sleep(3000);
    check("adopte le bracelet au contact", (await evalJSON("tag.state.owner")) === K.crc16(BRACELET));
    check("propriétaire persisté (kctag.json)", (await evalJSON('require("Storage").readJSON("kctag.json").owner')) === K.crc16(BRACELET));
    check("trame mise à jour avec le propriétaire",
      JSON.stringify(await evalJSON("T.adv.manufacturerData")) === JSON.stringify(K.encodeTag(K.crc16(BRACELET))));
    check("LED verte 1 s à l'adoption", (await pulses("green")).some((p) => p[1] === 1000));

    // ── Sonnerie demandée par le bracelet (de loin) ─────────────────────────
    await send(inject(-75, 0, OWN) + ";T.pwm=[]");
    await sleep(3000);
    check("sonne en moins de 3 s", (await evalJSON("tag.isRinging(Date.now())")) === true);
    check("piézo piloté à 4 kHz", (await evalJSON("T.pwm.length>0&&T.pwm[0][2]")) === 4000);

    // ── Acquittement par le bouton ──────────────────────────────────────────
    await noErrors("appui bouton pendant la sonnerie", "T.btn()", 300);
    await sleep(300);
    check("acquittée : silence", (await evalJSON("[tag.isRinging(Date.now()),T.piezo]")).join() === "false,false");
    await send(inject(-75, 0, 0));
    await sleep(2800);
    await send(inject(-75, 0, OWN));
    await sleep(2800);
    check("réarmée : sonne à nouveau quand le bracelet la redésigne", (await evalJSON("tag.isRinging(Date.now())")) === true);

    // ── Perte du bracelet ───────────────────────────────────────────────────
    await send(stopFeed);
    await sleep(6500);
    check("bracelet perdu 6 s → plus de sonnerie", (await evalJSON("[tag.isRinging(Date.now()),T.piezo]")).join() === "false,false");
    check("bracelet perdu → couleur rouge", (await evalJSON("tag.color(Date.now())")) === "red");

    // ── Ordre d'un bracelet étranger ────────────────────────────────────────
    await send("if(global.FEED)clearInterval(FEED);global.FEED=setInterval(function(){if(T.scanning)T.scanCb({id:\"66:55:44:33:22:11 random\",rssi:-75,manufacturer:0xFFFF,manufacturerData:" +
      JSON.stringify(K.encodeBracelet(OWN, OWN)) + "});},100)");
    await sleep(2800);
    check("ordre d'un autre bracelet ignoré", (await evalJSON("[tag.isRinging(Date.now()),tag.state.owner]")).join() === "false," + K.crc16(BRACELET));
    await send(stopFeed);

    // ── Bouton hors sonnerie, pile faible ───────────────────────────────────
    await send("T.pulses=[];T.btn()");
    check("bouton hors sonnerie → flash d'état immédiat", (await evalJSON("T.pulses.length")) >= 1);
    await send("T.batt=10;T.pulses=[]");
    await sleep(2900);
    check("pile < 15 % → double flash rouge", (await pulses("red")).some((p) => Array.isArray(p[1]) && p[1].length === 3));

    const mem = await evalJSON("process.memory()");
    console.log("Mémoire : " + mem.usage + " blocs utilisés (un Puck.js en a ~2500 au total)");
    check("tient dans la mémoire d'un Puck.js (< 60 % de 2500 blocs)", mem.usage < 1500, mem);
  } catch (e) {
    emu.fail(e);
  }
  emu.finish("émulation Puck");
});
