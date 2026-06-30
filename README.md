# KeepClose v2

Système embarqué de détection de proximité BLE pour personnes malvoyantes.
Cas d'usage principal : alerter si la canne blanche sort du périmètre.

---

## Matériel

Deux modules **Seeed Studio XIAO BLE nRF52840**, un par appareil.

### Brochage XIAO BLE nRF52840

Connecteur USB-C en haut, vue de dessus :

```
         USB-C
     ┌────┤├────┐
D0   │●        ●│ 3V3
D1   │●        ●│ GND
D2   │●        ●│ VUSB
D3   │●        ●│ D10 / MOSI
D4   │●        ●│ D9  / MISO
D5   │●        ●│ D8  / SCK
D6   │●        ●│ D7  / RX
     └──────────┘
```

LEDs intégrées : active LOW (LOW = allumée, HIGH = éteinte).
GND = 2e broche depuis le haut à droite (côté USB-C).

---

## Architecture électronique

### Bracelet (scanner)

| Broche | Composant | Notes |
|--------|-----------|-------|
| D0 | Moteur vibrant | Via transistor 2N2222 + diode de roue libre |
| D1 | Bouton push | Actif bas, pull-up interne |
| BAT+ / BAT− (verso) | LiPo 3.7 V 250 mAh | Connecteur JST 1.25 mm |
| LED_RED / LED_GREEN / LED_BLUE | LED RGB intégrée | Active LOW |

**Schéma complet bracelet :**

```
                         ┌────┤├────┐  USB-C (recharge batterie)
                         │  XIAO   │
            LiPo 3.7 V   │  BLE    │
            250 mAh   [verso]      │
            (+) ── BAT+  │         │
            (-) ── BAT-  │         │
                         │         │
          ┌── Moteur (+) │  3V3  ● │
          │              │         │
      [1N4148]           │  D0   ● │──[1 kΩ]──┐
    (roue libre)         │         │           │ Base
          │              │  D1   ● │──────[Bouton]──── GND
          └── Collecteur │         │           │
                    │    │  GND  ● │──── GND   │ (pull-up interne)
              2N2222│    │         │           ▼
                    │    │ LED_RED ├── (intégrée, active LOW)
                Base┘    │ LED_GRN ├── (intégrée, active LOW)
                 │       │ LED_BLU ├── (intégrée, active LOW)
              [1 kΩ]     │         │
                 │       │ P0.14 * ├── enable diviseur batterie (interne)
                D0       │ P0.29 * ├── lecture tension batterie  (interne)
                         └─────────┘
              Émetteur ── GND        * accès interne uniquement
          Moteur (-) ─── Collecteur
```

**Détail moteur vibrant (D0) :**

```
3V3 ──────────────── Moteur (+)
                          │
                     [1N4148]   cathode=3V3, anode=collecteur
                          │
D0 ──[1kΩ]── Base    Collecteur ── Moteur (-)
               \    /
             2N2222
                │
            Émetteur
                │
               GND
```

Brochage 2N2222 TO-92 (face plate vers soi, pattes vers le bas) : Émetteur (gauche) — Base (milieu) — Collecteur (droite).

**Câblage bouton (D1) :**

```
D1 ──── [Bouton] ──── GND
```

Pas de résistance externe, pull-up interne activée par `INPUT_PULLUP`.

**Batterie :**

```
LiPo 3.7 V 250 mAh
    (+) ──── BAT+  ┐  pads au verso de la carte
    (-) ──── BAT-  ┘  connecteur JST 1.25 mm
```

Recharge automatique via USB-C (BQ25101, ~50 mA, ≈ 5 h pour 250 mAh).
Niveau affiché dans le heartbeat Serial (`batt=xx%` ou `batt=n/a` si non connectée).

---

### Balise (tag)

