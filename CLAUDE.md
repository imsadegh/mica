# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**mica** is a high-quality INMP441 microphone driver for ESP32-C3, optimized for professional-grade audio capture. The project provides:

- I2S microphone driver with multiple audio quality presets
- 6 example applications (basic capture, level meter, streaming, statistics, voice detection, USB streaming)
- Real-time audio processing and analysis
- USB audio streaming to Mac/PC
- Comprehensive documentation and Python receiver scripts

## Development Workflow

### Environment Setup

Before working on this project, ensure ESP-IDF is set up:

```bash
# Set up ESP-IDF environment (required every terminal session)
. ~/development/esp-idf/export.sh

# Or use the alias if configured:
get_idf
```

### Build and Flash

```bash
# Set target (first time only)
idf.py set-target esp32c3

# Build the project
idf.py build

# Find serial port:
# macOS: ls /dev/cu.usb*
# Linux: ls /dev/ttyUSB* or /dev/ttyACM*

# Flash and monitor (example for macOS)
idf.py -p /dev/cu.usbserial-14420 flash monitor

# Clean build
idf.py fullclean && idf.py build
```

### Project Configuration

Configure audio quality and example mode in `main/main.c`:

**Audio Presets** (uncomment one):

- `#define USE_VOICE_CONFIG` - 16 kHz, 32-bit (default, voice applications)
- `#define USE_MUSIC_CONFIG` - 44.1 kHz, 32-bit (high-fidelity)
- `#define USE_LOW_LATENCY_CONFIG` - 16 kHz, 16-bit (real-time, ~32ms latency)
- `#define USE_ULTRA_CONFIG` - 48 kHz, 32-bit (professional, ~171ms latency)

**Example Modes** (uncomment one):

- `EXAMPLE_BASIC_CAPTURE` - Simple audio capture with periodic logging
- `EXAMPLE_LEVEL_METER` - Real-time level visualization (default)
- `EXAMPLE_STREAM` - Continuous high-throughput capture
- `EXAMPLE_STATISTICS` - Detailed audio analysis
- `EXAMPLE_VOICE_DETECTION` - Voice activity detection
- `EXAMPLE_USB_STREAM` - USB streaming to Mac/PC

## Code Architecture

### Core Components

**Driver Layer** (`main/i2s_microphone.h` and `main/i2s_microphone.c`):

- Low-level I2S/DMA configuration
- Sample rate and bit depth handling
- Gain control and audio statistics
- Normalized 32-bit sample output
- Real-time RMS/peak level calculation

**Application Layer** (`main/main.c`):

- 6 configurable example implementations
- Audio quality presets with pre-tuned DMA parameters
- Task management and FreeRTOS integration
- Example-specific processing (level meter, streaming, statistics, etc.)

### Key Configuration Files

- **`sdkconfig.defaults`** - ESP-IDF configuration for ESP32-C3 (I2S, FreeRTOS, memory, performance)
- **`main/CMakeLists.txt`** - Component registration and build configuration
- **`CMakeLists.txt`** - Top-level project configuration

### Data Flow

1. I2S DMA transfers INMP441 data to buffers (24-bit audio)
2. Driver normalizes to 32-bit signed samples
3. Application reads samples via `i2s_mic_read_samples()`
4. Example mode processes samples (level meter, statistics, streaming)
5. Output to serial monitor or USB

## Important Development Patterns

### Audio Precision

- **INMP441 native resolution**: 24-bit, PDM format
- **Driver output**: Normalized to 32-bit signed integer (-2^31 to 2^31-1)
- **Sample rate**: Must match configuration (16k, 22.05k, 44.1k, or 48k Hz)
- **Bit depth**: 16, 24, or 32-bit modes available

### DMA Buffer Tuning

DMA buffers are pre-tuned for each quality preset:

- `dma_buf_count`: Number of buffers (4-8, trade-off: latency vs stability)
- `dma_buf_len`: Samples per buffer (256-1024, affects memory and latency)
- Total latency ≈ (dma_buf_count × dma_buf_len) / sample_rate

Example: Voice config (16kHz, 6×512) → ~192ms buffer, ~96ms effective latency

### Real-Time Considerations

- Runs on ESP32-C3 single core (160 MHz) with FreeRTOS
- I2S ISR handles data transfer; main task reads samples
- No blocking operations in critical paths
- Monitor CPU usage and adjust DMA parameters if needed

## Documentation

- **`README.md`** - Feature overview, quick start, API reference, performance specs
- **`CONFIGURATION.md`** - Detailed configuration guide for each preset
- **`WIRING.md`** - INMP441 connection diagram and troubleshooting
- **`SETUP_MACOS.md`** - Complete macOS setup (Intel and Apple Silicon)
- **`USB_STREAMING.md`** - USB streaming setup and Python receiver guide

## Testing & Validation

### Build Verification

```bash
# Check for compilation errors
idf.py build

# Monitor output after flash
idf.py monitor  # (if already flashed)
```

### Functional Testing

1. **Wiring Check**: Verify connections per WIRING.md
2. **Audio Capture**: Flash with `EXAMPLE_LEVEL_METER`, check serial output
3. **Level Detection**: Clap near microphone, observe peak level change
4. **USB Streaming**: Use Python receiver to record audio (see USB_STREAMING.md)

## Common Tasks

### Add New Audio Processing

1. Create processing function in `main/main.c`
2. Call `i2s_mic_read_samples()` in your example task
3. Process 32-bit samples (range: -2^31 to 2^31-1)
4. Example: apply gain, FFT, filtering, statistics

### Change Sample Rate or Bit Depth

1. Edit configuration in `main/main.c` (lines with `SAMPLE_RATE`, `BIT_DEPTH`)
2. Adjust DMA buffer parameters if needed (`DMA_BUF_COUNT`, `DMA_BUF_LEN`)
3. Rebuild: `idf.py fullclean && idf.py build`
4. Re-flash to ESP32-C3

### Debug Serial Output

```bash
# Monitor serial at custom baud rate (default 115200)
idf.py monitor -b 115200

# Filter by tag
idf.py monitor | grep "I2S_MIC"
```

## Troubleshooting Quick Reference

- **No Audio**: Check wiring (especially L/R pin to GND) and 3.3V power
- **Weak/Distorted**: Adjust gain with `i2s_mic_set_gain()` or increase DMA buffer size
- **Initialization Fails**: Verify ESP-IDF v5.0+, check GPIO conflicts
- **High CPU Usage**: Reduce sample rate or increase DMA buffer size

See WIRING.md and README.md troubleshooting sections for detailed guidance.

## Working with cc-sessions

This repository uses the cc-sessions framework for task management and context preservation. See `sessions/CLAUDE.sessions.md` for session-specific guidance.

### Common Session Commands

```bash
# Start a new session
sessions new

# Run kickstart (initial setup and context gathering)
sessions kickstart subagents

# Return to discussion mode after implementation
sessions mode discussion

# Check session status
sessions status
```

## Additional Guidance

@sessions/CLAUDE.sessions.md

This file provides instructions for Claude Code for working in the cc-sessions framework.
