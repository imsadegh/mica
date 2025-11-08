# macOS Setup Guide for ESP32-C3 INMP441 Microphone

Complete setup guide for macOS users (both Intel and Apple Silicon).

## Prerequisites

### 1. Install Homebrew (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install Required Tools

```bash
# Install Python 3
brew install python3

# Install CMake and Ninja
brew install cmake ninja

# Install USB-to-Serial drivers (if needed)
# Most modern ESP32-C3 boards work without additional drivers
# If you have issues, install:
brew install --cask silicon-labs-vcp-driver
```

## ESP-IDF Installation

### Option 1: Quick Install (Recommended)

```bash
# Create development directory
mkdir -p ~/development
cd ~/development

# Clone ESP-IDF
git clone -b v5.1.1 --recursive https://github.com/espressif/esp-idf.git

# Navigate to ESP-IDF
cd esp-idf

# Install only ESP32-C3 tools (saves ~1GB vs all targets)
python3 tools/idf_tools.py install --targets esp32c3

# Install Python environment
python3 tools/idf_tools.py install-python-env
```

### Option 2: Full Install (All ESP32 targets)

```bash
cd ~/development/esp-idf

# Install all ESP32 targets
./install.sh all
```

## Project Setup

### 1. Clone/Download the Project

```bash
# Navigate to your projects directory
cd ~/Downloads/myDocuments/_develop

# If you have the project, navigate to it
cd mica

# Or clone from repository:
# git clone <repository-url> mica
# cd mica
```

### 2. Set Up Environment Variables

You need to run this **every time** you open a new terminal:

```bash
cd ~/development/esp-idf
. ./export.sh
```

### 3. Create an Alias (Recommended)

Add this to your shell profile for easy access:

**For Zsh (default on modern macOS):**
```bash
echo 'alias get_idf=". ~/development/esp-idf/export.sh"' >> ~/.zshrc
source ~/.zshrc
```

**For Bash:**
```bash
echo 'alias get_idf=". ~/development/esp-idf/export.sh"' >> ~/.bash_profile
source ~/.bash_profile
```

Now you can simply type `get_idf` instead of the full path!

## Building the Project

### First Time Build

```bash
# 1. Set up ESP-IDF environment
get_idf
# Or: cd ~/development/esp-idf && . ./export.sh

# 2. Navigate to project
cd ~/Downloads/myDocuments/_develop/mica

# 3. Set target to ESP32-C3
idf.py set-target esp32c3

# 4. Build the project
idf.py build
```

The first build takes 1-3 minutes. Subsequent builds are much faster.

### Regular Build

```bash
# Set up environment (if not already done in this terminal session)
get_idf

# Navigate to project
cd ~/Downloads/myDocuments/_develop/mica

# Build
idf.py build
```

## Finding Your ESP32-C3 Serial Port

### 1. Without ESP32-C3 Connected

```bash
ls /dev/cu.*
```

Note the output.

### 2. Connect ESP32-C3 via USB

### 3. List Ports Again

```bash
ls /dev/cu.*
```

The new port is your ESP32-C3. Common patterns:
- `/dev/cu.usbserial-*` (e.g., `/dev/cu.usbserial-14420`)
- `/dev/cu.usbmodem*` (e.g., `/dev/cu.usbmodem14201`)
- `/dev/cu.SLAB_USBtoUART` (Silicon Labs driver)
- `/dev/cu.wchusbserial*` (WCH CH340 chip)

## Flashing to ESP32-C3

### Flash and Monitor

```bash
# Replace with your actual port
idf.py -p /dev/cu.usbserial-14420 flash monitor
```

### Flash Only (no monitor)

```bash
idf.py -p /dev/cu.usbserial-14420 flash
```

### Monitor Only (after flash)

```bash
idf.py -p /dev/cu.usbserial-14420 monitor
```

### Exit Monitor

Press `Ctrl + ]` to exit the serial monitor.

## Complete Workflow Example

Here's a complete example for macOS:

```bash
# Terminal 1: First time setup (only once)
cd ~/development/esp-idf
python3 tools/idf_tools.py install --targets esp32c3
python3 tools/idf_tools.py install-python-env

# Terminal 2: Every session
cd ~/development/esp-idf
. ./export.sh

cd ~/Downloads/myDocuments/_develop/mica
idf.py set-target esp32c3  # First time only
idf.py build

# Find port
ls /dev/cu.usb*

# Flash and monitor
idf.py -p /dev/cu.usbserial-14420 flash monitor
```

## Troubleshooting

### "Please use idf.py only in an ESP-IDF shell environment"

**Cause:** ESP-IDF environment not activated.

**Solution:**
```bash
cd ~/development/esp-idf
. ./export.sh
```

### "Permission denied" when accessing serial port

**Solution:**
```bash
# Add your user to dialout group (may require reboot)
sudo dseditgroup -o edit -a $USER -t user dialout

# Or use sudo (not recommended for regular use)
sudo idf.py -p /dev/cu.usbserial-14420 flash monitor
```

### ESP32-C3 not detected / No serial port

**Check USB cable:**
- Use a data cable (not charge-only)
- Try different USB ports
- Try a different cable

