---
date: '2026-05-01'
updated: '2026-05-01 — v3 (ajout décision proto bracelet PineTime vs Bangle.js 2)'
type: 'presentation-outline'
target: 'Gamma AI generator'
audience: 'Équipes Ti Poucet + N.I.C.O'
slides: 10
duration_estime: '15-20 minutes'
---

# Plan de présentation N.I.C.O v2 — Format Gamma (v3)

> **Instructions Gamma :** copier-coller le contenu ci-dessous (à partir du premier `#` jusqu'à la fin) dans l'option "Import outline" ou "Create with text" de Gamma. Les `---` séparent les slides. Les sections "*Notes orateur*" sont à laisser en speaker notes.

---

# N.I.C.O v2
## Une plateforme matérielle commune à Ti Poucet et N.I.C.O

Bilan des choix techniques, des coûts et des prochaines étapes

Nicolas — Mai 2026

*Notes orateur : présenter en 15-20 minutes les avancées hardware, la convergence des deux projets sur un socle commun, et les coûts. Ne pas entrer dans le code firmware — c'est une présentation de pilotage, pas de revue technique.*

> **Visuel suggéré :** photo ou rendu 3D d'un bracelet + 3 galets disposés autour. Ambiance technique mais pas froide.

---

# Le matériel étudié

**3 sessions d'exploration matérielle documentées, 14 plateformes évaluées.**

| Famille étudiée | Verdict |
|---|---|
| Montres connectées programmables (Bangle.js 2, PineTime, Kospet Magic 3 / Rock) | ⚠️ Sourcing en train de se réduire en 2026 — seule Bangle.js 2 conservée comme cible v3 commerciale |
| Montres "BT Call" récentes (Kospet Tank, Oukitel BT, gamme C26) | ❌ Chipset Realtek / Actions verrouillé, firmware fermé |
| Trackers fitness grand public (Fitbit, Garmin, Mi Band) | ❌ BLE scan arbitraire bloqué par les plateformes |
| Trackers anti-perte BLE (iTags, Chipolo, Tile) | ⚠️ Utiles uniquement pour proto UX, chipset variable |
| **Modules de développement nRF52840** (Seeed XIAO, Adafruit Feather) | ✅ Voie retenue |
| Modules ESP32 (T-Watch, M5StickC) | ❌ Autonomie incompatible avec l'objectif 7 jours |

*Notes orateur : message clé — on n'a pas choisi à l'aveugle, on a éliminé pour bonnes raisons. Les équipes peuvent demander un point sur un modèle en particulier, le doc d'exploration le couvre.*

> **Visuel suggéré :** matrice ou liste avec coches vert / rouge, pas de longue lecture.

---

# La solution retenue : Seeed XIAO nRF52840

Un seul module pour tout le système — bracelet **et** galets / balises.

**Pourquoi ce choix :**

- **Certification CE** acquise au niveau module (Seeed Studio) → simplifie le dossier produit final
- **Autonomie record** : 5 µA en veille, soit **30 × moins que l'ESP32** du bracelet actuel
- **BLE 5.0 natif**, supporte simultanément Central (scan, côté bracelet) et Peripheral (advertising, côté galet)
- **Un seul SDK** à maîtriser, une seule chaîne de développement, un seul fournisseur
- **Coût matière unitaire ~7,50 €** — itérations économiquement possibles
- Format ultra-compact 21 × 17 mm — tient dans un boîtier de galet ou de bracelet sans contrainte

**Conséquence stratégique :** Ti Poucet et N.I.C.O **partagent la même puce, le même firmware de base, le même fournisseur, la même certification CE.** Tout effort technique sert les deux programmes.

*Notes orateur : c'est LE point de bascule du projet. Avant, deux pistes hardware divergentes. Maintenant, une mutualisation matérielle, logicielle et industrielle.*

> **Visuel suggéré :** photo macro du XIAO nRF52840 + petite légende des 4 atouts.

---

# Architecture système — la même pour les deux projets

```
[ Galet 1 ]      [ Galet 2 ]      [ Galet 3 ]
   XIAO             XIAO             XIAO
  Buzzer +         Buzzer +         Buzzer +
 Interrupteur    Interrupteur     Interrupteur
 LiPo 200 mAh    LiPo 200 mAh    LiPo 200 mAh
 USB-C, ~12 mois  USB-C, ~12 mois  USB-C, ~12 mois
       \              |              /
        \             |             /
         \            |            /
              [  Bracelet maître  ]
                    XIAO
              Vibreur + buzzer
              2 boutons physiques
              LiPo 200 mAh, USB-C
              Autonomie 7-10 jours
              Boîtier 3D + bracelet silicone 20 mm
```

**Différence Ti Poucet ↔ N.I.C.O = uniquement du firmware** : la même carte se comporte différemment selon le mode actif. Pas de variante hardware à fabriquer, pas de SAV distinct, pas de double chaîne d'approvisionnement.

*Notes orateur : insister sur le fait que c'est la même carte électronique qui sert les deux usages. C'est inhabituel et précieux.*

> **Visuel suggéré :** reproduire ce schéma proprement, avec icônes claires.

---

# L'approche Ti Poucet — flux utilisateur

**Autonomie réelle attendue :**
- **Galets** : ~12-18 mois sur batterie LiPo 200 mAh rechargeable (recharge USB-C en ~1h, une fois par an environ)
- **Bracelet** : 7-10 jours sur LiPo 200 mAh, recharge USB-C ponctuelle
- **Unification** : un seul type de batterie pour tout le système, un seul câble USB-C pour tout recharger

**Le flux de retour, version v2 :**

1. L'utilisateur a posé 3 galets sur son trajet aller (boulangerie → école → maison, par exemple)
2. Au retour : **une pression sur le bracelet** → le galet le plus proche se met à sonner
3. L'utilisateur le suit au son, le ramasse
4. Il appuie sur l'**interrupteur du galet** → le galet se tait, **et le suivant le plus proche commence à sonner automatiquement**
5. Et ainsi de suite jusqu'au dernier galet

**Ce qui change pour le porteur :** un seul geste sur le bracelet déclenche tout le retour. Plus de télécommande, plus de manipulation séquentielle. Le système orchestre la collecte.

*Notes orateur : ce flux exploite directement le RSSI (estimation de distance BLE) — c'est le bracelet qui décide quel galet sonne, en fonction de qui est le plus près.*

> **Visuel suggéré :** schéma de parcours avec 3 galets posés, pictos appui bracelet → onde sonore galet → interrupteur galet → onde sonore galet suivant.

---

# Ti Poucet — ce qui change par rapport au prototype initial

| Dimension | Prototype initial (sonnerie 12 V) | Solution v2 (BLE nRF52840) |
|---|---|---|
| Consommation | 12 V, batteries lourdes, autonomie limitée | Pile CR2032, **12 mois** sans intervention |
| Détection de proximité | Aucune (sonnerie déclenchée à la demande) | **RSSI BLE** : le bracelet sait quel galet est le plus proche |
| Ordre de sonnerie | À mémoriser ou pré-programmer | **Auto-séquencement par proximité** |
| Geste d'utilisation | Une action par galet, télécommande dédiée | **Un seul appui bracelet** + interrupteur sur le galet collecté |
| Encombrement galet | Boîtier rigide avec sonnette + alimentation 12 V | Boîtier **30 × 25 × 10 mm** environ, format galet de poche |
| Discrétion sociale | Sonnerie déclenchable seulement | **Silence en veille**, ne sonne que sur demande |

**L'apport central :** le système **comprend la situation spatiale** au lieu d'exécuter aveuglément une commande. C'est ce qui rend le retour autonome **sans préparation préalable** par l'éducateur ou l'accompagnant.

*Notes orateur : valoriser le travail du prototype initial — il a permis de valider le besoin et l'ergonomie cognitive. La v2 lève les contraintes hardware qui limitaient son déploiement.*

---

# L'approche N.I.C.O — faiblesses du bracelet actuel

**Le bracelet N.I.C.O actuel = LilyGo T-Watch 2020 v3 (ESP32).**

Limites identifiées en usage réel :

- **Autonomie ESP32 catastrophique** : ~6 h en scan continu, **3 jours maximum** en deep sleep agressif. Recharge quotidienne en pratique → friction d'usage importante en milieu scolaire.
- **Boîtier de montre tactile** : écran inutile pour un utilisateur malvoyant, touchscreen difficile à manipuler. On paie de l'autonomie et du coût pour des fonctions non utilisées.
- **BSP figé sur Arduino ESP32 v2.0.17** : pas d'évolution possible vers les nouvelles versions du SDK. Dette technique croissante.
- **Une seule balise gérée** (filtre "Holy" dans le code). Pas de différenciation des objets.
- **Pas de buzzer audible** sur le bracelet, pas de capacité à faire sonner la balise pour localiser un objet.
- **Plateforme matérielle discontinuée** chez LilyGo — pas de pérennité sourcing.

*Notes orateur : ne pas dénigrer le travail initial — il a démontré la viabilité du concept et a permis de valider l'UX d'alerte par vibration. La v2 résout les blocages structurels qu'on connaît maintenant.*

> **Visuel suggéré :** côte à côte — photo T-Watch actuelle / rendu bracelet v2 minimaliste.

---

# N.I.C.O v2 — nouvelles capacités sur le même bracelet

**Avec le même bracelet, on récupère 3 nouvelles capacités :**

**1. Faire sonner la balise pour retrouver un objet perdu.**
L'utilisateur appuie longuement sur le bracelet → la balise la plus proche (celle dont l'objet est le plus probablement à proximité) sonne. L'utilisateur s'oriente au son, retrouve l'objet, appuie sur le bouton de la balise pour la silencier. **Inspiration directe du flux Ti Poucet.**

