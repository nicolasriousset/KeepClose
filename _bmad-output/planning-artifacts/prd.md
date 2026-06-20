---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-02b-vision', 'step-02c-executive-summary', 'step-03-success', 'step-04-journeys', 'step-05-domain', 'step-06-innovation', 'step-07-project-type', 'step-08-scoping', 'step-09-functional', 'step-10-nonfunctional', 'step-11-polish', 'step-12-complete']
completedAt: '2026-04-27'
workflowStatus: complete
releaseMode: single-release
inputDocuments:
  - '_bmad-output/project-context.md'
  - 'OneDrive/Ti Poucet/notes Petit Poucet.md'
  - 'OneDrive/Ti Poucet/Precanne.pdf (slides atelier 14/01/2025)'
  - 'OneDrive/NICO Livret outil pedagogique bracelet vibratoire.pdf'
  - 'OneDrive/Ti Poucet/AVV_25_01_MOBIDETECT.pdf (BLUEGRioT offre technique)'
  - 'OneDrive/Ti Poucet/0131_MOB_.pdf (Petit Poucet v2 livraison juin 2025)'
  - 'OneDrive/Ti Poucet/MOB_131_presentation_POC.pdf (design POC Mobidetect)'
classification:
  projectType: iot_embedded
  domain: assistive_technology
  complexity: medium
  projectContext: brownfield
workflowType: 'prd'
briefCount: 0
researchCount: 3
brainstormingCount: 0
projectDocsCount: 4
---

# Product Requirements Document - KeepClose v2

**Auteur :** Nicolas
**Date :** 2026-04-27

## Résumé exécutif

KeepClose v2 est un système de proximité BLE autonome pour personnes malvoyantes, ne nécessitant ni smartphone, ni infrastructure réseau, ni intervention d'un accompagnant. Il résout deux problèmes structurellement identiques sur une seule plateforme matérielle : **la prévention de perte d'objets** (mode KeepClose — détection du dépassement de seuil de proximité pour une canne blanche, des clés, un traitement) et **la navigation en retour autonome** (mode Ti Poucet — retrouver un chemin en déclenchant séquentiellement des balises posées en cours de route). Les deux modes s'exécutent sur un bracelet maître PCB sur mesure couplé à jusqu'à 3 balises BLE esclaves.

Le bracelet scanne en continu les balises appairées par estimation de distance RSSI. Un modèle d'alerte à deux seuils encode l'identité de la balise dans chaque signal : franchissement du seuil proche (~10 m) → **N impulsions haptiques** (la balise N dérive) ; franchissement du seuil lointain (~20 m) → escalade en **N bips sonores** (la balise N est hors de portée). Pour localiser un objet, l'utilisateur fait un **appui long (≥ 2 s)** sur le bracelet — la balise la plus proche sonne ; il l'atteint puis appuie sur son **interrupteur physique** pour la silencier. En mode Ti Poucet, cette action confirme la collecte et déclenche automatiquement la balise suivante. L'appairage s'effectue en touchant physiquement une balise contre le bracelet ; celui-ci confirme par N bips le numéro de slot attribué.

### Ce qui rend ce produit unique

**La surveillance silencieuse** est le différenciateur principal. Les balises embarquent un buzzer mais n'émettent aucun son en mode surveillance — elles ne sont déclenchées que par une action utilisateur ou par une alerte de proximité. Cela supprime la friction sociale des bips continus qui rend les dispositifs d'assistance concurrents inacceptables en classe, au travail ou dans les espaces publics.

**Le modèle d'interaction par encodage N** est l'insight central : un seul entier (le numéro de slot N de la balise) se propage uniformément à travers toutes les couches de retour — nombre d'impulsions haptiques, nombre de bips sonores, nombre de bips de confirmation à l'appairage. Un utilisateur qui associe "2 pulsations = mes clés" sait que deux bips signifient qu'elles sont plus loin, et que l'appairage a produit 2 bips de confirmation. Un seul modèle, aucun mode à apprendre.

**Impact réel sur l'autonomie, pas seulement sur la sécurité.** Les éducateurs utilisant la variante NICO (bracelet en contexte pédagogique) constatent que les enfants retrouvent leur canne blanche et parcourent des trajets d'entraînement sans intervention adulte — un gain mesurable sur les compétences d'autonomie ciblées par les cadres d'éducation inclusive. Cela positionne KeepClose comme outil pédagogique autant que dispositif d'assistance, ouvrant les voies de financement MDPH et de déploiement institutionnel.

