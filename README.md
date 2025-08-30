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
- Câble USB-C pour la programmation de la montre.  
- Un ordinateur avec **Arduino IDE**.  

---

## ⚙️ Installation de l’environnement de développement

### 1. Installer l’IDE Arduino
- Télécharger et installer la dernière version de [l’IDE Arduino](https://www.arduino.cc/en/software).  

### 2. Ajouter la carte ESP32 à l’IDE Arduino
- Ouvrir **Fichier → Préférences**.  
- Dans le champ *URL de gestionnaire de cartes supplémentaires*, ajouter :  
