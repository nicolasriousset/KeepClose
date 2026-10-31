# Spec Delta

## Purpose

Définit comment le bracelet Bangle.js 2 s'intègre à la montre : surveillance permanente en arrière-plan, état lisible d'un coup d'œil, geste d'action accessible sans regarder l'écran, app de consultation et réglages.

## ADDED Requirements

### Requirement: Surveillance en arrière-plan
Le bracelet SHALL scanner, émettre sa trame et produire les alertes en permanence, quelle que soit l'app affichée (cadran, lanceur, autre app). Un changement d'app SHALL interrompre la surveillance moins de 5 s.

#### Scenario: Cadran affiché
- **WHEN** la montre affiche son cadran et qu'une balise appairée sort du périmètre
- **THEN** l'alerte vibrante se déclenche sans que l'utilisateur ait ouvert l'app KeepClose

### Requirement: Continuité de l'état entre apps
Le mode, les appairages, les mises en sourdine et l'état « atteinte » du mode Ti Poucet SHALL être conservés lors d'un changement d'app sur la montre. Les mesures RSSI pourront être réinitialisées.

#### Scenario: Snooze conservé
- **WHEN** une balise est en sourdine et que l'utilisateur ouvre puis ferme une autre app
- **THEN** la balise est toujours en sourdine

### Requirement: Widget d'état
Le bracelet SHALL afficher dans la barre de widgets un indicateur d'état qui reprend les couleurs de la LED du bracelet XIAO :
- bleu : aucune balise appairée ;
- vert : toutes les balises appairées sont à portée ;
- rouge : au moins une balise appairée est hors de portée ;
- vert pendant 5 s après un appairage.

L'indicateur SHALL aussi différer par sa forme, pour ne pas reposer sur la couleur seule.

#### Scenario: Canne hors de portée
- **WHEN** une balise appairée est hors de portée
- **THEN** le widget affiche l'état rouge avec un pictogramme distinct de l'état vert

### Requirement: Geste d'action
L'action utilisateur unique SHALL être déclenchable par un double tap sur la montre depuis n'importe quel écran. Dans l'app KeepClose, elle SHALL aussi l'être par le bouton physique et par un grand bouton tactile. Chaque déclenchement SHALL être confirmé par une vibration brève (environ 50 ms).

#### Scenario: Snooze sans regarder la montre
- **WHEN** l'alerte vibre et que l'utilisateur tape deux fois sur la montre
- **THEN** l'alerte passe en sourdine

### Requirement: App de consultation
L'app plein écran KeepClose SHALL afficher :
- le mode actif ;
- la liste des balises appairées, triées par distance croissante (les balises non vivantes en dernier) ;
- pour chaque balise, sa distance estimée ou « hors portée », et ses états sourdine / atteinte / en sonnerie ;
- le niveau de batterie de la montre.

Les textes SHALL utiliser une police de grande taille.

#### Scenario: Consultation des balises
- **WHEN** l'utilisateur ouvre l'app avec deux balises appairées, l'une à 0,3 m et l'autre hors portée
- **THEN** la balise à 0,3 m apparaît en premier avec sa distance, et l'autre apparaît avec la mention « hors portée »

### Requirement: Réglages
Le bracelet SHALL proposer des réglages accessibles depuis le menu Réglages de la montre et depuis l'app :
- choix du mode (N.I.C.O. / Ti Poucet) ;
- « Nouveau trajet » (mode Ti Poucet) ;
- « Oublier les balises » ;
- activation ou désactivation de KeepClose.

Désactivé, le bracelet SHALL cesser de scanner, d'émettre sa trame KeepClose et d'alerter.

#### Scenario: Désactivation
- **WHEN** l'utilisateur désactive KeepClose dans les réglages
- **THEN** le bracelet n'émet plus de trame KC et ne vibre plus pour les balises
