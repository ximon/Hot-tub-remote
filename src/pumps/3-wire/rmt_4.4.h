
#include "driver/rmt.h"
/**
 * @brief Default configuration for Tx channel
 *
 */
rmt_config_t RMT_DEFAULT_CONFIG_TX(int gpio, int channel_id)
{
    rmt_config_t conf = {
        .rmt_mode = RMT_MODE_TX,
        .channel = channel_id,
        .gpio_num = gpio,
        .clk_div = 80,
        .mem_block_num = 1,
        //.flags = 0,
        .tx_config = {
            .carrier_freq_hz = 38000,
            .carrier_level = RMT_CARRIER_LEVEL_HIGH,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .carrier_duty_percent = 33,
            .carrier_en = false,
            .loop_en = false,
            .idle_output_en = true,
        }};
    return conf;
}

/**
 * @brief Default configuration for RX channel
 *
 */
rmt_config_t RMT_DEFAULT_CONFIG_RX(int gpio, int channel_id)
{
    rmt_config_t conf = {
        .rmt_mode = RMT_MODE_RX,
        .channel = channel_id,
        .gpio_num = gpio,
        .clk_div = 80,
        .mem_block_num = 1,
        //.flags = 0,
        .rx_config = {
            .idle_threshold = 12000,
            .filter_ticks_thresh = 100,
            .filter_en = true,
        }};

    return conf;
}
