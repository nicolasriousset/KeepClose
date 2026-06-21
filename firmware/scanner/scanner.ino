/*
 * KeepClose v2 — Firmware Scanner
 * Hardware : Seeed Studio XIAO BLE nRF52840
 * Role     : Bracelet porté par l'utilisateur
 *
 * Board package : Seeed nRF52 (Tools > Board > Seeed nRF52 Boards > Seeed XIAO nRF52840)
 * Bibliotheque  : bluefruit.h (incluse dans le BSP Seeed/Adafruit nRF52)
 *
 * Selection du projet : editer firmware/scanner/config.h
 *   #define PROJECT_NICO      -> alerte vibration si la canne sort du rayon
 *   #define PROJECT_TIPOUCET  -> sonne la balise la plus proche pour guider
 *
 * Appairage d'une balise : coller la balise contre le bracelet pendant 3 s.
 *   Le bracelet vibre 3 fois pour confirmer. L'appairage est sauvegarde en flash.
 */
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
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
  #error "Definir PROJECT_NICO ou PROJECT_TIPOUCET dans firmware/scanner/config.h"
#endif

BeaconRegistry registry;

// 3 vibrations courtes = appairage confirme
void confirmPairing() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(100);
    digitalWrite(PIN_VIBRATION, LOW);
    delay(150);
  }
}

// Callback appele par Bluefruit.Scanner pour chaque paquet advertising recu
void onBLEDeviceDiscovered(ble_gap_evt_adv_report_t* report) {
  // Filtrer par nom local (complete ou abrege)
  uint8_t nameBuffer[32] = { 0 };
  uint8_t nameLen = Bluefruit.Scanner.parseReportByType(report,
      BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, nameBuffer, sizeof(nameBuffer) - 1);
  if (nameLen == 0)
    nameLen = Bluefruit.Scanner.parseReportByType(report,
        BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, nameBuffer, sizeof(nameBuffer) - 1);
  if (nameLen == 0) return;
  if (strcmp((char*)nameBuffer, BEACON_LOCAL_NAME) != 0) return;

  // Convertir l'adresse BLE en chaine AA:BB:CC:DD:EE:FF
  const uint8_t* a = report->peer_addr.addr;
  char addr[18];
  snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X",
           a[5], a[4], a[3], a[2], a[1], a[0]);

  BeaconInfo* b = registry.findOrCreate(addr);
  if (!b) return;

  unsigned long now = millis();
  int rssi = (int)report->rssi;
  b->lastSeenMs = now;

  if (updatePairing(b, rssi, now)) {
    savePairedBeacons(registry);
    confirmPairing();
  }

  if (b->paired) {
    b->distance = estimateDistance(rssi);
    Serial.print(b->address);
    Serial.print(" -> ");
    Serial.print(b->distance, 2);
    Serial.println(" m");
  }
}

void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);

  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);

  loadPairedBeacons(registry);
  PROJECT_SETUP();

  Bluefruit.begin();
  Bluefruit.Scanner.setRxCallback(onBLEDeviceDiscovered);
  Bluefruit.Scanner.start(0);  // scan en continu, pas de timeout

  Serial.println("KeepClose v2 — Scanner pret");
}

void loop() {
  PROJECT_LOOP(registry);
  delay(10);
}
