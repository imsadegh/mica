/**
 * @file i2s_microphone.c
 * @brief High-quality I2S microphone driver implementation for INMP441
 */

#include "i2s_microphone.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

static const char *TAG = "I2S_MIC";

// I2S handle
static i2s_chan_handle_t rx_handle = NULL;

// Configuration
static i2s_mic_config_t mic_config;
static audio_stats_t stats = {0};
static bool is_initialized = false;
static bool is_running = false;

/**
 * @brief Convert 24-bit data to 32-bit signed integer
 */
static inline int32_t convert_24bit_to_32bit(const uint8_t *data) {
    int32_t value = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
    return value; // Sign-extended 24-bit to 32-bit
}

/**
 * @brief Apply gain to audio sample
 */
static inline int32_t apply_gain(int32_t sample, float gain) {
    int64_t result = (int64_t)sample * gain;

    // Clamp to 32-bit range
    if (result > INT32_MAX) return INT32_MAX;
    if (result < INT32_MIN) return INT32_MIN;

    return (int32_t)result;
}

/**
 * @brief Update audio statistics
 */
static void update_stats(const int32_t *samples, size_t num_samples) {
    int64_t sum_squares = 0;
    int32_t peak = 0;

    for (size_t i = 0; i < num_samples; i++) {
        int32_t abs_sample = abs(samples[i]);
        if (abs_sample > peak) {
            peak = abs_sample;
        }

        // Accumulate for RMS calculation
        int64_t normalized = samples[i] >> 16; // Scale down to prevent overflow
        sum_squares += normalized * normalized;
    }

    stats.samples_read += num_samples;
    stats.peak_amplitude = peak;

    if (num_samples > 0) {
        stats.rms_level = sqrtf((float)sum_squares / num_samples) * 65536.0f;
    }
}

esp_err_t i2s_mic_init(const i2s_mic_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (is_initialized) {
        ESP_LOGW(TAG, "Already initialized, deinitializing first");
        i2s_mic_deinit();
    }

    // Save configuration
    memcpy(&mic_config, config, sizeof(i2s_mic_config_t));

    ESP_LOGI(TAG, "Initializing I2S microphone:");
    ESP_LOGI(TAG, "  Sample Rate: %d Hz", config->sample_rate);
    ESP_LOGI(TAG, "  Bit Depth: %d bits", config->bit_depth);
    ESP_LOGI(TAG, "  Pins - SCK: %d, WS: %d, SD: %d",
             config->sck_pin, config->ws_pin, config->sd_pin);

    // Configure I2S channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = config->dma_buf_count;
    chan_cfg.dma_frame_num = config->dma_buf_len;
    chan_cfg.auto_clear = true;

    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure I2S standard mode for INMP441
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = config->sample_rate,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            (i2s_data_bit_width_t)config->bit_depth,
            I2S_SLOT_MODE_MONO
        ),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config->sck_pin,
            .ws = config->ws_pin,
            .dout = I2S_GPIO_UNUSED,
            .din = config->sd_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // Configure slot for left channel (INMP441 on L/R = GND)
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    ret = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S standard mode: %s", esp_err_to_name(ret));
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
        return ret;
    }

    // Reset statistics
    memset(&stats, 0, sizeof(audio_stats_t));

    is_initialized = true;
    ESP_LOGI(TAG, "I2S microphone initialized successfully");

    return ESP_OK;
}

esp_err_t i2s_mic_deinit(void) {
    if (!is_initialized) {
        return ESP_OK;
    }

    if (is_running) {
        i2s_mic_stop();
    }

    if (rx_handle != NULL) {
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
    }

    is_initialized = false;
    ESP_LOGI(TAG, "I2S microphone deinitialized");

    return ESP_OK;
}

esp_err_t i2s_mic_start(void) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (is_running) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }

    esp_err_t ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    is_running = true;
    ESP_LOGI(TAG, "I2S microphone started");

    return ESP_OK;
}

esp_err_t i2s_mic_stop(void) {
    if (!is_running) {
        return ESP_OK;
    }

    esp_err_t ret = i2s_channel_disable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    is_running = false;
    ESP_LOGI(TAG, "I2S microphone stopped");

    return ESP_OK;
}

esp_err_t i2s_mic_read(void *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (buffer == NULL || bytes_read == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_running) {
        ESP_LOGE(TAG, "Not running, call i2s_mic_start() first");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2s_channel_read(rx_handle, buffer, buffer_size, bytes_read,
                                     pdMS_TO_TICKS(timeout_ms));

    if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "Failed to read from I2S: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t i2s_mic_read_samples(int32_t *samples, size_t num_samples,
                               size_t *samples_read, uint32_t timeout_ms) {
    if (!is_initialized || !is_running) {
        ESP_LOGE(TAG, "Not initialized or not running");
        return ESP_ERR_INVALID_STATE;
    }

    if (samples == NULL || samples_read == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    size_t bytes_per_sample = mic_config.bit_depth / 8;
    size_t buffer_size = num_samples * bytes_per_sample;
    uint8_t *raw_buffer = malloc(buffer_size);

    if (raw_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = 0;
    esp_err_t ret = i2s_mic_read(raw_buffer, buffer_size, &bytes_read, timeout_ms);

    if (ret == ESP_OK || ret == ESP_ERR_TIMEOUT) {
        *samples_read = bytes_read / bytes_per_sample;

        // Convert samples based on bit depth
        if (mic_config.bit_depth == AUDIO_BIT_DEPTH_16) {
            int16_t *raw_samples = (int16_t *)raw_buffer;
            for (size_t i = 0; i < *samples_read; i++) {
                samples[i] = apply_gain((int32_t)raw_samples[i] << 16, mic_config.gain);
            }
        } else if (mic_config.bit_depth == AUDIO_BIT_DEPTH_24) {
            for (size_t i = 0; i < *samples_read; i++) {
                int32_t sample = convert_24bit_to_32bit(&raw_buffer[i * 3]);
                samples[i] = apply_gain(sample, mic_config.gain);
            }
        } else if (mic_config.bit_depth == AUDIO_BIT_DEPTH_32) {
            int32_t *raw_samples = (int32_t *)raw_buffer;
            for (size_t i = 0; i < *samples_read; i++) {
                samples[i] = apply_gain(raw_samples[i], mic_config.gain);
            }
        }

        // Update statistics
        update_stats(samples, *samples_read);
    }

    free(raw_buffer);
    return ret;
}

esp_err_t i2s_mic_get_stats(audio_stats_t *stats_out) {
    if (stats_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(stats_out, &stats, sizeof(audio_stats_t));
    return ESP_OK;
}

void i2s_mic_reset_stats(void) {
    memset(&stats, 0, sizeof(audio_stats_t));
}

esp_err_t i2s_mic_set_gain(float gain) {
    if (gain < 0.0f || gain > 10.0f) {
        ESP_LOGE(TAG, "Gain must be between 0.0 and 10.0");
        return ESP_ERR_INVALID_ARG;
    }

    mic_config.gain = gain;
    ESP_LOGI(TAG, "Gain set to %.2f", gain);

    return ESP_OK;
}