| Broche | Composant | Notes |
|--------|-----------|-------|
| D1 | Buzzer actif 12 mm | Connexion directe |
| D2 | Bouton push | Actif bas, pull-up interne |
| BAT+ / BAT− (verso) | LiPo 3.7 V 250 mAh | Connecteur JST 1.25 mm |
| LED_RED / LED_GREEN / LED_BLUE | LED RGB intégrée | Active LOW |

**Schéma complet balise :**

```
                         ┌────┤├────┐  USB-C (recharge batterie)
                         │  XIAO   │
            LiPo 3.7 V   │  BLE    │
            250 mAh   [verso]      │
            (+) ── BAT+  │         │
            (-) ── BAT-  │         │
                         │         │
                         │  D1   ● │──── Buzzer (+)
                         │         │     Buzzer (-) ──── GND
                         │  D2   ● │──────[Bouton]──── GND
                         │         │      (pull-up interne)
                         │  GND  ● │──── GND
                         │         │
                         │ LED_RED ├── (intégrée, active LOW)
                         │ LED_GRN ├── (intégrée, active LOW)
                         │ LED_BLU ├── (intégrée, active LOW)
                         │         │
                         │ P0.14 * ├── enable diviseur batterie (interne)
                         │ P0.29 * ├── lecture tension batterie  (interne)
                         └─────────┘
                                      * accès interne uniquement
```

**Câblage buzzer actif (D1) :**

```
D1 ──── Buzzer (+)
GND ─── Buzzer (-)
```

Utiliser un buzzer **actif** (avec marquage "+", ~12 mm) — pas un piézo passif.

**Câblage bouton (D2) :**

```
D2 ──── [Bouton] ──── GND
```

**Batterie :**

```
LiPo 3.7 V 250 mAh
    (+) ──── BAT+  ┐  pads au verso de la carte
    (-) ──── BAT-  ┘  connecteur JST 1.25 mm
```

Recharge automatique via USB-C (~50 mA, ≈ 5 h pour 250 mAh).
Niveau affiché dans le heartbeat Serial (`batt=xx%` ou `batt=n/a` si non connectée).

---

## Comportement

### LED bracelet

| Couleur | Signification |
|---------|--------------|
| Bleu | Actif, aucune balise appairée |
| Vert | Balise(s) appairée(s) à portée |
| Rouge | Au moins une balise appairée hors périmètre |

### LED balise

| Couleur | Signification |
|---------|--------------|
| Bleu | Non appairée |
| Vert | Appairée, bracelet à portée |
| Rouge | Appairée, bracelet hors périmètre ou signal perdu |

### Bouton bracelet (D1) — 3 comportements selon l'état

| Situation | Action |
|-----------|--------|
| Ring en cours | Annule le ring (balise arrête de sonner) |
| Moteur vibre (balise hors périmètre) | Snooze : moteur s'arrête. Reprend automatiquement à la prochaine sortie du périmètre après un retour dans le rayon |
| Rien (balise à portée) | Démarre le ring : la balise appairée la plus proche sonne pendant 10 s |

### Bouton balise (D2)

