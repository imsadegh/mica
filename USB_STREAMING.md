# USB Audio Streaming Guide

Stream high-quality audio from your ESP32-C3 + INMP441 microphone directly to your Mac/PC over USB.

## Overview

This feature allows you to use your ESP32-C3 with INMP441 as a USB microphone by streaming audio data over the USB serial connection. The audio can be played in real-time or saved to a WAV file.

**Features:**
- ✅ Real-time audio playback on Mac/PC
- ✅ Record to WAV files
- ✅ Low latency (~50-100ms)
- ✅ 16-bit PCM audio quality
- ✅ Error detection with checksums
- ✅ Live statistics display

## Quick Start

### 1. Flash ESP32-C3 with USB Streaming Mode

Edit `main/main.c` and change the example mode:

```c
// Change this line (around line 90)
#define EXAMPLE_MODE    EXAMPLE_USB_STREAM
```

Build and flash:

```bash
idf.py build
idf.py -p /dev/cu.usbmodem2101 flash
```

### 2. Install Python Dependencies

On your Mac/PC:

```bash
# Install Python packages
pip3 install -r requirements.txt
```

**macOS Note:** If PyAudio installation fails, install PortAudio first:

```bash
brew install portaudio
pip3 install pyaudio
```

**Linux Note:**

```bash
sudo apt-get install portaudio19-dev python3-pyaudio
pip3 install -r requirements.txt
```

**Windows Note:**

```bash
# Download PyAudio wheel from:
# https://www.lfd.uci.edu/~gohlke/pythonlibs/#pyaudio
pip install PyAudio‑0.2.13‑cp311‑cp311‑win_amd64.whl
pip install pyserial numpy
```

### 3. Run the Receiver

**Find your serial port:**

```bash
# macOS
ls /dev/cu.usb*

# Linux
ls /dev/ttyUSB* /dev/ttyACM*

# Or use the script
python3 usb_audio_receiver.py --list
```

**Stream audio (real-time playback):**

```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101
```

**Record to WAV file:**

```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save recording.wav
```

**Both (play and record):**

```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save recording.wav
```

## Configuration

### ESP32 Side

#### Sample Rate

Change the audio quality preset in `main/main.c`:

```c
// For voice (16 kHz) - DEFAULT
#define USE_VOICE_CONFIG

// For music (44.1 kHz) - Higher quality
// #define USE_MUSIC_CONFIG

// For low latency (16 kHz, 16-bit)
// #define USE_LOW_LATENCY_CONFIG
```

Then update the Python script's sample rate to match:

```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --rate 16000  # Voice
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --rate 44100  # Music
```

#### Buffer Size

In `main/main.c`, adjust `CHUNK_SIZE` for different latency/stability tradeoffs:

```c
const size_t CHUNK_SIZE = 256;  // Default - good balance
// const size_t CHUNK_SIZE = 128;  // Lower latency, less stable
// const size_t CHUNK_SIZE = 512;  // Higher latency, more stable
```

### Python Receiver Options

```bash
python3 usb_audio_receiver.py --help
```

**Options:**
- `-b, --baudrate`: Serial baud rate (default: 115200)
- `-s, --save FILE`: Save audio to WAV file
- `-r, --rate`: Sample rate in Hz (default: 16000)
- `-l, --list`: List available serial ports

## Protocol Details

The ESP32 sends binary audio packets over USB serial:

### Packet Format

```
+-------------+-------------+------------------+------------+
| Sync Header | Sample Count|   Audio Data     | Checksum   |
+-------------+-------------+------------------+------------+
|   2 bytes   |   2 bytes   | count * 2 bytes  |  1 byte    |
|  0xAA 0x55  | little-end. |   16-bit PCM     |   XOR      |
+-------------+-------------+------------------+------------+
```

**Fields:**
1. **Sync Header**: `0xAA 0x55` - Used to find packet boundaries
2. **Sample Count**: 16-bit little-endian - Number of samples in packet
3. **Audio Data**: 16-bit signed PCM samples, little-endian
4. **Checksum**: XOR of all audio data bytes

## Performance

### Typical Throughput

| Sample Rate | Bit Depth | Bandwidth | Latency |
|-------------|-----------|-----------|---------|
| 16 kHz      | 16-bit    | ~32 KB/s  | 50-80ms |
| 22.05 kHz   | 16-bit    | ~44 KB/s  | 60-90ms |
| 44.1 kHz    | 16-bit    | ~88 KB/s  | 80-120ms|

### Latency Breakdown

- I2S capture: ~16-32ms (depends on buffer size)
- Serial transmission: ~10-20ms
- Python processing: ~10-30ms
- Audio playback: ~20-40ms
- **Total: 56-122ms**

## Usage Examples

### Example 1: Quick Audio Test

```bash
# Flash ESP32
idf.py -p /dev/cu.usbmodem2101 flash

# Play audio immediately
python3 usb_audio_receiver.py /dev/cu.usbmodem2101
```

