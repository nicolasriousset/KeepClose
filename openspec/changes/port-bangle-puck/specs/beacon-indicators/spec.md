# Spec Delta

## Purpose

Définit les retours locaux de la balise Puck.js (LEDs, buzzer piézo, bouton, pile) et les contraintes d'autonomie liées à sa pile CR2032.

## ADDED Requirements

### Requirement: LEDs d'état en flash
La balise SHALL signaler son état par un flash bref (≤ 50 ms) toutes les 3 s environ, et jamais par une LED allumée en continu :
- bleu : non appairée ;
- vert : appairée, bracelet reçu depuis moins de 6 s et balise non désignée en alerte ;
- rouge : appairée, et soit désignée en alerte par son bracelet, soit sans trame du bracelet depuis 6 s ou plus.

#### Scenario: Balise neuve
- **WHEN** une balise non appairée est alimentée
- **THEN** elle émet un flash bleu toutes les 3 s environ

#### Scenario: Canne hors du périmètre
- **WHEN** le bracelet propriétaire désigne la balise comme balise en alerte
- **THEN** la balise émet des flashs rouges

### Requirement: Confirmation d'appairage sur la balise
Quand la balise adopte un nouveau bracelet propriétaire, elle SHALL allumer sa LED verte pendant 1 s.

#### Scenario: Contact avec le bracelet
- **WHEN** la balise vient d'adopter un bracelet
- **THEN** la LED verte s'allume 1 s, puis la balise reprend les flashs d'état

### Requirement: Buzzer piézo externe
La balise SHALL produire la sonnerie au moyen d'un buzzer piézo externe raccordé à deux broches documentées du Puck.js. La fréquence SHALL se situer dans la plage de résonance du piézo, autour de 4 kHz. Hors sonnerie, le buzzer SHALL être silencieux et ses broches au repos.

#### Scenario: Hors sonnerie
- **WHEN** la balise n'est pas désignée à sonner
- **THEN** le buzzer n'émet aucun son

### Requirement: Bouton de la balise
Le bouton du Puck.js SHALL être traité avec un anti-rebond d'environ 50 ms. Un appui pendant une sonnerie SHALL l'acquitter. Hors sonnerie, un appui SHALL déclencher un flash de l'état courant, pour permettre une vérification à la demande.

#### Scenario: Vérification de l'état
- **WHEN** l'utilisateur appuie sur le bouton d'une balise silencieuse
- **THEN** la balise flashe immédiatement la couleur de son état

### Requirement: Autonomie de la balise
La balise SHALL limiter la réception BLE à un rapport cyclique compatible avec plusieurs semaines d'autonomie sur CR2032. L'objectif est une latence ≤ 3 s pour prendre en compte un changement d'alerte ou de sonnerie publié par le bracelet.

#### Scenario: Latence de sonnerie
- **WHEN** le bracelet désigne la balise à sonner
- **THEN** la balise commence à sonner en moins de 3 s

### Requirement: Niveau de pile consultable
La balise SHALL exposer son niveau de pile en pourcentage sur la console Espruino. Quand ce niveau passe sous 15 %, elle SHALL ajouter un double flash rouge au flash d'état.

#### Scenario: Pile faible
- **WHEN** le niveau de pile de la balise passe sous 15 %
- **THEN** chaque flash d'état est suivi d'un double flash rouge
