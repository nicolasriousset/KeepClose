#pragma once
#include "scanner_core.h"

/*
 * N.I.C.O. — Comportement spécifique
 *
 * Alerte principale : le bracelet vibre quand la balise (canne) sort du rayon.
 * Commun            : peut sonner la balise la plus proche pour la localiser.
 */

#define VIBRATION_INTERVAL_MS   1000
#define NICO_ALERT_DISTANCE_M   0.20f   // LED allumée au-delà de 20 cm
#define PIN_LED_ALERT           D1      // LED de proximité (active HIGH)

static unsigned long nicoLastVibrationTime = 0;

void nicoSetup() {
  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);
  pinMode(PIN_LED_ALERT, OUTPUT);
  digitalWrite(PIN_LED_ALERT, LOW);
  Serial.println("Mode : N.I.C.O.");
}

void nicoLoop(BeaconRegistry& registry) {
  BeaconInfo* cane = registry.nearestActive();

  // LED allumée si balise absente ou à plus de 20 cm
  bool tooFar = !cane || cane->distance > NICO_ALERT_DISTANCE_M;
  digitalWrite(PIN_LED_ALERT, tooFar ? HIGH : LOW);

  if (!cane) return;

  bool outOfRange = cane->distance > DISTANCE_WARNING;
  unsigned long now = millis();

  if (outOfRange && (now - nicoLastVibrationTime >= VIBRATION_INTERVAL_MS)) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(200);
    digitalWrite(PIN_VIBRATION, LOW);
    nicoLastVibrationTime = now;
  }

  // TODO: bouton pour déclencher ringNearestBeacon() et localiser la canne
}