**2. Multi-objets : jusqu'à 3 balises simultanées sur un même bracelet.**
1 sur la canne blanche, 1 sur le trousseau de clés, 1 sur le sac. Le bracelet vibre **N fois selon l'objet concerné** (1 vibration = canne, 2 vibrations = clés, 3 vibrations = sac). Un seul modèle mental à apprendre, valable à l'appairage et à l'alerte.

**3. Autonomie 7-10 jours** (vs 1 jour avec la T-Watch) → utilisable une semaine scolaire complète sans recharge. Plus de bracelet déchargé un mardi à 11h.

**Le comportement de surveillance actuel est conservé :** le bracelet vibre **et sonne** quand une balise sort du seuil de proximité, exactement comme aujourd'hui. On n'enlève rien, on ajoute.

*Notes orateur : c'est le slide où les éducateurs vont se projeter sur l'usage scolaire. Insister sur le fait que multi-balises = multi-objets, pertinent pour l'autonomie ULIS.*

> **Visuel suggéré :** 3 pictos clairs (canne / clés / sac) avec respectivement 1 / 2 / 3 vibrations stylisées.

---

# Décision à prendre — quelle plateforme pour le proto bracelet ?

Le choix d'une **montre sur étagère** plutôt que d'attendre le PCB custom permet d'**accélérer la production de proto et de premières démos terrain**. Mais il introduit deux types de dépendances qu'il faut accepter consciemment : **dépendance au fabricant** (sourcing, pérennité) et **dépendance à l'écosystème logiciel**. Deux options réelles, toutes deux **open source et hackables**.

