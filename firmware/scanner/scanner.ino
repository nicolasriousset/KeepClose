/*
 * KeepClose v2 — Firmware Scanner
 * Hardware : Seeed Studio XIAO BLE nRF52840
 * Rôle     : Bracelet porté par l'utilisateur
 *
 * Board package : Seeed nRF52 (Tools > Board > Seeed nRF52 Boards > XIAO BLE)
 * URL           : https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
 * Bibliothèque  : ArduinoBLE (Gestionnaire de bibliothèques Arduino)
 *
 * Sélection du projet : éditer firmware/scanner/config.h
 *   #define PROJECT_NICO      → alerte vibration si la canne sort du rayon
 *   #define PROJECT_TIPOUCET  → sonne la balise la plus proche pour guider
 *
 * Appairage d'une balise : coller la balise contre le bracelet pendant 3 s.
 *   Le bracelet vibre 3 fois pour confirmer. L'appairage est sauvegardé en flash.
 */
#include <ArduinoBLE.h>
#include "config.h"
#include "scanner_core.h"

#if defined(PROJECT_NICO)
  #include "project_nico.h"
  #define PROJECT_SETUP()    nicoSetup()
  #define PROJECT_LOOP(reg)  nicoLoop(reg)
#elif defined(PROJECT_TIPOUCET)
  #include "project_tipoucet.h"
  #define PROJECT_SETUP()    tipoucetSetup()
  #define PROJECT_LOOP(reg)  tipoucetLoop(reg)
#else
  #error "Définir PROJECT_NICO ou PROJECT_TIPOUCET dans firmware/scanner/config.h"
#endif

BeaconRegistry registry;
unsigned long lastScanTime = 0;

// 3 vibrations courtes = appairage confirmé
void confirmPairing() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(100);
    digitalWrite(PIN_VIBRATION, LOW);
    delay(150);
  }
}

void onBLEDeviceDiscovered(BLEDevice device) {
  if (!device.hasLocalName()) return;
  if (String(device.localName()) != BEACON_LOCAL_NAME) return;

  BeaconInfo* b = registry.findOrCreate(device.address().c_str());
  if (!b) return;

  unsigned long now = millis();
  int rssi = device.rssi();

  b->lastSeenMs = now;

  // Vérifier si cette balise vient d'être appairée
  if (updatePairing(b, rssi, now)) {
    savePairedBeacons(registry);
    confirmPairing();
  }

  // Mise à jour de la distance uniquement pour les balises appairées
  if (b->paired) {
    b->distance = estimateDistance(rssi);
    Serial.print(b->address);
    Serial.print(" → ");
    Serial.print(b->distance, 2);
    Serial.println(" m");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);

  loadPairedBeacons(registry);
  PROJECT_SETUP();

  if (!BLE.begin()) {
    Serial.println("Erreur BLE");
    while (true);
  }

  BLE.setScanCallback(onBLEDeviceDiscovered);
  BLE.scan();
  Serial.println("KeepClose v2 — Scanner prêt");
}

void loop() {
  unsigned long now = millis();
  BLE.poll();

  if (now - lastScanTime >= SCAN_INTERVAL_MS) {
    lastScanTime = now;
    BLE.scan();
  }

  PROJECT_LOOP(registry);
  delay(10);
}
