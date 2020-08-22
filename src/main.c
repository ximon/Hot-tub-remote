#include "pumps/3-wire/54075.h"

/*                                D  T
 * Stage 1 - Logging             [ ][ ]
 *
 * Stage 2a - Pump  -> ESP       [ ][ ]
 *       2b   Pump <-> ESP       [ ][ ]
 *
 * Stage 3a - HTTP API           [ ][ ]
 *        b - MQTT               [ ][ ]
 *
 */
#include "esp_log.h"
#include <MQTT.h>

static const char *TAG = "Main";

void setup()
{
    ESP_LOGI(TAG, "Starting up....");

    setupPump();
    setupWiFi();
    //webServerSetup();
    //webSocketSetup();
    //mqttClientSetup();
}

void loop()
{
    processWiFi();
}