**Option A — PineTime (Pine64) — ~27 €**
- Hardware **et** firmware **open source intégraux** (schémas, PCB, bootloader, InfiniTime/WaspOS publics)
- nRF52832 — proche de la cible XIAO mais **pas exactement le même chip** (BLE 4.2 vs 5.0, 64 KB vs 256 KB RAM)
- **Autonomie en usage N.I.C.O** : estimation 3-5 jours (batterie 180 mAh, LCD IPS)
- Code métier portable à ~90 % vers le XIAO final
- **Sourcing volatile** : restocks par lots, ruptures cycliques. **PineTime Pro annoncée mars 2026** mais **prix non communiqué** — original probablement retiré du marché une fois les stocks épuisés. Risque d'effet falaise dans 12-18 mois.

**Option B — Bangle.js 2 (Espruino) — ~110 €**
- **Strictement le même chip** que le produit final (XIAO nRF52840) → code portable à ~99 %
- **Autonomie en usage N.I.C.O** : estimation 5-7 jours (batterie 175 mAh, écran transflectif MIP)
- Sourcing pérenne et stable (Espruino, Adafruit) depuis 2021
- Boîtier industriel certifié CE/FCC immédiatement utilisable pour démos institutionnelles
- **Prix dissuasif** : 4 × le coût de la PineTime → la seule motivation rationnelle est **la fidélité exacte au chip cible (nRF52840)** pour ne pas réécrire la couche BLE plus tard

**Enjeux stratégiques au-delà du seul prix d'achat :**

1. **Dépendance fabricant.** Bangle.js 2 = un acteur unique (Espruino, petite structure UK). PineTime = Pine64, plus gros, mais qui rationalise son catalogue à chaque génération. Dans les deux cas, le matériel cible final reste le **XIAO nRF52840 (Seeed Studio, grand acteur)** — la dépendance sur la montre proto est **bornée à la phase de développement**, pas reportée sur le produit fini.

2. **Effet falaise PineTime.** Si on bâtit notre validation sur la PineTime et qu'elle disparaît du marché dans 12 mois (Pine64 historiquement retire les originaux quand le successeur sort), les démos futures n'ont plus de plateforme physique tant que le PCB custom n'est pas prêt. À mitiger en commandant 3-4 unités d'un coup.

