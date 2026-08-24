#pragma once
#include "config.h"
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

static uint16_t ownCrc = 0;
static File     _scannerFlashFile(InternalFS);

// Doit être appelé après Bluefruit.begin()
void initOwnCrc() {
  ble_gap_addr_t addr = Bluefruit.getAddr();
  ownCrc = crc16(addr.addr, 6);
  Serial.print("[pairing] ownCrc=0x");
  Serial.println(ownCrc, HEX);
}

// ---------------------------------------------------------------------------
// Estimation de distance
// ---------------------------------------------------------------------------

// Inverse du modèle de propagation (cf. common/config.h).
// Indicatif : sert à l'affichage et au tri, jamais à la décision d'alerte —
// celle-ci se prend en dBm dans beaconOutOfRange().
float estimateDistance(float rssi) {
  return pow(10.0f, ((float)RSSI_AT_1M - rssi) / (10.0f * PATH_LOSS_N));
}

// ---------------------------------------------------------------------------
// Registre multi-balises
// ---------------------------------------------------------------------------

struct BeaconInfo {
  char          address[18];
  float         distance;
  int           rssi;         // RSSI brut du dernier paquet (pour affichage)
  float         rssiMedian;   // médiane de la fenêtre glissante (valeur à relever pour calibrer)
  float         rssiEma;      // RSSI filtré médiane + EMA — base de toutes les décisions
  bool          hasRssi;      // au moins une mesure reçue (rssiEma valide)
  bool          outOfRange;   // état hystérésis latché, cf. beaconOutOfRange()
  bool          rangeSeeded;  // le latch a été amorcé sur une fenêtre médiane pleine
  uint8_t       rangeConfirm; // mesures consécutives plaidant pour le basculement
  int8_t        rssiWindow[RSSI_MEDIAN_WINDOW];
  uint8_t       rssiWindowCount;
  uint8_t       rssiWindowIdx;
  uint16_t      tagCrc;    // CRC16 MAC de la balise — identifiant compact publié dans l'advertising
  bool          paired;
  bool          snoozed;
  unsigned long lastSeenMs;
  unsigned long pairingStartMs;  // instant où la balise a été détectée "au contact" (0 si non en cours)

  bool isAlive() const {
    return (millis() - lastSeenMs) < BEACON_TIMEOUT_MS;
  }
};

class BeaconRegistry {
public:
  BeaconInfo beacons[MAX_BEACONS];
  int count = 0;

  BeaconInfo* find(const char* addr) {
    for (int i = 0; i < count; i++)
      if (strncmp(beacons[i].address, addr, 17) == 0) return &beacons[i];
    return nullptr;
  }

  BeaconInfo* findOrCreate(const char* addr) {
    BeaconInfo* b = find(addr);
    if (b) return b;
    if (count >= MAX_BEACONS) return nullptr;
    BeaconInfo& n = beacons[count++];
    strncpy(n.address, addr, 17);
    n.address[17]    = '\0';
    n.distance       = -1.0f;
    n.rssi           = 0;
    n.rssiMedian     = 0.0f;
    n.rssiEma        = 0.0f;
    n.hasRssi        = false;
    n.outOfRange     = true;  // hors périmètre tant qu'aucune mesure ne prouve le contraire
    n.rangeSeeded    = false;
    n.rangeConfirm   = 0;
    n.rssiWindowCount = 0;
    n.rssiWindowIdx   = 0;
    n.tagCrc         = 0;
    n.paired         = false;
    n.snoozed        = false;
    n.lastSeenMs     = 0;
    n.pairingStartMs = 0;
    return &n;
  }

  // Seules les balises appairées, vivantes et non-snoozées sont considérées
  BeaconInfo* nearestActive() {
    BeaconInfo* nearest = nullptr;
    for (int i = 0; i < count; i++) {
      BeaconInfo& b = beacons[i];
      if (!b.paired || b.snoozed || !b.isAlive() || !b.hasRssi) continue;
      if (!nearest || b.distance < nearest->distance) nearest = &b;
    }
    return nearest;
  }
};

// ---------------------------------------------------------------------------
// Filtrage du RSSI et décision de périmètre
// ---------------------------------------------------------------------------

