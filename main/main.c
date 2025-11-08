/**
 * @file main.c
 * @brief High-quality INMP441 microphone example for ESP32-C3
 *
 * This example demonstrates various use cases for the INMP441 MEMS microphone:
 * - Basic audio capture
 * - Real-time audio level monitoring
 * - High-quality audio streaming
 * - Audio statistics and analysis
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "i2s_microphone.h"

static const char *TAG = "MAIN";

// GPIO pins for ESP32-C3 with INMP441
#define I2S_SCK_PIN     GPIO_NUM_4   // Serial Clock (BCLK)
#define I2S_WS_PIN      GPIO_NUM_5   // Word Select (LRCLK)
#define I2S_SD_PIN      GPIO_NUM_6   // Serial Data (DOUT from INMP441)

// Audio configuration - Select one based on your use case
// Uncomment the desired configuration below

// Configuration 1: High-quality voice (recommended for voice applications)
#define USE_VOICE_CONFIG

// Configuration 2: Music quality (high fidelity)
// #define USE_MUSIC_CONFIG

// Configuration 3: Low latency (real-time applications)
// #define USE_LOW_LATENCY_CONFIG

// Configuration 4: Ultra high quality (maximum fidelity)
// #define USE_ULTRA_CONFIG

#ifdef USE_VOICE_CONFIG
    #define SAMPLE_RATE     AUDIO_SAMPLE_RATE_16K
    #define BIT_DEPTH       AUDIO_BIT_DEPTH_32
    #define DMA_BUF_COUNT   6
    #define DMA_BUF_LEN     512
    #define CONFIG_NAME     "Voice Quality"
#elif defined(USE_MUSIC_CONFIG)
    #define SAMPLE_RATE     AUDIO_SAMPLE_RATE_44K
    #define BIT_DEPTH       AUDIO_BIT_DEPTH_32
    #define DMA_BUF_COUNT   8
    #define DMA_BUF_LEN     1024
    #define CONFIG_NAME     "Music Quality"
#elif defined(USE_LOW_LATENCY_CONFIG)
    #define SAMPLE_RATE     AUDIO_SAMPLE_RATE_16K
    #define BIT_DEPTH       AUDIO_BIT_DEPTH_16
    #define DMA_BUF_COUNT   4
    #define DMA_BUF_LEN     256
    #define CONFIG_NAME     "Low Latency"
#elif defined(USE_ULTRA_CONFIG)
    #define SAMPLE_RATE     AUDIO_SAMPLE_RATE_48K
    #define BIT_DEPTH       AUDIO_BIT_DEPTH_32
    #define DMA_BUF_COUNT   8
    #define DMA_BUF_LEN     1024
    #define CONFIG_NAME     "Ultra Quality"
#else
    // Default to voice config
    #define SAMPLE_RATE     AUDIO_SAMPLE_RATE_16K
    #define BIT_DEPTH       AUDIO_BIT_DEPTH_32
    #define DMA_BUF_COUNT   6
    #define DMA_BUF_LEN     512
    #define CONFIG_NAME     "Default Voice"
#endif

// Example selection - Choose which example to run
typedef enum {
    EXAMPLE_BASIC_CAPTURE,          // Basic audio capture and logging
    EXAMPLE_LEVEL_METER,            // Real-time audio level monitoring
    EXAMPLE_STREAM,                 // Continuous audio streaming
    EXAMPLE_STATISTICS,             // Detailed audio statistics
    EXAMPLE_VOICE_DETECTION,        // Voice activity detection
} example_mode_t;

// Select the example to run
#define EXAMPLE_MODE    EXAMPLE_LEVEL_METER

/**
 * @brief Convert dBFS (decibels relative to full scale)
 */
static float calculate_dbfs(int32_t sample) {
    if (sample == 0) return -96.0f;

    float normalized = (float)abs(sample) / (float)INT32_MAX;
    return 20.0f * log10f(normalized);
}

