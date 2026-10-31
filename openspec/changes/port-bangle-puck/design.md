# Design

## Context

Le comportement à porter est celui du firmware Arduino actuel :
- `firmware/scanner/` : `scanner.ino`, `scanner_core.h`, `project_nico.h`, `project_tipoucet.h` ;
- `firmware/tag/tag.ino` ;
- `firmware/common/config.h`.

Les exigences sont décrites dans `specs/`. Les motivations sont dans `proposal.md`.

Constats sur le code existant qui orientent le portage :
- **Échanges** : le bracelet et la balise ne communiquent déjà **que par advertising**. Le bracelet publie `[0xFFFF | alertHash | ringHash]` et la balise publie `[0xFFFF | crc bracelet propriétaire]`. Le service GATT « ring » est déclaré sur la balise, mais `sendRingCommand()` n'est pas implémentée : le mode Ti Poucet ne sonnait donc jamais réellement.
- **Hystérésis** : celle de la LED du bracelet est globale (variable `prevOutOfRange` partagée par toutes les balises), alors que l'alerte n'utilise que le seuil éloigné.
- **Balise et perte du bracelet** : la balise ne coupe jamais une sonnerie quand elle cesse de recevoir le bracelet.
- **Configuration** : le mode (N.I.C.O. / Ti Poucet) est choisi à la compilation.

Contraintes Espruino :
- l'interpréteur est mono-thread, sans `delay()` : tout passe par `setTimeout`/`setInterval` et des callbacks ;
- RAM limitée : environ 64 kB de variables sur Bangle.js 2, moins sur Puck.js ;
- sur la Bangle, chaque changement d'app (`load()`) **réinitialise l'interpréteur** et relance tous les fichiers `*.boot.js` ;
- l'advertising de la Bangle est partagé avec d'autres apps (Gadgetbridge, etc.) ;
- le Puck.js est alimenté par une pile CR2032 (environ 230 mAh), ce qui impose de mesurer chaque mA.

## Goals / Non-Goals

**Goals :**
- isoler toute la logique de décision (lissage, distance, hystérésis, appairage, choix alerte/sonnerie, machine d'état de l'action utilisateur, Ti Poucet) dans du JavaScript pur, testable sous Node, partagé par les deux appareils ;
- réduire le code propre à chaque appareil à un « câblage » : scan, advertising, vibreur, LEDs, piézo, stockage, interface.

**Non-Goals :**
- interopérabilité avec les modules XIAO ;
- publication de l'app sur l'App Loader officiel Espruino : l'installation se fait via un App Loader local ou le Web IDE ;
- optimisation fine de l'autonomie au-delà des réglages de rapport cyclique décrits ici ;
- localisation de la balise par trilatération, ou toute fonctionnalité nouvelle hors du périmètre des specs.

## Decisions

### D1 — Logique partagée dans un module pur `kccommon`
`firmware/common/kccommon.js` contient :
- les constantes ;
- le CRC16-CCITT sur une adresse `aa:bb:cc:dd:ee:ff` (octets dans l'ordre d'affichage) ;
- l'encodage et le décodage des trames KC ;
- le registre des balises (EMA, distance, état vivant, hystérésis **par balise**) ;
- la machine d'état du bracelet : appairage, alerte N.I.C.O., action unique, sonnerie 10 s, Ti Poucet ;
- la machine d'état de la balise : propriétaire, alerte, sonnerie, acquittement, péremption à 6 s.

Le temps (`now`) est injecté et le module n'appelle aucune API matérielle. Il est exporté via `exports` (compatible Espruino `require()` et Node).

*Alternatives écartées* :
- dupliquer la logique dans chaque firmware : risque de divergence du protocole ;
- module Espruino distant (URL) : dépendance réseau à l'installation.

### D2 — Petite chaîne de build Node
`firmware/tools/build.js` (Node, sans dépendance) :
1. copie `kccommon.js` dans le dossier de l'app Bangle, sous forme de fichier Storage `kccommon` déclaré dans `metadata.json` ;
2. produit `firmware/puck/dist/kc-tag.js`, un fichier unique où le module est inliné, prêt à être envoyé au Puck via le Web IDE (« Save to Flash »).

Les sorties générées sont commitées, pour que l'App Loader local et le Web IDE fonctionnent sans build. Les tests de logique tournent avec `node --test firmware/common/test/`.

