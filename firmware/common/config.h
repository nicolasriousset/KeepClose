#pragma once

// Identifiants BLE (local name en advertising)
#define BEACON_LOCAL_NAME       "KC-Tag"
#define SCANNER_LOCAL_NAME      "KC-Scanner"

// Seuil RSSI pour détecter un module "collé" (~5-8 cm)
#define PAIRING_RSSI_THRESHOLD  -38

// ── Puissance d'émission BLE ─────────────────────────────────────────────────
// Réglée explicitement des deux côtés. C'était auparavant la valeur par défaut
// du BSP (CFG_BLE_TX_POWER_LEVEL = 0 dBm) : identique, mais implicite — une mise
// à jour du board package aurait décalé tous les RSSI sans prévenir.
//
// ⚠️ Toute modification décale les RSSI mesurés de la même quantité, en dB.
//    Il faut alors reporter ce décalage sur RSSI_AT_1M, ALERT_RSSI_ENTER,
//    ALERT_RSSI_EXIT et PAIRING_RSSI_THRESHOLD, sinon le périmètre d'alerte
//    change silencieusement. Passer de 0 à +4 dBm, par exemple, impose
//    RSSI_AT_1M = -68 pour conserver l'alerte à 2 m.
//
// Valeurs acceptées par le nRF52840 : -40 -20 -16 -12 -8 -4 0 2 3 4 5 6 7 8
#define BLE_TX_POWER            0

// Service GATT "ring" — commun à tous les tags
#define RING_SERVICE_UUID       "12345678-1234-1234-1234-123456789ABC"
#define RING_CMD_CHAR_UUID      "12345678-1234-1234-1234-123456789ABD"

// Valeurs de la commande ring
#define RING_CMD_START          0x01
#define RING_CMD_STOP           0x00
#define RING_CMD_PAIR           0x02  // confirmation d'appairage → LED verte 5 s sur le tag

// ── Modèle de propagation RSSI → distance ────────────────────────────────────
//
//   RSSI(d) = RSSI_AT_1M − 10 · PATH_LOSS_N · log10(d)
//
// Calibré le 2026-08-23 sur deux points mesurés (intérieur, bracelet porté) :
//   contact ~5 cm → −38 dBm        3,25 m en ligne droite → −86 dBm
//   ⇒ PATH_LOSS_N = (86−38) / (10 · log10(3,25/0,05)) = 2,65
//     RSSI_AT_1M  = −38 − 10 · 2,65 · log10(0,05)     = −72
//
// RSSI_AT_1M n'est PAS la puissance d'émission : c'est le RSSI *reçu* à 1 m,
// antenne chip du XIAO et absorption du corps incluses. Les valeurs des tables
// iBeacon génériques (−59 / −61 dBm) supposent une antenne externe en espace
// libre ; les utiliser ici surestimait la distance d'un facteur ~3,5
// (3,25 m réels affichés 18 m) — la conversion étant exponentielle, une erreur
// de 11 dB devient un facteur 10^(11/20) ≈ 3,5 sur les mètres.
//
// Procédure de recalibrage : voir README.md § « Estimation de distance ».
#define RSSI_AT_1M              -72
#define PATH_LOSS_N             2.65f

// ── Seuil d'alerte « hors périmètre » ────────────────────────────────────────
//
// La décision se prend en dBm, pas en mètres. Le seuil est le même (la
// conversion est monotone), mais une erreur de calibration est un simple
// décalage constant en dBm — corrigeable par une seule mesure terrain — alors
// qu'en mètres c'est une erreur multiplicative qui croît avec la distance.
// La distance reste calculée pour l'affichage dans les logs.
//
// Hystérésis de 4 dB. Le bruit RSSI est de ±3 dB typiques même à l'arrêt, donc
// une bande plus étroite ferait osciller la LED et le vibreur — mais une bande
// plus large est pire : elle se comporte en cliquet à sens unique. Une balise
// immobile *dans* la bande finit toujours par voir un creux de bruit franchir
// ENTER, et ne peut alors plus revenir puisque le signal réel n'atteint jamais
// EXIT. Mesuré en simulation à 1,5 m sur 90 s : 6 % de blocages en alerte avec
// EXIT à -74 dBm, 0 % à -76.
#define ALERT_RSSI_ENTER        -80   // vert → rouge au-delà de ce niveau (≈ 2,0 m)
#define ALERT_RSSI_EXIT         -76   // rouge → vert en deçà de ce niveau (≈ 1,4 m)

// Nombre de mesures consécutives au-delà du seuil avant de basculer.
// L'hystérésis seule ne suffit pas : à l'intérieur de la bande (1,2–2,0 m) un
// seul creux de bruit fait franchir ENTER, et l'état est alors piégé — le
// signal réel n'y remonte jamais jusqu'à EXIT. La confirmation coûte ~0,5 s de
// latence et supprime ces bascules parasites.
#define RANGE_CONFIRM_SAMPLES   3

// ── Filtrage du RSSI ─────────────────────────────────────────────────────────
// Médiane glissante, puis EMA sur la médiane.
// La médiane élimine les paquets aberrants (un trajet multiple isolé décale un
// paquet de 10 dB) que l'EMA seule intégrait dans sa moyenne. La médiane
// absorbant déjà les valeurs extrêmes, l'EMA peut être plus réactive qu'avant
// (0,25 au lieu de 0,10) : l'alerte se déclenche ~2,5× plus vite.
#define RSSI_MEDIAN_WINDOW      5
#define RSSI_EMA_ALPHA          0.25f

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