**La convergence matérielle résout les deux blocages v1 en une seule conception.** La dépendance à la LilyGo T-Watch (BSP verrouillé sur ESP32 board package v2.0.17, facteur de forme discontinué, ergonomie non personnalisable) et le protocole de sonnette de porte de Ti Poucet (consommation 12 V excessive, pas de RSSI, pas de BLE) sont remplacés par un bracelet PCB sur mesure + balises BLE natives. Une seule base de code, une seule chaîne d'approvisionnement, deux cas d'usage.

## Classification du projet

| Attribut | Valeur |
|---|---|
| **Type de projet** | IoT / Embarqué — bracelet PCB sur mesure (maître) + balises BLE esclaves |
| **Domaine** | Technologies d'assistance |
| **Complexité** | Moyenne — firmware double mode, estimation RSSI, UX pour malvoyants, hardware sur mesure |
| **Contexte** | Brownfield — évolution depuis KeepClose v1 (T-Watch) + Ti Poucet v1 (protocole sonnette) |
| **Périmètre de déploiement** | Autonome — sans cloud, sans smartphone |
| **Partenaires clés** | RD2 (hardware), BLUEGRioT (intégration Ti Poucet), CNRHR (validation terrain) |
| **Voie de financement** | MDPH — classification accessoire d'assistance (vs. dispositif médical) |

## Critères de succès

### Succès utilisateur

- L'utilisateur malvoyant retrouve un objet tracé (canne blanche, clés, etc.) de façon autonome, sans intervention verbale ou physique d'un accompagnant
- L'utilisateur Ti Poucet retrace un chemin en activant les balises dans l'ordre inverse, sans aide extérieure
- Le produit est recommandé spontanément par les utilisateurs malvoyants et les éducateurs qui l'ont utilisé en situation réelle (validation qualitative CNRHR)
- L'utilisateur ne signale aucune fausse alarme en mode KeepClose — toute alerte correspond à un réel franchissement de seuil

### Succès business

- Déploiement validé dans au moins un établissement pilote avec retour terrain positif d'éducateurs et d'utilisateurs
- Le produit est identifié comme finançable via la MDPH en tant qu'accessoire d'assistance (hors classification dispositif médical)
- Les deux modes (KeepClose et Ti Poucet) sont opérationnels sur la même plateforme matérielle v2 — aucun compromis fonctionnel entre les deux cas d'usage

### Succès technique

- **Latence d'alerte ≤ 10 secondes** après franchissement d'un seuil de proximité
- **Zéro fausse alarme** en mode KeepClose — le modèle RSSI est suffisamment filtré pour ne pas déclencher sur des fluctuations transitoires
- **Autonomie bracelet ≥ 48 h** en usage continu (scan BLE actif, retour haptique et sonore inclus)
- **Autonomie balises ≥ 3 mois** sans recharge (BLE advertising basse consommation)
- Seuils proche (~10 m) et lointain (~20 m) calibrés et stables en environnement intérieur (bâtiment administratif, école, salle de rééducation)
- Système autonome : aucune dépendance à un smartphone, à Internet, à un réseau tiers

### Résultats mesurables

| Indicateur | Cible |
|---|---|
| Taux de fausses alarmes (KeepClose) | 0 |
| Latence d'alerte après franchissement de seuil | ≤ 10 s |
| Autonomie bracelet | ≥ 48 h |
| Autonomie balises | ≥ 3 mois |
| Modes opérationnels sur la plateforme v2 | 2 (KeepClose + Ti Poucet) |
| Validation terrain | Recommandé par utilisateurs + éducateurs |

## Périmètre produit

### Capacités indispensables (v2)

- Bracelet maître sur PCB sur mesure avec moteur haptique, rechargeable USB-C, autonomie ≥ 48 h (remplace LilyGo T-Watch)
- Balises BLE natives avec buzzer, interrupteur physique, LED état batterie, autonomie ≥ 3 mois (remplacent le protocole sonnette 12 V de Ti Poucet)
- Mode KeepClose : scan BLE continu, double seuil (haptique proche / sonore lointain), encodage N
- Mode Ti Poucet : appui long → balise la plus proche sonne, interrupteur balise → collecte confirmée, balise suivante sonne
- Appairage physique tap-bracelet → N bips de confirmation
- Appui long bracelet (≥ 2 s) → balise la plus proche sonne dans les deux modes
- Zéro dépendance réseau ou smartphone

