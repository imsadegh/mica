# Configuration Guide

This guide explains how to configure the INMP441 microphone driver for different use cases.

## Audio Quality Presets

The project includes 4 pre-configured quality presets in `main/main.c`. Select one by uncommenting the corresponding define:

### 1. Voice Quality (Default)

```c
#define USE_VOICE_CONFIG
```

**Specifications:**
- Sample Rate: 16,000 Hz
- Bit Depth: 32-bit
- DMA Buffers: 6 × 512 samples
- Buffer Size: 12 KB
- Latency: ~96 ms

**Best for:**
- Voice recording
- Speech recognition
- Voice commands (Alexa, Google Assistant style)
- VoIP applications
- Dictation

**Why this configuration:**
- 16 kHz is sufficient for voice (human voice: 85 Hz - 8 kHz)
- Lower sample rate = less CPU usage
- Good balance of quality and performance
- Low memory footprint

### 2. Music Quality

```c
#define USE_MUSIC_CONFIG
```

**Specifications:**
- Sample Rate: 44,100 Hz
- Bit Depth: 32-bit
- DMA Buffers: 8 × 1024 samples
- Buffer Size: 32 KB
- Latency: ~185 ms

**Best for:**
- Music recording
- Ambient sound capture
- High-fidelity applications
- Audio analysis
- Professional recording

**Why this configuration:**
- 44.1 kHz captures full human hearing range (20 Hz - 20 kHz)
- CD-quality audio
- Larger buffers for stability
- Higher CPU usage but excellent quality

### 3. Low Latency

```c
#define USE_LOW_LATENCY_CONFIG
```

**Specifications:**
- Sample Rate: 16,000 Hz
- Bit Depth: 16-bit
- DMA Buffers: 4 × 256 samples
- Buffer Size: 2 KB
- Latency: ~32 ms

**Best for:**
- Real-time audio effects
- Live audio monitoring
- Interactive applications
- Audio feedback systems
- Gaming audio input

**Why this configuration:**
- Minimal latency for real-time response
- Small buffers = quick response
- Lower bit depth = faster processing
- Reduced memory usage

### 4. Ultra Quality

```c
#define USE_ULTRA_CONFIG
```

**Specifications:**
- Sample Rate: 48,000 Hz
- Bit Depth: 32-bit
- DMA Buffers: 8 × 1024 samples
- Buffer Size: 32 KB
- Latency: ~171 ms

**Best for:**
- Professional audio recording
- Scientific measurements
- High-fidelity capture
- Archival recordings
- Audio research

**Why this configuration:**
- 48 kHz is professional studio standard
- Maximum dynamic range (32-bit)
- Captures ultrasonic content near 20 kHz
- Highest quality available

## Custom Configuration

To create your own configuration, modify the defines in `main/main.c`:

```c
#define SAMPLE_RATE     AUDIO_SAMPLE_RATE_16K  // Your choice
#define BIT_DEPTH       AUDIO_BIT_DEPTH_32     // Your choice
#define DMA_BUF_COUNT   6                      // 2-128
#define DMA_BUF_LEN     512                    // 8-1024
#define CONFIG_NAME     "My Custom Config"
```

### Sample Rate Selection

Available sample rates:

| Constant | Rate (Hz) | Use Case |
|----------|-----------|----------|
| `AUDIO_SAMPLE_RATE_16K` | 16,000 | Voice, speech |
| `AUDIO_SAMPLE_RATE_22K` | 22,050 | Low-bandwidth music |
| `AUDIO_SAMPLE_RATE_44K` | 44,100 | CD quality |
| `AUDIO_SAMPLE_RATE_48K` | 48,000 | Professional audio |

**Trade-offs:**
- Higher rate = better quality, more CPU, more memory
- Lower rate = less quality, less CPU, less memory

### Bit Depth Selection

Available bit depths:

| Constant | Bits | Dynamic Range |
|----------|------|---------------|
| `AUDIO_BIT_DEPTH_16` | 16 | 96 dB |
| `AUDIO_BIT_DEPTH_24` | 24 | 144 dB |
| `AUDIO_BIT_DEPTH_32` | 32 | 192 dB |

**Notes:**
- INMP441 outputs 24-bit samples internally
- 16-bit is sufficient for most applications
- 32-bit provides headroom for processing
- 24-bit is native but requires special handling

### DMA Buffer Configuration

```c
#define DMA_BUF_COUNT   6     // Number of buffers
#define DMA_BUF_LEN     512   // Samples per buffer
```

**Guidelines:**

**Buffer Count (2-128):**
- More buffers = more stability, higher memory
- Fewer buffers = lower latency, less memory
- Recommended: 4-8 buffers

**Buffer Length (8-1024 samples):**
- Larger buffers = more stability, higher latency
- Smaller buffers = lower latency, risk of underrun
- Recommended: 256-1024 samples

**Latency calculation:**
```
Latency (ms) = (DMA_BUF_COUNT × DMA_BUF_LEN / SAMPLE_RATE) × 1000
```

Example:
```
6 buffers × 512 samples / 16000 Hz = 192 ms
```

**Memory usage:**
```
Memory (bytes) = DMA_BUF_COUNT × DMA_BUF_LEN × (BIT_DEPTH/8)
```

Example:
```
6 × 512 × 4 = 12,288 bytes (12 KB)
```

## GPIO Pin Configuration

Change the GPIO pins in `main/main.c`:

```c
#define I2S_SCK_PIN     GPIO_NUM_4   // Serial Clock
#define I2S_WS_PIN      GPIO_NUM_5   // Word Select
#define I2S_SD_PIN      GPIO_NUM_6   // Serial Data
```

**ESP32-C3 Available GPIOs:**
- Safe: GPIO0-10 (with caution on strapping pins)
- Strapping pins: GPIO2, GPIO8, GPIO9 (avoid if possible)
- Reserved: GPIO11-21 (Flash/SPI)

## Gain Configuration

Adjust the microphone gain in `main/main.c`:

```c
mic_config.gain = 1.0f;  // Range: 0.1 to 10.0
```

**Gain values:**
- `0.1` - 10% (-20 dB) - Very quiet environments
- `0.5` - 50% (-6 dB) - Reduce sensitivity
- `1.0` - 100% (0 dB) - Unity gain (default)
- `2.0` - 200% (+6 dB) - Boost quiet sources
- `5.0` - 500% (+14 dB) - Significantly boost
- `10.0` - 1000% (+20 dB) - Maximum boost

**Guidelines:**
- Start with 1.0 (unity gain)
- Increase if audio is too quiet
- Decrease if audio is clipping/distorting
- Monitor peak levels to avoid clipping

**Dynamic gain adjustment:**
```c
// Adjust gain at runtime
i2s_mic_set_gain(2.5f);
```

## Example Mode Selection

Choose which example to run by setting in `main/main.c`:

```c
#define EXAMPLE_MODE    EXAMPLE_LEVEL_METER
```

**Available modes:**
- `EXAMPLE_BASIC_CAPTURE` - Simple capture and logging
- `EXAMPLE_LEVEL_METER` - Visual audio level display
- `EXAMPLE_STREAM` - Continuous streaming
- `EXAMPLE_STATISTICS` - Detailed statistics
- `EXAMPLE_VOICE_DETECTION` - Voice activity detection

## Advanced Configuration

### sdkconfig.defaults

The `sdkconfig.defaults` file contains ESP32-C3 system configuration. Key settings:

**CPU Frequency:**
```
CONFIG_ESP32C3_DEFAULT_CPU_FREQ_160=y
```
- 160 MHz for best performance
- Can reduce to 80 MHz for power saving

**Optimization:**
```
CONFIG_COMPILER_OPTIMIZATION_PERF=y
```
- Performance optimization (default)
- Can change to `SIZE` for smaller binary