*Alternative écartée* : le bundler du Web IDE. Il est manuel, non reproductible et ne permet pas les tests Node.

### D3 — App Bangle en quatre fichiers
Structure dans `firmware/bangle/keepclose/` :
- `keepclose.boot.js` : la surveillance ;
- `keepclose.wid.js` : le widget ;
- `keepclose.app.js` : l'app plein écran ;
- `keepclose.settings.js` : les réglages ;
- `metadata.json`, plus une icône.

Le code boot crée un objet global `KC`, qui contient l'état et les méthodes `action()`, `newTrip()` et `forget()`. Le widget et l'app lisent cet objet et s'abonnent à un événement `KC.on("change")`. Comme `load()` réinitialise tout, l'état durable (mode, activé, appairages, sourdines, atteintes) est stocké dans `keepclose.json` via `require("Storage")`. Il est **réécrit uniquement quand il change**, pour ménager la flash. Les mesures RSSI sont perdues à chaque changement d'app et se reconstituent en quelques secondes : c'est accepté par la spec `bracelet-interface`.

*Alternative écartée* : une app plein écran seule. La surveillance s'arrêterait hors de l'app ; l'utilisateur a choisi l'option « fond + widget ».

### D4 — Advertising du bracelet via `ble_advert` avec `whenConnected`
La trame KC est enregistrée via le module `ble_advert` de la Bangle. Ce module fusionne les advertising des différentes apps au lieu de les écraser. Le nom système est conservé. L'option `whenConnected: true` permet de continuer à émettre pendant une connexion à un téléphone. L'intervalle est d'environ 200 ms, pour que le scan intermittent du Puck capte au moins une trame par fenêtre. La trame n'est mise à jour qu'au changement de la balise en alerte ou de la balise à sonner (coalescence au tick suivant).

*Alternatives écartées* :
- `NRF.setAdvertising` direct : conflit avec Gadgetbridge et les autres apps ;
- connexion GATT bracelet → balise : coûteuse en énergie, limitée à une connexion à la fois et inutile puisque le protocole par advertising fonctionne déjà.

### D5 — Reconnaissance par *manufacturer data* `KC`
Les deux côtés scannent avec un filtre `manufacturerData: {0xFFFF: {}}`, puis vérifient le préfixe `0x4B 0x43` et le type dans `kccommon`. L'adresse BLE (`device.id`) sert de clé dans le registre.

*Alternative écartée* : le nom BLE. La Bangle doit garder le sien, et un nom en scan response imposerait un scan actif, plus coûteux.

### D6 — Scan : continu sur la Bangle, intermittent sur le Puck
- **Bangle** : scan passif continu avec le filtre. La batterie de la montre est rechargeable. L'objectif est d'au moins 2 jours d'autonomie, à mesurer ; le rapport cyclique est réglable (constante) si l'objectif n'est pas atteint.
- **Puck** : scan passif dans une fenêtre d'environ 600 ms toutes les 2,5 s (`setInterval`, puis `NRF.setScan` et `NRF.setScan()` pour arrêter). La latence est inférieure ou égale à 3 s, conformément à la spec `beacon-indicators`. L'advertising du Puck est réglé à 500 ms, avec la puissance d'émission par défaut.

*Alternative écartée* : scan continu sur le Puck. Il consomme plusieurs mA, ce qui épuiserait la CR2032 en quelques jours.

### D7 — Geste d'action : double tap, avec le bouton dans l'app
En arrière-plan, le bouton physique de la Bangle.js 2 est réservé au système : retour au lanceur, et réinitialisation sur appui long. Le code boot écoute donc `Bangle.on("tap")` et ne réagit qu'aux événements `double`. Dans l'app, `setWatch(BTN1)` et une grande zone tactile appellent aussi `KC.action()`.

*Alternative écartée* : `setWatch(BTN1)` en arrière-plan. Il entre en conflit avec la navigation du système.

### D8 — Vibrations et sons non bloquants
- Le motif « 200 ms par seconde » utilise `Bangle.buzz(200)` dans un `setInterval(1000)`, actif seulement pendant l'alerte.
- Les motifs de confirmation (×3, ×2 long) enchaînent les promesses de `Bangle.buzz`.
- Le bip-bip du Puck est piloté par une horloge à 100 ms, avec `analogWrite(PIEZO_A, 0.5, {freq: 4000})` puis `digitalWrite(PIEZO_A, 0)`. La broche `PIEZO_B` est maintenue à 0.
- Les broches par défaut sont `D1`/`D2`. Elles sont configurables dans le code et documentées dans le README.

