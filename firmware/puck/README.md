# Balise KeepClose — Puck.js

## Matériel

- Un **Puck.js** (v2 ou v2.1) et une pile CR2032.
- Un **buzzer piézo passif** (piézo nu, sans oscillateur intégré), raccordé ainsi :

```
  Puck.js D1 ──────── piézo (+)
  Puck.js D2 ──────── piézo (−)      (D2 est maintenue à 0)
```

Les broches sont définies en tête de `kc-tag.js` (`PIEZO_A`, `PIEZO_B`). Elles restent à confirmer selon la révision du Puck : voir la tâche 1.4 du change OpenSpec `port-bangle-puck`. Après modification, relancer `node firmware/tools/build.js`.

## Installation

1. Ouvrir l'[Espruino Web IDE](https://www.espruino.com/ide/) dans Chrome et se connecter au Puck (icône Bluetooth en haut à gauche).
2. Dans *Settings → Communications*, régler **Save on Send** sur **Direct to Flash (execute code at boot)**.
3. Ouvrir `firmware/puck/dist/kc-tag.js`. C'est le fichier **généré**, et non `kc-tag.js` à la racine de `puck/`.
4. Cliquer sur *Send to Espruino*. La console affiche `KeepClose balise prete, CRC=0x…`.
5. Déconnecter le Web IDE : la balise commence à émettre des flashs bleus.

Pour repartir de zéro (Puck réinitialisé) : taper `require("Storage").eraseAll(); reset();` dans la console, puis reprendre à l'étape 3.

## Utilisation

| LED (flash bref toutes les 2,5 s) | Signification |
|---|---|
| Bleu | Balise non appairée |
| Vert | Appairée, bracelet reçu, balise dans le périmètre |
| Rouge | Appairée, hors du périmètre ou bracelet absent depuis 6 s |
| Vert fixe 1 s | Appairage avec un bracelet |
| Double flash rouge en plus | Pile sous 15 % |

- **Appairer** : coller la balise contre la montre pendant 3 s.
- **Bouton** :
  - pendant une sonnerie, il coupe le son (acquittement) ;
  - sinon, il fait flasher immédiatement l'état.
- **Journal** : quand le Web IDE est connecté, la console affiche une ligne `[hb tag]` toutes les 10 s environ (pile, propriétaire, alerte, sonnerie).

## Pile

La balise retient son bracelet propriétaire en mémoire flash (`kctag.json`) : après un changement de pile, elle reprend sans nouvel appairage.
