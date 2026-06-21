#pragma once
#include "../common/config.h"

// ── Sélection du projet ──────────────────────────────────────────────────────
// Décommenter UNE seule ligne :
#define PROJECT_NICO
// #define PROJECT_TIPOUCET
// ─────────────────────────────────────────────────────────────────────────────

// Puissance TX du tag à 1 mètre (doit correspondre à la calibration du tag)
#define TX_POWER_AT_1M          -59

// Scan BLE
#define SCAN_INTERVAL_MS        2000

// Nombre maximum de balises trackées simultanément
#define MAX_BEACONS             10

// Délai sans détection avant de considérer une balise hors portée (ms)
#define BEACON_TIMEOUT_MS       6000

// ── Appairage ────────────────────────────────────────────────────────────────
// Seuil RSSI pour détecter une balise "collée" contre le bracelet (~5-10 cm)
#define PAIRING_RSSI_THRESHOLD  -45

// Durée de maintien au contact requise pour valider l'appairage (ms)
#define PAIRING_DURATION_MS     3000
// ─────────────────────────────────────────────────────────────────────────────

// Brochage XIAO BLE nRF52840
#define PIN_VIBRATION           D0      // Moteur vibrant
#define PIN_LED_STATUS          LED_BUILTIN