/**
 * @brief Example 1: Basic audio capture
 */
static void example_basic_capture(void) {
    ESP_LOGI(TAG, "Running basic audio capture example");

    int32_t samples[512];
    size_t samples_read = 0;

    while (1) {
        esp_err_t ret = i2s_mic_read_samples(samples, 512, &samples_read, 1000);

        if (ret == ESP_OK) {
            // Find peak sample
            int32_t peak = 0;
            for (size_t i = 0; i < samples_read; i++) {
                if (abs(samples[i]) > abs(peak)) {
                    peak = samples[i];
                }
            }

            float db = calculate_dbfs(peak);
            ESP_LOGI(TAG, "Read %zu samples, Peak: %"PRId32" (%.1f dBFS)",
                     samples_read, peak, db);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Example 2: Real-time audio level meter with visual bar
 */
static void example_level_meter(void) {
    ESP_LOGI(TAG, "Running audio level meter example");

    int32_t samples[256];
    size_t samples_read = 0;
    const int bar_width = 50;

    while (1) {
        esp_err_t ret = i2s_mic_read_samples(samples, 256, &samples_read, 100);

        if (ret == ESP_OK && samples_read > 0) {
            // Calculate RMS level
            int64_t sum_squares = 0;
            int32_t peak = 0;

            for (size_t i = 0; i < samples_read; i++) {
                int64_t normalized = samples[i] >> 16;
                sum_squares += normalized * normalized;

                if (abs(samples[i]) > abs(peak)) {
                    peak = samples[i];
                }
            }

            float rms = sqrtf((float)sum_squares / samples_read) * 65536.0f;
            float rms_db = calculate_dbfs((int32_t)rms);
            float peak_db = calculate_dbfs(peak);

            // Create visual bar (normalized to -60 dB to 0 dB range)
            int bar_level = (int)(((peak_db + 60.0f) / 60.0f) * bar_width);
            if (bar_level < 0) bar_level = 0;
            if (bar_level > bar_width) bar_level = bar_width;

            char bar[bar_width + 1];
            memset(bar, '=', bar_level);
            memset(bar + bar_level, ' ', bar_width - bar_level);
            bar[bar_width] = '\0';

            printf("\rLevel: [%s] Peak: %6.1f dB | RMS: %6.1f dB  ",
                   bar, peak_db, rms_db);
            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Update 20 times per second
    }
}

/**
 * @brief Example 3: Continuous audio streaming
 */
static void example_stream(void) {
    ESP_LOGI(TAG, "Running continuous streaming example");

    int32_t samples[1024];
    size_t samples_read = 0;
    uint32_t total_samples = 0;
    int64_t start_time = esp_timer_get_time();

    while (1) {
        esp_err_t ret = i2s_mic_read_samples(samples, 1024, &samples_read, 1000);

        if (ret == ESP_OK) {
            total_samples += samples_read;

            // Here you would process/stream the audio data
            // For example: send over WiFi, save to SD card, etc.

            // Log statistics every 5 seconds
            int64_t elapsed = esp_timer_get_time() - start_time;
            if (elapsed >= 5000000) { // 5 seconds
                float duration = elapsed / 1000000.0f;
                float samples_per_sec = total_samples / duration;

                ESP_LOGI(TAG, "Streaming: %.2f samples/sec (expected: %d)",
                         samples_per_sec, SAMPLE_RATE);

                start_time = esp_timer_get_time();
                total_samples = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // Minimal delay for streaming
    }
}

/**
 * @brief Example 4: Detailed audio statistics
 */
static void example_statistics(void) {
    ESP_LOGI(TAG, "Running statistics example");

    int32_t samples[512];
    size_t samples_read = 0;

    while (1) {
        esp_err_t ret = i2s_mic_read_samples(samples, 512, &samples_read, 1000);

        if (ret == ESP_OK) {
            audio_stats_t stats;
            i2s_mic_get_stats(&stats);

            float peak_db = calculate_dbfs(stats.peak_amplitude);
            float rms_db = calculate_dbfs((int32_t)stats.rms_level);

            ESP_LOGI(TAG, "Statistics:");
            ESP_LOGI(TAG, "  Total samples: %"PRIu32, stats.samples_read);
            ESP_LOGI(TAG, "  Peak amplitude: %"PRId32" (%.1f dBFS)",
                     stats.peak_amplitude, peak_db);
            ESP_LOGI(TAG, "  RMS level: %.0f (%.1f dBFS)",
                     stats.rms_level, rms_db);
            ESP_LOGI(TAG, "  Buffer overruns: %"PRIu32, stats.buffer_overruns);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // Update every 2 seconds
    }
}

/**
 * @brief Example 5: Voice activity detection
 */
static void example_voice_detection(void) {
    ESP_LOGI(TAG, "Running voice activity detection example");

    int32_t samples[256];
    size_t samples_read = 0;
    const float VOICE_THRESHOLD_DB = -40.0f; // Adjust based on environment
    bool voice_active = false;

    while (1) {
        esp_err_t ret = i2s_mic_read_samples(samples, 256, &samples_read, 100);

        if (ret == ESP_OK && samples_read > 0) {
            // Calculate energy
            int64_t energy = 0;
            for (size_t i = 0; i < samples_read; i++) {
                int64_t normalized = samples[i] >> 16;
                energy += normalized * normalized;
            }

            float rms = sqrtf((float)energy / samples_read) * 65536.0f;
            float db = calculate_dbfs((int32_t)rms);

            bool voice_detected = (db > VOICE_THRESHOLD_DB);

            if (voice_detected && !voice_active) {
                ESP_LOGI(TAG, "Voice detected! (%.1f dB)", db);
                voice_active = true;
            } else if (!voice_detected && voice_active) {
                ESP_LOGI(TAG, "Silence detected");
                voice_active = false;
            }

            if (voice_active) {
                printf("\r[VOICE] Level: %.1f dB  ", db);
            } else {
                printf("\r[QUIET] Level: %.1f dB  ", db);
            }
            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "ESP32-C3 INMP441 Microphone Example");
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "Configuration: %s", CONFIG_NAME);
    ESP_LOGI(TAG, "Sample Rate: %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "Bit Depth: %d bits", BIT_DEPTH);
    ESP_LOGI(TAG, "====================================");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configure microphone
    i2s_mic_config_t mic_config = {
        .sck_pin = I2S_SCK_PIN,
        .ws_pin = I2S_WS_PIN,
        .sd_pin = I2S_SD_PIN,
        .sample_rate = SAMPLE_RATE,
        .bit_depth = BIT_DEPTH,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .enable_aec = false,
        .enable_agc = false,
        .gain = 1.0f,  // Unity gain, adjust as needed (0.1 to 10.0)
    };

    // Initialize microphone
    ret = i2s_mic_init(&mic_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize microphone: %s", esp_err_to_name(ret));
        return;
    }

    // Start audio capture
    ret = i2s_mic_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start microphone: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Microphone started successfully!");
    ESP_LOGI(TAG, "Running example mode: %d", EXAMPLE_MODE);

    // Wait for I2S to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Run selected example
    switch (EXAMPLE_MODE) {
        case EXAMPLE_BASIC_CAPTURE:
            example_basic_capture();
            break;

        case EXAMPLE_LEVEL_METER:
            example_level_meter();
            break;

        case EXAMPLE_STREAM:
            example_stream();
            break;

        case EXAMPLE_STATISTICS:
            example_statistics();
            break;

        case EXAMPLE_VOICE_DETECTION:
            example_voice_detection();
            break;

        default:
            ESP_LOGE(TAG, "Unknown example mode");
            break;
    }
}
