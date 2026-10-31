# Spec Delta

## Purpose

Permet d'associer une balise à un bracelet par simple contact physique, de conserver cette association après un redémarrage et de la défaire automatiquement ou à la demande.

## ADDED Requirements

### Requirement: Appairage par contact côté bracelet
Le bracelet SHALL appairer une balise dont le RSSI brut reste supérieur ou égal au seuil d'appairage (configurable, -38 dBm sur XIAO, à recalibrer) pendant au moins 3 s sans interruption. Si le RSSI repasse sous le seuil avant 3 s, le délai SHALL repartir de zéro.

#### Scenario: Contact maintenu 3 s
- **WHEN** l'utilisateur colle une balise non appairée contre la montre pendant 3 s
- **THEN** la balise est appairée au bracelet

#### Scenario: Contact rompu trop tôt
- **WHEN** la balise est éloignée après 2 s de contact
- **THEN** la balise n'est pas appairée, et un nouveau contact doit à nouveau durer 3 s

### Requirement: Confirmation d'appairage sur le bracelet
À la fin d'un appairage, le bracelet SHALL émettre 3 vibrations courtes (environ 100 ms chacune, séparées d'environ 150 ms). Il SHALL aussi afficher une indication visuelle « appairée » pendant 5 s.

#### Scenario: Retour haptique
- **WHEN** une balise vient d'être appairée
- **THEN** la montre vibre 3 fois brièvement

### Requirement: Appairage par contact côté balise
La balise SHALL adopter comme bracelet propriétaire tout bracelet KeepClose qu'elle reçoit avec un RSSI supérieur ou égal au seuil d'appairage. Elle SHALL ensuite publier l'identifiant de ce bracelet dans sa trame. Un nouveau contact avec un autre bracelet SHALL remplacer le propriétaire.

#### Scenario: Changement de propriétaire
- **WHEN** une balise appairée au bracelet A est collée contre le bracelet B
- **THEN** la balise publie l'identifiant du bracelet B

### Requirement: Persistance des appairages
Le bracelet SHALL conserver la liste de ses balises appairées en mémoire non volatile. La balise SHALL conserver l'identifiant de son bracelet propriétaire. Ces données SHALL survivre à un redémarrage, à un changement d'app sur la montre et à un remplacement de pile.

#### Scenario: Redémarrage de la montre
- **WHEN** la Bangle.js 2 redémarre ou que l'utilisateur ouvre une autre app
- **THEN** les balises précédemment appairées sont toujours appairées, sans nouveau contact

#### Scenario: Changement de pile de la balise
- **WHEN** la pile du Puck.js est remplacée
- **THEN** la balise publie à nouveau l'identifiant de son bracelet propriétaire

### Requirement: Désappairage automatique
Le bracelet SHALL désappairer une balise dont la trame publie un identifiant de bracelet propriétaire non nul et différent du sien. Cette suppression SHALL être enregistrée de façon persistante.

#### Scenario: Balise reprise par un autre bracelet
- **WHEN** une balise appairée au bracelet A publie l'identifiant du bracelet B
- **THEN** le bracelet A retire cette balise de ses appairages et ne génère plus d'alerte pour elle

### Requirement: Oubli manuel des balises
Le bracelet SHALL proposer, dans ses réglages, une action « Oublier les balises » protégée par une confirmation. Cette action supprime tous les appairages du bracelet.

#### Scenario: Oubli confirmé
- **WHEN** l'utilisateur confirme « Oublier les balises »
- **THEN** le bracelet n'a plus aucune balise appairée et n'émet plus d'alerte
