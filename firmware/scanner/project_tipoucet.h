#pragma once
#include "scanner_core.h"

/*
 * Ti Poucet — Comportement spécifique
 *
 * L'utilisateur dépose des balises le long d'un chemin.
 * Pour rentrer, le bracelet sonne la balise active la plus proche.
 * L'utilisateur la rejoint → snooze automatique → balise suivante sonne.
 *
 * Commun : ringNearestBeacon() est partagé avec N.I.C.O.
 */

// Distance en dessous de laquelle une balise est considérée atteinte → snooze
#define DISTANCE_SNOOZE         1.0f

// Période de re-sonnerie si l'utilisateur n'avance pas (ms)
#define RING_INTERVAL_MS        5000

static unsigned long tipoucetLastRingTime = 0;

void tipoucetSetup() {
  Serial.println("Mode : Ti Poucet");
}

void tipoucetLoop(BeaconRegistry& registry) {
  unsigned long now = millis();

  // Snooze automatique : balise atteinte
  for (int i = 0; i < registry.count; i++) {
    BeaconInfo& b = registry.beacons[i];
    if (!b.snoozed && b.isAlive() && b.distance >= 0.0f && b.distance < DISTANCE_SNOOZE) {
      Serial.print("Balise atteinte, snooze : ");
      Serial.println(b.address);
      b.snoozed = true;
      stopRinging(registry, b.address);
      tipoucetLastRingTime = 0;  // forcer une sonnerie immédiate sur la suivante
    }
  }

  // Sonner périodiquement la prochaine balise active
  if (now - tipoucetLastRingTime >= RING_INTERVAL_MS) {
    ringNearestBeacon(registry);
    tipoucetLastRingTime = now;
  }

  // TODO: bouton pour snooze manuel (si l'utilisateur préfère snooze explicite)
  // TODO: réinitialiser tous les snoozes au début d'un nouveau trajet
}