3. **Communautés open source — opportunité d'association.** Espruino et Pine64 ne sont pas de simples fournisseurs commerciaux — ce sont **des communautés open source vivantes**, autour desquelles gravitent des contributeurs alignés avec les missions accessibilité / éducation (cf. travaux Pine64 sur PinePhone et outils d'assistance). **S'associer à l'une d'elles peut apporter :**
   - Visibilité du projet N.I.C.O / Ti Poucet auprès d'une audience technique solidaire
   - Contributions logicielles entrantes (apps Bangle, watchfaces InfiniTime spécifiques malvoyants)
   - Co-développement possible sur des cas d'usage (Pine64 a déjà publié des dispositifs adaptés)
   - Effet de levier sur la communication produit ("Build on top of open hardware")

**3 stratégies envisageables :**

| Stratégie | Coût | Force | Faiblesse |
|---|---|---|---|
| PineTime seule (2-3 unités) | ~55-80 € | Low-cost, multi-démos, open source intégral, alignement écosystème InfiniTime | ~10 % de refacto BLE au portage, sourcing volatile, effet falaise |
| Bangle.js 2 seule | ~110 € | Code 99 % portable, démo institutionnelle propre, sourcing stable | 1 seule unité, terrain limité, prix élevé |
| **Combinaison PineTime + Bangle.js 2** | ~137 € | Couverture maximale, alignement avec **deux** communautés open source | +27 € vs Bangle.js seule |

**Question ouverte aux équipes :**
1. Quelle stratégie privilégier au regard du budget et des besoins de démo terrain ?
2. **Souhaitons-nous nous positionner publiquement en partenaire d'une de ces communautés open source** (Espruino, Pine64, ou les deux) — et si oui, sous quelle forme : contributions logicielles, présentation du projet, mention dans les communications ?

*Notes orateur : les deux questions sont liées. Si on choisit la combinaison (stratégie 3), on peut visibilité sur les deux écosystèmes. Si on choisit une option seule, autant aller jusqu'au bout du positionnement partenaire. C'est aussi un sujet à porter avec la CNRHR — la dimension open source / accessibilité peut résonner avec leur mission.*

> **Visuel suggéré :** deux photos de montre côte à côte (PineTime ronde colorée / Bangle.js 2 rectangulaire transflective) + tableau des 3 stratégies + un encart "Communautés open source" en bas droite avec logos Espruino + Pine64.

---

# Le bracelet — impression 3D + coûts matériels

**Le boîtier bracelet est le seul élément de design industriel à concevoir.** Stratégie en deux temps :

| Étape | Boîtier | Délai | Coût unitaire |
|---|---|---|---|
| **Proto + démos** | Imprimé 3D (PLA / PETG) | Quelques jours/itération | ~1-2 € matière |
| **Produit fini** | Injecté plastique | 8-12 semaines + outillage | ~1 € unitaire, 5-15 k€ tooling |

L'impression 3D débloque les tests terrain CNRHR immédiatement, sans attendre l'investissement industriel.

**Coûts matière unitaire — produit fini (assemblage manuel + boîtier 3D PLA) :**

| | Bracelet | Galet / Balise |
|---|---|---|
| Module XIAO nRF52840 | 7,50 € | 7,50 € |
| Vibreur / Buzzer | 0,40 € | 0,30 € |
| Boutons, transistors, résistances | 0,40 € | 0,30 € |
| **LiPo 200 mAh + JST 1.25 mm** | **3,50 €** | **3,50 €** |
| Bracelet silicone 20 mm | 1,50 € | — |
| Boîtier 3D + fils + soudure | 2,20 € | 1,40 € |
| **Total matière unitaire** | **~15-16 €** | **~13-14 €** |

**Système complet (1 bracelet + 3 galets) : ~55-58 € de matière.**

**Coût phase prototype :** ~175 € (panier AliExpress complet, incluant 5 LiPo) + 27 à 137 € (montre proto selon stratégie retenue slide précédent) = **~200 à 310 €** au total pour démarrer.

**Projection v3 (PCB injecté, série >100 u/an) :** bracelet ~10-12 €, galet ~10-12 €, système complet ~**40-48 €** — un quart d'économie sur le coût matière, avec un parc batterie unifié et rechargeable.

**Conséquence de l'unification LiPo :** plus de pile bouton consommable à remplacer chaque année (économie ~3 €/an/balise = ~9 €/an/système), gain environnemental, et compatibilité avec les évolutions v2.1 type synthèse vocale (qui étaient incompatibles avec la CR2032).

*Notes orateur : ces chiffres sont des coûts matière, hors temps de montage et hors marge. Utile pour cadrer une discussion MDPH ou tarification accessoire pédagogique. C'est ici qu'on peut aussi demander aux équipes les critères ergonomiques prioritaires pour le boîtier (taille poignet enfant ULIS, emplacement boutons, fixation discrète).*

> **Visuel suggéré :** rendu 3D du boîtier bracelet à gauche, tableau coûts à droite, encart "système complet 45-50 €" en bas.
