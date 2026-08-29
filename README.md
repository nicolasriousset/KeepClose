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
| D0 | Moteur vibrant | Via transistor 2N2222 |
| D1 | Bouton push | Actif bas, pull-up interne |
| LED_RED / LED_GREEN / LED_BLUE | LED RGB intégrée | Tricolore |

**Câblage moteur vibrant (D0) :**

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

> ⚠️ **Moteur (+) sur 3V3, PAS sur VUSB.** Les pastilles `3V3` / `GND` / `VUSB` se suivent
> sur la même colonne. Un moteur soudé par erreur sur VUSB vibre parfaitement sur USB et
> reste **totalement muet sur batterie** (VUSB = 0 V sans câble) — tout le reste (LED, BLE,
> bouton) continuant de fonctionner, le défaut passe pour un bug firmware.
> Vérification : sur batterie, USB débranché, mesurer moteur (+) → GND = **~3,3 V**.

**Câblage bouton (D1) :**

```
D1 ──── [Bouton] ──── GND
```

Pas de résistance externe, pull-up interne activée par `INPUT_PULLUP`.

---

### Balise (tag)

| Broche | Composant | Notes |
|--------|-----------|-------|
| D1 | Buzzer 12 mm | Connexion directe — actif ou passif, voir ci-dessous |
| D2 | Bouton push | Actif bas, pull-up interne |
| LED_RED / LED_GREEN / LED_BLUE | LED RGB intégrée | Tricolore |

**Câblage buzzer :**

```
Passif, push-pull (recommandé)      Actif, ou passif simple
─────────────────────────────       ───────────────────────
D1 ──── Buzzer                      D1 ──── Buzzer (+)
D0 ──── Buzzer (autre borne)        GND ─── Buzzer (-)
```

Un piézo n'est pas polarisé : en push-pull les deux bornes sont équivalentes.
Un buzzer **actif** l'est, lui — « + » sur D1, l'autre fil sur GND, et pas de
push-pull.

**Actif ou passif : le firmware doit le savoir.** Les deux composants se
ressemblent mais se pilotent de façon incompatible, et `firmware/tag/config.h`
doit être réglé en conséquence :

| Type | Ce qu'il contient | Pilotage | Réglage `config.h` |
|------|-------------------|----------|--------------------|
| Actif | Un oscillateur intégré | Tension continue (`digitalWrite`) | `// #define BUZZER_PASSIVE` |
| Passif | Un simple transducteur | PWM à sa résonance | `#define BUZZER_PASSIVE` |

Le mauvais réglage ne donne pas « un son un peu faible » mais un symptôme franc :
un **passif piloté en continu ne fait que « tic »** à chaque front, sans rien
entre les deux. (Un actif piloté en PWM, lui, sonne quand même — sa propre note,
un peu plus faible : moins visible, mais à corriger aussi.)

En cas de doute, un croquis jetable tranche en 10 s : appliquer une tension
continue 1 s (`digitalWrite(HIGH)`), puis balayer 1 → 5 kHz en carré. Un actif
sonne dans les deux cas et sa note ne suit pas le balayage ; un passif ne fait
que « tic » sous tension continue et joue la gamme au balayage. Le balayage
relève au passage la **résonance** du passif, à reporter dans `BUZZER_FREQ_HZ` —
ce n'est pas un détail de confort : hors résonance, un piézo perd 15 à 20 dB.

### Volume

Ce qui se règle en logiciel, dans l'ordre du gain obtenu :

1. *La fréquence* (`BUZZER_FREQ_HZ`), **15 à 20 dB** entre le pic et le hors
   résonance. Le plus gros levier, et le seul gratuit — à relever au balayage,
   pas à deviner. Réglée à **2700 Hz**, retenu à l'oreille : un peu en deçà du
   maximum, mais tenable près de l'oreille sur une sonnerie longue. Voir la mise
   en garde dans `firmware/tag/config.h` avant de la remonter.
