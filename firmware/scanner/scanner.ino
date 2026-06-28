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
  #define PROJECT_SETUP()           nicoSetup()
  #define PROJECT_LOOP(reg)         nicoLoop(reg)
  #define PROJECT_LED(reg, pairing) nicoLedUpdate(reg, pairing)
  #define PROJECT_ALERT(reg)        nicoComputeAlert(reg)
#elif defined(PROJECT_TIPOUCET)
  #include "project_tipoucet.h"
  #define PROJECT_SETUP()           tipoucetSetup()
  #define PROJECT_LOOP(reg)         tipoucetLoop(reg)
  #define PROJECT_LED(reg, pairing) /* LED non gérée en mode Ti Poucet */
  #define PROJECT_ALERT(reg)        ((uint16_t)0)
#else
  #error "Definir PROJECT_NICO ou PROJECT_TIPOUCET dans firmware/scanner/config.h"
#endif

BeaconRegistry registry;

static unsigned long pairingLedUntil      = 0;
static uint16_t      currentAlertHash     = 0;
static uint16_t      currentRingHash      = 0;
static bool          pendingAdvRebuild    = false;

// Reconstruit l'advertising avec les hashes d'alerte et de sonnerie courants.
// À appeler depuis loop() uniquement (pas depuis un callback BLE).
void rebuildScannerAdvertising() {
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addName();
  uint8_t mfr[6] = {
    (uint8_t)(PAIRING_MANUFACTURER_ID & 0xFF),
    (uint8_t)(PAIRING_MANUFACTURER_ID >> 8),
    (uint8_t)(currentAlertHash & 0xFF),
    (uint8_t)(currentAlertHash >> 8),
    (uint8_t)(currentRingHash  & 0xFF),
    (uint8_t)(currentRingHash  >> 8)
  };
  Bluefruit.Advertising.addManufacturerData(mfr, sizeof(mfr));
  Bluefruit.Advertising.setInterval(480, 480);
  Bluefruit.Advertising.start(0);
  Serial.print("[adv scanner] alertHash=0x"); Serial.print(currentAlertHash, HEX);
  Serial.print(" ringHash=0x");               Serial.println(currentRingHash, HEX);
}

// 3 vibrations courtes = confirmation haptique
void confirmPairing() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_VIBRATION, HIGH);
    delay(100);
    digitalWrite(PIN_VIBRATION, LOW);
    delay(150);
  }
}

// Le scanner advertise KC-Scanner en continu (démarré dans setup) :
// le tag peut ainsi détecter sa présence et estimer la distance.
void signalPairingByAdvertising() {
  Serial.println("Advertising KC-Scanner actif (visible par les tags)");
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

  // Mémoriser le CRC de la balise (identifiant compact pour l'advertising bracelet)
  if (b->tagCrc == 0) {
    b->tagCrc = crc16(report->peer_addr.addr, 6);
  }

  // Lire le CRC du bracelet propriétaire depuis le manufacturer data de la balise
  uint8_t mfrBuf[10] = { 0 };
  uint8_t mfrLen = Bluefruit.Scanner.parseReportByType(report,
      BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, mfrBuf, sizeof(mfrBuf));
  if (mfrLen >= 4 && b->paired) {
    uint16_t companyId = (uint16_t)mfrBuf[0] | ((uint16_t)mfrBuf[1] << 8);
    uint16_t tagCrc    = (uint16_t)mfrBuf[2] | ((uint16_t)mfrBuf[3] << 8);
    if (companyId == PAIRING_MANUFACTURER_ID && tagCrc != 0 && tagCrc != ownCrc) {
      b->paired         = false;
      b->pairingStartMs = 0;
      savePairedBeacons(registry);
      Serial.print("Balise désappairée (appairée à un autre bracelet) : ");
      Serial.println(b->address);
      return;
    }
  }

  if (updatePairing(b, rssi, now)) {
    savePairedBeacons(registry);
    confirmPairing();
    pairingLedUntil = millis() + 5000;
    signalPairingByAdvertising();
  }

  if (b->paired) {
    b->rssi    = rssi;
    b->rssiEma = (b->rssiEma == 0.0f) ? (float)rssi
                                       : RSSI_EMA_ALPHA * rssi + (1.0f - RSSI_EMA_ALPHA) * b->rssiEma;
    b->distance = estimateDistance(b->rssiEma);
  }
}