**Install drivers:**
```bash
# For CP210x (Silicon Labs)
brew install --cask silicon-labs-vcp-driver

# For CH340
# Download from: http://www.wch.cn/downloads/CH341SER_MAC_ZIP.html
```

**Reset ESP32-C3:**
- Press and hold BOOT button
- Press RESET button
- Release RESET
- Release BOOT
- Try flashing again

### Build errors about missing tools

**Solution:**
```bash
cd ~/development/esp-idf

# Reinstall tools
python3 tools/idf_tools.py install --targets esp32c3

# Re-export environment
. ./export.sh
```

### Python version errors

ESP-IDF 5.1.1 requires Python 3.8-3.11. If you have Python 3.12+:

```bash
# Install Python 3.11
brew install python@3.11

# Use it with ESP-IDF
cd ~/development/esp-idf
python3.11 tools/idf_tools.py install --targets esp32c3
python3.11 tools/idf_tools.py install-python-env
```

### "xcrun: error: invalid active developer path"

**Solution:**
```bash
xcode-select --install
```

### Port busy / already in use

**Solution:**
```bash
# Find process using the port
lsof | grep usbserial

# Kill the process
kill -9 <PID>

# Or restart terminal and try again
```

## Performance Tips

### Speed Up Builds

```bash
# Use ccache (caching compiler)
brew install ccache

# Configure ESP-IDF to use it
idf.py menuconfig
# Navigate to: Compiler options -> Enable ccache
```

### Reduce Build Time

```bash
# Build with multiple jobs (use number of CPU cores)
idf.py -j8 build

# Or automatically detect cores
idf.py -j$(sysctl -n hw.ncpu) build
```

## Monitoring Serial Output

### Use screen (built-in)

```bash
# Connect with screen
screen /dev/cu.usbserial-14420 115200

# Exit: Ctrl+A, then K, then Y
```

### Use minicom

```bash
# Install minicom
brew install minicom

# Connect
minicom -D /dev/cu.usbserial-14420 -b 115200

# Exit: Ctrl+A, then Q
```

## Directory Structure on macOS

After installation, your structure should look like:

```
~/development/
├── esp-idf/                    # ESP-IDF framework
│   ├── components/
│   ├── tools/
│   └── export.sh
│
~/.espressif/                   # ESP-IDF tools (hidden)
├── tools/
│   ├── riscv32-esp-elf/       # ESP32-C3 compiler
│   ├── riscv32-esp-elf-gdb/   # Debugger
│   ├── openocd-esp32/         # Flash tool
│   └── esp-rom-elfs/          # ROM binaries
└── python_env/
    └── idf5.1_py3.12_env/     # Python virtual environment

~/Downloads/myDocuments/_develop/
└── mica/                       # Your project
    ├── main/
    ├── CMakeLists.txt
    └── sdkconfig.defaults
```

## Apple Silicon (M1/M2/M3) Notes

ESP-IDF works natively on Apple Silicon. The tools automatically download ARM64 versions.

**Verify you're using native tools:**
```bash
file ~/.espressif/tools/riscv32-esp-elf/*/riscv32-esp-elf/bin/riscv32-esp-elf-gcc

# Should show: Mach-O 64-bit executable arm64
```

If running under Rosetta (not recommended):
```bash
# Check if Terminal is running under Rosetta
arch
# Should show: arm64 (native) or i386 (Rosetta)
```

## Updating ESP-IDF

```bash
cd ~/development/esp-idf

# Fetch updates
git fetch

# Check available versions
git tag

# Update to specific version
git checkout v5.2.0
git submodule update --init --recursive

# Reinstall tools
python3 tools/idf_tools.py install --targets esp32c3
python3 tools/idf_tools.py install-python-env

# Re-export
. ./export.sh
```

## Useful Commands Reference

```bash
# Set up environment
get_idf

# Build commands
idf.py menuconfig          # Configure project settings
idf.py build              # Build project
idf.py clean              # Clean build files
idf.py fullclean          # Remove all build files and config

# Flash commands
idf.py flash              # Flash only
idf.py monitor            # Monitor only
idf.py flash monitor      # Flash and monitor

# Device commands
idf.py -p PORT erase-flash        # Erase flash
idf.py -p PORT erase-otadata      # Erase OTA data

# Information
idf.py size               # Show binary size breakdown
idf.py size-components    # Show size per component
idf.py --version          # Show ESP-IDF version
```

## Getting Help

```bash
# ESP-IDF help
idf.py --help

# Specific command help
idf.py build --help
idf.py flash --help

# Check installation
idf.py doctor
```

## Next Steps

1. Follow the [WIRING.md](WIRING.md) guide to connect your INMP441 to ESP32-C3
2. Review [CONFIGURATION.md](CONFIGURATION.md) to customize audio settings
3. Check [README.md](README.md) for API documentation and examples

---

**Quick Reference Card:**

```bash
# One-time setup
cd ~/development/esp-idf
python3 tools/idf_tools.py install --targets esp32c3
python3 tools/idf_tools.py install-python-env

# Every session
get_idf
cd ~/Downloads/myDocuments/_develop/mica
idf.py build
idf.py -p /dev/cu.usbserial-XXXXX flash monitor
```

Happy coding! 🚀
