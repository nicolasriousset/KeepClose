#pragma once
#include "config.h"
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

static uint16_t ownCrc = 0;
static File     _scannerFlashFile(InternalFS);

// Doit être appelé après Bluefruit.begin()
void initOwnCrc() {
  ble_gap_addr_t addr = Bluefruit.getAddr();
  ownCrc = crc16(addr.addr, 6);
  Serial.print("[pairing] ownCrc=0x");
  Serial.println(ownCrc, HEX);
}

// ---------------------------------------------------------------------------
// Registre multi-balises
// ---------------------------------------------------------------------------

struct BeaconInfo {
  char          address[18];
  float         distance;
  int           rssi;      // RSSI brut du dernier paquet (pour affichage)
  float         rssiEma;   // RSSI lissé par EMA (pour distance et LED)
  uint16_t      tagCrc;    // CRC16 MAC de la balise — identifiant compact publié dans l'advertising
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
    n.rssi           = 0;
    n.rssiEma        = 0.0f;  // 0 = pas encore de mesure
    n.tagCrc         = 0;
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
  if (!_scannerFlashFile.open("/paired.txt", FILE_O_WRITE)) {
    Serial.println("[flash] savePairedBeacons — erreur ouverture");
    return;
  }
  for (int i = 0; i < registry.count; i++) {
    if (registry.beacons[i].paired) {
      _scannerFlashFile.print(registry.beacons[i].address);
      _scannerFlashFile.print('\n');
    }
  }
  _scannerFlashFile.close();
  Serial.println("[flash] paired beacons sauvegardés");
}

void loadPairedBeacons(BeaconRegistry& registry) {
  if (!_scannerFlashFile.open("/paired.txt", FILE_O_READ)) return;
  char    line[18];
  uint8_t idx = 0;
  while (_scannerFlashFile.available()) {
    char c = (char)_scannerFlashFile.read();
    if (c == '\n' || idx >= 17) {
      if (idx == 17) {
        line[17] = '\0';
        BeaconInfo* b = registry.findOrCreate(line);
        if (b) b->paired = true;
      }
      idx = 0;
    } else {
      line[idx++] = c;
    }
  }
  _scannerFlashFile.close();
  Serial.println("[flash] paired beacons chargés");
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

// TODO: implémenter avec l'API Bluefruit Central (BLEClientService / BLEClientCharacteristic)
//       Nécessite une gestion de connexion asynchrone via callbacks.
//       Pour l'instant la commande est loguée mais pas envoyée.
bool sendRingCommand(const char* address, byte cmd) {
  Serial.print("[ring] commande 0x");
  Serial.print(cmd, HEX);
  Serial.print(" vers ");
  Serial.print(address);
  Serial.println(" — non implemente");
  return false;
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
