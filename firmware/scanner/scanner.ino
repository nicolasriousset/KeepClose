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
 * Appairage : coller la balise contre le bracelet pendant 3 s.
 *   Confirmation : 3 vibrations + LED verte 5 s.
 *   Le scanner advertise "KC-Scanner" 5 s pour que le tag puisse aussi
 *   detecter la proximite et allumer sa propre LED verte.
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

static unsigned long pairingLedUntil = 0;

// 3 vibrations courtes = confirmation haptique
void confirmPairing() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(100);
    digitalWrite(PIN_VIBRATION, LOW);
    delay(150);
  }
}

// Advertise "KC-Scanner" 5 s : le tag le detecte et allume sa LED verte
void signalPairingByAdvertising() {
  Bluefruit.Advertising.start(5);  // 5 s puis arret automatique
  Serial.println("Advertising KC-Scanner 5 s (signal appairage)");
}

// Callback appele pour chaque paquet advertising recu
void onBLEDeviceDiscovered(ble_gap_evt_adv_report_t* report) {
  // nRF52 SDK15+ : reprendre le scan apres chaque rapport (obligatoire)
  Bluefruit.Scanner.resume();

  // ── Filtrer par nom KC-Tag ────────────────────────────────────────────────
  uint8_t nameBuffer[32] = { 0 };
  uint8_t nameLen = Bluefruit.Scanner.parseReportByType(report,
      BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, nameBuffer, sizeof(nameBuffer) - 1);
  if (nameLen == 0)
    nameLen = Bluefruit.Scanner.parseReportByType(report,
        BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, nameBuffer, sizeof(nameBuffer) - 1);

  if (nameLen == 0 || strcmp((char*)nameBuffer, BEACON_LOCAL_NAME) != 0) return;

  // ── KC-Tag trouve ─────────────────────────────────────────────────────────
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
    pairingLedUntil = millis() + 5000;
    signalPairingByAdvertising();
  }

  if (b->paired) {
    b->distance = estimateDistance(rssi);
  }
}

// LED verte : 5 s apres appairage, puis indicateur "balise vivante"
void updateStatusLed() {
  if (pairingLedUntil > 0) {
    if (millis() < pairingLedUntil) {
      digitalWrite(LED_GREEN, LOW);
      return;
    }
    pairingLedUntil = 0;
  }
  bool paireeVivante = false;
  for (int i = 0; i < registry.count; i++) {
    if (registry.beacons[i].paired && registry.beacons[i].isAlive()) {
      paireeVivante = true;
      break;
    }
  }
  // LED_GREEN active LOW : LOW = allumee, HIGH = eteinte
  digitalWrite(LED_GREEN, paireeVivante ? LOW : HIGH);
}

void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);

  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH);

  loadPairedBeacons(registry);
  PROJECT_SETUP();

  Bluefruit.begin(1, 1);  // 1 periph (advertising appairage) + 1 central (scan)

  // Preparer l'advertising "KC-Scanner" (demarre seulement lors de l'appairage)
  Bluefruit.setName(SCANNER_LOCAL_NAME);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addName();

  // Lancer le scan BLE
  Bluefruit.Scanner.setInterval(160, 80);  // 100 ms interval, 50 ms window
  Bluefruit.Scanner.useActiveScan(true);   // inclure les scan response (nom dans ScanResponse)
  Bluefruit.Scanner.setRxCallback(onBLEDeviceDiscovered);
  bool scanOk = Bluefruit.Scanner.start(0);

  Serial.println("KeepClose v2 — Scanner pret");
  Serial.print("  Scan BLE demarre : ");
  Serial.println(scanOk ? "OK" : "ECHEC");
  Serial.print("  PAIRING_RSSI_THRESHOLD = ");
  Serial.println(PAIRING_RSSI_THRESHOLD);
  Serial.print("  PAIRING_DURATION_MS    = ");
  Serial.println(PAIRING_DURATION_MS);
}

void loop() {
  PROJECT_LOOP(registry);
  updateStatusLed();

  static unsigned long lastHb = 0;
  if (millis() - lastHb >= 5000) {
    lastHb = millis();
    int paired = 0;
    for (int i = 0; i < registry.count; i++)
      if (registry.beacons[i].paired) paired++;
    Serial.print("[hb] t=");
    Serial.print(millis() / 1000);
    Serial.print("s  balises vues=");
    Serial.print(registry.count);
    Serial.print("  appairees=");
    Serial.println(paired);
    for (int i = 0; i < registry.count; i++) {
      BeaconInfo& b = registry.beacons[i];
      Serial.print("  ");
      Serial.print(b.address);
      Serial.print(b.paired ? " [appairee]" : " [non appairee]");
      Serial.print(b.isAlive() ? " vivante" : " hors portee");
      if (b.paired && b.distance >= 0) {
        Serial.print("  ");
        Serial.print(b.distance, 2);
        Serial.print(" m");
      }
      Serial.println();
    }
  }

  delay(10);
}