// Intègre un paquet reçu : fenêtre glissante → médiane → EMA → distance.
void pushRssi(BeaconInfo* b, int rssi) {
  b->rssi = rssi;
  b->rssiWindow[b->rssiWindowIdx] = (int8_t)rssi;
  b->rssiWindowIdx = (uint8_t)((b->rssiWindowIdx + 1) % RSSI_MEDIAN_WINDOW);
  if (b->rssiWindowCount < RSSI_MEDIAN_WINDOW) b->rssiWindowCount++;

  // Médiane par tri par insertion sur une copie (fenêtre de 5 éléments)
  int8_t w[RSSI_MEDIAN_WINDOW];
  memcpy(w, b->rssiWindow, b->rssiWindowCount);
  for (uint8_t i = 1; i < b->rssiWindowCount; i++) {
    int8_t key = w[i];
    int j = (int)i - 1;
    while (j >= 0 && w[j] > key) { w[j + 1] = w[j]; j--; }
    w[j + 1] = key;
  }
  b->rssiMedian = (float)w[b->rssiWindowCount / 2];

  b->rssiEma = b->hasRssi
             ? RSSI_EMA_ALPHA * b->rssiMedian + (1.0f - RSSI_EMA_ALPHA) * b->rssiEma
             : b->rssiMedian;
  b->hasRssi  = true;

  // Amorçage du latch d'hystérésis, une fois la fenêtre médiane pleine (~0,8 s).
  //
  // Sans amorçage, une balise posée à 1,5 m — dans la bande d'hystérésis —
  // resterait indéfiniment en alerte : le latch démarre à true et ne se libère
  // qu'en deçà de ALERT_RSSI_EXIT (~1,4 m). Il faudrait rapprocher la canne pour
  // faire taire le vibreur, au démarrage du bracelet comme après la moindre
  // coupure BLE (le corps de l'utilisateur en provoque régulièrement).
  //
  // Attendre la fenêtre pleine plutôt que d'amorcer sur le premier paquet :
  // un échantillon isolé porte ±3 dB de bruit, soit ~14 % de risque d'amorcer
  // du mauvais côté du seuil ; la médiane de 5 ramène ce risque à ~2 %.
  if (!b->rangeSeeded && b->rssiWindowCount >= RSSI_MEDIAN_WINDOW) {
    // L'EMA accumulée pendant le remplissage part de médianes sur 1, 2, 3
    // échantillons — plus bruitées que la médiane pleine. On la réinitialise
    // sur celle-ci : amorçage plus sûr, et ré-acquisition sans traînée.
    b->rssiEma      = b->rssiMedian;
    b->outOfRange   = (b->rssiEma < ALERT_RSSI_ENTER);
    b->rangeSeeded  = true;
    b->rangeConfirm = 0;
  } else if (b->rangeSeeded) {
    // Hystérésis + confirmation. La décision avance au rythme des paquets reçus,
    // pas des tours de boucle : beaconOutOfRange() peut ainsi être appelé
    // librement par la LED, le vibreur et le calcul d'alerte sans fausser le
    // compteur ni désynchroniser leurs verdicts.
    bool candidate = b->outOfRange ? (b->rssiEma < ALERT_RSSI_EXIT)
                                   : (b->rssiEma < ALERT_RSSI_ENTER);
    if (candidate == b->outOfRange) {
      b->rangeConfirm = 0;
    } else if (++b->rangeConfirm >= RANGE_CONFIRM_SAMPLES) {
      b->outOfRange   = candidate;
      b->rangeConfirm = 0;
    }
  }

  b->distance = estimateDistance(b->rssiEma);
}

// Balise hors périmètre ? Simple lecture de l'état calculé par pushRssi.
// Sans effet de bord hormis la remise à zéro sur perte de signal : appelable
// autant de fois que voulu dans un même tour de boucle.
bool beaconOutOfRange(BeaconInfo& b) {
  // Plus de paquets ou aucune mesure : hors périmètre par défaut de sécurité.
  // La fenêtre est vidée — après BEACON_TIMEOUT_MS de silence les échantillons
  // ne décrivent plus rien, et les garder ralentirait la ré-acquisition.
  // hasRssi repasse à false : l'état redevient « inconnu », pas « loin », et
  // pushRssi réamorcera la décision sur la première mesure fraîche.
  if (!b.isAlive() || !b.hasRssi) {
    b.outOfRange      = true;
    b.rangeSeeded     = false;
    b.rangeConfirm    = 0;
    b.hasRssi         = false;
    b.rssiWindowCount = 0;
    b.rssiWindowIdx   = 0;
    return true;
  }

  // Fenêtre pas encore pleine après une reprise : état inconnu → alerte par
  // sécurité, sans latcher (pushRssi tranchera dès la fenêtre remplie).
  if (!b.rangeSeeded) return true;

  return b.outOfRange;
}

// ---------------------------------------------------------------------------
// Persistance des appairages en flash
// ---------------------------------------------------------------------------

// Fichier texte "/paired.txt" sur InternalFS, une adresse MAC par ligne.

// Ouvre en écrasant réellement le contenu précédent.
//
// Le BSP ouvre FILE_O_WRITE en LFS_O_RDWR|LFS_O_CREAT puis se place en fin de
// fichier (Adafruit_LittleFS_File.cpp:73) : sans truncate explicite, chaque
// sauvegarde s'ajoute aux précédentes. Les doublons étaient inoffensifs à la
// relecture — findOrCreate déduplique par adresse — ce qui a masqué le
// problème, mais un désappairage n'était jamais persisté : l'adresse retirée
// restait présente en queue de fichier et revenait au redémarrage suivant.
static bool openTruncated(File& f, const char* path) {
  if (!f.open(path, FILE_O_WRITE)) return false;
  f.truncate(0);
  f.seek(0);
  return true;
}

