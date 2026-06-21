#pragma once

// Identifiant des balises (local name affiché en advertising)
#define BEACON_LOCAL_NAME       "KC-Tag"

// Service GATT "ring" — commun à tous les tags
#define RING_SERVICE_UUID       "12345678-1234-1234-1234-123456789ABC"
#define RING_CMD_CHAR_UUID      "12345678-1234-1234-1234-123456789ABD"

// Valeurs de la commande ring
#define RING_CMD_START          0x01
#define RING_CMD_STOP           0x00

// Seuils d'alerte (mètres)
#define DISTANCE_WARNING        3.0f
#define DISTANCE_ALARM          6.0f

// Exposant de perte de trajet (à calibrer selon l'environnement)
#define PATH_LOSS_N             2.0f