### Capacités souhaitables (v2, si délai le permet)

- Calibration des seuils RSSI ajustable sans recompilation
- Indication batterie faible bracelet par signal perceptible

### Hors périmètre v2

- Application smartphone de configuration
- Mise à jour firmware OTA
- GPS pour Ti Poucet en extérieur (v2.1)
- Multi-profils utilisateur sur un même bracelet

### Risques et mitigations

| Risque | Niveau | Mitigation |
|---|---|---|
| BLE bidirectionnel balises (GATT server + buzzer + interrupteur) | Élevé | Prototyper la connexion bracelet↔balise en priorité sur module nRF52 de développement, avant le PCB final |
| PCB bracelet sur mesure — délai et faisabilité | Moyen | Commencer le firmware sur dev board ESP32-C3, découpler développement software et hardware |
| Calibration RSSI en intérieur — seuils incorrects | Moyen | Sessions de calibration terrain avec CNRHR dès le premier prototype, seuils configurables |

### Ressources requises

- Développement firmware bracelet (ESP32) : NimBLE/BLE GATT, Arduino/C++
- Développement firmware balise (nRF52832) : BLE advertising + GATT server, gestion interrupteur/buzzer
- Conception PCB bracelet : partenaire RD2
- Validation terrain : CNRHR (éducateurs et utilisateurs malvoyants)

## Parcours utilisateurs

### Parcours 1 — Malak, 10 ans, élève en ULIS (chemin nominal KeepClose)

Malak a une déficience visuelle sévère. Scolarisée en classe ULIS, elle pose sa canne blanche près de son bureau et l'oublie quand elle se déplace. Résultat habituel : elle appelle l'éducatrice 3 à 4 fois par séance pour savoir où elle est.

C'est lundi matin. Malak porte le bracelet NICO au poignet gauche. Sa canne est équipée de la balise 1 (appairée par l'éducatrice, confirmée par 1 bip). Elle se lève pour aller aux toilettes et laisse sa canne contre le bureau.

À 8 mètres dans le couloir, le bracelet vibre une fois. Malak reconnaît le signal — une vibration, c'est sa canne. Elle n'a pas besoin qu'on lui explique quelle balise est concernée.

Au retour, elle veut la retrouver seule. Elle fait un appui long sur le bracelet. La canne émet un bip. Malak oriente son oreille, s'approche, la récupère. L'éducatrice n'a rien eu à faire.

L'éducatrice note dans son carnet : *"Malak a récupéré sa canne seule deux fois ce matin."* Pour la première fois depuis la rentrée, Malak n'a pas demandé d'aide. La classe est moins interrompue.

**Capacités révélées :** détection proximité, alerte haptique encodée N, localisation acoustique par appui long, usage sans interface visuelle.

---

### Parcours 2 — Jean-Pierre, 52 ans, adulte malvoyant autonome (multi-objets + cas limite)

Jean-Pierre vit seul depuis un AVC qui a affecté sa vision à 38 ans. Il perd régulièrement ses clés et son pilulier. Un AirTag a été suggéré — mais sans iPhone à proximité, il ne fonctionne pas. Il ne veut pas de smartphone.

Il porte le bracelet. Deux balises sont appairées : balise 1 (clés, 1 bip), balise 2 (pilulier, 2 bips). En rentrant du marché, les clés glissent sous un sac sans qu'il le remarque.

Deux minutes plus tard, il fait un appui long sur le bracelet. La balise la plus proche sonne — c'est la balise 1 (les clés), étouffée sous le sac. Il soulève le sac vers la source du son. Les clés sont là. Moins de 15 secondes de recherche.

**Cas limite :** Un jour, il oublie son pilulier chez son médecin à 25 mètres. Le bracelet émet 2 bips sonores (seuil lointain franchi). Il comprend immédiatement — c'est le pilulier. Il retourne à l'accueil et le récupère avant de quitter le cabinet.

Jean-Pierre n'a pas besoin de son fils pour gérer ses oublis. Pour lui, c'est une question de dignité.

**Capacités révélées :** multi-balises simultanées, double seuil (haptique proche / sonore lointain), encodage N fiable avec plusieurs objets, zéro fausse alarme comme condition de confiance.

---

