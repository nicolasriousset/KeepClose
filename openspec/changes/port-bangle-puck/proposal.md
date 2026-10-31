# Proposal

## Why

Le prototype KeepClose v2 tourne sur deux modules Seeed XIAO BLE nRF52840 câblés à la main (moteur vibrant, buzzer, bouton, batterie LiPo). Ce montage est difficile à reproduire et à porter au quotidien. Le bracelet passe sur une **Bangle.js 2** : une montre du commerce avec écran, vibreur, bouton, écran tactile et batterie intégrés. La balise passe sur un **Puck.js** : un boîtier étanche avec LEDs, bouton et pile CR2032. Ces deux plateformes s'exécutent sous Espruino (JavaScript) et non sous Arduino. Le firmware doit donc être réécrit, en conservant le comportement actuel.

## What Changes

- **Nouveau firmware bracelet** pour Bangle.js 2, sous forme d'app Espruino :
  - surveillance en arrière-plan (code `boot`) : la montre garde son cadran ;
  - un widget affiche l'état (aucune balise / à portée / hors de portée) ;
  - une app plein écran donne la liste des balises, les actions et les réglages.
- **Nouveau firmware balise** pour Puck.js : advertising, détection du bracelet, LEDs d'état, sonnerie par **buzzer piézo externe** (ajout matériel) et bouton d'acquittement.
- **Portage du comportement existant** :
  - appairage par contact (RSSI ≥ seuil pendant 3 s) et désappairage automatique quand la balise appartient à un autre bracelet ;
  - estimation de distance avec lissage EMA et hystérésis ;
  - mode **N.I.C.O.** : vibration quand une balise sort du périmètre ; une action unique annule la sonnerie, met l'alerte en sourdine (snooze) ou fait sonner la balise la plus proche ; levée automatique du snooze au retour dans le périmètre ;
  - mode **Ti Poucet** : la balise non atteinte la plus proche sonne, et une balise est mise en sourdine quand on l'atteint ;
  - le choix entre les deux modes se fait dans les réglages de l'app Bangle (sur le XIAO, il fallait recompiler).
- **Protocole BLE redéfini** (**BREAKING** : incompatible avec les modules XIAO).
  - Identification par *manufacturer data* (0xFFFF + marqueur `KC`) au lieu du nom BLE, pour que la Bangle garde son nom système.
  - La commande de sonnerie et l'alerte passent **uniquement par l'advertising** du bracelet. Le service GATT « ring », déclaré sur le XIAO mais jamais utilisé, est supprimé.
- **Adaptations imposées par le matériel** :
  - sur le Puck, LEDs en flash bref au lieu d'un allumage continu, et scan BLE à rapport cyclique réduit, tous deux pour préserver la CR2032 ;
  - sur la Bangle, le geste d'action passe par un **double tap** sur la montre, car le bouton physique est réservé au système en arrière-plan ;
  - la mesure de batterie utilise l'API Espruino.
- **Petits ajouts** liés à la nouvelle interface :
  - action « Nouveau trajet » en Ti Poucet, pour réinitialiser les snoozes (c'était un TODO du code XIAO) ;
  - action « Oublier les balises ».
- **Archivage** du firmware Arduino XIAO dans `legacy/xiao-firmware/`. Le nouveau code se trouve dans `firmware/bangle/`, `firmware/puck/` et `firmware/common/`.

## Capabilities

### New Capabilities
- `ble-protocol` : format des trames d'advertising échangées entre le bracelet et les balises (identifiants, CRC d'identité, champs alerte/sonnerie, appartenance de la balise).
- `pairing` : appairage par contact, persistance des appairages, désappairage automatique et manuel.
- `proximity-alert` : estimation de distance, état à portée / hors de portée avec hystérésis, alerte vibrante et snooze (mode N.I.C.O.).
- `beacon-ring` : sonnerie d'une balise commandée par le bracelet (déclenchement, durée, annulation, acquittement sur la balise).
- `trail-guidance` : mode Ti Poucet (guidage de balise en balise, snooze à l'arrivée, nouveau trajet).
- `bracelet-interface` : exécution en arrière-plan sur la Bangle.js 2, widget d'état, app plein écran, geste d'action, réglages (mode, oubli des balises), affichage de la batterie.
- `beacon-indicators` : retours locaux de la balise Puck.js (LEDs d'état, motif du buzzer, bouton, niveau de pile).

### Modified Capabilities
<!-- Aucune : openspec/specs/ est vide, le comportement XIAO n'avait pas été spécifié. -->

## Impact

- **Code** :
  - `firmware/scanner/`, `firmware/tag/`, `firmware/common/config.h` et `firmware/test/` sont déplacés dans `legacy/xiao-firmware/` ;
  - nouveaux dossiers `firmware/bangle/` (app Bangle : `boot`, `app`, `wid`, `settings`, `metadata.json`), `firmware/puck/` (code balise) et `firmware/common/` (module JS partagé : constantes, CRC16, encodage et décodage des trames).
- **Matériel** :
  - plus de câblage côté bracelet ;
  - côté balise, ajout d'un buzzer piézo sur deux broches du Puck.js ;
  - pile CR2032 au lieu de la LiPo, ce qui rend l'autonomie de la balise critique.
- **Outillage** : Espruino Web IDE et App Loader (installation locale de l'app Bangle) remplacent l'IDE Arduino et le BSP Seeed nRF52.
- **Calibration** : les constantes RSSI (puissance à 1 m, seuil d'appairage, seuils de proximité) doivent être recalibrées, car les antennes et les boîtiers sont différents.
- **Compatibilité** : un bracelet XIAO et une balise Puck (ou l'inverse) ne fonctionnent pas ensemble.
- **Documentation** : `README.md` (matériel, installation, brochage du piézo) et `doc/` doivent être mis à jour.
