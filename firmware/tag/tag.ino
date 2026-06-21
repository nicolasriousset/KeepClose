/*
 * KeepClose v2 — Firmware Tag
 * Hardware : Seeed Studio XIAO BLE nRF52840
 * Rôle     : Balise BLE posée sur un objet (canne, repère de chemin…)
 *            • Advertise en permanence pour être détectable
 *            • Répond à la commande BLE "ring" pour sonner sur demande
 *
 * Board package : Seeed nRF52 (Tools > Board > Seeed nRF52 Boards > XIAO BLE)
 * URL           : https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
 * Bibliothèque  : ArduinoBLE (Gestionnaire de bibliothèques Arduino)
 */
#include <ArduinoBLE.h>
#include "config.h"

BLEService           ringService(RING_SERVICE_UUID);
BLEByteCharacteristic ringCmdChar(RING_CMD_CHAR_UUID, BLEWrite);

bool ringing = false;

void onRingCommand(BLEDevice central, BLECharacteristic characteristic) {
  byte cmd = ringCmdChar.value();
  ringing = (cmd == RING_CMD_START);
  Serial.println(ringing ? "Ring ON" : "Ring OFF");
}

void updateBuzzer() {
  // TODO: générer une tonalité sur PIN_BUZZER (tone() ou PWM)
  digitalWrite(PIN_BUZZER, ringing ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  if (!BLE.begin()) {
    Serial.println("Erreur : impossible d'initialiser le BLE");
    while (true);
  }

  ringService.addCharacteristic(ringCmdChar);
  BLE.addService(ringService);
  ringCmdChar.setEventHandler(BLEWritten, onRingCommand);

  BLE.setLocalName(BEACON_LOCAL_NAME);
  BLE.setAdvertisedService(ringService);
  // TODO: configurer TX power et intervalle d'advertising (ADV_INTERVAL_MS)

  BLE.advertise();
  Serial.println("Tag actif — advertising + service ring prêts");
}

void loop() {
  BLE.poll();
  updateBuzzer();
  // TODO: mise en veille basse consommation entre les connexions
}