### Parcours 3 — Romain, 41 ans, malvoyant en fauteuil roulant (mode Ti Poucet)

Romain doit renouveler sa carte d'invalidité dans une préfecture qu'il n'a jamais visitée. Bâtiment de 4 étages, plusieurs guichets, panneaux illisibles pour lui. Il utilise le bracelet en mode Ti Poucet.

À l'entrée, il pose la balise 1. À l'ascenseur du 2e étage, la balise 2. Au guichet Invalidité, la balise 3. Il fait ses démarches.

Au départ, il fait un appui long — la balise 3 (la plus proche, juste devant lui) sonne. Il appuie sur son interrupteur ; la balise 3 se tait et la balise 2 sonne automatiquement dans le couloir à gauche. Il navigue vers elle, appuie sur son interrupteur ; la balise 1 sonne près des portes d'entrée. Il sort sans avoir demandé son chemin une seule fois.

Il peut revenir seul la semaine suivante pour un rendez-vous de suivi. Il commence à utiliser le mode Ti Poucet dans d'autres contextes : salle d'attente CNRHR, clinique.

**Capacités révélées :** mode Ti Poucet (pose de jalons), localisation séquentielle par appui long + interrupteur, désappairage/récupération de balise, usage en mobilité réduite (gestes simples).

---

### Parcours 4 — Sophie, éducatrice spécialisée (configuration et suivi)

Sophie gère une classe ULIS de 6 élèves malvoyants. Elle reçoit 2 bracelets et 4 balises. Elle doit les configurer avant la rentrée et former les parents.

Elle tape la balise 1 sur le bracelet — 1 bip. Balise 2 — 2 bips. Elle teste en éloignant la balise 2 à 12 mètres : le bracelet vibre 2 fois. Elle fait un appui long : la balise 2 (la plus proche) sonne. Validé.

Le lundi matin, elle installe le bracelet à Malak : *"1 vibration = ta canne. Si tu la cherches, appuie longtemps sur le bracelet."* Malak teste immédiatement. Ça marche. Aucun outil informatique, aucune application, aucune connexion requise.

Sophie configure un bracelet en moins de 5 minutes. Elle forme les parents en 10 minutes à la réunion de rentrée. La batterie du bracelet tient 3 jours — elle charge le vendredi. Les balises, elle ne les a pas rechargées depuis 2 mois.

**Capacités révélées :** appairage sans outil, procédure mémorisable sans documentation, information d'état batterie visible par un voyant, durée de vie balise suffisante pour disparaître de la charge cognitive.

---

### Parcours 5 — Claire, mère de Malak (ré-appairage autonome pendant les vacances)

Claire a 2 enfants, travaille à temps plein. Malak a ramené le bracelet pour les vacances. Une balise s'est déconnectée (pile faible signalée par LED clignotante). Claire doit ré-apparier sans l'éducatrice.

Elle se souvient du geste montré par Sophie : taper la balise sur le bracelet. Elle le fait. 1 bip. Elle éloigne la balise dans le couloir — le bracelet de Malak vibre. Elle replace la balise sur la canne.

3 minutes. Sans documentation, sans appel.

