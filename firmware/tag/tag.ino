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
static bool          ringAcked         = false;  // utilisateur a acquitté la sonnerie en cours
static unsigned long scannerLastSeenMs = 0;       // ts du dernier paquet KC-Scanner reçu
static bool          tagBtnState       = HIGH;  // état debouncé
static unsigned long tagBtnChangeMs    = 0;
static unsigned long tagBtnPressMs     = 0;     // début de l'appui en cours
static bool          tagBtnTicked      = false; // palier de reset déjà signalé
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

// Le BSP ouvre FILE_O_WRITE en LFS_O_RDWR|LFS_O_CREAT puis se place en fin de
// fichier (Adafruit_LittleFS_File.cpp:73) : sans truncate, chaque sauvegarde
// ajoutait 2 octets au lieu d'ecraser. Comme loadPairedScannerCrc ne lit que
// les 2 PREMIERS octets, la balise repartait au redemarrage sur le tout premier
// bracelet qu'elle avait connu — un re-appairage ne survivait jamais au reboot.
void savePairedScannerCrc(uint16_t crc) {
  if (flashFile.open("/scanner.bin", FILE_O_WRITE)) {
    flashFile.truncate(0);
    flashFile.seek(0);
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
      bool newRing = (ringHash != 0 && ringHash == ownTagCrc);
      if (ringing && !newRing) ringAcked = false;  // bracelet a coupe le ring → reset pour la prochaine fois
      ringing = newRing;
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

// ── Pilotage du buzzer ───────────────────────────────────────────────────────
// Le reste du fichier ne connaît que buzzerSet(true/false) ; le type de buzzer
// (BUZZER_PASSIVE dans config.h) ne se voit que dans les deux implémentations
// ci-dessous. Un passif piloté en continu ne fait que « tic » à chaque front.

// pinMode() laisse la broche en « standard drive » (S0S1) : ~2 mA sous 0,4 V de
// chute sur nRF52840, alors qu'un buzzer en réclame 10 à 30. « High drive »
// (H0H1) monte à ~5–9 mA. À rappeler après tout pinMode(), qui réécrit PIN_CNF —
// y compris celui que HardwarePWM::addPin() fait en interne.
static void buzzerHighDrive(uint8_t arduinoPin) {
  uint32_t p = g_ADigitalPinMap[arduinoPin];
  NRF_GPIO_Type* port = (p < 32) ? NRF_P0 : NRF_P1;
  uint32_t bit = p & 31;
  port->PIN_CNF[bit] = (port->PIN_CNF[bit] & ~GPIO_PIN_CNF_DRIVE_Msk)
                     | ((uint32_t)GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos);
}

#ifdef BUZZER_PASSIVE

// PWM matérielle : le signal continue de sortir pendant que loop() scanne le
// BLE. Un carré bit-bangé dans loop() serait modulé par le scan et donnerait un
// son sale. HwPWM3 n'existe que sur nRF52840 et n'est jamais pris par
// analogWrite() (qui pioche dans HwPWM0..2), donc pas de conflit possible.
static const uint16_t BUZZER_TOP = 1000000UL / BUZZER_FREQ_HZ;  // période, en µs

static void buzzerBegin() {
  HwPWM3.setClockDiv(PWM_PRESCALER_PRESCALER_DIV_16);  // 16 MHz / 16 = 1 MHz → 1 tick = 1 µs
  HwPWM3.setMaxValue(BUZZER_TOP);
  HwPWM3.addPin(PIN_BUZZER);                           // fait le pinMode en interne
  buzzerHighDrive(PIN_BUZZER);                         // ...donc high drive après, pas avant
#ifdef BUZZER_PUSH_PULL
  // Même instance PWM que PIN_BUZZER : les deux voies partagent le compteur et
  // le COUNTERTOP, donc l'opposition de phase est exacte et le reste par
  // construction. Deux instances séparées dériveraient l'une par rapport à
  // l'autre et le gain s'effondrerait.
  HwPWM3.addPin(PIN_BUZZER_N);
  buzzerHighDrive(PIN_BUZZER_N);
#endif
  HwPWM3.stop();
  digitalWrite(PIN_BUZZER, LOW);
#ifdef BUZZER_PUSH_PULL
  digitalWrite(PIN_BUZZER_N, LOW);
#endif
}

static void buzzerSet(bool on) {
  if (on) {
    HwPWM3.writePin(PIN_BUZZER, BUZZER_TOP / 2);          // rapport cyclique 50 %
#ifdef BUZZER_PUSH_PULL
    HwPWM3.writePin(PIN_BUZZER_N, BUZZER_TOP / 2, true);  // même signal, phase opposée
#endif
  } else {
    // stop() puis digitalWrite : couper la PWM laisse les broches dans leur
    // dernier état, qui n'est pas forcément bas. Et un piézo laissé sous tension
    // continue reste déformé — silencieux, mais il consomme et il vieillit.
    // Les deux broches au même niveau, c'est 0 V aux bornes de l'élément.
    HwPWM3.stop();
    digitalWrite(PIN_BUZZER, LOW);
#ifdef BUZZER_PUSH_PULL
    digitalWrite(PIN_BUZZER_N, LOW);
#endif
  }
}

#else  // buzzer actif

static void buzzerBegin() {
  pinMode(PIN_BUZZER, OUTPUT);
  buzzerHighDrive(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
}

static void buzzerSet(bool on) {
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

#endif

// Motif sonore bloquant, pour les confirmations ponctuelles (appui long).
// updateBuzzer() reprend la main au tour de boucle suivant.
static void tagBeep(uint8_t count, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < count; i++) {
    buzzerSet(true);
    delay(onMs);
    buzzerSet(false);
    if (i + 1 < count) delay(offMs);
  }
}

void updateBuzzer() {
  if (!ringing || ringAcked) { buzzerSet(false); return; }
  // Bip-bip : 100 ms ON — 100 ms OFF — 100 ms ON — 700 ms silence (période 1 s)
  unsigned long t = millis() % 1000UL;
  bool bip = (t < 100) || (t >= 200 && t < 300);
  buzzerSet(bip);
}

void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);

  buzzerBegin();
  pinMode(PIN_BUTTON, INPUT_PULLUP);
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
  // Puissance TX explicite : le bracelet calibre sa distance dessus (cf. RSSI_AT_1M)
  Bluefruit.setTxPower(BLE_TX_POWER);

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
  // Intervalle unique : setInterval(fast, slow) avec fast == slow, pour que le
  // debit de paquets reste constant. Avec le decoupage par defaut du BSP
  // (20 ms pendant 30 s puis 152,5 ms), chaque rebuild d'advertising — a chaque
  // appairage — relancait une phase rapide, faisant varier le debit de mesures
  // du bracelet et donc sa latence d'alerte.
  Bluefruit.Advertising.setIntervalMS(ADV_INTERVAL_MS, ADV_INTERVAL_MS);
  Bluefruit.Advertising.start(0);    // advertise en continu

  // Scanner en parallele pour detecter KC-Scanner lors d'un appairage
  Bluefruit.Scanner.setInterval(160, 80);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.setRxCallback(onScannerFound);
  bool scanOk = Bluefruit.Scanner.start(0);

  Serial.println("Tag actif — advertising + scan KC-Scanner prets");
  Serial.print("  TX power        : "); Serial.print(BLE_TX_POWER);   Serial.println(" dBm");
  Serial.print("  Intervalle adv  : "); Serial.print(ADV_INTERVAL_MS); Serial.println(" ms");
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

  // Bouton : appui court = acquitter la sonnerie, appui long = oublier le bracelet.
  // L'action se joue au RELACHEMENT, seule facon de distinguer court et maintenu.
  bool btnRaw = digitalRead(PIN_BUTTON);
  unsigned long now = millis();
  if (btnRaw != tagBtnState && now - tagBtnChangeMs > BUTTON_DEBOUNCE_MS) {
    tagBtnState    = btnRaw;
    tagBtnChangeMs = now;
    if (btnRaw == LOW) {                 // debut d'appui
      tagBtnPressMs = now;
      tagBtnTicked  = false;
    } else if (now - tagBtnPressMs >= BUTTON_RESET_MS) {
      // Oublier le bracelet appaire. La sauvegarde et la reconstruction de
      // l'advertising sont faites par le bloc pendingAdvRebuild ci-dessus.
      pairedScannerCrc  = 0;
      ledAlert          = false;
      ringing           = false;
      ringAcked         = false;
      scannerLastSeenMs = 0;
      pendingAdvRebuild = true;
      Serial.println("[btn tag] appairage efface");
      tagBeep(5, 80, 120);
    } else if (ringing && !ringAcked) {
      ringAcked = true;
      Serial.println("[btn tag] sonnerie acquittee");
    }
  }

  // Bip de repere au franchissement du palier : indique quand relacher
  if (tagBtnState == LOW && !tagBtnTicked && now - tagBtnPressMs >= BUTTON_RESET_MS) {
    tagBtnTicked = true;
    tagBeep(1, 80, 0);
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