void savePairedBeacons(BeaconRegistry& registry) {
  if (!openTruncated(_scannerFlashFile, "/paired.txt")) {
    Serial.println("[flash] savePairedBeacons — erreur ouverture");
    return;
  }
  for (int i = 0; i < registry.count; i++) {
    if (registry.beacons[i].paired) {
      _scannerFlashFile.print(registry.beacons[i].address);
      _scannerFlashFile.print('\n');
    }
  }
  _scannerFlashFile.close();
  Serial.println("[flash] paired beacons sauvegardés");
}

void loadPairedBeacons(BeaconRegistry& registry) {
  if (!_scannerFlashFile.open("/paired.txt", FILE_O_READ)) return;
  char    line[18];
  uint8_t idx = 0;
  while (_scannerFlashFile.available()) {
    char c = (char)_scannerFlashFile.read();
    if (c == '\n' || idx >= 17) {
      if (idx == 17) {
        line[17] = '\0';
        BeaconInfo* b = registry.findOrCreate(line);
        if (b) b->paired = true;
      }
      idx = 0;
    } else {
      line[idx++] = c;
    }
  }
  _scannerFlashFile.close();
  Serial.println("[flash] paired beacons chargés");
}

// ---------------------------------------------------------------------------
// Retrait d'appairage
// ---------------------------------------------------------------------------

// Désappaire les balises que le bracelet ne détecte plus du tout : perdue,
// batterie à plat, laissée ailleurs. Les balises captées sont conservées, même
// hors périmètre — être hors du rayon n'est pas être perdu.
//
// Le critère est bien « non détectable » et non « en alerte » : une balise
// jamais revue depuis le démarrage a tagCrc == 0, elle n'est donc jamais
// désignée dans alertHash et un critère fondé sur l'alerte la manquerait
// précisément dans le cas qui nous intéresse.
//
// Retourne le nombre de balises oubliées.
int forgetUndetectedBeacons(BeaconRegistry& registry) {
  int forgotten = 0;
  for (int i = 0; i < registry.count; i++) {
    BeaconInfo& b = registry.beacons[i];
    if (!b.paired || b.isAlive()) continue;
    Serial.print("[oubli] balise non detectee : ");
    Serial.println(b.address);
    b.paired         = false;
    b.snoozed        = false;
    b.pairingStartMs = 0;
    forgotten++;
  }
  if (forgotten > 0) savePairedBeacons(registry);
  return forgotten;
}

// Efface tous les appairages. Le registre est vidé : les balises alentour
// seront redécouvertes au prochain paquet reçu, non appairées.
// Les balises, elles, conservent leur propre appairage en flash — elles
// l'écraseront au prochain contact d'appairage (cf. onScannerFound côté tag).
int factoryResetPairings(BeaconRegistry& registry) {
  int erased = 0;
  for (int i = 0; i < registry.count; i++) {
    if (registry.beacons[i].paired) erased++;
    registry.beacons[i].paired = false;
  }
  savePairedBeacons(registry);
  registry.count = 0;
  Serial.print("[reset] appairages effaces : ");
  Serial.println(erased);
  return erased;
}

// ---------------------------------------------------------------------------
// Appairage par proximité
// ---------------------------------------------------------------------------

// Retourne true si la balise vient d'être appairée lors de cet appel.
// La logique : si le RSSI brut dépasse PAIRING_RSSI_THRESHOLD pendant
// PAIRING_DURATION_MS sans interruption, la balise est appairée.
bool updatePairing(BeaconInfo* b, int rssi, unsigned long now) {
  if (b->paired) return false;

  if (rssi >= PAIRING_RSSI_THRESHOLD) {
    if (b->pairingStartMs == 0) {
      b->pairingStartMs = now;
      Serial.print("Appairage en cours : ");
      Serial.println(b->address);
    } else if (now - b->pairingStartMs >= PAIRING_DURATION_MS) {
      b->paired         = true;
      b->pairingStartMs = 0;
      Serial.print("Balise appairée : ");
      Serial.println(b->address);
      return true;
    }
  } else {
    // Contact rompu avant la fin du délai → reset
    if (b->pairingStartMs != 0) {
      Serial.println("Appairage annulé (balise éloignée trop tôt)");
      b->pairingStartMs = 0;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Commande ring via BLE GATT
// ---------------------------------------------------------------------------

// TODO: implémenter avec l'API Bluefruit Central (BLEClientService / BLEClientCharacteristic)
//       Nécessite une gestion de connexion asynchrone via callbacks.
//       Pour l'instant la commande est loguée mais pas envoyée.
bool sendRingCommand(const char* address, byte cmd) {
  Serial.print("[ring] commande 0x");
  Serial.print(cmd, HEX);
  Serial.print(" vers ");
  Serial.print(address);
  Serial.println(" — non implemente");
  return false;
}

void ringNearestBeacon(BeaconRegistry& registry) {
  BeaconInfo* nearest = registry.nearestActive();
  if (!nearest) {
    Serial.println("Aucune balise active à sonner");
    return;
  }
  Serial.print("Sonnerie → ");
  Serial.println(nearest->address);
  sendRingCommand(nearest->address, RING_CMD_START);
}

void stopRinging(BeaconRegistry& registry, const char* address) {
  sendRingCommand(address, RING_CMD_STOP);
}
