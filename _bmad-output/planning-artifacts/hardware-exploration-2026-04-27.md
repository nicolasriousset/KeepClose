---
date: '2026-04-27'
updated: '2026-05-04'
status: 'draft — en cours'
type: 'hardware-exploration'
sessions:
  - '2026-04-27 — exploration initiale bracelet + balises'
  - '2026-04-28 — décision XIAO nRF52840 comme plateforme unique'
  - '2026-05-01 — élargissement sourcing montres nRF52840 + reconsidération Bangle.js 2'
  - '2026-05-04 — exploration LoRa/Meshtastic pour Ti Poucet grande portée'
---

# Exploration matérielle KeepClose v2 — Synthèse
*Mis à jour le 2026-05-01 — session sourcing montres + reconsidération Bangle.js 2*

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

### Session complémentaire 2026-05-01 — élargissement sourcing montres nRF52840

Le constat de la session du 2026-04-28 était : *"aucun équivalent T-Wristband basé sur nRF52840 n'existe sur le marché 2025-2026"*. Cette session a élargi la recherche à plusieurs gammes commerciales pour vérifier ce constat et identifier d'éventuelles opportunités manquées.

#### Montres examinées

| Modèle | MCU | Buzzer | Vibreur | App | Hackable | Prix | Verdict 2026-05-01 |
|---|---|---|---|---|---|---|---|
| **Kospet Magic 3 (alias C17)** | nRF52840 ✅ | ❌ | ✅ | DaFit | ✅ Espruino + DFU OTA | ~30-35€ | **Écarté — fin de vie commerciale.** Encore listé chez Kospet officiel mais en rupture sur AliExpress en mai 2026. Sourcing peu fiable. |
| **Kospet Rock** (2021) | nRF52840 ✅ | ❌ | ✅ | DaFit | ✅ documentée par fanoush/atc1441 | ~40€ | **Piste à valider** — équivalent fonctionnel Magic 3, format 1.69" rectangulaire. Vérifier disponibilité réelle Amazon/eBay (ne plus produire neuf chez Kospet). |
| **Kospet Tank T2 / T3 / T3 Ultra** (2023-2025) | Realtek RTL8763EW / Actions ATS3085L ❌ | speaker BT Call | ✅ | FitCloudPro | ❌ chip propriétaire | 50-130€ | **Écarté.** Kospet a abandonné le nRF52840 sur sa nouvelle gamme rugged. AMOLED + AOD + BT Call = Realtek/Actions, fermé. |
| **Oukitel BT10/BT20/BT80/BT101/BT103/BT12** | Realtek RTL8763EW ❌ | speaker BT Call | ✅ | FitCloudPro | ❌ | 30-80€ | **Écarté.** Toute la gamme Oukitel BT est sur Realtek, non hackable. |
| **Oukitel WP200 Pro / WP100 Titan** | MediaTek (smartphone) ❌ | — | — | — | — | 400-700€ | **Hors sujet.** Smartphones rugged, pas des montres standalone (le WP200 Pro a un module détachable mais ce n'est pas le format cible). |
| **C26 (générique : Gravity, Rainbuvvy, NJYUAN, MISIRUN…)** | Realtek RTL8763EW ❌ | speaker BT Call | ✅ | FitCloudPro | ❌ | 25-40€ | **Écarté.** Nom commercial générique réutilisé par 10+ marques chinoises, toutes sur Realtek. |
| **PineTime original (Pine64)** | nRF52832 (proche cible, pas identique) | ❌ | ✅ | InfiniTime / WaspOS | ✅ open source de bout en bout | ~27 $ (~25 €) chez Pine64 EU | **Réintégrée 2026-05-01 — voir re-évaluation ci-dessous.** Compromis buzzer levé par décision 2026-05-01 (vibreur seul suffit sur bracelet). Reste un compromis chipset 52832 vs 52840, mais le code métier porte à ~90 %. Open source intégral (hardware + firmware + bootloader), communauté très active. Restock courant mi-mai 2026. |
| **PineTime Pro** (annoncé mars 2026) | "chip custom" (non communiqué) | ? | ? | InfiniTime/WaspOS en cours d'adaptation | ? | inconnu | **À surveiller.** Production retardée par "AI bubble RAM price storm" (Hackster). Vitalité communautaire confirmée par cette annonce — InfiniTime et WaspOS s'adaptent à la nouvelle plateforme en parallèle. Trop incertain pour intégrer au plan v2, mais signal positif sur la pérennité de l'écosystème. |
| **Bangle.js 2** *(re-examen)* | nRF52840 ✅ | ❌ (vibreur HF inaudible) | ✅ | Espruino / Bangle Apps | ✅ SWD exposé arrière | ~110€ | **Réintégré au plan — voir reconsidération ci-dessous.** Vibreur seul suffit côté bracelet (cf. décision 2026-05-01 sur le buzzer bracelet non indispensable). |

#### Filtre rapide pour identifier une montre hackable

Critère le plus fiable pour pré-qualifier une montre AliExpress avant même d'analyser les specs (que les vendeurs falsifient régulièrement) : **regarder l'app companion exigée par la montre**.

| App companion | Écosystème probable | Hackabilité |
|---|---|---|
| **DaFit** / **DaFlasher** | nRF52832 ou nRF52840 (DaFit historique) | ✅ Probablement hackable. Communauté fanoush/atc1441/Espruino active. |
| **FitCloudPro** | Realtek RTL8763 | ❌ Fermé. App propriétaire, pas de DFU custom documenté. |
| **GloryFit** / **WearPro** / **VeryFitPro** / **Da Fit Pro** | Realtek ou Actions | ❌ Fermé. |
| **Mi Fit / Zepp Life** | Bestechnic / Huami custom | ❌ Très verrouillé. |
| **Espruino Web IDE** | nRF52840 | ✅ Plateforme ouverte par construction. |

Cette règle élimine 95% des candidats AliExpress en regardant simplement la fiche produit avant achat.

#### État du marché nRF52840 retail au 2026-05-01

Le marché grand public des montres programmables nRF52840 est en train de **se réduire**, pas de se développer :

- **Kospet a abandonné le nRF52840** pour ses nouvelles gammes (Tank → Realtek/Actions). La Magic 3 (2021) et la Rock (2021) sont en fin de vie commerciale.
- **PineTime Pro** abandonne le nRF52832 historique pour un "chip custom" non documenté → rupture probable de l'écosystème InfiniTime / WaspOS sur la nouvelle génération.
- **Les fabricants chinois** privilégient Realtek RTL8763EW / Actions ATS3085L pour les montres avec speaker BT Call et AMOLED — fonctions inaccessibles au nRF52840 (pas de codec audio, pas de DAC). Ce sont précisément ces fonctions qui dominent le marketing 2024-2026.
- **Bangle.js 2** (Espruino, 2021) reste le seul produit retail nRF52840 *certifié CE/FCC, sourcable de manière pérenne, hackable de jure*. Prix élevé pour cette raison (~110€).

**Implication stratégique :** la décision 2026-04-28 de converger sur le **module XIAO nRF52840 + boîtier custom** est validée *a fortiori*. La stratégie qui consisterait à s'appuyer sur une montre commerciale hackable comme cible v3 doit se limiter à la **Bangle.js 2 uniquement** pour préserver la traçabilité du sourcing — toute autre option (Magic 3, Rock, clones DaFit) est instable à 12-24 mois.

#### Reconsidération Bangle.js 2 (décision 2026-05-01)

La session 2026-04-28 avait écarté la Bangle.js 2 pour deux raisons :
1. Prix 110€ trop élevé pour itérations multiples
2. Pas de buzzer (vibreur HF inaudible)

**Le second point a été ré-arbitré le 2026-05-01 :** côté bracelet maître, **le buzzer n'est pas indispensable au MVP v2**. Le modèle d'alerte peut s'appuyer uniquement sur le vibreur (seuil proche : N vibrations courtes / seuil lointain : N vibrations longues, par exemple). Le buzzer côté bracelet est repoussé en évolution future. Côté **balise**, le buzzer reste indispensable (≥85 dB pour localisation acoustique) — non concerné par cette reconsidération.

**Conséquence :** la Bangle.js 2 redevient une cible matérielle pertinente pour le proto firmware bracelet ET comme cible v3 commerciale low-volume.

| Avantage Bangle.js 2 | Détail |
|---|---|
| Même MCU que la cible XIAO custom | nRF52840 strict — code BLE/RSSI/encodage N portable à 100% |
| CE/FCC déjà acquise | Distribution EU possible sans certifier le bracelet final |
| Boîtier industriel IP67 | Pas besoin d'imprimante 3D pour démo terrain CNRHR / éducateurs |
| SWD exposé arrière | Firmware 100% remplaçable (BangleJS2BareMetal documenté) |
| Autonomie 1-2 semaines | Confortable pour test utilisateur étendu sans recharge |
| Sourcing pérenne | Espruino / Adafruit, distribution stable depuis 2021 |

| Limite Bangle.js 2 | Mitigation |
|---|---|
| Pas de buzzer audible | OK — buzzer bracelet repoussé en évolution future (décision 2026-05-01) |
| 1 seul bouton physique (vs 2 cible) | 2e bouton émulé via touchscreen / appui long. HAL `hal_button(0\|1)` pour rendre le portage trivial. |
| Écran tactile présent (vs aucun écran cible) | Discipline : développer "headless first", écran utilisé uniquement comme console debug. |
| Prix 110€ vs ~15€ XIAO custom | Acceptable pour 1 unité proto + démo. Pas pour itérations multiples balise. |

**Décision 2026-05-01 :** acquérir **1× Bangle.js 2** (chez Espruino ou Adafruit) en complément du panier XIAO custom existant. Rôle : plateforme proto firmware sur le bon MCU + dispositif portable certifié pour démos terrain dès semaine 4-6, avant le proto XIAO custom monté en boîtier 3D.

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

#### Court terme — proto firmware ← **mis à jour 2026-05-01**
**Option A — M5StickC Plus 2 + HAT Vibreur (~22€)** : ESP32, zéro refacto code Arduino/NimBLE, commandable immédiatement. Plateforme de développement BLE rapide.

**Option B — Bangle.js 2 (~110€) ← ajouté 2026-05-01** : nRF52840 strict (= MCU cible), CE/FCC acquise, boîtier IP67 pour démo terrain immédiate. Code portable à ~99 % vers XIAO. À combiner avec A.

**Option C — PineTime (~27€) ← ajouté 2026-05-01 re-évaluation** : nRF52832 (proche cible mais pas identique), InfiniTime/WaspOS mature, hardware + firmware **open source intégral**, prix permettant d'acheter 2-3 unités pour tests utilisateur sans douleur. Code métier portable à ~90 % vers XIAO 52840.

**Décision à arbitrer** : Option B seule (fidélité maximale au chip cible) / Option C seule (low-cost, plus d'unités terrain) / Combinaison B+C (couverture maximale ~137 €). Cf. tableau comparatif dans la section "Re-évaluation PineTime" plus haut.

#### Moyen terme — proto produit ← **décision session 2026-04-28, confirmée 2026-05-01**
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

#### Long terme — v3 commerciale ← **clarifié 2026-05-01**
Deux pistes parallèles, pas exclusives :

- **v3 low-volume / pré-commercial** : **Bangle.js 2 rebrandée N.I.C.O** + firmware custom flashé via SWD. CE/FCC acquise, boîtier industriel, sourcing pérenne. Permet une mise en marché rapide en marge du PCB custom, à coût unitaire élevé (~110€ + marge) mais sans investissement industriel. Cible : pilotes CNRHR, MDPH, démos institutionnelles. *Note : la PineTime pourrait théoriquement remplir ce rôle à coût bien inférieur (~27€), mais le compromis nRF52832 vs cible 52840 sur le produit final brouille la convergence hardware visée — à réserver au proto, pas au pré-commercial.*
- **v3 high-volume / MDPH** : **PCB custom nRF52840** ou **carte porteuse XIAO** dans boîtier injecté propriétaire. Coût unitaire bas (<30€ matière), CE à refaire sur produit fini (directive RED 2014/53/EU). Cible : déploiement scolaire ULIS, distribution accessoire d'assistance MDPH.

Le panier 2026-04-30 finance les deux pistes en parallèle : développement firmware sur Bangle.js 2 et/ou PineTime (achat séparé) + proto produit sur XIAO custom (panier AliExpress).

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

### Composants retenus — balise (par unité) ← **mis à jour 2026-05-01 : LiPo au lieu de CR2032**

| Composant | Spécification | Conso | Prix est. |
|---|---|---|---|
| XIAO nRF52840 | MCU + BLE Peripheral/Advertiser + chargeur LiPo intégré BQ25101 | ~5µA veille advertising | ~7.50€ |
| Buzzer piézo passif 3V | Ø9mm traversant, PWM 3kHz | ~20mA actif | ~0.30€ |
| Bouton tactile 4mm | Couper la sonnerie | 0 | ~0.05€ |
| Transistor NPN BC547 + résistance 1kΩ | Driver buzzer | — | ~0.15€ |
| **LiPo 200mAh 402030 + JST 1.25mm** | 3.7V nominal, 4×20×30mm, rechargeable USB-C via XIAO | ~5-10µA moyen | ~3.50€ |

**Décision 2026-05-01 — passage CR2032 → LiPo 200mAh rechargeable :**
- Unification du parc batterie : **même type de batterie pour le bracelet et les balises** → un seul câble USB-C pour tout recharger, un seul circuit de charge à connaître (BQ25101 onboard XIAO), simplification logistique terrain.
- **Énergie disponible légèrement supérieure** : 200 mAh × 3.7V = 740 mWh vs CR2032 220 mAh × 3.0V = 660 mWh (+12 %)
- **Pic de courant BLE TX et activation buzzer non problématiques** : la LiPo a une impédance interne très basse (<200 mΩ), elle absorbe les pics 10-50 mA sans chute de tension significative — le condensateur tantale 100 µF prévu pour découpler la CR2032 devient optionnel.
- **Rechargeable** → 0 € de pile à remplacer chaque année sur la durée de vie du produit (vs 3 piles CR2032/an = ~3 €/balise/an).
- **Ouvre la voie aux évolutions v2.1** (speaker pour TTS notamment, qui était strictement incompatible avec la CR2032 à cause des pics de courant).

**Calcul autonomie balise (LiPo 200mAh, advertising 1s) :**
```
XIAO advertising (intervalle 1s)   : ~8µA moyen
Buzzer alarme (rare)               : négligeable en moyenne
Total moyen estimé                 : ~8-10µA
Autonomie théorique                : ~2-2.5 ans
Autodécharge LiPo (~3 %/mois)      : ~30 %/an en pertes additionnelles
Autonomie réelle utilisateur       : **~12-18 mois entre recharges**, recharge USB-C en ~1 heure ✅
```

**Compromis vs CR2032 — points à connaître :**
- Boîtier balise légèrement plus volumineux (~30×30×10 mm pour loger la LiPo 402030 vs 25×25×6 mm pour CR2032 disc)
- Boîtier doit exposer le port USB-C du XIAO (cutout dans le 3D)
- Recharge périodique nécessaire (~1×/an) — friction d'usage marginale, à intégrer dans le protocole de maintenance terrain
- Plage de température LiPo plus étroite (-20 à +60 °C vs CR2032 -20 à +70 °C) — non bloquant pour usage intérieur scolaire

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
LiPo 200mAh USB-C    LiPo 200mAh USB-C    LiPo 200mAh USB-C
12-18 mois/recharge  12-18 mois/recharge  12-18 mois/recharge
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

## BOM commande groupée — 1 bracelet + 3 balises ← **mis à jour 2026-05-01 : LiPo partout**

| Composant | Qté | Source suggérée | Prix est. |
|---|---|---|---|
| XIAO nRF52840 | 4 (+ 2 spares = 6) | AliExpress / Seeed Studio | ~45€ |
| Vibreur disque 10mm | 5 (lot) | AliExpress | ~1€ |
| Buzzer piézo passif 3V Ø9mm | 10 (lot) | AliExpress | ~1.50€ |
| Boutons tactiles 4mm SMD | 20 (lot) | AliExpress | ~0.50€ |
| Transistors NPN 2N2222/BC547 | 20 (lot) | AliExpress | ~0.50€ |
| **LiPo 402030 200mAh JST 1.25mm** | **5 (1 bracelet + 3 balises + 1 spare)** | AliExpress | **~17.50€** |
| Bracelet silicone 20mm | 2 | AliExpress | ~3€ |
| Résistances 10kΩ + 1kΩ | 20+20 (lots) | AliExpress | ~1€ |
| **Total estimé** | | | **~70€** |

**Décision 2026-05-01 — unification batterie LiPo :**
- **CR2032 + supports retirés de la BOM essentielle** (étaient à 3€). Restent disponibles en option pour back-up ou expérimentations.
- **Condensateur tantale 100µF retiré** (non nécessaire avec LiPo basse impédance — était recommandé pour découpler la CR2032 pendant les pics BLE TX).
- **Quantité LiPo passée de 2 à 5** : 1 bracelet + 3 balises + 1 spare. Coût marginal +11.50€, simplifie la logistique de recharge (un seul type, un seul chargeur USB-C XIAO).

**Points d'attention commande :**
- LiPo : vérifier connecteur **JST PH 1.25mm** (pas 2.0mm) — format natif du XIAO
- LiPo : capacité réelle ≥180 mAh, circuit de protection intégré (surcharge / sous-décharge)
- Buzzer : choisir **passif** (sans oscillateur interne) pour contrôle PWM depuis nRF52840
- Boîtier balise 3D : prévoir cutout USB-C pour recharge sans démontage

---

## Impact sur le code existant

| Changement | BLE/NimBLE | UI/HAL | Risque |
|---|---|---|---|
| T-Watch → XIAO nRF52840 (Arduino) | ~40% réécriture (NimBLE → Bluefruit/NimBLE nRF) | ~30% | Moyen |
| T-Watch → XIAO nRF52840 (Zephyr) | ~100% réécriture | ~100% | Élevé |

Le code de scan/RSSI du bracelet ESP32 actuel sert de référence fonctionnelle mais ne transfère pas directement sur nRF52840. La logique métier (seuils RSSI, patterns vibration, machine à états) est réutilisable ; la couche BLE est à réécrire.

Filtre `"Holy"` sur le nom de balise : à remplacer par un filtre sur UUID iBeacon configurable lors du provisioning des balises.

---

## Pistes explicitement différées hors MVP v2 ← **ajouté 2026-05-01**

Les pistes suivantes ont été examinées et explicitement reportées en évolution v2.1 ou v3, pour préserver le focus du MVP. Elles sont documentées ici pour ne pas être ré-instruites de zéro plus tard.

### Synthèse vocale (TTS) sur le bracelet maître

**Idée :** remplacer (ou compléter) l'encodage N vibrations par une annonce vocale courte type *"balise 2 — clés"*, *"balise 1 hors de portée"*, *"appairage confirmé"*. Différenciateur UX majeur pour public malvoyant et particulièrement enfants ULIS — supprime la charge cognitive de décoder N vibrations.

**Hardware nécessaire** (ajouts au bracelet XIAO nRF52840) :
- DAC I2S **MAX98357A** (class D, ~2€ AliExpress)
- Mini-speaker 8Ω 0.5-1W Ø10-13mm (~1€)
- ~30-60 secondes d'échantillons WAV 16 kHz 8-bit pré-générés (espeak-ng → flash) tenant dans le 1 MB flash interne du nRF52840 — vocabulaire fermé d'environ 20-30 phrases courtes

**Coût marginal :** ~3-4€/bracelet, ~5g supplémentaires, autonomie quasi-préservée (TTS ne joue que quelques secondes par jour).

**Pourquoi pas au MVP v2 :**
- Décision utilisateur 2026-05-01 : *"la synthèse vocale n'est pas nécessaire à ce stade"*
- Augmente la complexité firmware (lecteur PCM I2S, gestion samples flash, transitions d'états)
- Aucune montre hackable nRF52840 du marché ne dispose d'un speaker — paradoxe documenté : les montres avec speaker BT Call ont migré vers Realtek RTL8763 et Actions ATS3085L, **précisément les chipsets non hackables**. Ce qui veut dire que la TTS ne peut pas se prototyper sur Bangle.js 2 — elle nécessite directement le proto XIAO custom + module audio.

**Réservation v2.1 :** garder 2 GPIO du XIAO libres pour I2S DOUT + BCLK + LRCLK (3 pins). Aucun impact sur le firmware MVP.

### Buzzer audible sur le bracelet maître

**Idée originale (PRD) :** escalade au seuil lointain en N bips sonores audibles pour localisation à distance.

**Pourquoi pas au MVP v2 :**
- Décision utilisateur 2026-05-01 : *"le buzzer n'est pas indispensable sur le bracelet, le vibreur peut être suffisant"*. L'escalade peut se faire en vibrations longues (vs courtes au seuil proche) sans perte d'information utile.
- Permet d'utiliser la **Bangle.js 2 sans modification hardware** comme proto et cible v3 low-volume (Bangle.js 2 n'a pas de buzzer audible).

**Réservation v2.1 :** la BOM XIAO custom inclut déjà un piézo passif Ø9mm dans les composants retenus du bracelet — il peut être ajouté ou non au moment de l'assemblage selon validation UX. Aucun coût marginal d'attendre.

### Speaker sur les balises

**Idée :** annonce vocale sur la balise elle-même (*"ici la balise 2"*) plutôt qu'un buzzer N bips. Améliore la localisation acoustique avec contexte sémantique.

**Pourquoi pas au MVP v2 :**
- Pic de courant speaker (~80-150 mA pendant 1-2 s) **incompatible avec CR2032** (chute de tension catastrophique) → forcerait la migration en LiPo, perdant l'avantage autonomie 12-18 mois et imposant une recharge périodique des balises.
- Alternative non hardware : jouer un sample audio court via le **piézo passif déjà prévu**, en PWM 8-bit PCM. Qualité médiocre mais nombres ("deux", "trois") reconnaissables — coût hardware nul, juste du firmware. À explorer en v2.1 sans changer la BOM balise.

### Migration nRF52832 (E73) sur les balises pour réduction coût

**Idée :** module E73-2G4M08S1C nRF52832 à ~3.50€ vs XIAO nRF52840 à ~7.50€. Sur 3 balises = ~12€ d'économie.

**Pourquoi pas au MVP v2 :**
- Décision 2026-04-28 : convergence sur plateforme unique XIAO nRF52840 pour simplifier la stack de dev (un seul SDK, une seule chaîne CI).
- À reconsidérer pour la **v3 high-volume** uniquement, quand le coût unitaire devient un facteur de marge.

### LoRa / Meshtastic pour Ti Poucet grande portée ← **exploré 2026-05-04, écarté pour v2**

**Idée :** équiper les balises d'un module LoRa (SX1262, 868 MHz) en plus du BLE, pour déclencher le buzzer à des distances supérieures aux 10-50m du BLE en intérieur. Protocole envisagé : Meshtastic (mesh LoRa open source).

**Ce qui est vrai :**
- LoRa 868 MHz pénètre mieux les murs que BLE 2.4 GHz (atténuation ~10-15 dB vs 15-20 dB par mur béton armé)
- Portée indoor réaliste : 50-150m à travers 2-3 murs (vs 5-15m pour BLE)
- Le nRF52840 peut piloter un SX1262 via SPI — architecture hybride BLE+LoRa physiquement réalisable
- Plateformes existantes : LILYGO T-Echo (nRF52840 + SX1262, ~40€) et RAK Wireless RAK4631 + module LoRa (~30-40€)
- LoRa ouvre un use case distinct : **Ti Poucet en espace ouvert** (forêt, campus, parc), où la distance dépasse 100m et BLE est inutilisable

**Pourquoi écarté pour le MVP v2 :**

1. **Meshtastic inadapté au contrôle d'actionneurs.** Conçu pour la messagerie texte humain-à-humain, pas pour déclencher un buzzer avec contrainte de latence. Latence constatée : 500ms à 10s. Taux de livraison sur un saut en indoor : ~80-90%. Overhead firmware : ~800 KB pour envoyer 10 bytes. Un protocole LoRa maison (10 bytes, adressage direct, ACK) serait 10× plus léger et fiable — mais ajoute un effort de développement non justifié au stade v2.

2. **LoRa inutile pour KeepClose.** KeepClose repose sur l'estimation de distance fine par RSSI BLE (1-20m). La variance RSSI LoRa est de ±10-15 dBm à position fixe — inutilisable pour la proximité. Ajouter LoRa ne bénéficie qu'à 50% du système (Ti Poucet uniquement).

3. **Budget énergétique critique.** SX1262 en réception continue : **4.6 mA** — CR2032 morte en 47h. La solution (duty cycle à 1% : écoute 30ms toutes les 3s) descend à ~60 µA moyen et préserve l'autonomie, mais introduit une latence de réception jusqu'à **3s** et complexifie le protocole (répétition de paquets côté bracelet, gestion des ACK).

4. **Problème d'audibilité non résolu.** LoRa permettrait de déclencher une balise à 100m — mais si la balise est à 100m dans un espace bruyant, l'utilisateur n'entend pas le buzzer. Le goulot d'étranglement n'est pas le déclenchement, c'est **l'audibilité**. LoRa ne le résout pas.

5. **Alternatives plus simples non épuisées :**
   - **BLE Coded PHY (Long Range, BLE 5.0)** : nRF52840 le supporte nativement, coût zéro. Portée 4-8× supérieure au BLE standard (50-150m en intérieur). À tester avant d'ajouter toute radio supplémentaire.
   - **Stratégie de placement** : deux balises à 15m d'intervalle dans un couloir de 30m coûtent moins cher qu'un module LoRa à 5€ et résolvent le problème sans complexité.

**Use case où LoRa reste pertinent (réservé v3+) :**

Ti Poucet en espace ouvert grande échelle (forêt, campus hospitalier, parc urbain) où BLE est mort par distance pure (>50m), pas par obstacle. Dans ce contexte, un protocole LoRa maison + mesh balise-à-balise ouvre un use case distinct impossible en BLE : les balises peuvent se relayer l'information ("l'utilisateur approche") sans hub central. Ce serait un **produit distinct** de KeepClose/Ti Poucet intérieur, à instruire séparément avec ses propres exigences utilisateurs.

**Plateformes documentées pour une exploration future :**
- **LILYGO T-Echo** (nRF52840 + SX1262 + E-ink, ~40€) : bon proto de validation du concept hybride, mais trop grand/cher pour une balise de production
- **RAK Wireless RAK4631 + RAK13300** (nRF52840 + SX1262 modulaire, ~35€) : meilleur candidat proto si l'exploration LoRa est relancée

### Cibles montres commerciales hackables (Magic 3, Rock, PineTime)

Examinées en session 2026-05-01. Statuts finaux :
- **Magic 3** : écartée — fin de vie commerciale, sourcing peu fiable
- **Rock** : écartée — peu testée, format rectangulaire, sourcing à valider
- **Tank, Oukitel BT, C26 et clones** : écartés — Realtek/Actions non hackables
- **PineTime** : **réintégrée** — voir section dédiée ci-dessous
- **Bangle.js 2** : conservée comme plateforme proto firmware + cible v3 commerciale low-volume

### Re-évaluation PineTime (2026-05-01)

La PineTime avait été écartée pour deux compromis cumulés : nRF52832 (au lieu de 52840) **et** absence de buzzer. La décision 2026-05-01 sur le buzzer bracelet non indispensable au MVP **fait tomber le second compromis**. La PineTime redevient une option proto bracelet sérieuse, à mettre en parallèle de la Bangle.js 2.

**Comparaison proto bracelet — décision à instruire :**

| Critère | PineTime (~27 €) | Bangle.js 2 (~110 €) |
|---|---|---|
| MCU | nRF52832 (proche cible) | **nRF52840 (identique cible XIAO)** ✅ |
| RAM | 64 KB | 256 KB |
| Batterie | 180 mAh | 175 mAh |
| Autonomie annoncée | ~7 jours usage typique InfiniTime | 2 semaines (display 1×/sec) à 1 mois (1×/min) |
| Autonomie estimée scan BLE continu N.I.C.O | ~3-5 jours | ~5-7 jours (avantage écran MIP transflectif sans backlight permanent) |
| Vibreur | ✅ | ✅ |
| BLE Central scan | ✅ (BLE 4.2) | ✅ (BLE 5.0) |
| Charge | Pogo magnétique | Pogo magnétique |
| Firmware open source | **InfiniTime mature (C++/FreeRTOS)** + WaspOS | Espruino (JS) ou bare-metal C |
| Hardware open source | **✅ schémas + PCB + bootloader publics** | Partiel (schémas) |
| CE | ✅ Pine64 | ✅ commercial Espruino |
| Sourcing | Cyclique (restocks par lots, courant mi-mai 2026) | Stable continu chez Espruino / Adafruit |
| Portabilité code → XIAO 52840 | ~90 % (BLE 5.0 features non testables sur 52832) | ~99 % (chip identique) |
| Pérennité écosystème | PineTime Pro annoncé mars 2026 → vitalité confirmée | Espruino actif depuis 2014, stable |
| Communauté | Très active (InfiniTime, WaspOS, fanoush, atc1441) | Active (Espruino, Gordon Williams) |
| Prix unitaire | **27 €** | 110 € |

**Stratégies envisageables :**

1. **PineTime seule (~27-80 € pour 1-3 unités)** — option low-cost. On accepte le compromis chipset 52832. Avantages : prix permettant de tester sur plusieurs unités, donner à un utilisateur sans douleur, écosystème open source intégral. Risque : ~10 % de code à réécrire lors du portage final XIAO 52840 (BLE 5.0 specifics si utilisés).
2. **Bangle.js 2 seule (~110 €)** — option fidélité maximale à la cible. Code portable à 99 %. Démos terrain immédiates dans un boîtier industriel certifié CE. Coût élevé pour itérer.
3. **Combinaison PineTime + Bangle.js 2 (~137 €)** — PineTime pour démos terrain bon marché et UX, Bangle.js 2 pour développement firmware sur le bon MCU. Couverture maximale, +27 € seulement par rapport à Bangle.js 2 seule.

**Recommandation à arbitrer avec les équipes** : la stratégie 3 (combinaison) maximise la couverture des risques pour un surcoût marginal. Si le budget est contraint, la stratégie 1 (PineTime seule) est viable parce que la logique métier (RSSI, encodage N, machine à états) porte d'un nRF52 à l'autre sans réécriture significative.

#### Enjeux stratégiques au-delà du prix d'achat (2026-05-01)

**1. Dépendance fabricant — risque borné à la phase proto.** Le choix d'une montre sur étagère accélère la production de proto et de démos, mais introduit une dépendance au fabricant (sourcing, pérennité). Cette dépendance est **bornée à la phase de développement** : le matériel cible final reste le XIAO nRF52840 (Seeed Studio, grand fournisseur stable). La montre proto est un outil de transition, pas une dépendance long terme.

**2. Effet falaise PineTime à anticiper.** Pine64 a historiquement retiré les modèles originaux du marché quand le successeur sort (cf. cycle PineTime → PineTime Pro annoncé mars 2026). Risque : si on bâtit le développement firmware sur PineTime et qu'elle disparaît dans 12-18 mois, les démos institutionnelles n'ont plus de plateforme tant que le PCB custom n'est pas prêt. **Mitigation : commander 3-4 unités en un lot pour absorber l'effet falaise.**

**3. Bangle.js 2 prix dissuasif — justifiable uniquement par la fidélité chip.** À 110 € unitaire (4 × le prix d'une PineTime), la Bangle.js 2 n'est rationnelle que si on valorise très haut la portabilité du code (~99 % vs ~90 %). Pour un proto destiné à valider l'UX et les seuils RSSI, la PineTime suffit fonctionnellement. La Bangle.js 2 prend tout son sens si **on veut écrire le firmware une fois et le déployer tel quel sur le XIAO** sans refacto BLE.

**4. Communautés open source — opportunité d'association.** Espruino et Pine64 ne sont pas de simples fournisseurs commerciaux. Ce sont **des communautés open source vivantes** (firmware, hardware, documentation, contributions extérieures) dont les valeurs s'alignent souvent avec les missions accessibilité/éducation. Apports possibles d'une association explicite :
- Visibilité du projet auprès d'une audience technique solidaire (apps Espruino, watchfaces InfiniTime / WaspOS)
- Contributions logicielles entrantes (community-driven) sur les besoins spécifiques malvoyants
- Co-développement possible — Pine64 a déjà publié des dispositifs adaptés (PinePhone accessible)
- Effet de levier communication ("Built on open hardware") pour les acteurs financeurs sensibles à la transparence (MDPH, fondations)

**Question complémentaire à porter dans l'arbitrage équipes :** souhaitons-nous nous positionner publiquement en partenaire d'une de ces communautés (Espruino, Pine64, ou les deux) ? Sous quelle forme : contributions logicielles upstream, mention dans les communications, présentation à un événement open hardware ?

---

## Questions ouvertes

### Bracelet
- [ ] Valider autonomie XIAO nRF52840 + 200mAh LiPo en mode scan continu (1s interval)
- [x] ~~Décision : proto firmware sur T-Watch 2020 v3 d'abord ou directement XIAO nRF52840 ?~~ → **Tranché 2026-05-01** : proto firmware sur Bangle.js 2 (même MCU que la cible) + banc de test rapide M5StickC Plus 2 (ESP32) en complément. T-Watch v3 abandonnée.
- [ ] Concevoir boîtier 3D 30×20×10mm avec barrettes bracelet 20mm
- [x] ~~Valider que le buzzer passif Ø9mm est audible dans le boîtier~~ → **Différé 2026-05-01** : buzzer bracelet repoussé en évolution v2.1, vibreur seul au MVP.
- [ ] **Décision à arbitrer (2026-05-01)** : proto bracelet sur Bangle.js 2 (~110€, MCU strictement identique cible) / PineTime (~27€, nRF52832 proche, open source intégral) / combinaison des deux (~137€). Cf. section "Re-évaluation PineTime".

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