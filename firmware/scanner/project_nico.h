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
static bool          nicoRingActive        = false;
static uint16_t      nicoRingHash          = 0;
static unsigned long nicoRingUntilMs       = 0;
static bool          nicoLastBtnState      = HIGH;
static unsigned long nicoLastBtnChangeMs   = 0;

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

// Retourne le tagCrc de la balise appairée hors-rayon la plus proche.
// 0 si toutes les balises sont à portée ou qu'aucune n'est appairée.
// "Plus proche" parmi les hors-rayon → l'utilisateur la récupère en premier.
uint16_t nicoComputeAlert(BeaconRegistry& reg) {
  BeaconInfo* alertBeacon = nullptr;
  float alertDist = 1e9f;
  for (int i = 0; i < reg.count; i++) {
    BeaconInfo& b = reg.beacons[i];
    if (!b.paired || b.tagCrc == 0) continue;
    bool oor = !b.isAlive() || b.rssiEma == 0.0f || b.distance > PROXIMITY_FAR_DISTANCE_M;
    if (!oor) continue;
    float d = (b.isAlive() && b.distance >= 0.0f) ? b.distance : 1e9f;
    if (!alertBeacon || d < alertDist) { alertDist = d; alertBeacon = &b; }
  }
  return alertBeacon ? alertBeacon->tagCrc : 0;
}

void nicoSetup() {
  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Serial.println("Mode : N.I.C.O.");
}

// Retourne le tagCrc à sonner (0 si aucune sonnerie active)
uint16_t nicoComputeRing(BeaconRegistry& reg) {
  (void)reg;
  return nicoRingActive ? nicoRingHash : 0;
}

void nicoLoop(BeaconRegistry& registry) {
  unsigned long now = millis();

  // ── Dé-snooze automatique : balise revenue dans le périmètre ────────────────
  for (int i = 0; i < registry.count; i++) {
    BeaconInfo& b = registry.beacons[i];
    if (!b.paired || !b.snoozed) continue;
    if (b.isAlive() && b.rssiEma != 0.0f && b.distance <= PROXIMITY_NEAR_DISTANCE_M) {
      b.snoozed = false;
      Serial.print("[btn] snooze leve : "); Serial.println(b.address);
    }
  }

  // ── Détection balises hors périmètre (hors-snooze) ──────────────────────────
  bool shouldVibrate = false;
  for (int i = 0; i < registry.count; i++) {
    BeaconInfo& b = registry.beacons[i];
    if (!b.paired || b.snoozed) continue;
    bool oor = !b.isAlive() || b.rssiEma == 0.0f || b.distance > PROXIMITY_FAR_DISTANCE_M;
    if (oor) { shouldVibrate = true; break; }
  }

  // ── Lecture bouton avec anti-rebond ─────────────────────────────────────────
  bool btnRaw = digitalRead(PIN_BUTTON);
  bool btnPressed = false;
  if (btnRaw != nicoLastBtnState && now - nicoLastBtnChangeMs > BUTTON_DEBOUNCE_MS) {
    nicoLastBtnState  = btnRaw;
    nicoLastBtnChangeMs = now;
    if (btnRaw == LOW) btnPressed = true;  // front descendant = appui
  }

  if (btnPressed) {
    if (shouldVibrate) {
      // ── Snooze : suspendre l'alerte jusqu'au prochain retour en périmètre ──
      for (int i = 0; i < registry.count; i++) {
        BeaconInfo& b = registry.beacons[i];
        if (!b.paired) continue;
        bool oor = !b.isAlive() || b.rssiEma == 0.0f || b.distance > PROXIMITY_FAR_DISTANCE_M;
        if (oor) { b.snoozed = true; Serial.print("[btn] snooze : "); Serial.println(b.address); }
      }
      shouldVibrate = false;
    } else {
      // ── Ring : sonner la balise appairée la plus proche ──────────────────
      BeaconInfo* nearest = nullptr;
      for (int i = 0; i < registry.count; i++) {
        BeaconInfo& b = registry.beacons[i];
        if (!b.paired || b.tagCrc == 0) continue;
        if (!nearest || (b.isAlive() && (!nearest->isAlive() || b.distance < nearest->distance)))
          nearest = &b;
      }
      if (nearest) {
        nicoRingHash    = nearest->tagCrc;
        nicoRingUntilMs = now + RING_DURATION_MS;
        nicoRingActive  = true;
        Serial.print("[btn] ring → 0x"); Serial.println(nicoRingHash, HEX);
      }
    }
  }

  // ── Auto-stop ring après RING_DURATION_MS ───────────────────────────────────
  if (nicoRingActive && now >= nicoRingUntilMs) {
    nicoRingActive = false;
    nicoRingHash   = 0;
    Serial.println("[btn] ring auto-stop");
  }

  // ── Vibration ────────────────────────────────────────────────────────────────
  if (shouldVibrate && (now - nicoLastVibrationTime >= VIBRATION_INTERVAL_MS)) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(200);
    digitalWrite(PIN_VIBRATION, LOW);
    nicoLastVibrationTime = now;
  }
}
