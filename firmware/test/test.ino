/*
 * KeepClose — Test de soudures
 * Hardware : Seeed Studio XIAO BLE nRF52840
 *
 * A televerser sur chaque module fraichement soude avant toute autre chose.
 *
 * Phase 1 : Serie     -> verifie USB + chip
 * Phase 2 : LEDs RGB  -> verifie la puce elle-meme (sans cablage externe)
 * Phase 3 : GPIO scan -> verifie chaque broche soudee une par une
 * Phase 4 : BLE       -> verifie l'antenne (necessite ArduinoBLE fonctionnel)
 *
 * Montage Phase 3 :
 *   broche testee --[220 ohm]--[LED anode]--[LED cathode]-- GND
 *
 * Reglage moniteur serie : 115200 bauds, mode "Nouvelle ligne"
 *
 * Pour activer la Phase 4 BLE, decommentez la ligne suivante
 * apres avoir resolu la bibliotheque BLE :
 */

#include <Adafruit_TinyUSB.h>

// Decommentez pour activer le test BLE (Phase 4)
#define TEST_BLE_ENABLED

#ifdef TEST_BLE_ENABLED
#include <bluefruit.h>  // bibliotheque BLE du BSP Seeed/Adafruit nRF52 — pas ArduinoBLE
#endif

// Toutes les broches utilisateur du XIAO BLE nRF52840
const int   PINS[]      = { D0,  D1,  D2,  D3,  D4,  D5,  D6,  D7,  D8,  D9, D10 };
const char* PIN_NAMES[] = {"D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","D10"};
const int   PIN_COUNT   = 11;

// ── Phase 2 ──────────────────────────────────────────────────────────────────
// Les LEDs intégrées du XIAO BLE sont "active LOW" : LOW = allumée, HIGH = éteinte
void testLEDsIntegrees() {
  Serial.println("\n[Phase 2] LEDs integrees");
  Serial.println("Vous devez voir clignoter Rouge, puis Vert, puis Bleu sur la puce.");

  int leds[]         = { LED_RED, LED_GREEN, LED_BLUE };
  const char* noms[] = { "Rouge", "Verte",   "Bleue"  };

  for (int i = 0; i < 3; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], HIGH);  // éteinte
  }

  for (int i = 0; i < 3; i++) {
    Serial.print("  LED ");
    Serial.print(noms[i]);
    Serial.println(" allumee...");
    digitalWrite(leds[i], LOW);   // allume
    delay(1000);
    digitalWrite(leds[i], HIGH);  // éteint
    delay(300);
  }
  Serial.println("  -> Si vous avez vu les 3 couleurs : puce OK");
}

// ── Phase 3 ──────────────────────────────────────────────────────────────────
void attendreEnter() {
  while (Serial.available()) Serial.read();   // vide le buffer
  while (Serial.read() != '\n');              // attend Entrée
}

void testGPIO() {
  Serial.println("\n[Phase 3] Test GPIO broche par broche");
  Serial.println("Montage : broche testee -> resistance 220 ohm -> LED -> GND");
  Serial.println("Appuyez sur Entree dans le moniteur serie pour avancer.\n");

  for (int i = 0; i < PIN_COUNT; i++) {
    pinMode(PINS[i], OUTPUT);
    digitalWrite(PINS[i], LOW);
  }

  for (int i = 0; i < PIN_COUNT; i++) {
    Serial.print(">>> Broche ");
    Serial.print(PIN_NAMES[i]);
    Serial.println(" -> HIGH   (votre LED doit s'allumer)");
    digitalWrite(PINS[i], HIGH);

    Serial.println("    Appuyez sur Entree quand vous avez verifie...");
    attendreEnter();

    digitalWrite(PINS[i], LOW);
    Serial.println("    OK. Broche suivante.\n");
  }

  Serial.println("  -> Toutes les broches testees");
}

// ── Phase 4 ──────────────────────────────────────────────────────────────────
void testBLE() {
#ifdef TEST_BLE_ENABLED
  Serial.println("\n[Phase 4] Test BLE");

  Bluefruit.begin();
  Bluefruit.setName("XIAO-TEST");

  // Construire le paquet d'advertising avec le nom visible
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();  // inclut "XIAO-TEST" dans le paquet
  Bluefruit.Advertising.start(0);   // 0 = advertise indéfiniment

  Serial.println("  BLE actif. Lancez un scan sur votre smartphone.");
  Serial.println("  Application recommandee : nRF Connect (Android/iOS, gratuit).");
  Serial.println("  Vous devez voir apparaitre 'XIAO-TEST' dans la liste.");
  Serial.println("  Appuyez sur Entree quand vous avez verifie...");
  attendreEnter();

  Bluefruit.Advertising.stop();
  Serial.println("  -> Antenne BLE OK");
#else
  Serial.println("\n[Phase 4] Test BLE — desactive");
  Serial.println("  Commentez #define TEST_BLE_ENABLED en haut du fichier pour desactiver.");
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  unsigned long t = millis();
  while (!Serial && millis() - t < 5000);

  Serial.println("Appuyez sur Entree pour demarrer les tests...");
  attendreEnter();

  Serial.println("================================");
  Serial.println("  KeepClose — Test de soudures");
  Serial.println("================================");
  Serial.println("\n[Phase 1] Communication serie");
  Serial.println("  Si vous lisez ceci : USB + chip OK");

  Serial.println("\n[Diagnostic] Valeurs internes des broches :");
  for (int i = 0; i < PIN_COUNT; i++) {
    Serial.print("  ");
    Serial.print(PIN_NAMES[i]);
    Serial.print(" = ");
    Serial.println(PINS[i]);
  }

  testLEDsIntegrees();
  testGPIO();
  testBLE();

  Serial.println("\n================================");
  Serial.println("  Tous les tests termines !");
  Serial.println("================================");
}

void loop() {
  // Battement de coeur sur la LED verte pour confirmer que la carte tourne
  digitalWrite(LED_GREEN, LOW);
  delay(50);
  digitalWrite(LED_GREEN, HIGH);
  delay(2000);
}