void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);

  pinMode(PIN_VIBRATION, OUTPUT);
  digitalWrite(PIN_VIBRATION, LOW);
  pinMode(LED_RED,   OUTPUT); digitalWrite(LED_RED,   HIGH);
  pinMode(LED_GREEN, OUTPUT); digitalWrite(LED_GREEN, HIGH);
  pinMode(LED_BLUE,  OUTPUT); digitalWrite(LED_BLUE,  HIGH);

  InternalFS.begin();
  loadPairedBeacons(registry);
  PROJECT_SETUP();

  Bluefruit.begin(1, 1);  // 1 periph (advertising appairage) + 1 central (scan)
  initOwnCrc();

  // Advertising "KC-Scanner" en continu.
  // Manufacturer data : [company_id(2) | alertHash(2) | ringHash(2)]
  // Les balises lisent ces champs pour connaître leur état LED et buzzer.
  Bluefruit.setName(SCANNER_LOCAL_NAME);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addName();
  uint8_t mfr0[6] = {
    (uint8_t)(PAIRING_MANUFACTURER_ID & 0xFF), (uint8_t)(PAIRING_MANUFACTURER_ID >> 8),
    0, 0, 0, 0  // alertHash=0, ringHash=0 à l'initialisation
  };
  Bluefruit.Advertising.addManufacturerData(mfr0, sizeof(mfr0));
  Bluefruit.Advertising.setInterval(480, 480);  // 300 ms (480 × 0.625 ms)
  Bluefruit.Advertising.start(0);               // continu

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
  PROJECT_LED(registry, pairingLedUntil > 0 && millis() < pairingLedUntil);

  // Recalculer l'hash d'alerte et déclencher un rebuild advertising si nécessaire
  uint16_t newAlert = PROJECT_ALERT(registry);
  if (newAlert != currentAlertHash) {
    currentAlertHash  = newAlert;
    pendingAdvRebuild = true;
  }
  if (pendingAdvRebuild) {
    pendingAdvRebuild = false;
    rebuildScannerAdvertising();
  }

  static unsigned long lastHb = 0;
  if (millis() - lastHb >= 5000) {
    lastHb = millis();

    // Collecter les balises appairees et les trier par distance (insertion sort)
    BeaconInfo* sorted[MAX_BEACONS];
    int pairedCount = 0;
    for (int i = 0; i < registry.count; i++)
      if (registry.beacons[i].paired) sorted[pairedCount++] = &registry.beacons[i];
    for (int i = 1; i < pairedCount; i++) {
      BeaconInfo* key = sorted[i];
      float keyDist = (key->isAlive() && key->distance >= 0) ? key->distance : 1e9f;
      int j = i - 1;
      while (j >= 0) {
        float jDist = (sorted[j]->isAlive() && sorted[j]->distance >= 0) ? sorted[j]->distance : 1e9f;
        if (jDist <= keyDist) break;
        sorted[j + 1] = sorted[j];
        j--;
      }
      sorted[j + 1] = key;
    }

    Serial.print("[hb] t=");
    Serial.print(millis() / 1000);
    Serial.print("s  balises vues=");
    Serial.print(registry.count);
    Serial.print("  appairees=");
    Serial.print(pairedCount);
    Serial.print("  [");
    for (int i = 0; i < pairedCount; i++) {
      if (i > 0) Serial.print(", ");
      if (sorted[i]->isAlive() && sorted[i]->distance >= 0) {
        Serial.print(sorted[i]->distance, 1);
        Serial.print("m ");
        Serial.print(sorted[i]->rssi);
        Serial.print("dBm(");
        Serial.print((int)sorted[i]->rssiEma);
        Serial.print("ema)");
      } else {
        Serial.print("hors portee");
      }
    }
    Serial.println("]");
  }

  delay(10);
}
