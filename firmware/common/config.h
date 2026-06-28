#pragma once

// Identifiants BLE (local name en advertising)
#define BEACON_LOCAL_NAME       "KC-Tag"
#define SCANNER_LOCAL_NAME      "KC-Scanner"

// Seuil RSSI pour détecter un module "collé" (~5-8 cm)
#define PAIRING_RSSI_THRESHOLD  -38

// Service GATT "ring" — commun à tous les tags
#define RING_SERVICE_UUID       "12345678-1234-1234-1234-123456789ABC"
#define RING_CMD_CHAR_UUID      "12345678-1234-1234-1234-123456789ABD"

// Valeurs de la commande ring
#define RING_CMD_START          0x01
#define RING_CMD_STOP           0x00
#define RING_CMD_PAIR           0x02  // confirmation d'appairage → LED verte 5 s sur le tag

// Seuils de proximité avec hystérésis (tag et scanner partagent ces valeurs)
// Vert → Rouge : distance dépasse FAR
// Rouge → Vert : distance repasse sous NEAR
#define PROXIMITY_NEAR_DISTANCE_M   0.45f
#define PROXIMITY_FAR_DISTANCE_M    0.55f

// Seuils d'alerte (mètres)
#define DISTANCE_WARNING            3.0f   // vibration / sonnerie au-delà de cette distance
#define DISTANCE_ALARM              6.0f

// Seuil RSSI pour l'alerte "hors de portée" (dBm, à calibrer empiriquement)
// Avec TX_POWER_AT_1M=-59 et PATH_LOSS_N=2, -70 dBm ≈ 3.5 m en espace libre.
// Utiliser ce seuil plutôt que la distance : moins sensible aux réflexions.
#define RSSI_WARNING            -70

// Exposant de perte de trajet (à calibrer selon l'environnement)
#define PATH_LOSS_N             2.0f

// Coefficient de lissage EMA du RSSI (0.0–1.0 : plus élevé = plus réactif, plus bruité)
#define RSSI_EMA_ALPHA          0.1f

// Identification du bracelet propriétaire dans l'advertising de la balise
// Company ID 0xFFFF = non-assigné (usage prototype)
#define PAIRING_MANUFACTURER_ID  0xFFFF

// CRC16-CCITT : identifie le bracelet propriétaire (6 octets MAC → uint16_t)
static inline uint16_t crc16(const uint8_t* data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++)
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (uint16_t)(crc << 1);
  }
  return crc;
}
