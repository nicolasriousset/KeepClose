# KeepClose – Propositions d'améliorations techniques

## Résumé de l'existant

Le prototype actuel repose sur :
- **Bracelet** : LilyGo T-Watch 2020 v3 (ESP32, écran couleur tactile, moteur de vibration, batterie ~300 mAh) — ~50 €
- **Balise** : iBeacon BLE (type Holy-IoT) placée sur la canne
- **Firmware** : scan BLE toutes les 2 s, machine d'état WAITING → GUARDING → WARNING → ALARM, alerte par vibration

Points faibles identifiés : coût élevé du bracelet, autonomie insuffisante (ESP32 + écran = consommation importante), et quelques bugs dans le firmware.

---

## 1. Corrections firmware prioritaires

### 1.1 Bug dans `updateLabels()` — état WARNING jamais affiché

```cpp
// KeepClose.ino, lignes 207-208 — code mort, jamais atteint
} else if (state == GUARDING) {   // ← toujours faux ici
    lv_label_set_text(label_distance, "Attention !");
```

La condition `state == GUARDING` est vérifiée deux fois de suite ; la seconde branche est morte. L'état WARNING ne s'affiche jamais à l'écran. Corriger en remplaçant par `state == WARNING`.

### 1.2 Scan BLE actif inutile

```cpp
pBLEScan->setActiveScan(true);  // ligne 139
```

Le scan **actif** envoie des requêtes à chaque appareil détecté, ce qui consomme davantage de radio. Les balises iBeacon diffusent en **broadcast** : un scan **passif** (`setActiveScan(false)`) est suffisant et réduit la consommation BLE.

### 1.3 Fenêtre BLE saturée

```cpp
pBLEScan->setInterval(100);  // ligne 140
pBLEScan->setWindow(100);    // ligne 141
```

Interval = Window signifie que la radio BLE est active **100 % du temps** pendant le scan. Réduire le ratio (ex. window=50, interval=200) économise ~60 % de la consommation radio sans impact notable sur la détection d'une balise statique.

### 1.4 Détection de perte de signal

Actuellement, si la balise sort de portée et n'est plus du tout détectée, l'état ne change pas (le dernier `beaconDistance` reste en mémoire). Il faudrait déclencher une alerte si aucune détection n'a eu lieu depuis N secondes :

```cpp
// Exemple : timeout de 6 secondes sans détection → ALARM
if (state == GUARDING && millis() - lastDetectionTime > 6000) {
    state = ALARM;
    turnOnScreen();
}
```

### 1.5 `updateLabels()` appelé à chaque itération de boucle

La fonction est appelée toutes les 100 ms même si rien n'a changé, forçant des re-renders LVGL inutiles. L'appeler uniquement lors des transitions d'état (déjà fait partiellement) suffit.

---

## 2. Améliorations algorithmiques

### 2.1 Hysteresis sur les transitions d'état

L'estimation de distance via RSSI fluctue de ±30 à 50 %. Sans hystérésis, l'état oscille entre WARNING et GUARDING autour du seuil, provoquant des vibrations intempestives. Appliquer un seuil bas/haut différencié :

| Transition | Seuil actuel | Seuil proposé |
|---|---|---|
| GUARDING → WARNING | > 10 m | > 12 m |
| WARNING → GUARDING | < 10 m | < 8 m |
| GUARDING → ALARM | > 20 m | > 25 m |
| ALARM → WARNING | < 20 m | < 18 m |

### 2.2 Filtre de Kalman à la place de la moyenne glissante

La moyenne glissante sur 3 mesures est simple mais réagit lentement aux vrais changements et mal aux pics. Un filtre de Kalman mono-dimensionnel (quelques dizaines de lignes en C++) s'adapte automatiquement au bruit et converge plus vite lors d'une vraie variation de distance. Des implémentations légères existent pour ESP32 (bibliothèque `SimpleKalmanFilter`).

### 2.3 Utiliser le TxPower embarqué dans la trame iBeacon

Le firmware utilise déjà `oBeacon.getSignalPower()`, ce qui est bien. S'assurer que la balise est configurée avec le TxPower mesuré à 1 m (valeur de calibration), car c'est le paramètre clé de l'estimation de distance.

---

## 3. Réduction des coûts matériels

### 3.1 Remplacer la T-Watch par un bracelet sur mesure

La T-Watch 2020 v3 (~50 €) embarque des composants inutiles pour ce cas d'usage : écran tactile couleur, GPS, capteur cardiaque, etc. Un bracelet dédié peut être construit pour **5 à 15 €** en série.

**Option A — ESP32-C3 (faible coût, écosystème Arduino)**

| Composant | Coût unitaire (AliExpress, qté 10) |
|---|---|
| Module ESP32-C3 SuperMini | ~1,50 € |
| Batterie LiPo 400 mAh | ~2 € |
| Circuit de charge TP4056 | ~0,30 € |
| Moteur vibreur + driver | ~0,50 € |
| Buzzer piézo (alerte sonore) | ~0,20 € |
| Bouton + boîtier imprimé 3D | ~1 € |
| **Total** | **~5,50 €** |

Avantage : compatibilité totale avec le firmware existant (Arduino + NimBLE).  
Inconvénient : ESP32-C3 en veille légère consomme ~130 µA ; deep sleep ~5 µA mais nécessite de reconfigurer le BLE à chaque réveil.

