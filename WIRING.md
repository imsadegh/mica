# INMP441 to ESP32-C3 Wiring Guide

## Pin Connections

Connect the INMP441 MEMS microphone to your ESP32-C3 as follows:

| INMP441 Pin | ESP32-C3 Pin | Function | Description |
|-------------|--------------|----------|-------------|
| VDD         | 3.3V         | Power    | Supply voltage (3.3V) |
| GND         | GND          | Ground   | Ground reference |
| SD          | GPIO6        | Data     | Serial Data Output |
| WS          | GPIO5        | Clock    | Word Select (LRCLK) |
| SCK         | GPIO4        | Clock    | Serial Clock (BCLK) |
| L/R         | GND          | Select   | Channel select (GND=Left, VDD=Right) |

## Wiring Diagram

```
ESP32-C3                    INMP441
┌─────────┐                ┌─────────┐
│         │                │         │
│  3.3V   ├────────────────┤  VDD    │
│         │                │         │
│  GND    ├─────┬──────────┤  GND    │
│         │     │          │         │
│  GPIO4  ├─────┼──────────┤  SCK    │
│         │     │          │         │
│  GPIO5  ├─────┼──────────┤  WS     │
│         │     │          │         │
│  GPIO6  ├─────┼──────────┤  SD     │
│         │     │          │         │
└─────────┘     └──────────┤  L/R    │
                           │         │
                           └─────────┘
```

## Pin Configuration Notes

### Default Pins (as configured in main.c)
- **SCK (GPIO4)**: Serial Clock / Bit Clock (BCLK)
- **WS (GPIO5)**: Word Select / Left-Right Clock (LRCLK)
- **SD (GPIO6)**: Serial Data Out from INMP441

### Customizing Pins

You can change the GPIO pins by modifying these defines in `main/main.c`:

```c
#define I2S_SCK_PIN     GPIO_NUM_4   // Change to your SCK pin
#define I2S_WS_PIN      GPIO_NUM_5   // Change to your WS pin
#define I2S_SD_PIN      GPIO_NUM_6   // Change to your SD pin
```

### Available GPIO Pins on ESP32-C3

Safe to use for I2S:
- GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, GPIO10

**Note**: Avoid using strapping pins (GPIO2, GPIO8, GPIO9) if possible, as they affect boot mode.

## L/R Channel Selection

The INMP441 has a L/R pin for channel selection:
- **L/R = GND**: Microphone outputs on LEFT channel (recommended)
- **L/R = VDD (3.3V)**: Microphone outputs on RIGHT channel

The driver is configured for LEFT channel by default. To change to RIGHT channel, modify `i2s_microphone.c:123`:

```c
// Change from:
std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

// To:
std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
```

## Power Considerations

### Power Supply
- INMP441 requires **3.3V** (do NOT use 5V)
- Current consumption: ~1.4mA typical
- Use a stable 3.3V power source
- Add a 0.1µF decoupling capacitor near the INMP441 VDD pin for best performance

### Power Supply Decoupling

```
         ESP32-C3 3.3V
              │
              ├─── 0.1µF ──┐
              │            │
              └────────────┴─── INMP441 VDD

              GND ───────────── INMP441 GND
```

## Signal Quality Tips

### Wire Length
- Keep wires as short as possible (< 10cm recommended)
- Use twisted pair for SCK/WS lines if length > 10cm
- Shield cables if in noisy environment

### Physical Mounting
- Mount the microphone away from sources of vibration
- Ensure the sound port (hole on top) is unobstructed
- Orient the microphone with sound port facing the audio source

### Grounding
- Use a solid ground connection
- Connect ESP32-C3 GND to a good ground plane
- Avoid ground loops

## Testing the Connection

After wiring, you can test the connection by:

1. Build and flash the project
2. Monitor the serial output
3. You should see initialization messages
4. The level meter will show audio activity when you make noise

If you see all zeros or no response:
- Check all connections
- Verify 3.3V power supply
- Ensure L/R pin is properly connected to GND
- Check that GPIO pins are not in use by other peripherals

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|----------------|----------|
| No audio signal | Wrong wiring | Double-check all connections |
| Very weak signal | Missing decoupling capacitor | Add 0.1µF cap near VDD |
| Distorted audio | Long wires / noise | Shorten wires, add shielding |
| One-sided output | Wrong L/R setting | Check L/R pin connection |
| Intermittent audio | Loose connection | Secure all connections |
| No initialization | Wrong GPIO pins | Verify pin configuration in code |

## Advanced: Multiple Microphones

You can connect multiple INMP441 microphones for stereo or microphone arrays:

### Stereo Configuration (2 microphones)

```
ESP32-C3          INMP441 #1 (Left)    INMP441 #2 (Right)
  SCK    ─────────┬─ SCK                ─ SCK
  WS     ─────────┼─ WS                 ─ WS
  GPIO6  ─────────┼─ SD
  GPIO7  ─────────┼─────────────────────── SD
         │        └─ L/R → GND           ─ L/R → VDD
```

This requires modifications to the driver to support stereo input.
