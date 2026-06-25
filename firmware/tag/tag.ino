/*
 * KeepClose v2 — Firmware Tag
 * Hardware : Seeed Studio XIAO BLE nRF52840
 * Role     : Balise BLE posee sur un objet (canne, repere de chemin...)
 *            • Advertise en permanence pour etre detectable
 *            • Repond a la commande BLE "ring" pour sonner sur demande
 *
 * Board package : Seeed nRF52 (Tools > Board > Seeed nRF52 Boards > Seeed XIAO nRF52840)
 * Bibliotheque  : bluefruit.h (incluse dans le BSP Seeed/Adafruit nRF52)
 */
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "config.h"

using namespace Adafruit_LittleFS_Namespace;

BLEService        ringService(RING_SERVICE_UUID);
BLECharacteristic ringCmdChar(RING_CMD_CHAR_UUID);

bool ringing = false;
static unsigned long greenLedUntil = 0;

static uint16_t pairedScannerCrc = 0;   // CRC16 du bracelet propriétaire (0 = non appairé)
static bool     pendingAdvRebuild = false;
static File     flashFile(InternalFS);

void savePairedScannerCrc(uint16_t crc) {
  if (flashFile.open("/scanner.bin", FILE_O_WRITE)) {
    flashFile.write((const uint8_t*)&crc, sizeof(crc));
    flashFile.close();
    Serial.print("[flash] CRC bracelet sauvegardé : 0x");
    Serial.println(crc, HEX);
  }
}

uint16_t loadPairedScannerCrc() {
  uint16_t crc = 0;
  if (flashFile.open("/scanner.bin", FILE_O_READ)) {
    flashFile.read(&crc, sizeof(crc));
    flashFile.close();
    Serial.print("[flash] CRC bracelet chargé : 0x");
    Serial.println(crc, HEX);
  }
  return crc;
}

// Reconstruit le ScanResponse avec le CRC du bracelet propriétaire.
// À appeler depuis loop() uniquement (pas depuis un callback BLE).
void rebuildScanResponse(uint16_t crc) {
  Bluefruit.Advertising.stop();
  Bluefruit.ScanResponse.clearData();
  Bluefruit.ScanResponse.addName();
  if (crc != 0) {
    uint8_t mfr[4] = {
      (uint8_t)(PAIRING_MANUFACTURER_ID & 0xFF),
      (uint8_t)(PAIRING_MANUFACTURER_ID >> 8),
      (uint8_t)(crc & 0xFF),
      (uint8_t)(crc >> 8)
    };
    Bluefruit.ScanResponse.addManufacturerData(mfr, sizeof(mfr));
  }
  Bluefruit.Advertising.start(0);
  Serial.print("[adv] ScanResponse mis à jour, CRC=0x");
  Serial.println(crc, HEX);
}

void onConnect(uint16_t conn_handle) {
  digitalWrite(LED_GREEN, LOW);  // vert allume : scanner connecte
  Serial.println("Scanner connecte");
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  ringing = false;
  // Conserver le vert si le timer d'appairage tourne encore
  if (greenLedUntil == 0 || millis() >= greenLedUntil) {
    digitalWrite(LED_GREEN, HIGH);
  }
  Serial.println("Scanner deconnecte");
}

void onRingCommand(uint16_t conn_handle, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len < 1) return;
  if (data[0] == RING_CMD_PAIR) {
    // Appairage confirme : LED verte 5 s, pas de sonnerie
    digitalWrite(LED_GREEN, LOW);
    greenLedUntil = millis() + 5000;
    Serial.println("Appairage confirme — LED verte 5 s");
    return;
  }
  ringing = (data[0] == RING_CMD_START);
  Serial.println(ringing ? "Ring ON" : "Ring OFF");
}