### Example 2: Record Voice Memo

```bash
# Record 30 seconds (Ctrl+C to stop)
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save memo.wav

# Play back the recording
afplay memo.wav  # macOS
aplay memo.wav   # Linux
```

### Example 3: High-Quality Music Recording

In `main/main.c`:
```c
#define USE_MUSIC_CONFIG
```

```bash
idf.py build flash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --rate 44100 --save music.wav
```

### Example 4: Real-time Voice Chat

Use the microphone input in applications:

```bash
# Stream to stdout, pipe to other apps
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save - | your_app
```

## Troubleshooting

### No Audio Output

**Check serial connection:**
```bash
python3 usb_audio_receiver.py --list
```

**Check ESP32 logs:**
```bash
# Before running Python script, monitor ESP32
idf.py -p /dev/cu.usbmodem2101 monitor
# You should see "Streaming started!"
```

**Check audio device:**
```bash
# macOS - Check system audio output
# System Settings > Sound > Output

# Linux
aplay -l
```

### Choppy or Distorted Audio

**1. Increase buffer size on ESP32:**

In `main/main.c`:
```c
const size_t CHUNK_SIZE = 512;  // Larger chunks
```

**2. Close serial monitor:**

Make sure `idf.py monitor` is NOT running when using the Python script.

**3. Check CPU usage:**

The Python script should use <5% CPU. If higher, close other applications.

### High Error Rate (>5%)

**1. Check USB cable:**
- Use a high-quality data cable
- Try a different USB port
- Avoid USB hubs

**2. Reduce baud rate:**
```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 -b 115200
```

**3. Check for interference:**
- Move ESP32 away from WiFi routers
- Avoid running near motors or power supplies

### "Port is busy" Error

**Close other programs using the port:**
```bash
# macOS - Find process using port
lsof | grep usbmodem

# Kill the process
kill -9 <PID>

# Or just close idf.py monitor
```

### PyAudio Installation Issues

**macOS:**
```bash
brew install portaudio
pip3 install --global-option='build_ext' --global-option='-I/opt/homebrew/include' --global-option='-L/opt/homebrew/lib' pyaudio
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install portaudio19-dev python3-pyaudio
```

**Windows:**

Download precompiled wheel from:
https://www.lfd.uci.edu/~gohlke/pythonlibs/#pyaudio

## Statistics Display

While running, the receiver shows real-time statistics:

```
📊 Packets: 1234 | Rate: 62.5 pkt/s | Throughput: 32.1 KB/s | Errors: 5 (0.4%)
```

**Metrics:**
- **Packets**: Total packets received
- **Rate**: Packets per second
- **Throughput**: Data rate in KB/s
- **Errors**: Corrupted packets (should be <1%)

## Advanced Usage

### Custom Audio Processing

Modify `usb_audio_receiver.py` to add your own processing:

```python
def process_audio(self, audio_data):
    # Convert to numpy array
    samples = np.frombuffer(audio_data, dtype=np.int16)

    # Your custom processing here
    # - Apply filters
    # - Feature extraction
    # - Real-time analysis

    # Play processed audio
    if self.audio_stream:
        self.audio_stream.write(audio_data)
```

### Integration with Other Software

**Stream to FFmpeg:**
```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save - | \
  ffmpeg -f s16le -ar 16000 -ac 1 -i - output.mp3
```

**Stream to SoX:**
```bash
python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save - | \
  sox -t raw -r 16000 -b 16 -c 1 -e signed - -t wav - | \
  your_audio_app
```

## Next Steps

### Phase 2: USB Audio Class (UAC)

For native USB microphone support (works like any USB mic):
- Appears in system audio devices
- Works with Zoom, Discord, etc.
- No Python script needed
- More complex to implement

Let me know if you'd like me to implement Phase 2!

## FAQ

**Q: Can I use this while debugging?**

A: No, the serial port can only be used by one application at a time. Use `EXAMPLE_LEVEL_METER` for debugging, then switch to `EXAMPLE_USB_STREAM` for audio streaming.

**Q: What's the maximum sample rate?**

A: USB CDC serial can handle up to 48 kHz comfortably. Higher rates (96 kHz) may work but could have data loss.

**Q: Can I stream over WiFi instead?**

A: Yes! I can add a WiFi streaming mode if you're interested.

**Q: Does this work on Windows?**

A: Yes, just use the Windows serial port name (e.g., `COM3`) instead of `/dev/cu.*`.

**Q: Can I use multiple microphones?**

A: Yes, connect multiple ESP32s to different USB ports and run multiple Python scripts.

## See Also

- [README.md](README.md) - Main project documentation
- [CONFIGURATION.md](CONFIGURATION.md) - Audio configuration guide
- [WIRING.md](WIRING.md) - Hardware wiring guide
- [SETUP_MACOS.md](SETUP_MACOS.md) - macOS setup guide

---

**Have questions?** Open an issue on GitHub!
