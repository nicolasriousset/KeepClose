---
date: '2026-04-27'
updated: '2026-04-28'
status: 'draft — en cours'
type: 'hardware-exploration'
---

# Exploration matérielle KeepClose v2 — Synthèse
*Mis à jour le 2026-04-28 — session complémentaire bracelet maître*

---

## Bracelet / dispositif maître

### Exigences retenues (mises à jour)
- BLE Central/Scanner (RSSI, estimation de distance)
- Vibreur haptique (N impulsions = numéro de balise)
- Buzzer/speaker (alertes sonores)
- 2 boutons physiques
- Charge USB-C
- Autonomie ≥ 7 jours (objectif révisé à la hausse)
- Pas d'écran obligatoire (utilisateur malvoyant — écran utile pour accompagnateur uniquement)
- Certification CE requise pour distribution

---

### Options envisagées — session du 2026-04-27

#### Montres connectées programmables
| Option | MCU | Vibreur | CE | Prix EU | Verdict |
|---|---|---|---|---|---|
| **LilyGo T-Watch 2020 V3** | ESP32 | ✅ | ❌ à faire | ~40€ | Fonctionne, mais autonomie ~3 jours max, ESP32 trop gourmand |
| **Bangle.js 2** | nRF52840 | ✅ | ✅ obtenu | **110€ (99 GBP)** | CE intégré = atout majeur, firmware bare-metal C possible via SWD, mais prix trop élevé pour prototypage |
| **PineTime** | nRF52832 | ✅ | ⚠️ incertain | ~67€ EU | Rupture de stock chronique, CE à vérifier |

**Notes complémentaires sur la Bangle.js 2 :**
- Firmware 100% remplaçable : pins SWD accessibles sur les 4 contacts arrière (via câble de charge USB). Dépôt `BangleJS2BareMetal` documenté par Espruino.
- Autonomie : 1 mois (horloge 1×/min), 2 semaines (1×/sec). Avec écran désactivé par firmware → gain supplémentaire.
- Bouton physique : 1. Vibreur : ✅. Touchscreen : disponible pour accompagnateur.
- **Raison d'exclusion : 110€ rend les itérations multiples impossibles.**

**Notes complémentaires sur la LilyGo T-Watch 2020 v3 :**
- Autonomie ESP32 chiffrée : ~6h (tout actif) / ~3 jours (deep sleep optimisé) / ~7 jours (réveil périodique très agressif, BLE minimal).
- Bouton physique latéral : ✅ (réveil depuis deep sleep). Vibreur : ✅. Speaker MAX98357 : ✅.
- Conserve toute la stack code existante (ESP32/NimBLE/Arduino) sans refacto.
- **Rôle envisageable : plateforme de développement firmware uniquement, avant migration vers nRF52840.**

#### Trackers fitness grand public
Fitbit, Amazfit, Xiaomi Mi Band, Garmin — **tous exclus.** BLE scanner arbitraire verrouillé par les plateformes. Cul-de-sac architectural confirmé.

