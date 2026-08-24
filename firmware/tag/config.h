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

// Anti-rebond bouton (ms)
#define BUTTON_DEBOUNCE_MS      50
