# KeepClose

**KeepClose** est une application embarquée qui vise à prévenir les pertes d'objets essentiels en surveillant leur proximité via Bluetooth Low Energy (BLE).  
Un cas concret : alerter une personne malvoyante si sa **canne blanche** sort de son périmètre de sécurité.

## 🎯 Contexte et enjeux du projet

Le problème de base que nous cherchons à résoudre est de s'assurer que les enfants malvoyants ne quittent pas un atelier en ayant oublié leur canne. Plus généralement, l'objectif est de créer un mécanisme pour rappeler aux utilisateurs qu'ils sont partis sans leur canne, peu importe d'où.

### Défis spécifiques aux malvoyants

Les solutions existantes comme les AirTags présentent deux limitations majeures pour les personnes malvoyantes :

1. **Accessibilité** : Les malvoyants n'ont pas accès facilement à un smartphone pour localiser une balise
2. **Fonctionnalité** : Les solutions existantes permettent de retrouver un objet égaré mais pas d'alerter quand on s'en éloigne

### Approche technique

Notre solution repose sur :
- Une **balise Bluetooth** peu onéreuse de type iBeacon attachée à la canne (autonomie de plusieurs mois/années)
- Un **bracelet-montre** qui estime la distance avec la balise et alerte par vibration ou signal sonore quand l'utilisateur s'éloigne trop

---

## 🚀 Fonctionnalités principales

### Version actuelle (Prototype)
- Détection automatique de la proximité de la balise (quelques centimètres)
- Surveillance continue une fois la balise détectée
- **Alerte à 15 mètres** : vibration + message à l'écran pour les accompagnateurs
- Arrêt de l'alarme par bouton physique ou rapprochement automatique
- Interface embarquée sur la **LilyGo T-Watch 2020 v3**
- Basé sur une balise **Bluetooth Low Energy iBeacon**

### Évolutions futures envisagées
- **Double seuil d'alerte** :
  - 15m : vibration silencieuse
  - 30m : vibration + alerte vocale
- **Aide à la localisation sonore** : guidage vocal ou système type "détecteur de métaux" avec bips
- **Messages personnalisés** : "Tu as oublié ta canne Marc"
- **Bracelet sur mesure** : électronique minimale pour une meilleure autonomie

---