### D9 — LEDs du Puck en flash
Les LEDs du Puck (`LED1`, `LED2`, `LED3` : rouge, vert, bleu) sont actives à l'état haut, contrairement au XIAO. Le flash d'état dure 30 à 50 ms toutes les 3 s et est piloté par le même `setInterval` que le scan intermittent, pour limiter les réveils.

### D10 — Calibration
Les constantes RSSI sont les suivantes :
- `P1M` : -61 sur XIAO ;
- seuil d'appairage : -38 ;
- seuils proche / éloigné : 0,45 / 0,55 m ;
- seuil d'atteinte Ti Poucet : 1,0 m.

Elles vivent dans `kccommon`. L'app Bangle a un écran de débogage qui affiche le RSSI brut et lissé par balise, pour calibrer sans console. Les valeurs XIAO servent de point de départ et seront remplacées après mesure.

### D11 — Archivage du firmware XIAO
Les dossiers `firmware/scanner`, `firmware/tag` et `firmware/test`, ainsi que `firmware/common/config.h`, sont déplacés avec `git mv` vers `legacy/xiao-firmware/`, ce qui préserve l'historique. L'arborescence interne est inchangée, pour que les `#include "../common/config.h"` restent valides.

## Risks / Trade-offs

- **[Risque]** `whenConnected` ou `ble_advert` ne permettent pas d'émettre pendant une connexion Gadgetbridge sur le firmware Espruino installé. → **Atténuation** : un spike (tâche 1) le vérifie avant tout développement. En repli, on documente que Gadgetbridge doit être déconnecté, ou on implémente une écriture GATT ponctuelle vers la balise.
- **[Risque]** Le double tap de l'accéléromètre de la Bangle.js 2 produit des faux positifs (gestes quotidiens) ou des faux négatifs. → **Atténuation** : vérification pendant le spike. Le seuil est réglable. En repli, on utilise un appui long sur l'écran tactile, ou un ouvreur rapide de l'app qui expose le bouton.
- **[Risque]** Une app installée par l'utilisateur appelle `NRF.setScan` et écrase le scan KeepClose. → **Atténuation** : le code boot vérifie toutes les 10 s qu'il reçoit encore des trames ou qu'il est toujours enregistré, et relance son scan. Ce point est documenté comme incompatibilité connue.
- **[Risque]** L'autonomie de la CR2032 est insuffisante (pic de courant du piézo, scans). → **Atténuation** : mesure du courant moyen pendant la validation et réglage de la fenêtre et de la période de scan. Les sonneries étant bornées à 10 s en N.I.C.O., le coût reste limité.
- **[Risque]** Le portage de la sonnerie Ti Poucet en « désignation continue » fait sonner la balise pendant tout le trajet. → **Trade-off accepté** : la balise sonne précisément pendant qu'on la cherche, et la pause du guidage permet de l'arrêter.
- **[Trade-off]** L'hystérésis devient propre à chaque balise : avant, elle était globale pour la LED. C'est plus juste avec plusieurs balises, mais le comportement diffère légèrement de celui du XIAO.
- **[Trade-off]** La balise cesse de sonner si elle ne reçoit plus le bracelet depuis 6 s. Cela évite une sonnerie sans fin qui viderait la pile, mais une balise très éloignée ne peut plus être sonnée. C'était déjà le cas, puisque la commande transite par l'advertising.

## Migration Plan

1. Faire le spike sur le matériel réel (D4, D7) et ajuster le design si un repli est nécessaire.
2. Développer et tester `kccommon` sous Node, puis le firmware Puck, puis l'app Bangle.
3. Valider sur le terrain avec une balise sur la canne : calibration, autonomie.
4. Archiver le firmware XIAO et mettre à jour le README.

**Rollback** : le code XIAO reste compilable depuis `legacy/xiao-firmware/`. Il suffit de reflasher les modules XIAO. Aucune donnée utilisateur n'est à migrer : les appairages sont refaits par contact.

## Open Questions

- Révision exacte du Puck.js (v2 ou v2.1) : elle détermine les broches libres pour le piézo. `D1`/`D2` par défaut sont à confirmer à la réception du matériel ; sans impact sur l'architecture.
- Valeurs de calibration RSSI définitives : elles seront mesurées avec le matériel (D10).
