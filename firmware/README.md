# Firmware KeepClose — Bangle.js 2 + Puck.js

| Dossier | Contenu |
|---|---|
| `common/kccommon.js` | Logique partagée, pure et testable : protocole BLE, CRC, distance, appairage, N.I.C.O., Ti Poucet, état de la balise |
| `common/test/` | Tests unitaires Node de `kccommon` |
| `bangle/keepclose/` | App bracelet Bangle.js 2 (`boot`, `wid`, `app`, `settings`, `metadata.json`) — voir [bangle/README.md](bangle/README.md) |
| `puck/kc-tag.js` | Source du firmware balise Puck.js — voir [puck/README.md](puck/README.md) |
| `puck/dist/kc-tag.js` | Firmware balise **à envoyer** au Puck (généré) |
| `tools/` | Build et simulation |

L'ancien firmware Arduino (XIAO BLE nRF52840) est archivé dans `../legacy/xiao-firmware/`.

## Commandes

Node ≥ 18, sans dépendance.

```sh
node --test "firmware/common/test/*.test.js"   # tests de la logique partagée
node firmware/tools/build.js                    # régénère les fichiers installables
node firmware/tools/simulate-bangle.js          # simulation du code boot Bangle (API Espruino factices)
```

### Exécution dans le vrai interpréteur Espruino

`emulate-bangle.js` et `emulate-puck.js` font tourner l'app Bangle et le firmware balise **généré** dans l'interpréteur Espruino réel, compilé en JavaScript pour l'émulateur Bangle.js 2 du Web IDE. Le BLE, le vibreur, les LEDs, le piézo et le bouton sont interceptés. Ces scripts détectent les incompatibilités que Node ne voit pas : syntaxe, API, ordre d'exécution, mémoire. Ils rejouent aussi les scénarios des specs. Ils ne remplacent pas les essais sur le matériel.

```sh
git clone --depth 1 --filter=blob:none --sparse https://github.com/espruino/EspruinoWebIDE.git /chemin/EspruinoWebIDE
(cd /chemin/EspruinoWebIDE && git sparse-checkout set emu)
node firmware/tools/emulate-bangle.js /chemin/EspruinoWebIDE/emu   # EMU_SHOTS=<dossier> : captures d'écran
node firmware/tools/emulate-puck.js  /chemin/EspruinoWebIDE/emu
```

Pièges Espruino rencontrés :

- Au niveau global, les fonctions ne sont **pas hoistées** : il faut les déclarer avant de les utiliser.
- `clearInterval()` et `clearTimeout()` appelés avec `undefined` **effacent tous les timers**. Il faut toujours tester la variable avant l'appel.

`build.js` génère :

- `bangle/keepclose/kccommon.js` : copie de `common/kccommon.js`, installée sur la montre comme module Storage `kccommon` ;
- `bangle/keepclose/app.png` et `app-icon.js` : l'icône de l'app ;
- `puck/dist/kc-tag.js` : `puck/kc-tag.js` avec `kccommon` inliné.

Les fichiers générés sont commités et le build est déterministe : relancer `build.js` sans modification affiche `inchangé` partout. **Après toute modification de `common/kccommon.js` ou de `puck/kc-tag.js`, relancer le build.**

## Protocole BLE (résumé)

Chaque appareil émet un *manufacturer data* dont le company ID est `0xFFFF` :

| Émetteur | Octets |
|---|---|
| Bracelet | `4B 43 01` · CRC de la balise en alerte (2, LE) · CRC de la balise à sonner (2, LE) |
| Balise | `4B 43 02` · CRC du bracelet propriétaire (2, LE) |

Le CRC est un CRC16-CCITT des 6 octets de l'adresse BLE, dans l'ordre d'affichage. La valeur 0 signifie « aucun ». Aucune connexion n'est nécessaire entre bracelet et balise. Spécification complète : `openspec/changes/port-bangle-puck/specs/ble-protocol/` (puis `openspec/specs/` après archivage).
