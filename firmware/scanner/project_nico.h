#pragma once
#include "scanner_core.h"

/*
 * N.I.C.O. — Comportement spécifique
 *
 * Alerte principale : le bracelet vibre quand la balise (canne) sort du rayon.
 * Commun            : peut sonner la balise la plus proche pour la localiser.
 */

#define VIBRATION_INTERVAL_MS   1000

static unsigned long nicoLastVibrationTime = 0;

void nicoSetup() {
  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);
  Serial.println("Mode : N.I.C.O.");
}

void nicoLoop(BeaconRegistry& registry) {
  BeaconInfo* cane = registry.nearestActive();
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
