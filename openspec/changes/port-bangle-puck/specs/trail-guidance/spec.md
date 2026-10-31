# Spec Delta

## Purpose

C'est le mode Ti Poucet. L'utilisateur dépose des balises le long d'un chemin, puis le bracelet le guide au retour en faisant sonner successivement la balise la plus proche qui n'a pas encore été atteinte.

## ADDED Requirements

### Requirement: Sélection du mode Ti Poucet
Le mode Ti Poucet SHALL être activable depuis les réglages du bracelet, à la place du mode N.I.C.O. Le mode choisi SHALL persister après un redémarrage. En mode Ti Poucet, l'alerte vibrante de sortie de périmètre SHALL être désactivée.

#### Scenario: Bascule de mode
- **WHEN** l'utilisateur choisit « Ti Poucet » dans les réglages
- **THEN** le bracelet ne vibre plus quand une balise s'éloigne et commence le guidage

### Requirement: Guidage vers la balise suivante
En mode Ti Poucet, le bracelet SHALL désigner en continu comme balise à sonner la balise appairée, vivante et non atteinte la plus proche. Le bracelet SHALL publier 0 comme balise en alerte.

#### Scenario: Départ du retour
- **WHEN** trois balises appairées sont vivantes et aucune n'est atteinte
- **THEN** la plus proche sonne

### Requirement: Balise atteinte
Une balise vivante dont la distance estimée devient inférieure à 1,0 m (seuil configurable) SHALL être marquée atteinte. Elle cesse alors immédiatement d'être désignée, et la balise non atteinte la plus proche suivante SHALL être désignée sans attendre.

#### Scenario: Passage d'une balise à la suivante
- **WHEN** l'utilisateur arrive à 0,8 m de la balise qui sonne
- **THEN** cette balise se tait et la balise non atteinte la plus proche se met à sonner

### Requirement: Fin de trajet
Quand toutes les balises appairées vivantes sont atteintes, le bracelet SHALL émettre deux vibrations longues (environ 500 ms) une seule fois. Il SHALL ensuite cesser de désigner une balise à sonner.

#### Scenario: Dernière balise atteinte
- **WHEN** la dernière balise non atteinte est atteinte
- **THEN** la montre vibre deux fois longuement et plus aucune balise ne sonne

### Requirement: Nouveau trajet
Le bracelet SHALL proposer une action « Nouveau trajet » qui efface l'état « atteinte » de toutes les balises. L'état « atteinte » SHALL persister jusqu'à cette action ou jusqu'à un changement de mode.

#### Scenario: Recommencer un parcours
- **WHEN** l'utilisateur lance « Nouveau trajet »
- **THEN** toutes les balises appairées redeviennent candidates et la plus proche sonne

### Requirement: Pause du guidage
En mode Ti Poucet, l'action utilisateur unique SHALL basculer le guidage entre pause et reprise. En pause, aucune balise n'est désignée à sonner.

#### Scenario: Mise en pause
- **WHEN** l'utilisateur déclenche l'action pendant le guidage
- **THEN** la balise qui sonnait se tait en moins de 3 s, et une nouvelle action reprend le guidage
