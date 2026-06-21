#pragma once
#include <ArduinoBLE.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Registre multi-balises
// ---------------------------------------------------------------------------

struct BeaconInfo {
  char          address[18];
  float         distance;
  bool          paired;
  bool          snoozed;
  unsigned long lastSeenMs;
  unsigned long pairingStartMs;  // instant où la balise a été détectée "au contact" (0 si non en cours)

  bool isAlive() const {
    return (millis() - lastSeenMs) < BEACON_TIMEOUT_MS;
  }
};

class BeaconRegistry {
public:
  BeaconInfo beacons[MAX_BEACONS];
  int count = 0;

  BeaconInfo* find(const char* addr) {
    for (int i = 0; i < count; i++)
      if (strncmp(beacons[i].address, addr, 17) == 0) return &beacons[i];
    return nullptr;
  }

  BeaconInfo* findOrCreate(const char* addr) {
    BeaconInfo* b = find(addr);
    if (b) return b;
    if (count >= MAX_BEACONS) return nullptr;
    BeaconInfo& n = beacons[count++];
    strncpy(n.address, addr, 17);
    n.address[17]    = '\0';
    n.distance       = -1.0f;
    n.paired         = false;
    n.snoozed        = false;
    n.lastSeenMs     = 0;
    n.pairingStartMs = 0;
    return &n;
  }

  // Seules les balises appairées, vivantes et non-snoozées sont considérées
  BeaconInfo* nearestActive() {
    BeaconInfo* nearest = nullptr;
    for (int i = 0; i < count; i++) {
      BeaconInfo& b = beacons[i];
      if (!b.paired || b.snoozed || !b.isAlive() || b.distance < 0) continue;
      if (!nearest || b.distance < nearest->distance) nearest = &b;
    }
    return nearest;
  }
};

// ---------------------------------------------------------------------------
// Persistance des appairages en flash
// ---------------------------------------------------------------------------

// TODO: implémenter avec InternalFS (BSP Adafruit nRF52) ou FlashIAP (BSP Seeed/mbed)
//       Format suggéré : fichier texte "/paired.txt", une adresse MAC par ligne.

void savePairedBeacons(BeaconRegistry& registry) {
  // TODO: écrire les adresses registry.beacons[i].address pour lesquelles paired==true
  Serial.println("[flash] savePairedBeacons — non implémenté");
}

void loadPairedBeacons(BeaconRegistry& registry) {
  // TODO: lire le fichier et appeler registry.findOrCreate(addr)->paired = true
  //       pour chaque adresse sauvegardée
  Serial.println("[flash] loadPairedBeacons — non implémenté");
}

// ---------------------------------------------------------------------------
// Appairage par proximité
// ---------------------------------------------------------------------------

// Retourne true si la balise vient d'être appairée lors de cet appel.
// La logique : si le RSSI brut dépasse PAIRING_RSSI_THRESHOLD pendant
// PAIRING_DURATION_MS sans interruption, la balise est appairée.
bool updatePairing(BeaconInfo* b, int rssi, unsigned long now) {
  if (b->paired) return false;

  if (rssi >= PAIRING_RSSI_THRESHOLD) {
    if (b->pairingStartMs == 0) {
      b->pairingStartMs = now;
      Serial.print("Appairage en cours : ");
      Serial.println(b->address);
    } else if (now - b->pairingStartMs >= PAIRING_DURATION_MS) {
      b->paired         = true;
      b->pairingStartMs = 0;
      Serial.print("Balise appairée : ");
      Serial.println(b->address);
      return true;
    }
  } else {
    // Contact rompu avant la fin du délai → reset
    if (b->pairingStartMs != 0) {
      Serial.println("Appairage annulé (balise éloignée trop tôt)");
      b->pairingStartMs = 0;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Estimation de distance
// ---------------------------------------------------------------------------

float estimateDistance(float rssi) {
  // TODO: ajouter un filtre RSSI par balise (moyenne glissante ou Kalman)
  return pow(10.0f, ((float)TX_POWER_AT_1M - rssi) / (10.0f * PATH_LOSS_N));
}

// ---------------------------------------------------------------------------
// Commande ring via BLE GATT
// ---------------------------------------------------------------------------

bool sendRingCommand(const char* address, byte cmd) {
  BLE.stopScan();

  BLEDevice device = BLE.available();
  // TODO: filtrer par adresse MAC pour cibler la bonne balise
  if (!device) { BLE.scan(); return false; }
  if (!device.connect()) { BLE.scan(); return false; }
  if (!device.discoverAttributes()) { device.disconnect(); BLE.scan(); return false; }

  BLECharacteristic ringChar = device.characteristic(RING_CMD_CHAR_UUID);
  if (!ringChar || !ringChar.canWrite()) { device.disconnect(); BLE.scan(); return false; }

  ringChar.writeValue(cmd);
  device.disconnect();
  BLE.scan();
  return true;
}

void ringNearestBeacon(BeaconRegistry& registry) {
  BeaconInfo* nearest = registry.nearestActive();
  if (!nearest) {
    Serial.println("Aucune balise active à sonner");
    return;
  }
  Serial.print("Sonnerie → ");
  Serial.println(nearest->address);
  sendRingCommand(nearest->address, RING_CMD_START);
}

void stopRinging(BeaconRegistry& registry, const char* address) {
  sendRingCommand(address, RING_CMD_STOP);
}
