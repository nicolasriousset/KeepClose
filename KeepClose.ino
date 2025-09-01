#include "config.h"
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"
#include <cctype>

TTGOClass* ttgo;
NimBLEScan* pBLEScan;
bool beaconFound = false;
float beaconDistance = -1;
bool mustRefreshDisplay = false;
unsigned long lastScanTime = 0;
unsigned long lastReviewTime = 0;
const unsigned long SCAN_INTERVAL = 5000;  // 5 secondes en millisecondes
const int SCAN_DURATION = 1000;            // Durée du scan en milisecondes
bool irq = false;
bool screenOn = true;
unsigned long lastActivityTime = 0;
const unsigned long SCREEN_TIMEOUT = 10000;

lv_obj_t* label_distance = NULL;


#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))



bool startsWithIgnoreCase(const std::string& str, const std::string& prefix) {
  if (str.size() < prefix.size()) return false;
  for (size_t i = 0; i < prefix.size(); i++) {
    if (std::tolower((unsigned char)str[i]) != std::tolower((unsigned char)prefix[i])) {
      return false;
    }
  }
  return true;
}

float estimateDistance(int txPower, float rssi, float n = 2.0) {
  return pow(10.0, ((float)txPower - rssi) / (10.0 * n));
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {

    if (!advertisedDevice->haveName() || !startsWithIgnoreCase(advertisedDevice->getName(), "Holy")) {
      // Ignore devices that are not named 'Holy-IOT'
      return;
    }
    Serial.print("Balise détectée: ");
    Serial.print(advertisedDevice->getName().c_str());
    Serial.println();

    if (advertisedDevice->haveManufacturerData()) {
      std::string strManufacturerData = advertisedDevice->getManufacturerData();
      if (strManufacturerData.length() == 25 && strManufacturerData[0] == 0x4C && strManufacturerData[1] == 0x00) {
        NimBLEBeacon oBeacon = NimBLEBeacon();
        oBeacon.setData(reinterpret_cast<const uint8_t*>(strManufacturerData.data()), strManufacturerData.length());
        Serial.printf("iBeacon Frame\n");
        Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n",
                      oBeacon.getManufacturerId(),
                      ENDIAN_CHANGE_U16(oBeacon.getMajor()),
                      ENDIAN_CHANGE_U16(oBeacon.getMinor()),
                      oBeacon.getProximityUUID().toString().c_str(),
                      oBeacon.getSignalPower());

        int txPower = oBeacon.getSignalPower();
        float rssi = advertisedDevice->getRSSI();
        beaconDistance = estimateDistance(txPower, rssi);
        Serial.printf("Estimation de la distance : %.2fm\n", beaconDistance);
      }
      return;
    }
  }

  void onScanEnd(const NimBLEScanResults& scanResults, int reason) override {
    Serial.printf("End of scan, devices found : %d\n", scanResults.getCount());
    updateDisplay();
  }

} scanCallbacks;

void initNimBLE() {
  NimBLEDevice::init("Beacon-scanner");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setScanCallbacks(&scanCallbacks);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);
}

void updateDisplay() {
  if (beaconDistance >= 1) {
    lv_label_set_text_fmt(label_distance, "a %d m", (int)beaconDistance);
    lv_obj_align(label_distance, NULL, LV_ALIGN_CENTER, 0, 0);
  } else if (beaconDistance > 0) {
    lv_label_set_text_fmt(label_distance, "a %d cm", (int)(beaconDistance * 100.0));
    lv_obj_align(label_distance, NULL, LV_ALIGN_CENTER, 0, 0);
  } else {
    lv_label_set_text(label_distance, "Recherche...");
    lv_obj_align(label_distance, NULL, LV_ALIGN_CENTER, 0, 0);
  }
}

void startBeaconScan() {
  Serial.println("=== DÉBUT DU SCAN ===");
  beaconFound = false;
  lastScanTime = millis();

  pBLEScan->start(SCAN_DURATION);
}

void handleButtonPress() {
    if (screenOn) {
        // Si l'écran est allumé, le mettre en veille
        turnOffScreen();
    } else {
        // Si l'écran est éteint, l'allumer
        turnOnScreen();
    }
    lastActivityTime = millis();
}

void turnOnScreen() {
    ttgo->openBL(); // Active le rétroéclairage
    ttgo->displayWakeup(); // Réveille l'écran
       
    screenOn = true;
    Serial.println("Écran allumé");
}

void turnOffScreen() {
    ttgo->closeBL(); // Éteint le rétroéclairage
    ttgo->displaySleep(); // Met l'écran en veille
    
    screenOn = false;
    Serial.println("Écran éteint");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  initNimBLE();

  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->motor_begin();
  ttgo->lvgl_begin();

  turnOnScreen();
  // Label pour la distance ou le smiley
  label_distance = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_font(label_distance, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);
  updateDisplay();

  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(
    AXP202_INT, [] {
      irq = true;
    },
    FALLING);

  //!Clear IRQ unprocessed  first
  ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
  ttgo->power->clearIRQ();
}

void loop() {
  lv_task_handler();

  unsigned long currentTime = millis();

  if (!pBLEScan->isScanning()) {
    // Vérifier si il est temps de lancer un nouveau scan
    if (currentTime - lastScanTime >= SCAN_INTERVAL) {
      startBeaconScan();
    }
  }

  if (irq) {
    irq = false;
    ttgo->power->readIRQ();

    if (ttgo->power->isPEKShortPressIRQ()) {
      handleButtonPress();
    }
    ttgo->power->clearIRQ();
  }

   if (screenOn && (millis() - lastActivityTime > SCREEN_TIMEOUT)) {
        turnOffScreen();
    }  

  delay(100);
}

