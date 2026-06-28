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

// Active LOW : LOW = allumée, HIGH = éteinte
static inline void setRgbLed(bool r, bool g, bool b) {
  digitalWrite(LED_RED,   r ? LOW : HIGH);
  digitalWrite(LED_GREEN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE,  b ? LOW : HIGH);
}

// Bleu  : actif, aucune balise appairée
// Vert  : balise(s) appairée(s) à portée
// Rouge : au moins une balise appairée hors portée
void nicoLedUpdate(BeaconRegistry& reg, bool pairingActive) {
  if (pairingActive) { setRgbLed(false, true, false); return; }
  static bool prevOutOfRange = true;
  bool hasPaired = false, hasOutOfRange = false;
  for (int i = 0; i < reg.count; i++) {
    BeaconInfo& b = reg.beacons[i];
    if (!b.paired) continue;
    hasPaired = true;
    // Hystérésis : seuil de retour au vert plus bas que seuil de passage au rouge
    float thr = prevOutOfRange ? PROXIMITY_NEAR_DISTANCE_M : PROXIMITY_FAR_DISTANCE_M;
    if (!b.isAlive() || b.rssiEma == 0.0f || b.distance > thr)
      hasOutOfRange = true;
  }
  prevOutOfRange = hasPaired ? hasOutOfRange : true;
  static uint8_t lastState = 255;
  uint8_t state = hasOutOfRange ? 2 : hasPaired ? 1 : 0;
  if (state != lastState) {
    lastState = state;
    const char* labels[] = { "BLEU (aucune balise appairee)", "VERT (a portee)", "ROUGE (hors de portee)" };
    Serial.print("[led scanner] "); Serial.println(labels[state]);
  }
  if (hasOutOfRange)  setRgbLed(true,  false, false);
  else if (hasPaired) setRgbLed(false, true,  false);
  else                setRgbLed(false, false, true);
}

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
