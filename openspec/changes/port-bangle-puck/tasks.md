# Tasks

## 1. Spike matériel (lève les risques D4 / D7 avant développement)

- [ ] 1.1 Sur la Bangle.js 2, enregistrer une trame *manufacturer data* KC via `ble_advert` avec `whenConnected: true`, connecter Gadgetbridge (ou l'App Loader) et vérifier avec nRF Connect que la trame KC reste visible ; consigner le résultat et la version du firmware Espruino dans `design.md` (D4)
- [ ] 1.2 Sur la Bangle.js 2, journaliser les événements `Bangle.on("tap")` doubles pendant 10 min de port normal puis 20 doubles taps volontaires ; consigner le taux de faux positifs et de faux négatifs dans `design.md` (D7), et appliquer le repli prévu si le geste est inutilisable
- [ ] 1.3 Sur le Puck.js, vérifier qu'une fenêtre de scan de 600 ms toutes les 2,5 s capte la trame de la Bangle (intervalle 200 ms) dans au moins 95 % des fenêtres, et noter le courant moyen mesuré
- [ ] 1.4 Raccorder le piézo au Puck.js (broches à confirmer selon la révision), vérifier que `analogWrite(…, 0.5, {freq: 4000})` est audible à 3 m, et mettre à jour l'Open Question correspondante de `design.md`

## 2. Module partagé `kccommon` et outillage

- [x] 2.1 Créer `firmware/common/kccommon.js` avec les constantes (seuils, durées, P1M, n, EMA) et le CRC16-CCITT sur une adresse texte ; un test `node --test` vérifie le CRC sur un vecteur connu et sa valeur identique quelle que soit la casse de l'adresse
- [x] 2.2 Ajouter l'encodage et le décodage des trames KC bracelet (type 0x01) et balise (type 0x02), avec rejet des préfixes ou types inconnus ; les tests couvrent l'aller-retour, le petit-boutiste et le rejet d'une trame étrangère
- [x] 2.3 Implémenter le registre des balises : EMA initialisée par la première mesure, distance, vivacité 6 s, hystérésis par balise ; les tests reprennent les scénarios de la spec `proximity-alert` (première mesure à 1,0 m, pic lissé, oscillation 0,48–0,52 m, retour à 0,40 m)
- [x] 2.4 Implémenter l'appairage côté bracelet (3 s au-dessus du seuil, remise à zéro en cas de rupture) et le désappairage automatique sur un CRC propriétaire étranger ; les tests reprennent les scénarios de la spec `pairing`
- [x] 2.5 Implémenter la machine d'état N.I.C.O. : alerte vibrante, balise publiée en alerte, action unique (annuler la sonnerie, puis snooze, puis sonnerie de la plus proche), sonnerie de 10 s, levée du snooze ; les tests reprennent les specs `proximity-alert` et `beacon-ring` (côté bracelet)
- [x] 2.6 Implémenter la machine d'état Ti Poucet : désignation de la plus proche non atteinte, atteinte sous 1,0 m, fin de trajet, nouveau trajet, pause ; les tests reprennent les scénarios de la spec `trail-guidance`
- [x] 2.7 Implémenter la machine d'état de la balise : adoption du propriétaire au contact, alerte, sonnerie seulement depuis le propriétaire, acquittement et réarmement, péremption à 6 s, couleur d'état ; les tests reprennent les specs `beacon-ring` (côté balise) et `beacon-indicators`
- [x] 2.8 Écrire `firmware/tools/build.js`, qui copie `kccommon.js` dans l'app Bangle et produit `firmware/puck/dist/kc-tag.js` avec le module inliné ; vérifier que `node firmware/tools/build.js` régénère les fichiers à l'identique (pas de diff) et documenter la commande dans `firmware/README.md`

## 3. Firmware balise Puck.js

- [ ] 3.1 Écrire `firmware/puck/kc-tag.js` : calcul de l'identifiant depuis `NRF.getAddress()`, advertising KC type 0x02 toutes les 500 ms, persistance du propriétaire dans `kctag.json` ; vérifier avec nRF Connect la trame d'une balise neuve (propriétaire 0x0000)
- [ ] 3.2 Ajouter le scan intermittent (fenêtre d'environ 600 ms toutes les 2,5 s, filtre 0xFFFF) relié à la machine d'état de la balise ; vérifier sur la console Espruino la réception des trames d'une Bangle de test
- [ ] 3.3 Ajouter les LEDs en flash (bleu, vert, rouge), le vert continu 1 s à l'adoption d'un propriétaire, et le double flash rouge sous 15 % de pile ; vérifier chaque couleur en simulant les états depuis la console
- [ ] 3.4 Ajouter le piézo (motif bip-bip sur une période de 1 s) et le bouton (`setWatch` avec anti-rebond de 50 ms : acquittement, ou flash d'état à la demande) ; vérifier à l'oreille et au chronomètre le motif 100/100/100/700 ms
- [ ] 3.5 Ajouter un journal console périodique (pile en %, propriétaire, âge de la dernière trame du bracelet, alerte, sonnerie), équivalent du `[hb tag]` du XIAO ; vérifier sa sortie dans le Web IDE
- [ ] 3.6 Documenter dans `firmware/puck/README.md` l'installation (Web IDE, « Save to Flash »), le câblage du piézo et le remplacement de la pile ; vérifier que la procédure fonctionne sur un Puck réinitialisé

## 4. App bracelet Bangle.js 2

- [ ] 4.1 Créer `firmware/bangle/keepclose/metadata.json`, l'icône et l'arborescence (`boot`, `wid`, `app`, `settings`, `kccommon`) ; vérifier l'installation via un App Loader local
- [ ] 4.2 Écrire `keepclose.boot.js` : objet global `KC`, chargement et sauvegarde de `keepclose.json` (écriture seulement sur changement), scan passif continu filtré, advertising KC via `ble_advert` (environ 200 ms, `whenConnected`), coalescence des mises à jour de trame ; vérifier qu'une balise appairée reste suivie après un passage cadran → lanceur → cadran
- [ ] 4.3 Brancher les retours haptiques non bloquants : alerte de 200 ms par seconde, ×3 à l'appairage, vibration longue en cas d'échec de sonnerie, ×2 longue en fin de trajet, 50 ms d'accusé d'action ; vérifier chaque motif sur la montre
- [ ] 4.4 Brancher le double tap sur `KC.action()` et ajouter la relance du scan si elle a été écrasée (contrôle toutes les 10 s) ; vérifier le snooze d'une alerte par double tap depuis le cadran
- [ ] 4.5 Écrire `keepclose.wid.js` : états bleu, vert et rouge, chacun avec un pictogramme distinct, plus le vert pendant 5 s après un appairage ; vérifier les trois états avec une balise réelle
- [ ] 4.6 Écrire `keepclose.app.js` : mode, balises triées (distance ou « hors portée », sourdine, atteinte, sonnerie), batterie de la montre, grand bouton d'action, `BTN1` pour l'action, écran de débogage RSSI brut et lissé ; vérifier le tri avec deux balises
- [ ] 4.7 Écrire `keepclose.settings.js` : mode N.I.C.O. / Ti Poucet, « Nouveau trajet », « Oublier les balises » avec confirmation, activation et désactivation ; vérifier que la désactivation fait disparaître la trame KC dans nRF Connect et que le mode persiste après redémarrage
- [ ] 4.8 Documenter dans `firmware/bangle/README.md` l'installation, l'usage (double tap, widget, réglages) et les incompatibilités connues (apps qui écrasent le scan) ; vérifier la procédure sur une montre réinitialisée

## 5. Calibration et validation de bout en bout

- [ ] 5.1 Mesurer avec l'écran de débogage le RSSI lissé à 1 m (P1M), le RSSI au contact (seuil d'appairage) et les distances 0,45 / 0,55 / 1,0 m, puis reporter les valeurs dans `kccommon.js` ; vérifier qu'un appairage n'aboutit pas à 20 cm sans contact
- [ ] 5.2 Dérouler les scénarios N.I.C.O. avec la balise sur une canne : appairage, sortie du périmètre, snooze par double tap, retour, sonnerie, annulation, acquittement sur la balise ; consigner le résultat de chaque scénario des specs dans `firmware/VALIDATION.md`
- [ ] 5.3 Dérouler un parcours Ti Poucet avec 3 balises (guidage, atteinte, fin de trajet, nouveau trajet, pause) et consigner le résultat dans `firmware/VALIDATION.md`
- [ ] 5.4 Vérifier le désappairage automatique avec deux Bangle (ou une Bangle et un faux bracelet émis depuis nRF Connect) et le consigner
- [ ] 5.5 Mesurer l'autonomie : courant moyen du Puck (cible de plusieurs semaines sur CR2032) et décharge de la Bangle sur 24 h (cible ≥ 2 jours) ; ajuster la fenêtre et la période de scan si besoin et consigner les mesures

## 6. Archivage XIAO et documentation projet

- [x] 6.1 Déplacer `firmware/scanner`, `firmware/tag`, `firmware/test` et `firmware/common/config.h` vers `legacy/xiao-firmware/` avec `git mv` ; vérifier avec `git log --follow` que l'historique est conservé et que les `#include` relatifs restent valides
- [x] 6.2 Mettre à jour le `README.md` racine (matériel Bangle.js 2 + Puck.js + piézo, protocole KC, renvois vers les README de `firmware/` et vers `legacy/xiao-firmware/`) ; vérifier que tous les liens relatifs existent
- [x] 6.3 Mettre à jour le contexte de `openspec/config.yaml` (plateformes, langage Espruino, arborescence) ; vérifier avec `openspec validate --all`

## Workflow follow-up

- Archiver le change avec `/opsx:archive` une fois la validation terrain (groupe 5) acceptée, pour créer les specs principales dans `openspec/specs/`.
