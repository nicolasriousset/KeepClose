#pragma once
#include "../common/config.h"

// ── Sélection du projet ──────────────────────────────────────────────────────
// Décommenter UNE seule ligne :
#define PROJECT_NICO
// #define PROJECT_TIPOUCET
// ─────────────────────────────────────────────────────────────────────────────

// RSSI_AT_1M / PATH_LOSS_N / ALERT_RSSI_* : voir common/config.h
// (le modèle de propagation est commun, pas un réglage propre au bracelet)

// Scan BLE
#define SCAN_INTERVAL_MS        2000

// Nombre maximum de balises trackées simultanément
#define MAX_BEACONS             10

// Délai sans détection avant de considérer une balise hors portée (ms)
#define BEACON_TIMEOUT_MS       6000

// ── Appairage ────────────────────────────────────────────────────────────────
// PAIRING_RSSI_THRESHOLD est défini dans common/config.h (-45 dBm)

// Durée de maintien au contact requise pour valider l'appairage (ms)
#define PAIRING_DURATION_MS     3000
// ─────────────────────────────────────────────────────────────────────────────

// Brochage XIAO BLE nRF52840
#define PIN_VIBRATION           D0      // Moteur vibrant
#define PIN_BUTTON              D1      // Interrupteur push (actif bas, pull-up interne)
#define PIN_LED_STATUS          LED_BUILTIN

// Durée de sonnerie balise déclenchée par le bouton (ms)
#define RING_DURATION_MS        10000
// Anti-rebond bouton (ms)
#define BUTTON_DEBOUNCE_MS      50

// ── Appuis longs sur le bouton du bracelet ───────────────────────────────────
// Une impulsion de repère est émise à chaque palier atteint : l'utilisateur
// non-voyant doit savoir quand relâcher sans avoir à compter les secondes.
#define BUTTON_FORGET_MS        3000   // oublier les balises non détectées
#define BUTTON_RESET_MS         10000  // effacer tous les appairages