// Detecte l'advertising "KC-Scanner" emis par le bracelet lors d'un appairage
void onScannerFound(ble_gap_evt_adv_report_t* report) {
  Bluefruit.Scanner.resume();
  uint8_t nameBuffer[32] = { 0 };
  uint8_t nameLen = Bluefruit.Scanner.parseReportByType(report,
      BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, nameBuffer, sizeof(nameBuffer) - 1);
  if (nameLen == 0)
    nameLen = Bluefruit.Scanner.parseReportByType(report,
        BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, nameBuffer, sizeof(nameBuffer) - 1);
  if (nameLen == 0 || strcmp((char*)nameBuffer, SCANNER_LOCAL_NAME) != 0) return;

  if ((int)report->rssi >= PAIRING_RSSI_THRESHOLD) {
    uint16_t newCrc = crc16(report->peer_addr.addr, 6);
    if (newCrc != pairedScannerCrc) {
      pairedScannerCrc = newCrc;
      pendingAdvRebuild = true;   // rebuild depuis loop(), pas depuis ce callback
      Serial.print("Nouveau bracelet propriétaire, CRC=0x");
      Serial.println(newCrc, HEX);
    }
    digitalWrite(LED_GREEN, LOW);
    greenLedUntil = millis() + 5000;
    Serial.println("Appairage confirme — LED verte 5 s");
  }
}

void updateBuzzer() {
  // TODO: generer une tonalite sur PIN_BUZZER (tone() ou PWM)
  digitalWrite(PIN_BUZZER, ringing ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH);  // eteinte au demarrage

  InternalFS.begin();
  pairedScannerCrc = loadPairedScannerCrc();

  Bluefruit.begin(1, 1);  // 1 periph (advertising) + 1 central (scan KC-Scanner)
  Bluefruit.Periph.setConnectCallback(onConnect);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);
  Bluefruit.setName(BEACON_LOCAL_NAME);
  // TODO: configurer TX power (Bluefruit.setTxPower()) et ADV_INTERVAL_MS

  // Declarer le service GATT ring
  ringService.begin();

  ringCmdChar.setProperties(CHR_PROPS_WRITE);
  ringCmdChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  ringCmdChar.setFixedLen(1);
  ringCmdChar.setWriteCallback(onRingCommand);
  ringCmdChar.begin();

  // Configurer l'advertising
  // Taille paquet : flags(3) + service UUID 128bit(18) + nom "KC-Tag"(8) = 29 octets < 31 max
  // TxPower retire pour faire de la place au nom dans le paquet principal
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(ringService);
  Bluefruit.Advertising.addName();   // nom dans le paquet principal (visible sans scan actif)
  if (pairedScannerCrc != 0) {
    uint8_t mfr[4] = {
      (uint8_t)(PAIRING_MANUFACTURER_ID & 0xFF),
      (uint8_t)(PAIRING_MANUFACTURER_ID >> 8),
      (uint8_t)(pairedScannerCrc & 0xFF),
      (uint8_t)(pairedScannerCrc >> 8)
    };
    Bluefruit.ScanResponse.addManufacturerData(mfr, sizeof(mfr));
  }
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.start(0);    // advertise en continu

  // Scanner en parallele pour detecter KC-Scanner lors d'un appairage
  Bluefruit.Scanner.setInterval(160, 80);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.setRxCallback(onScannerFound);
  bool scanOk = Bluefruit.Scanner.start(0);

  Serial.println("Tag actif — advertising + scan KC-Scanner prets");
  Serial.print("  Scan BLE demarre : ");
  Serial.println(scanOk ? "OK" : "ECHEC");
  Serial.print("  PAIRING_RSSI_THRESHOLD = ");
  Serial.println(PAIRING_RSSI_THRESHOLD);
}

void loop() {
  if (pendingAdvRebuild) {
    pendingAdvRebuild = false;
    savePairedScannerCrc(pairedScannerCrc);
    rebuildScanResponse(pairedScannerCrc);
  }

  if (greenLedUntil > 0 && millis() >= greenLedUntil) {
    greenLedUntil = 0;
    if (!Bluefruit.connected()) {
      digitalWrite(LED_GREEN, HIGH);
    }
  }
  updateBuzzer();
  // TODO: mise en veille basse consommation entre les connexions
}
