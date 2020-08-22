/*************************************/
/* Pin Count: 3                      */
/* Model No: #54075                  */
/* Protocol: one wire serial         */
/* Colour: Blue                      */
/* Notes: Semi-circular display      */
/*************************************/

#include "54075.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "rmt_4.4.h"
#include "ir_tools.h"
#include "freertos/task.h"

static const char *TAG = "3-wire-54075";

//static rmt_channel_t example_tx_channel = RMT_CHANNEL_0;
static rmt_channel_t example_rx_channel = RMT_CHANNEL_1;

static void example_ir_rx_task(void *arg)
{
    uint32_t addr = 0;
    uint32_t cmd = 0;
    uint32_t length = 0;
    bool repeat = false;
    RingbufHandle_t rb = NULL;
    rmt_item32_t *items = NULL;

    rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX(18, example_rx_channel);

    rmt_config(&rmt_rx_config);
    rmt_driver_install(example_rx_channel, 1000, 0);
    ir_parser_config_t ir_parser_config = IR_PARSER_DEFAULT_CONFIG((ir_dev_t)example_rx_channel);
    ir_parser_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5 have extended version)

    ir_parser_t *ir_parser = ir_parser_rmt_new_nec(&ir_parser_config);

    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(example_rx_channel, &rb);
    // Start receive
    rmt_rx_start(example_rx_channel, true);
    while (rb)
    {
        items = (rmt_item32_t *)xRingbufferReceive(rb, &length, 1000);
        if (items)
        {
            length /= 4; // one RMT = 4 Bytes
            if (ir_parser->input(ir_parser, items, length) == ESP_OK)
            {
                if (ir_parser->get_scan_code(ir_parser, &addr, &cmd, &repeat) == ESP_OK)
                {
                    ESP_LOGI(TAG, "Scan Code %s --- addr: 0x%04x cmd: 0x%04x", repeat ? "(repeat)" : "", addr, cmd);
                }
            }
            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(rb, (void *)items);
        }
        else
        {
            break;
        }
    }
    ir_parser->del(ir_parser);
    rmt_driver_uninstall(example_rx_channel);
    vTaskDelete(NULL);
}

void setupPump()
{
    ESP_LOGI(TAG, "Setting up 3 wire pump #54075");

    xTaskCreate(example_ir_rx_task, "ir_rx_task", 2048, NULL, 10, NULL);
}
