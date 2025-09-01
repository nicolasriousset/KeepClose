#include "config.h"
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"
#include <cctype>


typedef enum State { WAITING,
                     GUARDING,
                     WARNING,
                     ALARM };

// Est ce que la montre est en mode recherche de balise, ou en mode garde ?
State state = WAITING;

TTGOClass* ttgo;
NimBLEScan* pBLEScan;
const float UNKNOWN = -1.0;
float beaconDistance = UNKNOWN;
unsigned long lastDetectionTime = 0;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 5000;                         // Intervalle de scan en millisecondes
const int SCAN_DURATION = 1000;                                   // Durée du scan en milisecondes
const int WARNING_DISTANCE = 10;                          // distance de la balise à partir de laquelle on fait vibrer la montre
const int ALARM_DISTANCE = 2 * WARNING_DISTANCE;  // distance de la balise à partir de laquelle on fait sonner la montre
bool irq = false;
bool screenOn = true;
unsigned long lastActivityTime = 0;
const unsigned long SCREEN_TIMEOUT = 10000;
unsigned long lastVibration = 0;
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

/** Calculer uen moyenne des n dernières valeurs,
 afin de réduire les anomalies lors de l'estimation de la distance de la balise*/
class MovingAverage {
private:
  static const int BUFFER_SIZE = 3;
  float buffer[BUFFER_SIZE] = { 0 };
  int index = 0;
  bool filled = false;

public:
  float addValue(float value) {
    buffer[index] = value;
    index = (index + 1) % BUFFER_SIZE;
    if (!filled && index == 0) filled = true;

    return computeAverage();
  }

  boolean canComputeAverage() {
    return filled || index > 0;
  }

  float computeAverage() {
    float sum = 0;
    int count = filled ? BUFFER_SIZE : index;
    if (count == 0) {
      return UNKNOWN;
    }

    for (int i = 0; i < count; i++) {
      sum += buffer[i];
    }
    return sum / count;
  }

  void reset() {
    index = 0;
    filled = false;
  }
} averager;

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {

    if (!advertisedDevice->haveName() || !startsWithIgnoreCase(advertisedDevice->getName(), "Holy")) {
      // Ignore devices that are not named 'Holy-IOT'
      return;
    }
    lastDetectionTime = millis();
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
        float rssi = averager.addValue(advertisedDevice->getRSSI());
        beaconDistance = estimateDistance(txPower, rssi);
        Serial.printf("Estimation de la distance : %.2fm\n", beaconDistance);
        updateState(beaconDistance);        
      }
      return;
    }
  }

  void onScanEnd(const NimBLEScanResults& scanResults, int reason) override {
    Serial.printf("End of scan, devices found : %d\n", scanResults.getCount());
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

void resetState() {
  Serial.printf("Reseting state to WAITING");
  state = WAITING;
  averager.reset();
  beaconDistance = UNKNOWN;
  updateLabels();
}

char* getStateLabel(State state) {
  switch (state) {
    case WAITING: return "WAITING";
    case GUARDING: return "GUARDING";
    case WARNING: return "WARNING";
    case ALARM: return "ALARM";
    default: return "<UNKNOWN>";
  }
}

void updateState(float beaconDistance) {
  switch (state) {
    case WAITING:
      if (beaconDistance < WARNING_DISTANCE) {
        state = GUARDING;
      }
      break;
    case GUARDING:
      if (beaconDistance > ALARM_DISTANCE) {
        state = ALARM;
        turnOnScreen();
      } else if (beaconDistance > WARNING_DISTANCE) {
        state = WARNING;
        turnOnScreen();
      }
      break;    
    case WARNING:
    case ALARM:
      if (beaconDistance < WARNING_DISTANCE) {
        state = GUARDING;
      } else if (beaconDistance < ALARM_DISTANCE ) {
        state = WARNING;
      }
      break;
  }
  Serial.printf("Switched to %s", getStateLabel(state));
}

void updateLabels() {
  if (state == WAITING) {
    lv_label_set_text(label_distance, "Recherche...");
  } else if (state == GUARDING) {
    if (beaconDistance >= WARNING_DISTANCE) {
    } else if (beaconDistance >= 1) {
      char buffer[50] = { 0 };
      snprintf(buffer, sizeof(buffer), "à %.2f m", beaconDistance);
      lv_label_set_text(label_distance, buffer);
    } else if (beaconDistance > 0) {
      lv_label_set_text_fmt(label_distance, "a %d cm", (int)(beaconDistance * 100.0));
    }
  } else if (state == GUARDING) {
    lv_label_set_text(label_distance, "Attention !");    
  } else if (state == ALARM) {
    lv_label_set_text(label_distance, "!!! ALERTE !!!");    
  }
  lv_obj_align(label_distance, NULL, LV_ALIGN_CENTER, 0, 0);
}

void startBeaconScan() {
  Serial.println("=== DÉBUT DU SCAN ===");
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
  if (isAlertState()) {
    resetState();
  }
  lastActivityTime = millis();
}

void turnOnScreen() {
  ttgo->openBL();         // Active le rétroéclairage
  ttgo->displayWakeup();  // Réveille l'écran

  screenOn = true;
  Serial.println("Écran allumé");
}

void turnOffScreen() {
  ttgo->closeBL();       // Éteint le rétroéclairage
  ttgo->displaySleep();  // Met l'écran en veille

  screenOn = false;
  Serial.println("Écran éteint");
}

void vibrate() {
  ttgo->motor->onec();
  lastVibration = millis();
}

boolean mustVibrate() {
  const int VIBRATION_INTERVAL = 1000;
  return isAlertState() && (millis() - lastVibration >= VIBRATION_INTERVAL);
}

boolean isAlertState() {
  return (state == WARNING || state == ALARM);
}

boolean mustTurnOffScreen() {
  return screenOn && !isAlertState() && (millis() - lastActivityTime > SCREEN_TIMEOUT);
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
  updateLabels();

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
  unsigned long currentTime = millis();

  lv_task_handler();
  if (!pBLEScan->isScanning()) {
    // Vérifier si il est temps de lancer un nouveau scan
    if (currentTime - lastScanTime >= SCAN_INTERVAL) {
      startBeaconScan();
    }
  }

  updateLabels();

  if (irq) {
    irq = false;
    ttgo->power->readIRQ();

    if (ttgo->power->isPEKShortPressIRQ()) {
      handleButtonPress();
    }
    ttgo->power->clearIRQ();
  }

  if (mustTurnOffScreen()) {
    turnOffScreen();
  }

  if (mustVibrate()) {
    vibrate();
  }

  delay(100);
}
