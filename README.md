# KeepClose

**KeepClose** est un système embarqué de détection de proximité BLE destiné aux personnes malvoyantes, pour prévenir la perte d'objets essentiels (ex. canne blanche).

---

## Architecture v2 — XIAO BLE nRF52840

La v2 utilise deux modules **Seeed Studio XIAO BLE nRF52840** et supporte deux projets :

| Projet | Comportement |
|--------|-------------|
| **N.I.C.O.** | Le bracelet vibre quand la balise (canne) sort du rayon |
| **Ti Poucet** | Le bracelet sonne la balise la plus proche pour guider l'utilisateur sur un chemin |

| Module | Rôle | Firmware |
|--------|------|----------|
| **Tag** | Balise BLE posable sur un objet ; sonne sur commande | `firmware/tag/tag.ino` |
| **Scanner** | Bracelet porté par l'utilisateur | `firmware/scanner/scanner.ino` |

Le projet actif se sélectionne dans `firmware/scanner/config.h` (`#define PROJECT_NICO` ou `#define PROJECT_TIPOUCET`).

```
firmware/
├── common/
│   └── config.h               # UUID BLE, seuils, commande ring
├── tag/
│   ├── config.h               # TX power, brochage buzzer
│   └── tag.ino                # Advertising + service GATT ring
└── scanner/
    ├── config.h               # Sélection projet, brochage, timeouts
    ├── scanner_core.h         # Registre multi-balises, estimation distance, ring
    ├── project_nico.h         # Comportement N.I.C.O.
    ├── project_tipoucet.h     # Comportement Ti Poucet
    └── scanner.ino            # setup/loop — inclut le bon module projet
```

---

## Protocole BLE

Le tag est un **périphérique GATT** (pas un simple advertiser) :
- Il advertise en continu pour être détectable par le scanner
- Il expose un service GATT avec une caractéristique `ring` (write)
- Commandes : `0x01` = sonnerie ON, `0x00` = sonnerie OFF

---

## Matériel

### Composants

| Composant | Rôle | Qté |
|-----------|------|-----|
| Seeed Studio XIAO BLE nRF52840 | Microcontrôleur + BLE | 2 |
| Moteur vibrant | Alerte haptique (bracelet) | 1 |
| Buzzer piézo | Sonnerie (balise) | 1 |
| Résistance 220 Ω | Protection LED/moteur | selon besoin |

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

> **Attention** : le GND est la **2ème broche depuis le haut à droite** (côté USB-C),
> pas en bas. Les broches de puissance (3V3, GND, VUSB) sont du côté USB,
> les broches D7–D10 sont du côté opposé.

### Affectation des broches par firmware

| Broche | Scanner (bracelet) | Tag (balise) |
|--------|--------------------|--------------|
| D0     | Moteur vibrant     | —            |
| D1     | Buzzer             | Buzzer       |
| LED_BUILTIN | LED statut    | LED statut   |

### Soudure des pin headers

Les modules sont livrés sans pins soudés. Avant toute utilisation :

1. Insérer les deux barrettes de 7 pins dans une breadboard (elles maintiennent l'alignement)
2. Poser le module par-dessus
3. Souder chaque pin en moins de 3 secondes — joint brillant et conique = bonne soudure
4. Vérifier l'absence de ponts entre broches adjacentes

Utiliser `firmware/test/test.ino` pour valider toutes les soudures avant de continuer.

---

## Installation de l'environnement de développement

### 1. Installer l'IDE Arduino

Télécharger la dernière version depuis [arduino.cc/en/software](https://www.arduino.cc/en/software).

### 2. Ajouter le board package Seeed nRF52

- Ouvrir **Fichier → Préférences**
- Ajouter dans *URL de gestionnaire de cartes supplémentaires* :
  ```
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
  ```
- Aller dans **Outils → Type de carte → Gestionnaire de cartes**
- Rechercher **Seeed nRF52** et installer le package

### 3. Sélectionner la carte

**Outils → Type de carte → Seeed nRF52 Boards → Seeed XIAO nRF52840**

> Choisir **Seeed XIAO nRF52840** (pas la variante Sense).
> Le menu `Outils` doit afficher `SoftDevice: S140 7.3.0` — c'est normal.

### 4. Bibliothèques requises

- **Adafruit_TinyUSB** — nécessaire pour que `Serial` compile avec ce BSP.
  Inclure `#include <Adafruit_TinyUSB.h>` en tête de chaque sketch qui utilise Serial.

> La bibliothèque ArduinoBLE standard (Arduino.cc) n'est pas compatible avec
> le SoftDevice S140 de ce BSP. La prise en charge BLE est intégrée au BSP.

### 5. Compiler et téléverser

Le BSP utilise le **bootloader DFU** — le téléversement se fait en deux temps :

1. Double-appuyer rapidement sur le bouton **RST** du module
   → la LED orange pulse, un nouveau port COM (ex. COM5) apparaît
2. Sélectionner ce port dans **Outils → Port**
3. Cliquer **Téléverser** dans l'IDE
4. Après l'upload, le module redémarre et un port COM applicatif réapparaît

Pour le moniteur série, utiliser ce port applicatif (pas le port DFU).

Chaque sketch utilisant Serial doit inclure ce pattern de démarrage :

```cpp
Serial.begin(115200);
unsigned long t = millis();
while (!Serial && millis() - t < 5000);  // attend le moniteur, max 5 s
```

---

## Code V1 (archive)

Le code de la v1 (LilyGo T-Watch 2020 V3 / ESP32 / NimBLE / LVGL) est conservé dans [`legacy/`](legacy/) — voir [`legacy/README.md`](legacy/README.md) pour les instructions d'installation.

---

## Licence

Projet open-source sous licence MIT.
