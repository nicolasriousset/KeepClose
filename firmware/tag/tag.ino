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
static bool          ledAlert          = false;  // bracelet signale que cette balise est hors-rayon
static unsigned long scannerLastSeenMs = 0;       // ts du dernier paquet KC-Scanner reçu
static uint16_t ownTagCrc        = 0;
static uint16_t pairedScannerCrc = 0;
static bool     pendingAdvRebuild = false;
static File     flashFile(InternalFS);

static inline void setRgbLed(bool r, bool g, bool b) {
  digitalWrite(LED_RED,   r ? LOW : HIGH);
  digitalWrite(LED_GREEN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE,  b ? LOW : HIGH);
}

// Bleu  : non appairé
// Vert  : appairé, bracelet à portée (ledAlert == false, signal frais)
// Rouge : appairé et le bracelet signale que cette balise est hors-rayon,
//         ou signal du bracelet périmé
void updateLed() {
  bool isPaired = (pairedScannerCrc != 0);
  if (!isPaired) { setRgbLed(false, false, true); return; }
  bool fresh = scannerLastSeenMs > 0 &&
               (millis() - scannerLastSeenMs) < SCANNER_STALE_MS;
  if (fresh && !ledAlert) setRgbLed(false, true,  false);
  else                    setRgbLed(true,  false, false);
}

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
  Serial.println("Scanner connecte");
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  ringing = false;
  Serial.println("Scanner deconnecte");
}

void onRingCommand(uint16_t conn_handle, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len < 1) return;
  if (data[0] == RING_CMD_PAIR) {
    Serial.println("Appairage confirme (commande GATT)");
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

  // Mise à jour de la présence du bracelet
  scannerLastSeenMs = millis();

  // Lire alertHash et ringHash depuis le manufacturer data du bracelet
  // Format : [company_id(2) | alertHash(2) | ringHash(2)]
  uint8_t mfrBuf[8] = { 0 };
  uint8_t mfrLen = Bluefruit.Scanner.parseReportByType(report,
      BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, mfrBuf, sizeof(mfrBuf));
  if (mfrLen >= 6) {
    uint16_t compId    = (uint16_t)mfrBuf[0] | ((uint16_t)mfrBuf[1] << 8);
    uint16_t alertHash = (uint16_t)mfrBuf[2] | ((uint16_t)mfrBuf[3] << 8);
    uint16_t ringHash  = (uint16_t)mfrBuf[4] | ((uint16_t)mfrBuf[5] << 8);
    if (compId == PAIRING_MANUFACTURER_ID) {
      ledAlert = (alertHash != 0 && alertHash == ownTagCrc);
      ringing  = (ringHash  != 0 && ringHash  == ownTagCrc);
    }
  }

  if ((int)report->rssi >= PAIRING_RSSI_THRESHOLD) {
    uint16_t newCrc = crc16(report->peer_addr.addr, 6);
    if (newCrc != pairedScannerCrc) {
      pairedScannerCrc = newCrc;
      pendingAdvRebuild = true;   // rebuild depuis loop(), pas depuis ce callback
      Serial.print("[appairage] nouveau bracelet, CRC=0x");
      Serial.println(newCrc, HEX);
    }
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
  pinMode(LED_RED,   OUTPUT); digitalWrite(LED_RED,   HIGH);
  pinMode(LED_GREEN, OUTPUT); digitalWrite(LED_GREEN, HIGH);
  pinMode(LED_BLUE,  OUTPUT); digitalWrite(LED_BLUE,  HIGH);

  InternalFS.begin();
  pairedScannerCrc = loadPairedScannerCrc();

  Bluefruit.begin(1, 1);  // 1 periph (advertising) + 1 central (scan KC-Scanner)
  ownTagCrc = crc16(Bluefruit.getAddr().addr, 6);
  Serial.print("[tag] ownTagCrc=0x"); Serial.println(ownTagCrc, HEX);
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

  updateLed();
  updateBuzzer();

  static unsigned long lastHb = 0;
  if (millis() - lastHb >= 5000) {
    lastHb = millis();
    Serial.print("[hb tag] t=");
    Serial.print(millis() / 1000);
    Serial.print("s  appaire=");
    Serial.print(pairedScannerCrc != 0 ? "oui" : "non");
    if (pairedScannerCrc != 0) {
      Serial.print("  CRC=0x"); Serial.print(pairedScannerCrc, HEX);
      unsigned long scannerAge = (scannerLastSeenMs > 0) ? millis() - scannerLastSeenMs : UINT32_MAX;
      if (scannerLastSeenMs == 0) {
        Serial.print("  bracelet jamais detecte");
      } else if (scannerAge > SCANNER_STALE_MS) {
        Serial.print("  bracelet absent depuis ");
        Serial.print(scannerAge / 1000);
        Serial.print("s");
      } else {
        Serial.print("  vu il y a ");
        Serial.print(scannerAge / 1000);
        Serial.print("s  alerte=");
        Serial.print(ledAlert ? "oui" : "non");
        Serial.print("  ring=");
        Serial.print(ringing  ? "oui" : "non");
      }
    }
    Serial.println();
  }
  // TODO: mise en veille basse consommation entre les connexions
}
