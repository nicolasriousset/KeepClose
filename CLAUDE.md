# KeepClose — consignes pour Claude

## Toute évolution passe par OpenSpec

Ce dépôt utilise [OpenSpec](https://github.com/Fission-AI/OpenSpec) (`openspec/`) comme passage obligé
pour **toute évolution** : nouvelle fonctionnalité, changement de comportement firmware, correction de bug
non triviale, modification matérielle/schéma, refactor structurant.

Règles :

1. **Ne jamais modifier le code ou la doc technique sans change OpenSpec actif.** Si l'utilisateur demande
   une évolution directement, créer d'abord la proposition (`/opsx:propose` ou skill `openspec-propose`),
   la faire valider, puis implémenter.
2. Implémenter via `/opsx:apply` en cochant les tâches de `openspec/changes/<change>/tasks.md`.
3. Une fois terminé et validé, archiver via `/opsx:archive` pour fusionner les deltas dans `openspec/specs/`.
4. Pour réfléchir avant de proposer : `/opsx:explore`.
5. Exceptions (sans change) : coquilles, formatage, commentaires, mise à jour de dépendances sans effet
   fonctionnel — à signaler explicitement à l'utilisateur.

Les artefacts OpenSpec sont rédigés en français (mots-clés SHALL/MUST et titres structurels en anglais).
