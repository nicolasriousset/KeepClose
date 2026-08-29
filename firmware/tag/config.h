#pragma once
#include "../common/config.h"

// ── Intervalle d'advertising (ms) ────────────────────────────────────────────
// C'est ce paquet que le bracelet mesure pour estimer la distance : l'intervalle
// fixe donc le débit de mesures, et par là la latence d'alerte.
//
// Le bracelet scanne 50 ms toutes les 100 ms, il en capte donc environ la
// moitié. Latence d'alerte mesurée en simulation selon l'intervalle :
//
//   100 ms → 5,0 paquets/s → 3,1 s      300 ms → 1,7 paquet/s  → 6,0 s
//   150 ms → 3,3 paquets/s → 3,9 s      500 ms → 1,0 paquet/s  → 8,8 s
//   200 ms → 2,5 paquets/s → 4,6 s     1000 ms → 0,5 paquet/s  → 15,8 s
//
// La valeur 500 ms précédemment déclarée ici (jamais appliquée) visait
// l'autonomie, mais l'advertising ne pèse que ~50 µA à 500 ms contre ~167 µA à
// 150 ms : 117 µA d'économie pour 5 s de latence supplémentaire sur une alerte
// de sécurité. Et surtout, le scan permanent de la balise coûte des mA — voir
// README § Consommation. Tant que ce scan n'est pas réduit, ralentir
// l'advertising dégrade l'alerte sans gain d'autonomie mesurable.
#define ADV_INTERVAL_MS         150

// Délai sans détection du bracelet avant de considérer qu'il est hors de portée (ms)
#define SCANNER_TIMEOUT_MS      10000

// Délai au-delà duquel le dernier état reçu du bracelet est considéré périmé (ms)
// Le bracelet advertise toutes les 300 ms (scanner.ino), donc 6 s laisse une
// marge très large — c'est le rapport cyclique du scan de la balise qui domine
// le délai de réception, pas l'intervalle d'advertising du bracelet.
#define SCANNER_STALE_MS        6000

// Brochage XIAO BLE nRF52840
#define PIN_BUZZER              D1      // Buzzer piézo (sonnerie sur commande ring)
#define PIN_BUTTON              D2      // Interrupteur push (actif bas, pull-up interne)
#define PIN_LED_STATUS          LED_BUILTIN

// ── Type de buzzer ───────────────────────────────────────────────────────────
// Deux composants voisins d'aspect, deux pilotages incompatibles :
//
//   ACTIF  — oscillateur intégré. Une tension continue suffit : digitalWrite.
//   PASSIF — simple transducteur. Il faut lui fournir le signal : PWM.
//
// Se tromper ne donne pas « un son faible » mais un symptôme franc :
//   passif piloté en continu   → « tic » à chaque front, rien entre les deux
//   actif piloté en PWM        → sonne quand même, sa propre note, plus faible
//
// Commenter cette ligne pour un buzzer actif. En cas de doute sur le composant,
// voir README § « Buzzer » : tension continue puis balayage en fréquence, les
// deux types se séparent en 10 s.
#define BUZZER_PASSIVE

// Fréquence de pilotage d'un buzzer passif (Hz), ignorée si BUZZER_PASSIVE
// n'est pas défini.
//
// ⚠️ 2700 Hz est un COMPROMIS retenu à l'oreille le 2026-08-29, pas le maximum de
//    la courbe de réponse. Ne pas le « corriger » vers le pic mesuré sans
//    réécouter : au pic, le son gagne quelques dB mais devient agressif,
//    et une balise d'assistance sonne parfois longtemps, à proximité immédiate de
//    l'oreille de son porteur. Quelques dB ne valent pas ça.
//
// Le choix reste borné par la physique : hors résonance un piézo perd vite 15 à
// 20 dB, et l'oreille est au maximum de sa sensibilité entre 2 et 4 kHz. Ne pas
// descendre sous ~2 kHz « pour que ce soit moins strident » — on y perd sur les
// deux tableaux à la fois.
//
// Si le composant change, refaire un balayage 1 → 5 kHz en carré pour situer la
// nouvelle résonance, puis rechoisir en dessous à l'oreille.
#define BUZZER_FREQ_HZ          2700

// ── Pilotage push-pull du buzzer passif ──────────────────────────────────────
// Le piézo n'est plus câblé entre une broche et GND, mais entre DEUX broches
// pilotées en opposition de phase. Quand l'une est à 3,3 V l'autre est à 0, et
// inversement : la tension vue par l'élément passe de 3,3 Vpp à 6,6 Vpp, soit
// +6 dB, sans un seul composant ajouté. Le SPL d'un piézo suit la tension à ses
// bornes, et 3,3 V c'est peu — c'est le levier le plus rentable ici.
//
// ⚠️ Demande une modification de câblage : Buzzer(-) quitte GND pour PIN_BUZZER_N.
//    Si le fil reste sur GND, la broche N pilote dans le vide : le buzzer sonne
//    quand même, au volume d'avant. L'inverse est vrai aussi — buzzer câblé
//    entre les deux broches mais BUZZER_PUSH_PULL commenté = broche N flottante,
//    donc silence.
//
// Sans effet si BUZZER_PASSIVE n'est pas défini : un buzzer actif est polarisé
// et n'accepte pas d'être piloté ainsi.
#define BUZZER_PUSH_PULL
#define PIN_BUZZER_N            D0      // seconde borne du buzzer (phase opposée)

// Anti-rebond bouton (ms)
#define BUTTON_DEBOUNCE_MS      50

// Appui long sur le bouton de la balise : oublier le bracelet appairé (ms).
// Un bip de repère est émis au franchissement du palier.
#define BUTTON_RESET_MS         10000
