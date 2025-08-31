#include "config.h"
#include "NimBLEDevice.h"

TTGOClass* ttgo;
NimBLEScan* pBLEScan;
bool beaconFound = false;
bool scanInProgress = false;


int lastRSSI = 0;
lv_obj_t* label_btn1 = NULL;


// Classe callback pour traiter les résultats du scan
class MyScanCallbacks : public NimBLEScanCallbacks {
public:
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
  }

  void onScanEnd(const NimBLEScanResults& scanResults, int reason) override {
    Serial.println("=== Scan terminé via callback ===");
    scanInProgress = false;

    int deviceCount = scanResults.getCount();
    Serial.printf("Nombre total d'appareils: %d\n", deviceCount);

    // Traiter tous les résultats ici
    for (int i = 0; i < deviceCount; i++) {
      const NimBLEAdvertisedDevice* device = scanResults.getDevice(i);

      if (device->haveName()) {
        std::string deviceName = device->getName();
        Serial.printf("Final - Appareil %d: %s (RSSI: %d)\n", i + 1, deviceName.c_str(), device->getRSSI());

        if (deviceName == "Holy-IOT") {
          Serial.println("*** Holy-IOT trouvé dans onScanEnd! ***");
          lastRSSI = device->getRSSI();

          // Mettre à jour l'interface utilisateur
          if (label_btn1) {
            lv_label_set_text_fmt(label_btn1, "RSSI: %d", lastRSSI);
          }
        }
      }
    }

    if (!beaconFound) {
      Serial.println("⚠ Aucune balise Holy-IOT trouvée.");
    }
  }

  // Fonction pour décoder les URLs Eddystone
  String decodeEddystoneURL(const std::string& data) {
    if (data.length() < 3) return "";

    // Table des préfixes URL Eddystone
    const char* urlPrefixes[] = {
      "http://www.",
      "https://www.",
      "http://",
      "https://"
    };

    // Table des suffixes URL Eddystone
    const char* urlSuffixes[] = {
      ".com/", ".org/", ".edu/", ".net/", ".info/", ".biz/", ".gov/",
      ".com", ".org", ".edu", ".net", ".info", ".biz", ".gov"
    };

    String url = "";
    uint8_t prefixCode = data[2];

    if (prefixCode < 4) {
      url += urlPrefixes[prefixCode];
    }

    // Décoder le reste de l'URL
    for (size_t i = 3; i < data.length(); i++) {
      uint8_t c = data[i];
      if (c < 14) {
        url += urlSuffixes[c];
      } else if (c >= 32 && c <= 126) {
        url += (char)c;
      }
    }

    return url;
  }
};



void initNimBLE() {
  // Initialiser NimBLE
  Serial.println("Initialisation NimBLE...");
  NimBLEDevice::init("");

  // Créer le scanner BLE
  pBLEScan = NimBLEDevice::getScan();

  // Configurer le callback avec la classe MyScanCallbacks
  pBLEScan->setScanCallbacks(new MyScanCallbacks(), true);  // true pour recevoir les doublons

  // Configuration du scan
  pBLEScan->setActiveScan(true);        // Scan actif pour plus d'informations
  pBLEScan->setInterval(100);           // Intervalle en ms
  pBLEScan->setWindow(99);              // Fenêtre de scan en ms
  pBLEScan->setDuplicateFilter(false);  // Permettre les doublons pour suivi continu

  Serial.println("Scanner BLE configuré.");
  Serial.println("Recherche de la balise Holy-IOT...");
  Serial.println();
}

void startScanAsync() {
  if (scanInProgress) {
    Serial.println("Scan déjà en cours...");
    return;
  }
  
  Serial.println(">>> Démarrage du scan asynchrone...");
  
  // Nettoyer les résultats précédents
  pBLEScan->clearResults();
  scanInProgress = true;
  
  // Lancer un scan de 5 secondes
  bool scanStarted = pBLEScan->start(5, false);
  
  if (scanStarted) {
    Serial.println("Scan asynchrone démarré. Résultats via callback.");
  } else {
    Serial.println("Erreur lors du démarrage du scan.");
    scanInProgress = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  initNimBLE();

  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
  ttgo->motor_begin();
  ttgo->lvgl_begin();

  // Bouton 1
  lv_obj_t* btn1 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn1, event_handler);
  lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, -40);
  label_btn1 = lv_label_create(btn1, NULL);
  lv_label_set_text(label_btn1, "Button");

  // Bouton 2
  lv_obj_t* btn2 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn2, event_handler);
  lv_obj_align(btn2, NULL, LV_ALIGN_CENTER, 0, 40);
  lv_btn_set_checkable(btn2, true);
  lv_btn_toggle(btn2);
  lv_btn_set_fit2(btn2, LV_FIT_NONE, LV_FIT_TIGHT);
  lv_obj_t* label2 = lv_label_create(btn2, NULL);
  lv_label_set_text(label2, "Toggled");
}

static void event_handler(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    Serial.printf("Clicked\n");
    ttgo->motor->onec();
    startScanAsync();
    // Scan BLE Holy-IOT
  } else if (event == LV_EVENT_VALUE_CHANGED) {
    Serial.printf("Toggled\n");
  }
}

void loop() {
  lv_task_handler();
  delay(5);
}
