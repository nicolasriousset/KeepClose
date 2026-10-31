# KeepClose

Système de détection de proximité BLE pour personnes malvoyantes.
Cas d'usage principal : alerter si la canne blanche sort du périmètre.

---

## Matériel

| Rôle | Appareil | Ajout matériel |
|---|---|---|
| Bracelet | [Bangle.js 2](https://www.espruino.com/Bangle.js2) : montre avec écran, vibreur, bouton, tactile, batterie rechargeable | aucun |
| Balise (sur la canne, un objet, un repère de chemin…) | [Puck.js](https://www.espruino.com/Puck.js) : boîtier avec LEDs RGB, bouton, pile CR2032 | buzzer **piézo passif** entre `D1` et `D2` |

Les deux appareils tournent sous [Espruino](https://www.espruino.com/) (JavaScript).

```
  Puck.js D1 ──── piézo (+)
  Puck.js D2 ──── piézo (−)
```

---

## Fonctionnement

- **Appairage** : coller la balise contre la montre pendant 3 s. La montre vibre 3 fois et la LED de la balise s'allume en vert 1 s. Une balise n'appartient qu'à un seul bracelet ; si un autre bracelet la reprend, le premier la désappaire automatiquement.
- **Mode N.I.C.O.** (par défaut) : la montre vibre chaque seconde quand une balise appairée sort du périmètre (environ 0,5 m, avec hystérésis).
- **Mode Ti Poucet** : on dépose des balises le long d'un chemin. Au retour, la balise non encore atteinte la plus proche sonne. Une fois atteinte (moins de 1 m), c'est la suivante qui sonne. Deux vibrations longues signalent la fin du trajet.
- **Action unique** : un **double tap sur la montre**, depuis n'importe quel écran.
  - **N.I.C.O.** :
    - si une sonnerie est en cours, l'arrêter ;
    - sinon, si l'alerte vibre, la mettre en sourdine jusqu'au retour de la balise ;
    - sinon, faire sonner la balise la plus proche pendant 10 s.
  - **Ti Poucet** : mettre le guidage en pause, ou le reprendre.
- **Bouton de la balise** : pendant une sonnerie, il la coupe ; sinon, il fait flasher l'état de la balise.
- **Widget sur la montre** :
  - bleu (cercle vide) : aucune balise ;
  - vert ✓ : à portée ;
  - rouge ! : hors de portée.

  **LEDs de la balise** : un flash de la même couleur toutes les 2,5 s environ.

Le détail d'utilisation et d'installation se trouve dans [`firmware/bangle/README.md`](firmware/bangle/README.md) et [`firmware/puck/README.md`](firmware/puck/README.md).

---

## Architecture logicielle

Le bracelet et les balises communiquent **uniquement par advertising BLE**, sans connexion. Le bracelet publie la balise en alerte et la balise à faire sonner ; chaque balise publie son bracelet propriétaire. Le format exact est décrit dans [`firmware/README.md`](firmware/README.md#protocole-ble-résumé).

```
firmware/
├── common/
│   ├── kccommon.js          # logique partagée et testable (protocole, distance, N.I.C.O., Ti Poucet, balise)
│   └── test/                # tests unitaires Node
├── bangle/keepclose/        # app Bangle.js 2 : boot (surveillance), widget, app, réglages
├── puck/
│   ├── kc-tag.js            # source du firmware balise
│   └── dist/kc-tag.js       # firmware balise à envoyer (généré)
└── tools/
    ├── build.js             # génère les fichiers installables
    ├── simulate-bangle.js   # simulation du code boot Bangle sous Node
    ├── emulate-bangle.js    # app Bangle dans l'interpréteur Espruino réel (émulateur)
    └── emulate-puck.js      # firmware balise dans l'interpréteur Espruino réel
```

```sh
node --test "firmware/common/test/*.test.js"
node firmware/tools/build.js
node firmware/tools/simulate-bangle.js
```

Les évolutions du projet passent par [OpenSpec](https://github.com/Fission-AI/OpenSpec) (`openspec/`). Voir [`CLAUDE.md`](CLAUDE.md).

---

## Versions précédentes

- **v2, XIAO BLE nRF52840** (Arduino) : [`legacy/xiao-firmware/`](legacy/xiao-firmware/). Son protocole n'est pas compatible avec la version actuelle.
- **v1, LilyGo T-Watch 2020 V3** (ESP32 / NimBLE) : [`legacy/`](legacy/).