2. *Le push-pull* (`BUZZER_PUSH_PULL`), **+6 dB**. L'élément est câblé entre D1
   et D0 pilotées en opposition de phase : 6,6 Vpp au lieu de 3,3. Les deux
   voies sortent de la même instance PWM, l'opposition est donc exacte par
   construction. Attention au faux négatif dans les deux sens — buzzer resté sur
   GND avec l'option active → volume inchangé ; buzzer entre D1 et D0 avec
   l'option commentée → silence. **Pour trancher, piloter D0 seule**, D1 tenue à
   0 : un piézo n'étant pas polarisé, ça ne peut sonner que si le fil a bien
   quitté GND.
3. *Le drive GPIO*, quelques dB. Déjà appliqué : `pinMode()` laisse la broche en
   « standard drive » (~2 mA sous 0,4 V de chute), `buzzerHighDrive()` passe en
   H0H1 (~5–9 mA).
4. *L'évent.* Beaucoup de buzzers sont livrés avec un autocollant de protection
   sur le trou : à retirer. Un boîtier fermé sans trou en face coûte une dizaine
   de dB.

**Le plafond, et pourquoi le logiciel n'ira pas plus loin.** Le SPL d'un piézo
suit la tension à ses bornes. Un couloir bruyant, c'est 60–70 dB ambiants ; pour
qu'une balise s'y entende nettement il faut de l'ordre de 85–90 dB à 1 m, soit
~105 dB à 10 cm — la référence usuelle des fiches techniques, le SPL perdant 6 dB
par doublement de distance. Un piézo alimenté en 3,3 V (6,6 en push-pull) est
20 dB en dessous. Les quatre points ci-dessus optimisent le montage ; ils ne
comblent pas cet écart. Il faut changer quelque chose au matériel :

| Piste | Gain | Coût |
|-------|------|------|
| **Pastille nue → buzzer en boîtier** | 15–25 dB | le composant seul |
| **Buzzer actif fort (≥ 95 dB à 10 cm) + transistor sur VBAT** | 20–30 dB | 1 transistor, 1 résistance, 1 diode |
| **Self 10–33 mH en série + transistor** | 15–20 dB | 2 composants, et il faut régler l'accord |