**Logging:**
```
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
```
- `ERROR` - Minimal logging
- `WARN` - Warnings only
- `INFO` - General information (default)
- `DEBUG` - Detailed debug info
- `VERBOSE` - Everything

### I2S Driver Settings

In `i2s_microphone.c`, you can modify:

**MCLK Multiple:**
```c
.mclk_multiple = I2S_MCLK_MULTIPLE_256,
```
- Affects clock accuracy
- Higher = more accurate, more power

**Channel Selection:**
```c
std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
```
- `I2S_STD_SLOT_LEFT` - INMP441 L/R = GND
- `I2S_STD_SLOT_RIGHT` - INMP441 L/R = VDD

**Clock Source:**
```c
.clk_src = I2S_CLK_SRC_DEFAULT,
```
- Uses default PLL clock
- Most accurate for standard sample rates

## Performance Tuning

### For Maximum Quality
- Sample Rate: 48 kHz
- Bit Depth: 32-bit
- DMA Buffers: 8 × 1024
- CPU Freq: 160 MHz
- Gain: 1.0

### For Lowest Latency
- Sample Rate: 16 kHz
- Bit Depth: 16-bit
- DMA Buffers: 2 × 128
- CPU Freq: 160 MHz
- Small processing windows

### For Lowest Power
- Sample Rate: 16 kHz
- Bit Depth: 16-bit
- DMA Buffers: 4 × 256
- CPU Freq: 80 MHz
- Disable unnecessary peripherals

### For Best Stability
- Sample Rate: 16 kHz
- Bit Depth: 32-bit
- DMA Buffers: 8 × 512
- CPU Freq: 160 MHz
- Large buffers for tolerance

## Troubleshooting Configuration Issues

### Buffer Overruns/Underruns
**Symptoms:** Glitches, pops, clicks
**Solution:**
- Increase buffer count
- Increase buffer length
- Reduce processing load

### High CPU Usage
**Symptoms:** System slowdown, WiFi issues
**Solution:**
- Reduce sample rate
- Reduce bit depth
- Reduce buffer size
- Optimize processing code

### High Latency
**Symptoms:** Delay in audio response
**Solution:**
- Reduce buffer count
- Reduce buffer length
- Use LOW_LATENCY_CONFIG

### Memory Issues
**Symptoms:** Allocation failures
**Solution:**
- Reduce buffer count
- Reduce buffer length
- Reduce bit depth
- Free unused memory

### Audio Clipping
**Symptoms:** Distortion at loud sounds
**Solution:**
- Reduce gain
- Check for electrical noise
- Verify power supply quality

## Configuration Examples

### Walkie-Talkie Application
```c
#define SAMPLE_RATE     AUDIO_SAMPLE_RATE_16K
#define BIT_DEPTH       AUDIO_BIT_DEPTH_16
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     256
mic_config.gain = 2.0f;
```

### Music Recording
```c
#define SAMPLE_RATE     AUDIO_SAMPLE_RATE_44K
#define BIT_DEPTH       AUDIO_BIT_DEPTH_32
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     1024
mic_config.gain = 1.0f;
```

### Security System (Voice Detection)
```c
#define SAMPLE_RATE     AUDIO_SAMPLE_RATE_16K
#define BIT_DEPTH       AUDIO_BIT_DEPTH_16
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     512
mic_config.gain = 3.0f;
#define EXAMPLE_MODE    EXAMPLE_VOICE_DETECTION
```

### Audio Spectrum Analyzer
```c
#define SAMPLE_RATE     AUDIO_SAMPLE_RATE_44K
#define BIT_DEPTH       AUDIO_BIT_DEPTH_32
#define DMA_BUF_COUNT   6
#define DMA_BUF_LEN     1024
mic_config.gain = 1.0f;
```

---

For more information, see the [README.md](README.md) and [WIRING.md](WIRING.md).
