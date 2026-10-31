# Spec Delta

## Purpose

Surveille la distance entre le bracelet et chaque balise appairée et alerte l'utilisateur par vibration quand une balise (typiquement la canne) sort du périmètre. C'est le mode N.I.C.O.

## ADDED Requirements

### Requirement: Estimation de distance lissée
Pour chaque balise appairée, le bracelet SHALL lisser le RSSI par une moyenne exponentielle (coefficient 0,1). La première mesure SHALL initialiser la moyenne. La distance SHALL être estimée par `10^((P1m − RSSIlissé) / (10 × n))`, où P1m est la puissance mesurée à 1 m et n l'exposant de perte de trajet (2,0). P1m et n sont configurables.

#### Scenario: Première mesure
- **WHEN** le bracelet reçoit la première trame d'une balise appairée avec un RSSI de -61 dBm et P1m = -61
- **THEN** la distance estimée vaut 1,0 m

#### Scenario: Pic de RSSI isolé
- **WHEN** une seule trame arrive avec un RSSI supérieur de 20 dB à la moyenne courante
- **THEN** le RSSI lissé n'augmente que de 2 dB

### Requirement: Balise vivante
Une balise SHALL être considérée comme vivante si le bracelet a reçu une de ses trames au cours des 6 dernières secondes. Une balise appairée qui n'est pas vivante, ou qui n'a encore aucune mesure, SHALL être considérée hors de portée.

#### Scenario: Balise éteinte
- **WHEN** une balise appairée n'est plus reçue depuis 6 s
- **THEN** elle est considérée hors de portée

### Requirement: Périmètre avec hystérésis
Une balise à portée SHALL passer hors de portée quand sa distance estimée dépasse le seuil éloigné (0,55 m par défaut). Une balise hors de portée SHALL redevenir à portée seulement quand sa distance repasse sous le seuil proche, ou à son niveau (0,45 m par défaut). Les deux seuils SHALL être configurables, avec seuil proche < seuil éloigné.

#### Scenario: Oscillation autour du seuil
- **WHEN** la distance d'une balise oscille entre 0,48 m et 0,52 m après être passée hors de portée
- **THEN** la balise reste hors de portée

#### Scenario: Retour dans le périmètre
- **WHEN** la distance d'une balise hors de portée descend à 0,40 m
- **THEN** la balise redevient à portée

### Requirement: Alerte vibrante
En mode N.I.C.O., tant qu'au moins une balise appairée et non mise en sourdine est hors de portée, le bracelet SHALL vibrer 200 ms toutes les secondes. Cette alerte SHALL fonctionner quel que soit l'écran affiché sur la montre.

#### Scenario: La canne s'éloigne
- **WHEN** la balise de la canne dépasse le seuil éloigné pendant que la montre affiche son cadran
- **THEN** la montre vibre 200 ms chaque seconde jusqu'au retour de la balise ou à sa mise en sourdine

### Requirement: Publication de la balise en alerte
En mode N.I.C.O., le bracelet SHALL publier dans sa trame l'identifiant de la balise appairée hors de portée la plus proche. Une balise non vivante est classée comme la plus éloignée. Le bracelet SHALL publier 0 quand aucune balise appairée n'est hors de portée. La mise en sourdine ne retire pas une balise de cette publication.

#### Scenario: Deux balises hors de portée
- **WHEN** la balise A est hors de portée à 1,2 m et la balise B à 3 m
- **THEN** le bracelet publie l'identifiant de la balise A comme balise en alerte

### Requirement: Action utilisateur unique
Le bracelet SHALL offrir une action utilisateur unique, dont l'effet dépend de l'état, dans cet ordre de priorité :
1. si une sonnerie est en cours, l'annuler ;
2. sinon, si l'alerte vibrante est active, mettre en sourdine toutes les balises hors de portée ;
3. sinon, faire sonner la balise appairée la plus proche.

#### Scenario: Snooze d'une alerte
- **WHEN** l'alerte vibrante est active et l'utilisateur déclenche l'action
- **THEN** la vibration s'arrête et les balises hors de portée sont mises en sourdine

#### Scenario: Action sans alerte ni sonnerie
- **WHEN** aucune alerte ni sonnerie n'est active et l'utilisateur déclenche l'action
- **THEN** la balise appairée la plus proche se met à sonner

### Requirement: Levée automatique du snooze
Une balise en sourdine SHALL sortir de sourdine automatiquement dès qu'elle est vivante et que sa distance estimée est inférieure ou égale au seuil proche. Elle pourra alors déclencher une nouvelle alerte si elle ressort du périmètre.

#### Scenario: Canne récupérée puis reperdue
- **WHEN** une balise en sourdine revient à 0,3 m puis repart à 1 m
- **THEN** l'alerte vibrante se déclenche à nouveau