| Situation | Action |
|-----------|--------|
| Buzzer en cours | Acquitte la sonnerie (buzzer s'arrête). Se réinitialise automatiquement quand le bracelet arrête de publier le ring |

---

## Architecture logicielle

### Protocole BLE

Le bracelet advertise en continu (`KC-Scanner`, 300 ms).
Son paquet **manufacturer data** contient :

```
[company_id : 2 octets] [alertHash : 2 octets] [ringHash : 2 octets]
```

- **alertHash** : CRC16 de l'adresse MAC de la balise hors-périmètre la plus proche (0 si toutes à portée)
- **ringHash** : CRC16 de la balise à faire sonner (0 si aucune sonnerie en cours)

Chaque balise calcule son propre CRC16 au démarrage (`ownTagCrc`).
À chaque paquet reçu du bracelet :
- `alertHash == ownTagCrc` → LED rouge
- `ringHash == ownTagCrc` → buzzer ON
- Sinon → LED verte (si signal frais) ou rouge (si signal périmé)

La balise advertise aussi en continu (`KC-Tag`) avec son CRC dans le scan response :
le bracelet l'utilise pour détecter un appairage exclusif (balise déjà liée à un autre bracelet).

### Appairage

Coller balise et bracelet à moins de ~5 cm pendant **3 secondes**.
Confirmation : 3 vibrations courtes sur le bracelet.
L'appairage est sauvegardé en flash (survit aux redémarrages).
Une balise ne peut être appairée qu'à un seul bracelet à la fois.

### Estimation de distance (bracelet)

Le bracelet estime la distance de chaque balise appairée via le RSSI :

```
distance = 10 ^ ((TX_POWER_AT_1M - rssiEma) / (10 × PATH_LOSS_N))
```

- `TX_POWER_AT_1M = -61 dBm` (calibré à 1 m)
- `PATH_LOSS_N = 2.0` (espace libre)
- EMA sur le RSSI : `alpha = 0.1` (lissage fort)
- Hystérésis : passage rouge→vert à 45 cm, vert→rouge à 55 cm

La balise ne calcule pas de distance — elle réagit uniquement aux champs alertHash/ringHash publiés par le bracelet.

### Multi-balises

Le bracelet peut gérer jusqu'à 10 balises appairées simultanément.
Règle d'alerte : parmi les balises hors périmètre, le bracelet publie le CRC de la **plus proche** dans alertHash. L'utilisateur la récupère en premier, puis le bracelet passe à la suivante.

### Structure des fichiers

```
firmware/
├── common/
│   └── config.h              # Seuils partagés, CRC16, constantes BLE
├── test/
│   └── test.ino              # Test de soudures (phases 1-5)
├── tag/
│   ├── config.h              # TX power, brochage, timeouts
│   └── tag.ino               # Advertising + scan scanner + buzzer + bouton
└── scanner/
    ├── config.h              # Sélection projet, brochage, durées
    ├── scanner_core.h        # BeaconRegistry, appairage, estimation distance
    ├── project_nico.h        # Comportement N.I.C.O. (moteur + bouton + ring)
    ├── project_tipoucet.h    # Comportement Ti Poucet (sonnerie chemin)
    └── scanner.ino           # setup/loop, advertising, macros PROJECT_*
```

Sélectionner le mode dans `firmware/scanner/config.h` :
```cpp
#define PROJECT_NICO      // alerte vibration canne blanche
// #define PROJECT_TIPOUCET  // guide par sonnerie successive
```

---

## Installation

### Board package

- Ouvrir **Fichier → Préférences** dans l'IDE Arduino
- Ajouter dans *URL de gestionnaire de cartes supplémentaires* :
  ```
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
  ```
- **Outils → Gestionnaire de cartes** → installer **Seeed nRF52**
- Sélectionner : **Seeed nRF52 Boards → Seeed XIAO nRF52840** (pas la variante Sense)

### Téléversement

Le module utilise le bootloader DFU :

1. Double-appuyer sur **RST** → LED orange pulse, nouveau port COM (DFU) apparaît
2. Sélectionner ce port dans **Outils → Port**
3. Cliquer **Téléverser**
4. Après upload, sélectionner le port applicatif pour le moniteur série

### Validation des soudures

Téléverser `firmware/test/test.ino` avant tout autre firmware :

| Phase | Test |
|-------|------|
| 1 | Communication série (USB + puce) |
| 2 | LEDs RGB intégrées |
| 3 | GPIO broche par broche (LED externe + 220 Ω) |
| 4 | BLE (advertising visible depuis smartphone) |
| 5 | Moteur vibrant D0 (3 impulsions + 1 longue) |

---

## Archive v1

Code de la v1 (LilyGo T-Watch 2020 V3 / ESP32 / NimBLE) conservé dans [`legacy/`](legacy/).
