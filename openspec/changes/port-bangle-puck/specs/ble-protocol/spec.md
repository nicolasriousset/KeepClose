# Spec Delta

## Purpose

Définit le contrat d'échange BLE sans connexion entre le bracelet (Bangle.js 2) et les balises (Puck.js) : comment chacun se reconnaît, s'identifie et se transmet l'état d'alerte et de sonnerie.

## ADDED Requirements

### Requirement: Identifiant compact d'appareil
Chaque appareil SHALL calculer son identifiant compact sur 16 bits. C'est le CRC16-CCITT (polynôme 0x1021, valeur initiale 0xFFFF) des 6 octets de son adresse BLE, pris dans l'ordre d'affichage (octet de poids fort en premier). La valeur 0x0000 SHALL être réservée au sens « aucun ».

#### Scenario: Calcul identique des deux côtés
- **WHEN** le bracelet observe la balise d'adresse `AA:BB:CC:DD:EE:FF`
- **THEN** l'identifiant qu'il calcule pour cette balise est égal à celui que la balise calcule pour elle-même

### Requirement: Trame d'advertising de la balise
La balise SHALL émettre en continu, toutes les 500 ms au plus, une trame dont le champ *manufacturer data* (company ID 0xFFFF) commence par l'octet `0x4B 0x43` (« KC »), suivi du type d'appareil `0x02` (balise). Viennent ensuite, sur 2 octets en petit-boutiste, l'identifiant du bracelet propriétaire, ou 0x0000 si la balise n'est pas appairée.

#### Scenario: Balise non appairée
- **WHEN** une balise neuve est alimentée
- **THEN** elle émet une trame KC de type 0x02 dont le champ « bracelet propriétaire » vaut 0x0000

#### Scenario: Balise appairée
- **WHEN** une balise est appairée au bracelet d'identifiant 0x1A2B
- **THEN** sa trame contient les octets `0x2B 0x1A` dans le champ « bracelet propriétaire »

### Requirement: Trame d'advertising du bracelet
Le bracelet SHALL émettre en continu une trame *manufacturer data* (company ID 0xFFFF) composée de `0x4B 0x43`, du type `0x01` (bracelet), de l'identifiant de la balise en alerte (2 octets petit-boutiste, 0 = aucune) et de l'identifiant de la balise à faire sonner (2 octets, 0 = aucune). Le bracelet SHALL conserver son nom BLE système.

#### Scenario: Aucune alerte, aucune sonnerie
- **WHEN** toutes les balises appairées sont à portée et aucune sonnerie n'est demandée
- **THEN** la trame du bracelet contient 0x0000 pour la balise en alerte et pour la balise à sonner

#### Scenario: Mise à jour rapide de la trame
- **WHEN** la balise en alerte ou la balise à sonner change
- **THEN** la nouvelle trame est émise en moins de 1 s

### Requirement: Reconnaissance par trame et non par nom
Les deux appareils SHALL reconnaître un appareil KeepClose uniquement par le préfixe *manufacturer data* `0xFFFF` + `KC` + type. Ils SHALL ignorer les trames dont le préfixe ou le type ne correspond pas.

#### Scenario: Appareil BLE étranger
- **WHEN** le bracelet reçoit une trame d'un appareil qui n'est pas une balise KeepClose
- **THEN** cet appareil n'apparaît pas dans la liste des balises et n'influence aucune alerte

### Requirement: Échange sans connexion
Les commandes d'alerte et de sonnerie SHALL être transmises uniquement par advertising. Aucune connexion BLE entre bracelet et balise ne SHALL être nécessaire au fonctionnement. Le service GATT « ring » du firmware XIAO n'est pas repris, et l'identification par nom BLE (« KC-Tag » / « KC-Scanner ») est abandonnée.

#### Scenario: Bracelet connecté à un téléphone
- **WHEN** la Bangle.js 2 est connectée à un téléphone (Gadgetbridge ou App Loader)
- **THEN** les balises continuent de recevoir l'état d'alerte et de sonnerie du bracelet

