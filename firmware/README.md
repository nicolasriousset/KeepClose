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
