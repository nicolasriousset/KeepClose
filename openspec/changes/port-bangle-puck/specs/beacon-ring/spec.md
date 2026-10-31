# Spec Delta

## Purpose

Permet au bracelet de faire sonner une balise appairée pour que l'utilisateur la localise à l'oreille, et permet d'arrêter cette sonnerie depuis le bracelet ou depuis la balise.

## ADDED Requirements

### Requirement: Déclenchement de la sonnerie depuis le bracelet
Quand l'action utilisateur demande une sonnerie (mode N.I.C.O.), le bracelet SHALL choisir la balise appairée la plus proche parmi les balises vivantes, ou à défaut une balise appairée connue. Il SHALL publier l'identifiant de cette balise comme balise à sonner. Sans balise appairée, aucune sonnerie n'est demandée et le bracelet SHALL le signaler à l'utilisateur.

#### Scenario: Retrouver la canne posée
- **WHEN** l'utilisateur déclenche l'action sans alerte en cours, avec une balise appairée vivante
- **THEN** le bracelet publie l'identifiant de cette balise comme balise à sonner

#### Scenario: Aucune balise appairée
- **WHEN** l'utilisateur déclenche l'action alors qu'aucune balise n'est appairée
- **THEN** aucune balise ne sonne et le bracelet émet un retour indiquant l'échec (vibration longue unique)

### Requirement: Durée maximale de sonnerie
En mode N.I.C.O., une sonnerie demandée par le bracelet SHALL s'arrêter automatiquement au bout de 10 s : le bracelet publie alors 0 comme balise à sonner. L'action utilisateur pendant la sonnerie SHALL l'arrêter immédiatement.

#### Scenario: Arrêt automatique
- **WHEN** une sonnerie a été déclenchée il y a 10 s
- **THEN** la balise cesse de sonner en moins de 3 s

#### Scenario: Annulation par l'utilisateur
- **WHEN** l'utilisateur déclenche l'action pendant une sonnerie
- **THEN** le bracelet publie 0 comme balise à sonner et la balise se tait en moins de 3 s

### Requirement: Sonnerie sur la balise
Une balise SHALL sonner tant que la dernière trame reçue de son bracelet propriétaire la désigne comme balise à sonner et qu'elle n'a pas été acquittée. Le motif SHALL être un « bip-bip » d'une période de 1 s : 100 ms de son, 100 ms de silence, 100 ms de son, 700 ms de silence.

#### Scenario: Balise désignée
- **WHEN** la balise reçoit une trame de son bracelet qui la désigne comme balise à sonner
- **THEN** elle émet le motif bip-bip en moins de 3 s

#### Scenario: Ordre venant d'un autre bracelet
- **WHEN** une balise reçoit une trame d'un bracelet qui n'est pas son propriétaire et qui la désigne
- **THEN** la balise ne sonne pas

#### Scenario: Bracelet hors de portée
- **WHEN** la balise ne reçoit plus aucune trame de son bracelet depuis 6 s pendant une sonnerie
- **THEN** la balise cesse de sonner

### Requirement: Acquittement sur la balise
Un appui sur le bouton de la balise pendant qu'elle sonne SHALL la faire taire jusqu'à ce que le bracelet cesse de la désigner. Une demande de sonnerie ultérieure SHALL à nouveau la faire sonner.

#### Scenario: Balise retrouvée et acquittée
- **WHEN** l'utilisateur appuie sur le bouton de la balise qui sonne
- **THEN** la balise se tait, même si le bracelet la désigne encore

#### Scenario: Nouvelle demande après acquittement
- **WHEN** le bracelet a cessé de désigner la balise puis la désigne de nouveau
- **THEN** la balise sonne à nouveau
