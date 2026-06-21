# KeepClose

**KeepClose** est une application embarquée qui vise à prévenir les pertes d’objets essentiels en surveillant leur proximité via Bluetooth Low Energy (BLE).  
Un cas concret : alerter une personne aveugle si sa **canne blanche** sort de son périmètre de sécurité.

---

## 🚀 Fonctionnalités principales
- Détection en temps réel d'une balise BLE à proximité.  
- Alerte sonore et/ou vibration en cas d’éloignement.  
- Interface embarquée sur la **LilyGo T-Watch 2020 v3**.  
- Basé sur une balise **Bluetooth Low Energy Eddystone**.  

---

## 🛠️ Matériel requis
- [LilyGo T-Watch 2020 v3](https://lilygo.cc/products/t-watch-2020-v3)  
  - Montre connectée ESP32, écran tactile, faible coût, batterie intégrée.  
- [Balise Bluetooth Low Energy Eddystone](https://fr.aliexpress.com/item/1005003744908346.html?spm=a2g0o.order_list.order_list_main.5.35d65e5bmQrB1K&gatewayAdapt=glo2fra)  
  - Balise émettant un signal BLE détectable par la montre.  
- Câble micro USB pour la programmation de la montre.  
- Un ordinateur avec **Arduino IDE**.  

---

## ⚙️ Installation de l’environnement de développement

### 1. Installer l’IDE Arduino
- Télécharger et installer la dernière version de [l’IDE Arduino](https://www.arduino.cc/en/software).  

### 2. Ajouter la carte ESP32 à l’IDE Arduino
- Ouvrir **Fichier → Préférences**.  
- Dans le champ *URL de gestionnaire de cartes supplémentaires*, ajouter :  
https://raw.githubusercontent.com/espressif/arduino-esp32/refs/heads/gh-pages/package_esp32_dev_index.json

- Aller dans **Tools → Board Type → Boards Manager**.  
- Rechercher **ESP32** et installer la version 2.0.17 du package fourni par *Espressif Systems*. !!! ATTENTION !!! Les versions plus récentes (>=3.0) ne sont pas compatibles !!!

### 3. Installer les bibliothèques nécessaires
- Télécharger la bibliothèque TWatch depuis [https://github.com/Xinyuan-LilyGO/TTGO_TWATCH_Library](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/archive/refs/heads/master.zip)
- Dans l’IDE Arduino, aller dans **Sketch → Include library → Add .ZIP library** et installer la librairie

### 4. Sélectionner la carte
- Dans **Tools → Board**, choisir :  

ESP32 > TTGO T-Watch

- Sélectionner le bon port série (USB) correspondant à la montre.  

### 5. Tester le paramétrage avec un des projets d'exemple
- Charger le programme d'exemple depuis File > Examples > TTGO Watch Library > LVGL > Lvgl_Button
- Ouvrir le fichier config.h
- décommenter la ligne #define LILYGO_WATCH_2020_V3
- Cliquer sur le bouton "Upload",leprogramme devrait êtretéléchargé sur lamontre et fonctionnel.

### 6. Compiler et téléverser
- Ouvrir le projet `KeepClose.ino`.  
- Cliquer sur **✔️ Vérifier** pour compiler.  
- Cliquer sur **→ Téléverser** pour envoyer le code à la montre.  

---

## 📡 Utilisation
1. Allumer la montre **LilyGo T-Watch 2020 v3**.  
2. Activer la balise BLE Eddystone et la placer sur l’objet à surveiller (ex : canne blanche).  
3. La montre scanne en continu la présence de la balise.  
4. En cas d’éloignement (balise hors de portée), la montre déclenche une alerte (vibration et/ou son).  

---

## 📖 Feuille de route
- [ ] Ajuster la sensibilité/distance de détection BLE.  
- [ ] Ajouter une interface utilisateur sur la montre.  
- [ ] Configurer des seuils personnalisés d’alerte.  
- [ ] Ajouter une option de géolocalisation en cas de perte.  

---

## 🤝 Contribution
Les contributions sont les bienvenues !  
N’hésitez pas à ouvrir une *issue* ou proposer une *pull request*.  

---

## 📜 Licence
Projet open-source sous licence MIT.  
Vous êtes libres de l’utiliser, le modifier et le partager.  