## 🛠️ Matériel requis
- [LilyGo T-Watch 2020 v3](https://lilygo.cc/products/t-watch-2020-v3) (~50€)  
  - Montre connectée ESP32, écran tactile, faible coût, batterie intégrée
- [Balise Bluetooth Low Energy iBeacon](https://fr.aliexpress.com/item/1005003744908346.html?spm=a2g0o.order_list.order_list_main.5.35d65e5bmQrB1K&gatewayAdapt=glo2fra)  
  - Balise émettant un signal BLE détectable par la montre
- Câble micro USB pour la programmation de la montre
- Un ordinateur avec **Arduino IDE**

---

## ⚙️ Installation de l'environnement de développement

### 1. Installer l'IDE Arduino
- Télécharger et installer la dernière version de [l'IDE Arduino](https://www.arduino.cc/en/software)

### 2. Ajouter la carte ESP32 à l'IDE Arduino
- Ouvrir **Fichier → Préférences**
- Dans le champ *URL de gestionnaire de cartes supplémentaires*, ajouter :  
https://raw.githubusercontent.com/espressif/arduino-esp32/refs/heads/gh-pages/package_esp32_dev_index.json

- Aller dans **Tools → Board Type → Boards Manager**
- Rechercher **ESP32** et installer la version 2.0.17 du package fourni par *Espressif Systems*. !!! ATTENTION !!! Les versions plus récentes (>=3.0) ne sont pas compatibles !!!

### 3. Installer les bibliothèques nécessaires
- Télécharger la bibliothèque TWatch depuis [https://github.com/Xinyuan-LilyGO/TTGO_TWATCH_Library](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/archive/refs/heads/master.zip)
- Dans l'IDE Arduino, aller dans **Sketch → Include library → Add .ZIP library** et installer la librairie

### 4. Sélectionner la carte
- Dans **Tools → Board**, choisir : **ESP32 > TTGO T-Watch**
- Sélectionner le bon port série (USB) correspondant à la montre

### 5. Tester le paramétrage avec un des projets d'exemple
- Charger le programme d'exemple depuis File > Examples > TTGO Watch Library > LVGL > Lvgl_Button
- Ouvrir le fichier config.h
- décommenter la ligne #define LILYGO_WATCH_2020_V3
- Cliquer sur le bouton "Upload", le programme devrait être téléchargé sur la montre et fonctionnel

### 6. Compiler et téléverser
- Ouvrir le projet `KeepClose.ino`
- Cliquer sur **✔️ Vérifier** pour compiler
- Cliquer sur **→ Téléverser** pour envoyer le code à la montre

---

## 📡 Utilisation

1. Allumer la montre **LilyGo T-Watch 2020 v3**
2. Activer la balise BLE iBeacon et la placer sur l'objet à surveiller (ex : canne blanche)
3. Approcher la montre à quelques centimètres de la balise pour lancer la surveillance automatique
4. La montre surveille en continu la distance avec la balise
5. En cas d'éloignement estimé à plus de 15 mètres, la montre déclenche une alerte (vibration + message à l'écran)
6. Pour arrêter l'alarme : appuyer sur le bouton physique de la montre ou se rapprocher de la canne

---

## 🔧 Défis techniques et limitations

### Estimation de distance
- **Problème** : La mesure de distance BLE est imprécise en environnement réel (obstacles, interférences électromagnétiques)
- **Solution adoptée** : Utilisation d'un seuil de sécurité de 15m et détection de perte totale de signal

### Autonomie
- **Compromis** : Équilibre entre réactivité (surveillance continue) et durée de vie de la batterie
- **Objectif** : Autonomie d'au moins une journée complète, idéalement plusieurs jours

### Avantages de l'approche
- **Portabilité** : Pas d'installation fixe nécessaire dans chaque salle
- **Fiabilité** : Alerte garantie en cas de perte totale du signal
- **Simplicité** : Détection automatique et remise en veille

---

## 📖 État actuel et prochaines étapes

### ✅ Prototype fonctionnel
- [x] Détection automatique de proximité de balise
- [x] Surveillance continue avec seuil d'alerte à 15m
- [x] Interface utilisateur basique avec vibration et affichage
- [x] Arrêt d'alarme par bouton ou rapprochement

### 🔄 Tests en cours
- [ ] Validation de l'autonomie sur une journée complète
- [ ] Tests en conditions réelles d'utilisation
- [ ] Évaluation de l'acceptabilité par les utilisateurs cibles

### 🚀 Développements futurs

#### Phase 2 - Optimisations
- [ ] Remplacement de la T-Watch par un bracelet personnalisé sans écran
- [ ] Amélioration de l'autonomie (recharge hebdomadaire vs quotidienne)
- [ ] Simplification de l'appairage balise/bracelet
- [ ] Réduction des coûts de production

#### Phase 3 - Fonctionnalités avancées
- [ ] Double seuil d'alerte (15m silencieux, 30m vocal)
- [ ] Guidage vocal pour retrouver la canne
- [ ] Messages personnalisés avec nom de l'utilisateur
- [ ] Système de localisation sonore type "détecteur de métaux"

---

## 💡 Avantages par rapport aux alternatives

### vs Solutions smartphone (AirTags)
- ✅ Accessibilité pour malvoyants
- ✅ Alerte proactive d'éloignement
- ✅ Interface tactile adaptée

### vs Installation fixe
- ✅ Pas d'infrastructure nécessaire
- ✅ Mobilité entre différents espaces
- ✅ Coût réduit de déploiement

---

## 🤝 Contribution
Les contributions sont les bienvenues !  
N'hésitez pas à ouvrir une *issue* ou proposer une *pull request*.

Domaines d'amélioration prioritaires :
- Optimisation de l'algorithme d'estimation de distance
- Tests utilisateurs et retours d'expérience
- Développement du bracelet personnalisé
- Amélioration de l'interface utilisateur

---

## 📜 Licence
Projet open-source sous licence MIT.  
Vous êtes libres de l'utiliser, le modifier et le partager.
