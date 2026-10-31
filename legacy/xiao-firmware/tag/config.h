#pragma once
#include "../common/config.h"

// Puissance d'émission à 1 mètre (à calibrer empiriquement, dBm)
#define TX_POWER_AT_1M          -61

// Intervalle d'advertising (ms)
#define ADV_INTERVAL_MS         500

// Délai sans détection du bracelet avant de considérer qu'il est hors de portée (ms)
#define SCANNER_TIMEOUT_MS      10000

// Délai au-delà duquel la dernière mesure de distance est considérée périmée (ms)
// Le bracelet advertise toutes les ~2 s, donc 6 s = 3 intervalles manqués.
#define SCANNER_STALE_MS        6000

// Puissance d'émission du bracelet à 1 mètre, telle que mesurée par la balise (dBm).
// À calibrer indépendamment de TX_POWER_AT_1M (direction inverse, antenne différente).
#define SCANNER_TX_POWER_AT_1M  -61

// Brochage XIAO BLE nRF52840
#define PIN_BUZZER              D1      // Buzzer piézo (sonnerie sur commande ring)
#define PIN_BUTTON              D2      // Interrupteur push (actif bas, pull-up interne)
#define PIN_LED_STATUS          LED_BUILTIN

// Anti-rebond bouton (ms)
#define BUTTON_DEBOUNCE_MS      50
