#pragma once
#include "scanner_core.h"

/*
 * N.I.C.O. — Comportement spécifique
 *
 * Alerte principale : le bracelet vibre quand la balise (canne) sort du rayon.
 * Commun            : peut sonner la balise la plus proche pour la localiser.
 *
 * Bouton (D1), selon la durée de l'appui :
 *   court   → annuler le ring / snoozer l'alerte / sonner la balise la plus proche
 *   ≥ 3 s   → oublier les balises que le bracelet ne détecte plus (balise perdue)
 *   ≥ 10 s  → effacer tous les appairages
 */

#define VIBRATION_INTERVAL_MS   1000

static unsigned long nicoLastVibrationTime = 0;
static bool          nicoRingActive        = false;
static uint16_t      nicoRingHash          = 0;
static unsigned long nicoRingUntilMs       = 0;
static bool          nicoBtnState          = HIGH;  // état debouncé
static unsigned long nicoBtnChangeMs       = 0;
static unsigned long nicoBtnPressMs        = 0;     // début de l'appui en cours
static uint8_t       nicoBtnTicks          = 0;     // paliers déjà signalés

// Action déduite de la durée de l'appui, jouée au relâchement
enum NicoBtnAction { BTN_NONE, BTN_SHORT, BTN_FORGET, BTN_RESET };

// Motifs haptiques. Bloquants : c'est le seul moyen de rendre un motif
// reconnaissable sans machine à états dédiée, et ces événements sont rares.
static void nicoVibrate(uint8_t count, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < count; i++) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(onMs);
    digitalWrite(PIN_VIBRATION, LOW);
    if (i + 1 < count) delay(offMs);
  }
}

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
  bool hasPaired = false, hasOutOfRange = false;
  for (int i = 0; i < reg.count; i++) {
    BeaconInfo& b = reg.beacons[i];
    if (!b.paired) continue;
    hasPaired = true;
    // L'hystérésis est portée par chaque balise (cf. beaconOutOfRange), pas par
    // la LED : un état global fausserait le seuil dès la deuxième balise.
    if (beaconOutOfRange(b)) hasOutOfRange = true;
  }
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
    if (!beaconOutOfRange(b)) continue;
    float d = (b.isAlive() && b.hasRssi) ? b.distance : 1e9f;
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
    if (!beaconOutOfRange(b)) {
      b.snoozed = false;
      Serial.print("[btn] snooze leve : "); Serial.println(b.address);
    }
  }

  // ── Détection balises hors périmètre (hors-snooze) ──────────────────────────
  // Pas de break : toutes les balises doivent voir leur hystérésis réévaluée
  bool shouldVibrate = false;
  for (int i = 0; i < registry.count; i++) {
    BeaconInfo& b = registry.beacons[i];
    if (!b.paired || b.snoozed) continue;
    if (beaconOutOfRange(b)) shouldVibrate = true;
  }

  // ── Bouton : appui court, 3 s ou 10 s ───────────────────────────────────────
  // L'action se joue au RELÂCHEMENT, pas au front d'appui : c'est la seule
  // façon de distinguer un appui court d'un maintien. Le retard introduit sur
  // les actions courtes est la durée de l'appui lui-même, soit <300 ms.
  NicoBtnAction btnAction = BTN_NONE;
  bool btnRaw = digitalRead(PIN_BUTTON);
  if (btnRaw != nicoBtnState && now - nicoBtnChangeMs > BUTTON_DEBOUNCE_MS) {
    nicoBtnState    = btnRaw;
    nicoBtnChangeMs = now;
    if (btnRaw == LOW) {                 // front descendant = début d'appui
      nicoBtnPressMs = now;
      nicoBtnTicks   = 0;
    } else {                             // relâchement
      unsigned long held = now - nicoBtnPressMs;
      btnAction = (held >= BUTTON_RESET_MS)  ? BTN_RESET
                : (held >= BUTTON_FORGET_MS) ? BTN_FORGET
                                             : BTN_SHORT;
    }
  }

  // Impulsion de repère au franchissement de chaque palier
  bool btnHeld = (nicoBtnState == LOW);
  if (btnHeld) {
    unsigned long held = now - nicoBtnPressMs;
    if (nicoBtnTicks == 0 && held >= BUTTON_FORGET_MS) { nicoBtnTicks = 1; nicoVibrate(1, 80, 0);   }
    if (nicoBtnTicks == 1 && held >= BUTTON_RESET_MS)  { nicoBtnTicks = 2; nicoVibrate(2, 80, 120); }
  }

  if (btnAction == BTN_RESET) {
    // ── Effacer tous les appairages ───────────────────────────────────────
    factoryResetPairings(registry);
    nicoRingActive = false;
    nicoRingHash   = 0;
    shouldVibrate  = false;   // le registre est vide, plus rien à signaler
    nicoVibrate(5, 80, 120);
  } else if (btnAction == BTN_FORGET) {
    // ── Oublier les balises que le bracelet ne détecte plus ───────────────
    int forgotten = forgetUndetectedBeacons(registry);
    shouldVibrate = false;
    // 2 impulsions longues = au moins une balise oubliée, 1 = rien à oublier
    nicoVibrate(forgotten > 0 ? 2 : 1, 300, 200);
  } else if (btnAction == BTN_SHORT) {
    if (nicoRingActive) {
      // ── Annuler le ring en cours ──────────────────────────────────────────
      nicoRingActive = false;
      nicoRingHash   = 0;
      Serial.println("[btn] ring annule");
    } else if (shouldVibrate) {
      // ── Snooze : suspendre l'alerte jusqu'au prochain retour en périmètre ──
      for (int i = 0; i < registry.count; i++) {
        BeaconInfo& b = registry.beacons[i];
        if (!b.paired) continue;
        if (beaconOutOfRange(b)) { b.snoozed = true; Serial.print("[btn] snooze : "); Serial.println(b.address); }
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
  // Suspendue tant que le bouton est maintenu : sinon l'alerte se mélangerait
  // aux impulsions de repère et les rendrait indéchiffrables.
  if (shouldVibrate && !btnHeld && (now - nicoLastVibrationTime >= VIBRATION_INTERVAL_MS)) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(200);
    digitalWrite(PIN_VIBRATION, LOW);
    nicoLastVibrationTime = now;
  }
}