La première ligne est à vérifier avant tout achat : une **pastille de laiton nue**
(le disque doré, sans habillage) n'a aucune cavité de résonance et se traîne 15 à
25 dB sous un buzzer **en boîtier** (le cylindre noir percé d'un trou) de même
diamètre. Passer de 12 à 15 mm ne change presque rien à côté de ça — le diamètre
joue à la marge, la cavité joue tout.

Pour un dispositif d'assistance, la deuxième ligne est la plus raisonnable : un
buzzer **actif** fort ramène au pilotage `digitalWrite` (commenter
`BUZZER_PASSIVE`), supprime la question de la résonance, et reprend le montage à
transistor déjà utilisé pour le moteur du bracelet. La contrepartie est qu'on
perd la maîtrise de la note émise.

**Câblage bouton (D2) :**

```
D2 ──── [Bouton] ──── GND
```

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

### Bouton bracelet (D1)

L'action dépend de la **durée** de l'appui, et se joue au **relâchement** — seule
façon de distinguer un appui court d'un maintien.

**Appui court** — 3 comportements selon l'état :

| Situation | Action |
|-----------|--------|
| Ring en cours | Annule le ring (balise arrête de sonner) |
| Moteur vibre (balise hors périmètre) | Snooze : moteur s'arrête. Reprend automatiquement à la prochaine sortie du périmètre après un retour dans le rayon |
| Rien (balise à portée) | Démarre le ring : la balise appairée la plus proche sonne pendant 10 s |

**Appuis longs** — retrait d'appairage :

| Durée | Action | Repère pendant l'appui | Confirmation |
|-------|--------|------------------------|--------------|
| ≥ 3 s | Oublie les balises que le bracelet **ne détecte plus** (perdue, batterie à plat, restée ailleurs). Les balises captées sont conservées, même hors périmètre | 1 impulsion à 3 s | 2 impulsions longues (1 seule = rien à oublier) |
| ≥ 10 s | Efface **tous** les appairages | 2 impulsions à 10 s | 5 impulsions courtes |

L'impulsion de repère à chaque palier permet de savoir quand relâcher sans
compter les secondes. L'alerte est suspendue tant que le bouton est maintenu,
pour que les motifs restent lisibles.

Le critère d'oubli est « non détectable », pas « en alerte » : une balise jamais
revue depuis le démarrage a `tagCrc == 0` et n'est donc jamais désignée dans
`alertHash` — un critère fondé sur l'alerte la manquerait précisément dans le cas
qui nous intéresse.

> Après un effacement, une balise laissée au contact du bracelet se ré-appaire
> d'elle-même au bout de 3 s. Éloigner les balises avant de réinitialiser.

### Bouton balise (D2)

| Durée | Situation | Action |
|-------|-----------|--------|
| Court | Buzzer en cours | Acquitte la sonnerie (buzzer s'arrête). Se réinitialise automatiquement quand le bracelet arrête de publier le ring |
| ≥ 10 s | — | Oublie le bracelet appairé. Bip de repère à 10 s, 5 bips de confirmation |

L'appairage étant bidirectionnel, oublier une balise côté bracelet ne l'efface
pas côté balise : si elle réapparaît, elle affichera rouge jusqu'à un
ré-appairage au contact — qui fonctionne sans reset. L'appui long sur D2 sert à
repartir d'un état propre sans re-flasher.

---

## Architecture logicielle

### Protocole BLE

**Paramètres radio** — tous réglés explicitement, aucun défaut de bibliothèque :

| Paramètre | Valeur | Défini dans |
|-----------|--------|-------------|
| Puissance TX (les deux appareils) | 0 dBm | `common/config.h` → `BLE_TX_POWER` |
| Advertising balise | 150 ms, constant | `tag/config.h` → `ADV_INTERVAL_MS` |
| Advertising bracelet | 300 ms | `scanner.ino` |
| Scan (les deux appareils) | fenêtre 50 ms / intervalle 100 ms | `*.ino` |

> ⚠️ `BLE_TX_POWER` conditionne toute la calibration de distance. Le modifier
> décale tous les RSSI d'autant, et impose de reporter le décalage sur
> `RSSI_AT_1M`, `ALERT_RSSI_ENTER`, `ALERT_RSSI_EXIT` et
> `PAIRING_RSSI_THRESHOLD` — sans quoi le périmètre d'alerte change en silence.

L'advertising de la balise utilise un intervalle **unique** (`fast == slow`)
plutôt que le découpage 20 ms / 152,5 ms par défaut du BSP : le bracelet en tire
son débit de mesures, et un débit variable rendrait sa latence d'alerte
imprévisible. À 150 ms, le bracelet capte ~3,3 paquets/s et alerte en ~3,9 s.

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

**La décision d'alerte se prend en dBm, pas en mètres.** Les deux sont
équivalents (la conversion est monotone), mais une erreur de calibration est un
simple décalage constant en dBm — corrigeable par une seule mesure terrain —
alors qu'en mètres c'est une erreur multiplicative qui croît avec la distance.
La distance affichée dans les logs n'est qu'indicative.

**Modèle de propagation** (`common/config.h`) :

```
RSSI(d) = RSSI_AT_1M − 10 × PATH_LOSS_N × log10(d)
```

| Constante | Valeur | Signification |
|-----------|--------|---------------|
| `RSSI_AT_1M` | −72 dBm | RSSI **reçu** à 1 m — pas la puissance d'émission |
| `PATH_LOSS_N` | 2.65 | Exposant de perte de trajet, mesuré en intérieur |

> ⚠️ Ne pas reprendre les valeurs des tables iBeacon génériques (−59 / −61 dBm,
> n = 2.0) : elles supposent une antenne externe en espace libre. Avec l'antenne
> chip du XIAO et le corps de l'utilisateur, elles surestimaient la distance
> d'un facteur ~3,5 — 3,25 m réels affichés 18 m.

**Chaîne de filtrage** : médiane glissante sur 5 paquets → EMA (`alpha = 0.25`)
→ hystérésis → confirmation sur 3 mesures consécutives. La médiane élimine les
paquets aberrants dus aux trajets multiples ; la confirmation empêche qu'un
creux de bruit isolé ne bascule l'alerte.

**Seuils d'alerte** :

| Constante | Valeur | Distance |
|-----------|--------|----------|
| `ALERT_RSSI_ENTER` | −80 dBm | vert → rouge au-delà de ≈ **2,0 m** |
| `ALERT_RSSI_EXIT` | −76 dBm | rouge → vert en deçà de ≈ **1,4 m** |

Latence d'alerte mesurée en simulation : 2,9 s à 6 paquets/s, 4,1 s à 3 paquets/s.

#### Procédure de recalibrage

À refaire après tout changement de boîtier, de puissance d'émission ou
d'environnement — le modèle dépend autant du montage que de la physique.

1. Ouvrir le moniteur série du bracelet (115200 bauds) et relever la colonne
   `med` du heartbeat, seule valeur stable (le RSSI brut fluctue de ±3 dB) :

   ```
   [hb] t=120s  balises vues=1  appairees=1  [2.0m -80dBm(med -80, ema -80) dans perimetre]
   ```

2. Mesurer à **deux** distances écartées, balise immobile, bracelet porté au
   poignet dans sa position d'usage — par exemple 0,5 m et 3 m. Attendre 10 s
   de stabilisation à chaque point.

3. Calculer :

   ```
   PATH_LOSS_N = (RSSI₁ − RSSI₂) / (10 × log10(d₂ / d₁))
   RSSI_AT_1M  = RSSI₁ + 10 × PATH_LOSS_N × log10(d₁)
   ```

4. Reporter les deux constantes dans `common/config.h`, puis en déduire les
   seuils pour la distance d'alerte souhaitée :

   ```
   ALERT_RSSI_ENTER = RSSI_AT_1M − 10 × PATH_LOSS_N × log10(d_alerte)
   ALERT_RSSI_EXIT  = ALERT_RSSI_ENTER + 4        (bande d'hystérésis)
   ```

Le bracelet réaffiche ces seuils convertis en mètres au démarrage, ce qui permet
de vérifier la cohérence du réglage sans calcul.

La balise ne calcule pas de distance — elle réagit uniquement aux champs alertHash/ringHash publiés par le bracelet.

### Multi-balises

Le bracelet peut gérer jusqu'à 10 balises appairées simultanément.
Règle d'alerte : parmi les balises hors périmètre, le bracelet publie le CRC de la **plus proche** dans alertHash. L'utilisateur la récupère en premier, puis le bracelet passe à la suivante.

### Consommation — écart avec le budget prévu

⚠️ **Point ouvert, non résolu.** Les autonomies annoncées dans les documents de
conception (12-18 mois pour une balise, 7-10 jours pour le bracelet) supposaient
une balise qui se contente d'advertiser (~8 µA) et un bracelet qui scanne à
intervalle 1 s (~200 µA).

Depuis le passage à la diffusion d'état par advertising, **les deux appareils
scannent en permanence à 50 % de rapport cyclique** (fenêtre 50 ms / intervalle
100 ms). Le récepteur radio du nRF52840 consomme ~4,6 mA :

```
Scan 50 % du temps          : ~2,3 mA
Advertising 150 ms          : ~0,17 mA
MCU + softdevice            : ~0,3-1 mA
Total estimé                : ~3 mA
Autonomie sur LiPo 200 mAh  : ~3 jours
```

Soit un facteur ~150 sur l'objectif balise. L'estimation vient des chiffres du
datasheet et **reste à confirmer au multimètre**, en série sur la batterie.

L'intervalle d'advertising n'est pas le levier : il pèse 5 % du budget. Le levier
est le rapport cyclique du scan, mais le réduire ralentit d'autant la réaction de
la balise à une commande ring — arbitrage produit à trancher, pas réglage.

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