**Cas limite :** La pile de la balise est à plat — plus de signal LED. Claire constate que le bracelet ne détecte plus la balise 1. Elle remplace la pile (standard, référencée sur l'étiquette de la balise). Malak utilise à nouveau le bracelet le lendemain.

**Capacités révélées :** indicateur d'état balise accessible à un voyant (LED), pile standard remplaçable par un non-technicien, ré-appairage sans mémoire de la configuration précédente.

---

### Synthèse des capacités révélées par les parcours

| Capacité | Parcours déclencheur |
|---|---|
| Scan BLE continu multi-balises, double seuil (proche / lointain) | 1, 2 |
| Alerte haptique encodée N / escalade sonore encodée N | 1, 2 |
| Appui long bracelet → balise la plus proche sonne | 1, 2, 3 |
| Interrupteur balise → silencieux + (Ti Poucet) balise suivante sonne | 3 |
| Appairage physique tap-bracelet, confirmation N bips | 4, 5 |
| Mode Ti Poucet : pose de jalons, collecte séquentielle | 3 |
| Aucune interface visuelle requise pour l'usage courant | 1, 2, 3 |
| Indicateur d'état batterie balise (LED) accessible à un voyant | 4, 5 |
| Pile balise standard remplaçable sans outil | 5 |
| Système entièrement autonome (pas de réseau, pas de smartphone) | tous |

## Exigences spécifiques au domaine

### Conformité réglementaire

- **Classification : accessoire d'assistance** (hors périmètre MDR — Règlement Dispositifs Médicaux européen). Le produit n'émet pas de diagnostic, ne surveille pas de paramètre physiologique, et ne conditionne pas un acte médical.
- **Marquage CE obligatoire avant commercialisation**, relevant de la directive RED (Radio Equipment Directive 2014/53/UE) pour les émetteurs BLE. Non requis pour le prototype v2.
- **Stratégie hardware :** privilégier des modules radio et composants déjà certifiés CE/FCC (modules ESP32 ou nRF5x certifiés) pour minimiser le périmètre de certification du PCB final.
- **Financement MDPH** : la classification accessoire d'assistance est cohérente avec les voies de financement MDPH existantes pour les aides techniques à l'autonomie. Aucune validation clinique formelle requise.

### Contraintes techniques domaine

- **Interférences BLE en environnement dense** : les bâtiments administratifs, écoles et centres de rééducation comportent de nombreux émetteurs BLE (téléphones, objets connectés, beacons publicitaires). Le firmware filtre strictement sur les balises appairées et résiste aux fluctuations RSSI induites par les interférences.
- **Robustesse en intérieur** : la propagation BLE est perturbée par les murs, portes métalliques et corps humains. Les seuils (~10 m et ~20 m) sont calibrés en conditions réelles, pas en espace ouvert.
- **Sécurité de défaillance** : une panne du bracelet entraîne une perte d'autonomie, pas un risque vital. La défaillance acceptable est silencieuse (plus d'alerte) plutôt que bruyante (fausse alarme permanente).

### Vie privée et données

- **Zéro collecte de données** : le système ne transmet ni n'enregistre aucune donnée utilisateur. Tous les traitements sont locaux au bracelet. Aucune connectivité cloud, aucun identifiant personnel.
- **RGPD** : risque nul. Aucune exigence de consentement, de registre de traitement ou de DPO.
- Toute fonctionnalité future ajoutant de la connectivité (app de configuration, historique) fera l'objet d'une analyse RGPD à ce moment-là.

### Risques domaine et mitigations

| Risque | Mitigation |
|---|---|
| RSSI instable en intérieur → fausse alarme | Filtrage MovingAverage, seuil conservateur, hystérésis entre états |
| Matériel non certifié bloque la commercialisation | Utiliser des modules radio pré-certifiés dès la conception PCB |
| Mauvaise classification MDPH → pas de financement | Valider la classification avec un conseiller MDPH en amont du déploiement pilote |
| Balise confondue avec un beacon BLE ambiant | Filtre strict sur UUID/nom de balise appairée, jamais de scan générique |

## Innovation et patterns nouveaux

### Zones d'innovation identifiées

**1. Modèle d'interaction N-encoding**
Un seul entier (le numéro de slot N de la balise) propage l'identité à travers tous les canaux de feedback simultanément : N impulsions haptiques à l'alerte proche, N bips sonores à l'alerte lointaine, N bips de confirmation à l'appairage. Ce modèle unifié n'existe pas dans les dispositifs d'assistance malvoyants actuels, où chaque canal opère indépendamment et nécessite un apprentissage séparé. Une seule règle couvre tous les états du système.

**2. Surveillance silencieuse + localisation acoustique active sur demande**
Les systèmes de proximité existants alertent passivement (bip continu). Les systèmes de localisation guidant activement (GPS vocal, audio spatialisé). KeepClose v2 hybride les deux : le bracelet alerte sans bruit en surveillance, et l'utilisateur déclenche la localisation acoustique par appui long. Ce paradigme "alerte muette + guidance explicite à la demande" est inédit dans le domaine des aides techniques.