#### LILYGO T-Wristband
- Form factor bracelet silicone + PCB intégré — exactement le format idéal
- Puce : ESP32-PICO-D4 (même problème d'autonomie que T-Watch)
- **Statut : discontinué.** Aucun équivalent basé sur nRF52840 n'existe sur le marché en 2025-2026.
- **Verdict : écarté.**

#### SMA Q3
- Même hardware que Bangle.js 2 (nRF52840, SWD exposé), sans firmware Espruino
- Prix espéré : ~35-40€
- **Introuvable en pratique** : vendeurs peu fiables sur Alibaba, envoi de produits erronés signalé par la communauté Espruino (discussion #6847).
- **Verdict : écarté.**

#### Colmi P8 originale (2020)
- Puce : nRF52832. Firmware custom possible (InfiniTime porté `WATCH_COLMI_P8`, Arduino Nordic)
- Prix : ~15€ — stock 2020 quasi épuisé
- Les modèles récents (P8 Plus, P8 Max, P8 Mix) ont une puce différente et ne sont **pas hackables** de la même façon
- **Verdict : écarté (introuvable de façon fiable).**

---

### Modules de développement — candidats sérieux

| Option | MCU | Vibreur | Bouton | USB-C | LiPo intégré | CE | Prix | Verdict |
|---|---|---|---|---|---|---|---|---|
| **Nordic Thingy:52** | nRF52832 | ✅ intégré | ✅ | ❌ micro-USB | ✅ 1440mAh | ✅ | ~48€ | Tout intégré, proto immédiat, mais boîtier carré (pas bracelet) + SDK Zephyr/nRF |
| **Seeed XIAO nRF52840** | nRF52840 | ❌ ext. | ❌ natif | ✅ | ✅ JST 1.25mm | ✅ module | ~6-8€ | Très compact (21×17mm), charge LiPo intégrée, 5µA deep sleep — **retenu** |
| **Adafruit Feather nRF52840** | nRF52840 | ❌ ext. | ✅ | ✅ | ✅ JST | ⚠️ | ~24€ | Bonne base semi-DIY, vibreur à brancher sur GPIO |
| **Adafruit Circuit Playground Bluefruit** | nRF52840 | ❌ ext. | ✅ | ✅ | ❌ ext. | ❌ US | ~26€ | Buzzer intégré, pas de CE EU — proto labo uniquement |
| **M5Stack Atom Lite** | ESP32 | ❌ ext. | ✅ | ✅ | ❌ ext. | ⚠️ | ~10€ | NimBLE/Arduino sans modification, très compact, batterie externe obligatoire |
| **M5StickC Plus + HAT Vibreur** | ESP32 | ✅ HAT ~3€ | ✅ | ✅ | ❌ ext. | ⚠️ | ~22€ | Commandable immédiatement, vibreur enfichable, solution la plus rapide |
| **BBC micro:bit v2** | nRF52833 | ❌ | ✅ | ❌ | ❌ pile AAA | ✅ | ~16€ | Écarté — alimentation non rechargeable |

#### Impact code selon plateforme
| Changement | BLE/NimBLE | UI/HAL | Risque |
|---|---|---|---|
| T-Watch → ESP32 autre (M5Stack, Waveshare) | 0% | Driver display + PMU | Faible |
| ESP32 → nRF52840 Arduino (Adafruit/Seeed) | ~40% réécriture (NimBLE → Bluefruit) | ~30% | Moyen |
| ESP32 → nRF52840 Zephyr | ~100% réécriture | ~100% | Élevé |

---

### Solution bracelet retenue

#### Court terme — proto firmware
**M5StickC Plus + HAT Vibreur (~22€)** — zéro refacto code, commandable immédiatement.

#### Moyen terme — proto produit ← **décision session 2026-04-28**
**Seeed XIAO nRF52840 + composants discrets + boîtier 3D custom (~15-20€)**

Justification du choix XIAO nRF52840 :
- **Certification CE ✅** : confirmée par Seeed Studio (CE + FCC + AOC). Documents disponibles sur le forum Seeed.
- **5µA en deep sleep** : 500× moins que l'ESP32. Autonomie 7-10 jours réaliste avec LiPo 200mAh.
- **BLE 5.0 natif** : supporte simultanément Central (scan balises) et Peripheral — un seul chip pour tout le système.
- **Chargeur LiPo intégré** (BQ25101) : pas de circuit externe nécessaire.
- **Format 21×17.5mm** : s'intègre dans un boîtier imprimé 3D clipable sur bracelet silicone 20mm standard.
- **Programmable Arduino/CircuitPython/Zephyr** : flexibilité maximale.
- **Même puce que Bangle.js 2** : firmware identique possible.
- **Plateforme unique bracelet + balises** : un seul SDK, un seul environnement, une seule certification module.

**Point d'attention vibreur :** les GPIO du nRF52840 sont limitées à 15mA. Le moteur vibreur disc (~80mA) nécessite un transistor NPN (2N2222 ou BC547) en driver. Idem pour le buzzer si courant > 15mA.

**Form factor boîtier :** aucun bracelet silicone "prêt à loger" de l'électronique n'existe sur le marché (le T-Wristband était le seul, désormais discontinué). Solution : boîtier 3D minimaliste (~30×20×10mm) avec barrettes 20mm → accepte n'importe quel bracelet silicone standard.

#### Long terme — v3 commerciale
PCB custom nRF52840 ou migration vers Bangle.js 2 (CE acquis).

---

### Composants retenus — bracelet maître

| Composant | Spécification | Conso | Prix est. |
|---|---|---|---|
| XIAO nRF52840 | MCU + BLE 5.0 + chargeur LiPo | 5µA veille / ~10mA actif BLE | ~6-8€ |
| Vibreur disque 10mm | Coin vibration motor 3V | ~80mA actif | ~0.50€ |
| Buzzer piézo passif 3V | Ø9mm traversant, piloté PWM | ~20mA actif | ~0.30€ |
| Bouton tactile 4mm SMD | ×2 | 0 | ~0.10€ |
| Transistor NPN 2N2222/BC547 | Driver vibreur + buzzer | — | ~0.10€ |
| LiPo 402030 — 200mAh | JST 1.25mm, 4×20×30mm | — | ~3€ |
| Bracelet silicone 20mm | Standard montre | — | ~1-2€ |
| Résistances 10kΩ | Pull-up boutons | — | <0.10€ |

**Calcul autonomie bracelet (200mAh) :**
```
XIAO scan BLE (intervalle 1s)   : ~200µA moyen
Vibreur (2s/heure en moyenne)   :  ~45µA moyen
Buzzer (1s/heure en moyenne)    :   ~6µA moyen
Total moyen estimé              : ~250µA
Autonomie théorique 200mAh      : ~33 jours
Autonomie réelle (pertes, pics) : ~7-10 jours ✅
```

---

## Balises esclaves

### Exigences retenues (mises à jour)
- BLE 5.0 advertising (format iBeacon-like, connectable)
- Buzzer audible (≥85 dB à 30 cm pour localisation sonore)
- 1 bouton physique (arrêt sonnerie + confirmation "collectée" mode Petit Poucet)
- LED indicateur batterie
- Autonomie ≥ 3 mois (objectif : 12 mois)
- Maximum 3 balises simultanées

### Options commerciales — toutes exclues

| Option | Buzzer | Firmware | Verdict |
|---|---|---|---|
| April Brother (N02, N06, N07) | ❌ absent | Partiellement ouvert | **Exclu** — pas de buzzer confirmé |
| Feasycom FSC-BP108 | ❌ non confirmé | Partiel | **Exclu** — buzzer non vérifié |
| Chipolo ONE | ✅ | ❌ fermé | **Exclu** — non programmable |
| Estimote / Kontakt.io | ❌ | Fermé/cloud | **Exclu** — trop cher, cloud-dépendant |
| Ruuvi Tag | ❌ | ✅ open source | **Exclu** — pas de buzzer |
| Tile Pro / UGREEN Smart Finder | ❌ | ❌ fermé | **Exclu** — dépend Apple Find My / Google Find Hub |

**Conclusion :** Il n'existe pas de balise BLE commerciale avec buzzer intégré + firmware programmable + iBeacon configurable. Le PCB custom est justifié pour la production.

### Piste iTag / trackers anti-perte BLE (proto UX)

Les trackers anti-perte bon marché (iTags, ~2-4€ sur AliExpress) constituent une piste viable pour le **prototype uniquement**.

#### Protocole GATT iTag CC2541 (Texas Instruments)

Profil **Immediate Alert** standard BLE SIG — aucune authentification requise.

| Service | UUID | Rôle |
|---|---|---|
| Immediate Alert | `0x1802` | Buzzer |
| Alert Level char | `0x2A06` | Write : 0x00=off / 0x02=high |
| Custom button | `0xFFE0` / `0xFFE1` | Notify sur appui bouton |

Commande ring depuis ESP32/NimBLE : connexion BLE → GATT Write Without Response sur `0x2A06` avec valeur `0x02` → déconnexion. ~50 lignes de code, pas de pairing.

Format advertising : **pas iBeacon**. Identification par Local Name `"iTAG"` ou UUID service `0x1802`. RSSI utilisable pour distance, mais pas de TX Power dans l'advertising → calibration manuelle à 1m nécessaire.

Bouton physique → notification BLE sur `0xFFE1` vers le central (bracelet). Exploitable pour confirmation "collectée" en mode Petit Poucet.

#### Limitations techniques

| Problème | Impact |
|---|---|
| Latence connect + write | **1-3s** — perceptible, à valider UX |
| Balise invisible pendant connexion | Pas d'advertising pendant connexion BLE → bracelet ne peut pas scanner simultanément |
| Niveau sonore | ~60-65 dB — faible vs 85+ dB visé. À tester en conditions réelles |
| RSSI variance ±6-10 dBm | Filtrage MovingAverage déjà en place côté bracelet |
| UUIDs variables selon clone | CC2541 vs nRF51822 vs TLSR8251 Telink — discovery dynamique nécessaire |
| Deep sleep inactivité | Certains modèles arrêtent d'advertiser — à tester |
| Chipset change silencieux | Un même listing AliExpress peut changer de chipset entre commandes |

#### Comparatif trackers avec buzzer + bouton

| Produit | Chipset | Prix | Buzzer | Bouton | Protocole documenté |
|---|---|---|---|---|---|
| **iTag CC2541** | TI CC2541 | ~2,50€ | ✅ ~65dB | ✅ | ✅ Immediate Alert 0x1802 |
| **iTag nRF51822** | Nordic | ~3,50€ | ✅ | ✅ | ✅ + flashable SWD |
| Nut Find 3 clone | Variable | ~4€ | ✅ | ✅ | ⚠️ partiel |
| Chipolo ONE clone | Variable | ~5€ | ✅ ~90dB | ✅ | ❌ auth récente |
| Tile Mate clone | Variable | ~6€ | ✅ | ✅ | ❌ cloud |

#### Certification CE

Les iTags AliExpress affichent un logo CE quasi systématiquement. Il est **apocryphe dans la grande majorité des cas** — pas de Declaration of Conformity vérifiable, pas de rapports de tests EN 300 328. Acceptable pour prototype interne, **bloquant pour commercialisation**.

#### Scénario hybride recommandé

**Phase proto (0-6 mois) :** 20 × iTag CC2541 commandés en un seul lot = ~60€. Validation UX dans la semaine sans développement hardware. Risque chipset minimisé par lot unique.

**Phase production (6-18 mois) :** XIAO nRF52840 custom, firmware maîtrisé, CE réelle.

### Solution balise retenue — XIAO nRF52840 ← **décision session 2026-04-28**

**Convergence sur la même plateforme que le bracelet : Seeed XIAO nRF52840**

Justification :
- Un seul SDK à maîtriser pour l'ensemble du système
- Un seul environnement de dev, un seul débogueur, une seule chaîne CI
- Firmware balise écrit une fois, déployé sur tous les modules identiques
- Certification CE déjà acquise sur le module XIAO
- Le nRF52840 supporte nativement les deux modes BLE : Peripheral/Advertiser (balise) et Central/Scanner (bracelet)

**Note :** le nRF52832 (solution Winston initiale) reste valide techniquement et légèrement moins cher (~3.50€ module E73). La convergence sur nRF52840 (XIAO) est préférée pour simplifier la stack de développement au stade proto.

### Composants retenus — balise (par unité)

| Composant | Spécification | Conso | Prix est. |
|---|---|---|---|
| XIAO nRF52840 | MCU + BLE Peripheral/Advertiser | ~5µA veille advertising | ~6-8€ |
| Buzzer piézo passif 3V | Ø9mm traversant, PWM 3kHz | ~20mA actif | ~0.30€ |
| Bouton tactile 4mm | Couper la sonnerie | 0 | ~0.05€ |
| Transistor NPN BC547 | Driver buzzer | — | ~0.05€ |
| CR2032 + support | 220mAh | ~5-10µA moyen | ~0.50€ |

**Calcul autonomie balise (CR2032 220mAh, advertising 1s) :**
```
XIAO advertising (intervalle 1s) : ~8µA moyen
Buzzer alarme (rare)             : négligeable en moyenne
Total moyen estimé               : ~8-10µA
Autonomie théorique              : ~2-3 ans
Autonomie réelle (autodécharge CR2032) : ~12-18 mois ✅
```

**Note batterie :** si intervalle advertising < 500ms → préférer CR2477 (~1000mAh) ou LiPo 100mAh pour préserver l'autonomie et éviter les chutes de tension lors des pics courant BLE.

**Machine à états firmware balise :**
```
INIT
 └→ ADVERTISING (ADV_IND connectable, 500-1000ms interval, ~8µA moyen)
      └→ [CONNECT_REQ] CONNECTED / WAIT_COMMAND (timeout 5s)
           └→ [GATT Write RING=0x01] RINGING (PWM 3kHz, LED 1Hz, ~20mA, timeout 300s)
                ├→ [bouton pressé] STOP + Notify STATUS=0x02 (collected)
                └→ [timeout 300s] STOP + Notify STATUS=0x03 (timeout)
                     └→ WAIT_DISCONNECT (2s) → retour ADVERTISING
```

**Points de robustesse firmware :**
- Watchdog 30s (balise muette = catastrophique pour l'utilisateur)
- Slot ID en NVM flash (FDS Nordic), survivant aux resets
- Alerte batterie : LED clignotante si < 2.5V, notification BLE "low battery" à la connexion

---

## Certification CE — synthèse

| Niveau | Certification | Suffisant pour… |
|---|---|---|
| Puce nRF52840 (Nordic) | CE ✅ + déclaration de conformité EU | Usage interne, développement |
| Module XIAO nRF52840 (Seeed) | CE ✅ + FCC + AOC | Prototype, usage personnel |
| Produit final assemblé | CE à refaire (directive RED 2014/53/EU) | Vente / distribution EU |

Le module XIAO certifié CE simplifie considérablement le dossier de certification du produit final. Si le projet évolue vers une aide technique pour malvoyants, la directive MDR (Medical Device Regulation) peut s'appliquer — à anticiper dès la conception.

---

## Architecture système finale

```
[ Balise 1 ]         [ Balise 2 ]         [ Balise 3 ]
XIAO nRF52840        XIAO nRF52840        XIAO nRF52840
Peripheral/Adv.      Peripheral/Adv.      Peripheral/Adv.
Buzzer + bouton      Buzzer + bouton      Buzzer + bouton
CR2032               CR2032               CR2032
12-18 mois           12-18 mois           12-18 mois
        ↓                    ↓                    ↓
                  [ Bracelet maître ]
                  XIAO nRF52840
                  Central/Scanner
                  Mesure RSSI → distance estimée
                  → Vibreur haptique
                  → Buzzer sonore
                  → 2 boutons physiques
                  LiPo 200mAh — 7-10 jours
                  Boîtier 3D + bracelet silicone 20mm
```

**Avantages de la plateforme unique nRF52840 :**
- Un seul SDK (Arduino + bibliothèque Adafruit Bluefruit, ou nRF Connect SDK)
- Certification CE acquise sur le module
- BLE 5.0 : portée, stabilité, low energy
- 5µA deep sleep : autonomie longue durée réelle
- Supporte Central + Peripheral simultanément sur le même chip

---

## BOM commande groupée — 1 bracelet + 3 balises

| Composant | Qté | Source suggérée | Prix est. |
|---|---|---|---|
| XIAO nRF52840 | 4 | AliExpress / Seeed Studio | ~28-32€ |
| Vibreur disque 10mm | 5 (lot) | AliExpress | ~1€ |
| Buzzer piézo passif 3V Ø9mm | 10 (lot) | AliExpress | ~1.50€ |
| Boutons tactiles 4mm SMD | 20 (lot) | AliExpress | ~0.50€ |
| Transistors NPN 2N2222/BC547 | 20 (lot) | AliExpress | ~0.50€ |
| LiPo 402030 200mAh JST 1.25mm | 2 | AliExpress | ~6€ |
| CR2032 + support | 5 | AliExpress ou Fnac/Darty | ~3€ |
| Bracelet silicone 20mm | 1 | AliExpress | ~1-2€ |
| Résistances 10kΩ | 20 (lot) | AliExpress | ~0.50€ |
| **Total estimé** | | | **~42-47€** |

**Points d'attention commande :**
- LiPo : vérifier connecteur JST **1.25mm** (pas 2.0mm) — format natif du XIAO
- Buzzer : choisir **passif** (sans oscillateur interne) pour contrôle PWM depuis nRF52840
- CR2032 : si intervalle advertising < 500ms → préférer CR2477 ou LiPo 100mAh

---

## Impact sur le code existant

| Changement | BLE/NimBLE | UI/HAL | Risque |
|---|---|---|---|
| T-Watch → XIAO nRF52840 (Arduino) | ~40% réécriture (NimBLE → Bluefruit/NimBLE nRF) | ~30% | Moyen |
| T-Watch → XIAO nRF52840 (Zephyr) | ~100% réécriture | ~100% | Élevé |

Le code de scan/RSSI du bracelet ESP32 actuel sert de référence fonctionnelle mais ne transfère pas directement sur nRF52840. La logique métier (seuils RSSI, patterns vibration, machine à états) est réutilisable ; la couche BLE est à réécrire.

Filtre `"Holy"` sur le nom de balise : à remplacer par un filtre sur UUID iBeacon configurable lors du provisioning des balises.

---

## Questions ouvertes

### Bracelet
- [ ] Valider autonomie XIAO nRF52840 + 200mAh LiPo en mode scan continu (1s interval)
- [ ] Décision : proto firmware sur T-Watch 2020 v3 d'abord (zéro refacto) ou directement XIAO nRF52840 ?
- [ ] Concevoir boîtier 3D 30×20×10mm avec barrettes bracelet 20mm
- [ ] Valider que le buzzer passif Ø9mm est audible dans le boîtier (volume suffisant pour malvoyant ?)

### Balise — proto iTag (validation UX)
- [ ] Commander lot unique 20 × iTag CC2541 (~60€) pour validation UX rapide
- [ ] Tester latence appui long → sonnerie (3-5s acceptable ?)
- [ ] Tester niveau sonore iTag en appartement (60-65 dB suffisant ?)
- [ ] Vérifier UUID GATT sur unités reçues (0x1802/0x2A06 ou variante ?)
- [ ] Vérifier pads SWD accessibles sur variante nRF51822 (option firmware custom ~3€/unité)
- [ ] Tester comportement advertising après inactivité (deep sleep ?)

### Balise — proto XIAO nRF52840
- [ ] Tester niveau sonore buzzer piézo Ø9mm dans boîtier 3D
- [ ] Valider que ADV_IND connectable + payload iBeacon-like est détecté correctement par le scanner bracelet
- [ ] Mesurer consommation réelle XIAO en advertising (confirmer ~8µA)
- [ ] Valider autonomie CR2032 réelle en conditions KeepClose

### Certification
- [ ] Anticiper directive MDR si le projet est présenté comme aide technique pour malvoyants
- [ ] Identifier laboratoire de test CE RED pour la phase commercialisation