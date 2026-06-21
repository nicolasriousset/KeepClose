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
#include "config.h"

BLEService        ringService(RING_SERVICE_UUID);
BLECharacteristic ringCmdChar(RING_CMD_CHAR_UUID);

bool ringing = false;

void onRingCommand(uint16_t conn_handle, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len < 1) return;
  ringing = (data[0] == RING_CMD_START);
  Serial.println(ringing ? "Ring ON" : "Ring OFF");
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

  Bluefruit.begin();
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
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(ringService);
  Bluefruit.ScanResponse.addName();  // nom dans le scan response (paquet secondaire)
  Bluefruit.Advertising.start(0);    // advertise en continu

  Serial.println("Tag actif — advertising + service ring prets");
}

void loop() {
  updateBuzzer();
  // TODO: mise en veille basse consommation entre les connexions
}