**3. Convergence d'usages anti-perte et navigation jalons sur une UX haptique/sonore sans écran**
KeepClose (prévention de perte d'objets proches) et Ti Poucet (navigation par jalons en environnement inconnu) sont structurellement distincts mais résolus par la même UX et le même hardware. Unifier deux cas d'usage assistive technology sur une plateforme sans écran, sans smartphone, sans réseau, avec un modèle d'interaction identique, constitue une architecture système originale.

### Contexte marché et paysage concurrentiel

Les solutions existantes se fragmentent en trois catégories :
- **AirTag / Tile** : dépendance smartphone et réseau, inutilisables en zone isolée ou sans iPhone à proximité, non conçus pour malvoyants
- **Dispositifs GPS pour malvoyants** : coûteux, dépendants réseau, inefficaces en intérieur
- **Systèmes de localisation sonore en établissement** : fixes, non portables, coûts d'infrastructure élevés

Aucun produit du marché ne combine : standalone BLE + UX haptique/sonore unifiée + double usage anti-perte/navigation + appairage sans outil + silencieux en surveillance.

### Approche de validation

- **Test d'appairage à froid** : un utilisateur sans formation préalable appaire une balise en ≤ 3 tentatives — valide l'intuitivité du geste physique
- **Test N-encoding en aveugle** : un utilisateur malvoyant associe correctement N vibrations à l'objet N après 5 minutes d'usage — valide la mémorabilité du modèle
- **Test de localisation acoustique** : un utilisateur retrouve un objet caché dans une pièce par appui long seul, sans aide verbale — valide l'efficacité de la localisation sonore
- **Test d'endurance terrain** : déploiement 2 semaines en classe ULIS avec CNRHR — valide la fiabilité zéro fausse alarme et l'autonomie batterie en conditions réelles

### Voies de financement innovation

| Dispositif | Périmètre | Pertinence |
|---|---|---|
| **CIR** (Crédit d'Impôt Recherche) | R&D — modèle N-encoding, estimation RSSI en intérieur, architecture firmware dual-mode | Élevée — activités de recherche applicative documentables |
| **CII** (Crédit d'Impôt Innovation) | Prototype innovant — conception PCB, développement firmware v2 | Élevée — prototype tangible avec différenciation marché démontrée |
| **JEI** (Jeune Entreprise Innovante) | Statut fiscal pour structure jeune consacrant >15 % de ses charges à la R&D | À évaluer selon la structure juridique portant le projet |

## Exigences spécifiques IoT / Embarqué

### Architecture du système

Système distribué composé d'un bracelet maître (scanner BLE actif) et de jusqu'à 3 balises esclaves (advertisers BLE passifs avec buzzer et interrupteur). Toute la logique applicative réside dans le bracelet. Les balises embarquent un buzzer (déclenchement à distance) et un interrupteur physique (confirmation de collecte en mode Ti Poucet).

### Spécifications hardware

**Bracelet maître :**
- MCU : **ESP32-C3 pour le prototype v2** (continuité NimBLE/Arduino) — **nRF52840 recommandé pour la version commerciale** (autonomie BLE supérieure)
- Moteur haptique (vibration)
- Indicateur batterie faible (LED ou bip au démarrage)
- Rechargeable USB-C — autonomie cible ≥ 48 h

**Balises esclaves (×3 max) :**
- MCU BLE basse consommation (nRF52832 ou équivalent)
- Buzzer intégré — déclenché à distance via connexion BLE point-à-point
- Interrupteur physique large, facilement repérable à la main — double usage : arrêt sonnerie + confirmation collecte (Ti Poucet) / tap d'appairage
- LED état batterie (visible par un accompagnant voyant)
- Rechargeable USB-C de préférence ; pile primaire acceptable si autonomie ≥ 3 mois
- Batterie/pile remplaçable sans outil

### Modèle d'interaction

**Mode KeepClose :**
- Bracelet vibre N fois → balise N franchit le seuil proche (~10 m)
- Bracelet émet N bips → balise N franchit le seuil lointain (~20 m)
- Appui long bracelet (≥ 2 s) → balise la plus proche sonne

**Mode Ti Poucet :**
- Appui long bracelet → balise la plus proche (RSSI le plus fort) sonne (N bips)
- L'utilisateur marche vers la source sonore
- Interrupteur sur la balise atteinte → balise silenciée ET bracelet déclenche la balise suivante la plus proche (si détectée à < ~1 m = collecte confirmée)
- Séquence répétée jusqu'au retour au point de départ

**Appairage :**
- Tap physique de la balise contre le bracelet → N bips de confirmation (slot N attribué)
- Pas d'authentification, pas de protection contre appairage non autorisé

### Protocole de connectivité

- **Transport exclusif : BLE 5.x** — zéro réseau, smartphone, cloud ou infrastructure tierce
- **Bracelet (scanner)** : scan actif, paramètres v1 de référence : intervalle 62,5 ms, fenêtre 62,5 ms, durée 1 s toutes les 2 s — à optimiser en v2 pour réduire la consommation
- **Balises (advertisers)** : intervalle advertising 500 ms–1 000 ms (compromis latence / autonomie)
- **Déclenchement buzzer** : connexion BLE point-à-point bracelet → balise cible
- **Filtre identification** : UUID ou préfixe nom propre aux balises appairées (v1 : préfixe `"Holy"`)

### Profil de consommation

| Composant | Cible | Levier principal |
|---|---|---|
| Bracelet | ≥ 48 h | Duty cycle scan BLE |
| Balises | ≥ 3 mois | Intervalle d'advertising (500 ms–1 s) |
| Buzzer balise | Négligeable | Activé uniquement sur commande explicite |

### Modèle de sécurité

- Pas d'authentification BLE — appairage physique comme contrôle d'accès implicite
- Zéro donnée personnelle transmise — paquets BLE contiennent uniquement identifiant balise et RSSI
- Système fermé : pas de serveur, pas d'API exposée, surface d'attaque minimale

### Mises à jour firmware

- **Prototype v2** : mise à jour manuelle via câble USB
- **Post-MVP** : OTA via BLE — à concevoir une fois la plateforme hardware stabilisée

## Exigences fonctionnelles

### Détection et surveillance BLE

- **FR1 :** Le bracelet peut scanner en continu les émetteurs BLE environnants et identifier les balises appairées parmi eux
- **FR2 :** Le bracelet peut estimer la distance à chaque balise appairée par mesure RSSI
- **FR3 :** Le bracelet peut filtrer les fluctuations transitoires de RSSI pour éviter les fausses alertes
- **FR4 :** Le bracelet peut surveiller jusqu'à 3 balises appairées simultanément
- **FR5 :** Le bracelet peut détecter la perte de signal d'une balise appairée (hors de portée ou éteinte)

### Alertes de proximité (mode KeepClose)

- **FR6 :** Le bracelet peut alerter l'utilisateur par vibrations lorsqu'une balise franchit le seuil de proximité proche (~10 m)
- **FR7 :** Le bracelet peut escalader l'alerte en signal sonore lorsqu'une balise franchit le seuil de proximité lointain (~20 m)
- **FR8 :** Le bracelet peut encoder l'identité de la balise dans le nombre de vibrations émises (N vibrations = balise N)
- **FR9 :** Le bracelet peut encoder l'identité de la balise dans le nombre de bips émis (N bips = balise N)
- **FR10 :** Le bracelet peut gérer les alertes de plusieurs balises simultanément sans les confondre

### Localisation acoustique active

- **FR11 :** L'utilisateur peut déclencher la sonnerie de la balise la plus proche par un appui long sur le bracelet
- **FR12 :** La balise peut émettre un signal sonore (buzzer) sur commande du bracelet
- **FR13 :** Le signal sonore de la balise encode son numéro de slot (N bips)
- **FR14 :** L'utilisateur peut arrêter la sonnerie d'une balise en appuyant sur son interrupteur physique
- **FR15 :** Le bracelet peut établir une connexion BLE point-à-point avec une balise cible pour déclencher ou arrêter son buzzer

### Navigation par jalons (mode Ti Poucet)

- **FR16 :** L'utilisateur peut activer le mode Ti Poucet sur le bracelet
- **FR17 :** En mode Ti Poucet, le bracelet peut identifier la balise appairée la plus proche par comparaison RSSI
- **FR18 :** En mode Ti Poucet, un appui long sur le bracelet déclenche la sonnerie de la balise la plus proche uniquement
- **FR19 :** Le bracelet peut détecter qu'une balise a été collectée lorsque son interrupteur physique est actionné ET qu'elle est détectée à très courte portée (< ~1 m)
- **FR20 :** En mode Ti Poucet, après détection de collecte d'une balise, le bracelet peut automatiquement déclencher la sonnerie de la balise suivante la plus proche
- **FR21 :** Le bracelet peut gérer la séquence de collecte jusqu'à ce qu'aucune balise non collectée ne soit à portée

### Gestion des balises

- **FR22 :** L'utilisateur peut appairer une balise au bracelet par contact physique (tap)
- **FR23 :** Le bracelet peut confirmer un appairage réussi par un signal sonore encodant le numéro de slot attribué (N bips)
- **FR24 :** Le bracelet peut attribuer automatiquement un numéro de slot à chaque balise appairée (1, 2, 3)
- **FR25 :** Le bracelet peut conserver les informations d'appairage de façon persistante entre les utilisations
- **FR26 :** L'utilisateur peut désappairer une balise du bracelet
- **FR27 :** L'utilisateur peut réappairer une balise déjà connue sans modifier son numéro de slot

### Retour utilisateur

- **FR28 :** Le bracelet peut fournir un retour haptique (vibrations) utilisable sans interaction visuelle
- **FR29 :** Le bracelet peut fournir un retour sonore (bips) utilisable sans interaction visuelle
- **FR30 :** Le bracelet peut différencier un appui court d'un appui long sur son bouton principal
- **FR31 :** Toutes les interactions primaires du bracelet sont réalisables sans regarder le dispositif

### Gestion de l'alimentation et de l'état

- **FR32 :** Le bracelet peut signaler un niveau de batterie faible à l'utilisateur par un signal perceptible
- **FR33 :** La balise peut signaler un niveau de batterie faible via un indicateur visuel (LED)
- **FR34 :** La batterie ou pile de la balise peut être remplacée ou rechargée sans outil par un non-technicien

### Sélection du mode

- **FR35 :** L'utilisateur peut basculer entre le mode KeepClose et le mode Ti Poucet
- **FR36 :** Le bracelet peut conserver le dernier mode utilisé entre deux sessions

## Exigences non-fonctionnelles

### Performance

- **NFR1 :** La latence entre le franchissement d'un seuil de proximité et l'alerte perçue par l'utilisateur est ≤ 10 secondes
- **NFR2 :** Le bracelet complète un cycle de scan et de traitement RSSI en ≤ 2 secondes
- **NFR3 :** Le délai entre l'appui long sur le bracelet et le démarrage du buzzer de la balise cible est ≤ 3 secondes
- **NFR4 :** Le délai entre l'appui sur l'interrupteur de la balise et l'arrêt de sa sonnerie est ≤ 1 seconde

### Fiabilité

- **NFR5 :** Le taux de fausses alarmes en mode KeepClose est nul — toute alerte correspond à un franchissement de seuil réel et maintenu
- **NFR6 :** Le système ne génère pas d'alerte lors de fluctuations RSSI transitoires (passage de personnes, réflexions)
- **NFR7 :** L'appairage physique d'une balise réussit à la première tentative dans ≥ 90 % des cas
- **NFR8 :** Les informations d'appairage sont conservées après extinction et rallumage du bracelet sans intervention utilisateur
- **NFR9 :** Le système reste opérationnel en présence d'au moins 10 émetteurs BLE tiers simultanés dans l'environnement

### Accessibilité

- **NFR10 :** L'intégralité des interactions primaires (alerte, localisation, appairage) est utilisable sans vision ni lecture
- **NFR11 :** Aucune interaction primaire ne nécessite une coordination motrice fine (ex. : double-tap précis, geste multi-doigts)
- **NFR12 :** Un utilisateur sans formation préalable peut appairer une balise en ≤ 3 tentatives
- **NFR13 :** Un utilisateur sans formation préalable peut localiser un objet par appui long + interrupteur en ≤ 5 minutes de première utilisation
- **NFR14 :** Aucune interaction ne requiert de smartphone, d'application, de réseau ou d'infrastructure externe

### Autonomie

- **NFR15 :** Le bracelet assure une autonomie ≥ 48 heures en usage continu (scan BLE actif, alertes haptiques et sonores incluses)
- **NFR16 :** Les balises assurent une autonomie ≥ 3 mois en usage normal (advertising BLE continu, buzzer activé ponctuellement)
- **NFR17 :** Le bracelet signale un niveau de batterie faible suffisamment tôt pour permettre une recharge avant extinction (seuil ≥ 20 %)
- **NFR18 :** Les balises signalent un niveau de batterie faible via LED avant toute dégradation fonctionnelle

### Maintenabilité

- **NFR19 :** La batterie ou pile d'une balise peut être remplacée par un utilisateur non technicien en ≤ 5 minutes sans outil
- **NFR20 :** Un ré-appairage complet d'une balise (après remplacement ou réinitialisation) ne nécessite pas d'accès à un ordinateur ni à une application
- **NFR21 :** La mise à jour du firmware du bracelet (prototype v2) est réalisable via câble USB sans logiciel spécialisé au-delà de l'IDE Arduino
