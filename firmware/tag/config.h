#pragma once
#include "../common/config.h"

// Puissance d'émission à 1 mètre (à calibrer empiriquement, dBm)
#define TX_POWER_AT_1M          -59

// Intervalle d'advertising (ms)
#define ADV_INTERVAL_MS         500

// Brochage XIAO BLE nRF52840
#define PIN_BUZZER              D1      // Buzzer piézo (sonnerie sur commande ring)
#define PIN_LED_STATUS          LED_BUILTIN
