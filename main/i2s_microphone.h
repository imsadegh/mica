/**
 * @file i2s_microphone.h
 * @brief High-quality I2S microphone driver for INMP441 with ESP32-C3
 *
 * This driver provides optimized audio capture from INMP441 MEMS microphone
 * with support for multiple sample rates and bit depths.
 */

#ifndef I2S_MICROPHONE_H
#define I2S_MICROPHONE_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2s_std.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio sample rates supported
 */
typedef enum {
    AUDIO_SAMPLE_RATE_16K = 16000,
    AUDIO_SAMPLE_RATE_22K = 22050,
    AUDIO_SAMPLE_RATE_44K = 44100,
    AUDIO_SAMPLE_RATE_48K = 48000,
} audio_sample_rate_t;

/**
 * @brief Audio bit depth
 */
typedef enum {
    AUDIO_BIT_DEPTH_16 = 16,
    AUDIO_BIT_DEPTH_24 = 24,
    AUDIO_BIT_DEPTH_32 = 32,
} audio_bit_depth_t;

/**
 * @brief I2S microphone configuration
 */
typedef struct {
    // I2S pins
    gpio_num_t sck_pin;     ///< Serial Clock (SCK/BCLK)
    gpio_num_t ws_pin;      ///< Word Select (WS/LRCLK)
    gpio_num_t sd_pin;      ///< Serial Data (SD/DIN)

    // Audio settings
    audio_sample_rate_t sample_rate;  ///< Audio sample rate in Hz
    audio_bit_depth_t bit_depth;      ///< Bit depth per sample

    // Buffer settings
    uint32_t dma_buf_count;   ///< Number of DMA buffers (2-128)
    uint32_t dma_buf_len;     ///< Length of each DMA buffer in samples (8-1024)

    // Processing
    bool enable_aec;          ///< Enable Acoustic Echo Cancellation
    bool enable_agc;          ///< Enable Automatic Gain Control
    float gain;               ///< Gain multiplier (1.0 = unity gain)
} i2s_mic_config_t;

/**
 * @brief Audio statistics
 */
typedef struct {
    uint32_t samples_read;
    uint32_t buffer_overruns;
    uint32_t buffer_underruns;
    int32_t peak_amplitude;
    float rms_level;
} audio_stats_t;

/**
 * @brief Initialize I2S microphone
 *
 * @param config Pointer to microphone configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_init(const i2s_mic_config_t *config);

/**
 * @brief Deinitialize I2S microphone
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_deinit(void);

/**
 * @brief Read audio samples from microphone
 *
 * @param buffer Buffer to store audio samples
 * @param buffer_size Size of buffer in bytes
 * @param bytes_read Pointer to store actual bytes read
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_read(void *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Read audio samples as 32-bit integers (normalized)
 *
 * @param samples Array to store samples
 * @param num_samples Number of samples to read
 * @param samples_read Pointer to store actual samples read
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_read_samples(int32_t *samples, size_t num_samples, size_t *samples_read, uint32_t timeout_ms);

/**
 * @brief Get audio statistics
 *
 * @param stats Pointer to store statistics
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_get_stats(audio_stats_t *stats);

/**
 * @brief Reset audio statistics
 */
void i2s_mic_reset_stats(void);

/**
 * @brief Start audio capture
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_start(void);

/**
 * @brief Stop audio capture
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_stop(void);

/**
 * @brief Set microphone gain
 *
 * @param gain Gain multiplier (0.0 to 10.0)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2s_mic_set_gain(float gain);

#ifdef __cplusplus
}
#endif

#endif // I2S_MICROPHONE_H
