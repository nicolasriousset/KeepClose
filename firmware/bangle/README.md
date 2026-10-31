# Bracelet KeepClose — Bangle.js 2

L'app se trouve dans `keepclose/` et est installée via un App Loader. La surveillance tourne en arrière-plan : la montre garde son cadran habituel.

## Installation

Avec un **App Loader local** (recommandé) :

1. Cloner [espruino/BangleApps](https://github.com/espruino/BangleApps) et lancer son App Loader en local (voir son README : `npm install` puis serveur web sur le dossier).
2. Copier ou lier `firmware/bangle/keepclose/` dans `BangleApps/apps/keepclose/`.
3. Ouvrir l'App Loader local dans Chrome, se connecter à la montre et installer **KeepClose**.

Ou avec le **Web IDE**, pour un test rapide : téléverser chaque fichier listé dans `metadata.json` sous son nom Storage (`kccommon`, `keepclose.boot.js`, …), puis redémarrer la montre (appui long sur le bouton).

Avant d'installer, relancer `node firmware/tools/build.js` si `firmware/common/kccommon.js` a changé.

Pour repartir de zéro : désinstaller l'app depuis l'App Loader (cela supprime aussi `keepclose.json`), puis la réinstaller.

## Utilisation

| Widget (en haut à droite) | Signification |
|---|---|
| Cercle bleu vide | Aucune balise appairée |
| Rond vert ✓ | Toutes les balises sont à portée (également 5 s après un appairage) |
| Triangle rouge ! | Au moins une balise est hors de portée |
| Tiret gris | KeepClose désactivé |

- **Appairer** : coller la balise contre la montre pendant 3 s. La montre vibre 3 fois.
- **Double tap sur la montre** (depuis n'importe quel écran), c'est l'action unique :
  - **N.I.C.O.** :
    - si une sonnerie est en cours, l'arrêter ;
    - sinon, si l'alerte vibre, la mettre en sourdine jusqu'au retour de la balise ;
    - sinon, faire sonner la balise la plus proche (10 s maximum).
  - **Ti Poucet** : mettre le guidage en pause, ou le reprendre.
  - Chaque action est confirmée par une vibration brève. Une vibration longue signale qu'il n'y a aucune balise à faire sonner.
- **App KeepClose** (lanceur) :
  - balises triées par distance, batterie de la montre ;
  - grand bouton d'action en bas (le bouton physique fait la même chose) ;
  - glisser vers la gauche ou la droite pour afficher la vue débogage (RSSI brut et lissé, utile pour calibrer) ;
  - toucher le haut de l'écran pour ouvrir les réglages.
- **Réglages** (app, ou *Réglages → Apps → KeepClose*) : Actif, Mode (N.I.C.O. / Ti Poucet), Nouveau trajet, Oublier balises.

## Incompatibilités connues

- **Apps qui scannent le Bluetooth** (`NRF.setScan`) : elles remplacent le scan KeepClose. KeepClose le relance automatiquement si aucune trame n'a été reçue depuis 10 s, mais l'autre app perd alors le sien.
- **Apps qui modifient l'advertising** sans passer par `Bangle.bleAdvert` : la trame KC peut disparaître jusqu'à 10 s, le temps que KeepClose la rétablisse.
- La trame KC doit continuer d'être émise pendant une connexion Gadgetbridge (`whenConnected`). C'est à vérifier sur votre version du firmware Espruino (tâche 1.1 du change `port-bangle-puck`).