**Option B — nRF52840 (meilleure autonomie, BLE natif)**

| Composant | Coût unitaire |
|---|---|
| Module Seeed XIAO nRF52840 | ~10 € |
| Batterie LiPo 400 mAh | ~2 € |
| Moteur vibreur + driver | ~0,50 € |
| Buzzer + boîtier | ~1,20 € |
| **Total** | **~14 €** |

Avantage majeur : le nRF52840 consomme **~2 µA en deep sleep avec BLE en écoute** (mode advertiser scanning du coprocesseur BLE), contre ~20–50 mA pour l'ESP32 en scan actif. Autonomie potentielle de **plusieurs semaines** sur 400 mAh.  
Inconvénient : migration du firmware vers SDK nRF ou Zephyr (ou Arduino avec la bibliothèque ArduinoBLE).

### 3.2 Choix de la balise

La balise actuelle (Holy-IoT) fonctionne bien. Alternatives moins chères :

| Balise | Prix | Autonomie | Notes |
|---|---|---|---|
| Holy-IoT iBeacon | ~8 € | 1–2 ans (CR2477) | Utilisée actuellement |
| Minew E7 | ~6 € | 2 ans | Bien documentée, UUID configurable |
| MOKOSmart B10 | ~5 € | 1–3 ans | Format carte de crédit, discret |
| DIY nRF51822 | ~2 € | >2 ans | Nécessite programmation |

---

## 4. Améliorations de l'autonomie (firmware)

### 4.1 Réduire la fréquence de scan en état GUARDING

En état de surveillance normale (GUARDING), scanner toutes les 2 s est excessif. La canne est posée : elle ne bougera pas vite. Adapter la fréquence selon l'état :

| État | Fréquence de scan proposée | Justification |
|---|---|---|
| WAITING | 1 scan / 5 s | Recherche initiale, pas d'urgence |
| GUARDING | 1 scan / 3 s | Surveillance normale |
| WARNING | 1 scan / 1 s | Situation dégradée, réactivité accrue |
| ALARM | 1 scan / 1 s | Alerte active |

### 4.2 Utiliser le Light Sleep de l'ESP32 entre les scans

L'ESP32 supporte un mode **Light Sleep** où le CPU et la plupart des périphériques sont suspendus, mais la RAM est conservée et le réveil est rapide (~1 ms). Consommation : ~0,8 mA vs ~80 mA en actif.

```cpp
// Entre deux scans, mettre l'ESP32 en light sleep
esp_sleep_enable_timer_wakeup(SCAN_INTERVAL_US);
esp_light_sleep_start();
// Le CPU reprend ici après le timer
```

Compatibilité avec NimBLE : vérifier que le stack BLE est mis en pause proprement avant le sleep.

### 4.3 Éteindre l'écran par défaut dès le démarrage

Actuellement, `turnOnScreen()` est appelé dans `setup()`. Pour un bracelet sans écran ou pour économiser la batterie dès le boot, démarrer écran éteint et ne l'allumer qu'en cas d'alerte ou de bouton.

### 4.4 Réduire la durée du scan

`SCAN_DURATION = 1000 ms` est conservative. Une balise iBeacon émet typiquement toutes les 100–1000 ms. Un scan de **500 ms** suffit généralement à la détecter, économisant 500 ms de radio active par cycle.

---

## 5. Améliorations fonctionnelles

### 5.1 Alerte progressive

Plutôt qu'une vibration continue à 1 Hz, une alerte graduée est moins fatigante et plus informative :

- **10–15 m** : vibration courte toutes les 3 s
- **15–20 m** : vibration courte toutes les 1 s
- **> 20 m ou signal perdu** : vibration longue en continu

### 5.2 Confirmation sonore de l'appairage

Quand la balise est détectée pour la première fois (WAITING → GUARDING), émettre une vibration courte "OK" pour confirmer à l'utilisateur que la surveillance est active — sans nécessiter d'écran.

### 5.3 Mode nuit / discret

Permettre de désactiver la vibration et utiliser uniquement un signal sonore faible (buzzer), ou l'inverse, selon le contexte (salle de classe, nuit).

---

## 6. Synthèse et recommandations

### Court terme (firmware, sans changement matériel)

1. Corriger le bug `updateLabels()` état WARNING
2. Passer en scan passif (`setActiveScan(false)`)
3. Réduire le ratio window/interval BLE (window=50, interval=200)
4. Ajouter la détection de perte de signal (timeout)
5. Adapter la fréquence de scan selon l'état

**Gain estimé sur autonomie : +20 à +40 %** sur la T-Watch actuelle.

### Moyen terme (nouveau hardware, même firmware)

6. Prototyper sur ESP32-C3 SuperMini (~5,50 €/unité)
7. Supprimer l'écran (bracelet sans affichage)
8. Implémenter le Light Sleep ESP32

**Gain estimé : coût divisé par ~9, autonomie x2 à x4** (batterie 400 mAh → 1 à 2 jours).

### Long terme (cible production)

9. Migrer vers nRF52840 (Zephyr ou ArduinoBLE)
10. Boîtier bracelet imprimé 3D ou moulé
11. Charge sans fil (Qi)

**Cible : autonomie > 1 semaine, coût matériel < 15 €/unité.**

---

*Document rédigé le 19 avril 2026 — analyse basée sur le code source `KeepClose.ino` (commit d63fcdb)*
