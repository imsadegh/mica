#!/usr/bin/env python3
"""
USB Audio Receiver for ESP32-C3 INMP441 Microphone

Receives audio data from ESP32-C3 over USB serial and plays it in real-time.
Can also save to WAV file.

Requirements:
    pip install pyserial pyaudio numpy

Usage:
    # Play audio in real-time
    python3 usb_audio_receiver.py /dev/cu.usbmodem2101

    # Record to WAV file
    python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save recording.wav

    # List available serial ports
    python3 usb_audio_receiver.py --list
"""

import sys
import argparse
import struct
import serial
import serial.tools.list_ports
import pyaudio
import wave
import numpy as np
from collections import deque
import time

# Audio configuration (must match ESP32 settings)
SAMPLE_RATE = 16000  # Hz - Change if you use different config on ESP32
CHANNELS = 1         # Mono
SAMPLE_WIDTH = 2     # 16-bit = 2 bytes

# Protocol constants
SYNC_HEADER = b'\xAA\x55'
HEADER_SIZE = 2
COUNT_SIZE = 2
CHECKSUM_SIZE = 1

class AudioReceiver:
    def __init__(self, port, baudrate=115200, save_file=None):
        self.port = port
        self.baudrate = baudrate
        self.save_file = save_file
        self.serial_port = None
        self.audio_stream = None
        self.pyaudio_instance = None
        self.wav_file = None
        self.running = False

        # Statistics
        self.packets_received = 0
        self.packets_corrupted = 0
        self.bytes_received = 0
        self.start_time = None

        # Audio buffer for smoothing playback
        self.audio_buffer = deque(maxlen=10)

    def open_serial(self):
        """Open serial port connection"""
        print(f"Opening serial port: {self.port} @ {self.baudrate} baud")
        try:
            self.serial_port = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1.0,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            print("Serial port opened successfully")
            time.sleep(2)  # Wait for ESP32 to initialize
            self.serial_port.reset_input_buffer()
            return True
        except Exception as e:
            print(f"Error opening serial port: {e}")
            return False

    def open_audio(self):
        """Initialize PyAudio for playback"""
        try:
            self.pyaudio_instance = pyaudio.PyAudio()
            self.audio_stream = self.pyaudio_instance.open(
                format=pyaudio.paInt16,
                channels=CHANNELS,
                rate=SAMPLE_RATE,
                output=True,
                frames_per_buffer=256
            )
            print(f"Audio output initialized: {SAMPLE_RATE} Hz, 16-bit mono")
            return True
        except Exception as e:
            print(f"Error initializing audio: {e}")
            return False

    def open_wav_file(self):
        """Open WAV file for recording"""
        if self.save_file:
            try:
                self.wav_file = wave.open(self.save_file, 'wb')
                self.wav_file.setnchannels(CHANNELS)
                self.wav_file.setsampwidth(SAMPLE_WIDTH)
                self.wav_file.setframerate(SAMPLE_RATE)
                print(f"Recording to: {self.save_file}")
                return True
            except Exception as e:
                print(f"Error opening WAV file: {e}")
                return False
        return True

    def find_sync(self):
        """Find sync header in serial stream"""
        buffer = bytearray()
        while len(buffer) < 2:
            byte = self.serial_port.read(1)
            if not byte:
                return False
            buffer.append(byte[0])
            if len(buffer) > 2:
                buffer.pop(0)
            if bytes(buffer) == SYNC_HEADER:
                return True
        return False

    def read_packet(self):
        """Read one audio packet from serial port"""
        # Find sync header
        if not self.find_sync():
            return None

        # Read sample count (2 bytes, little-endian)
        count_bytes = self.serial_port.read(COUNT_SIZE)
        if len(count_bytes) != COUNT_SIZE:
            return None

        sample_count = struct.unpack('<H', count_bytes)[0]

        # Validate sample count
        if sample_count == 0 or sample_count > 1024:
            self.packets_corrupted += 1
            return None

        # Read audio data
        data_size = sample_count * SAMPLE_WIDTH
        audio_data = self.serial_port.read(data_size)
        if len(audio_data) != data_size:
            self.packets_corrupted += 1
            return None

        # Read checksum
        checksum_byte = self.serial_port.read(CHECKSUM_SIZE)
        if len(checksum_byte) != CHECKSUM_SIZE:
            self.packets_corrupted += 1
            return None

        received_checksum = checksum_byte[0]

        # Verify checksum
        calculated_checksum = 0
        for byte in audio_data:
            calculated_checksum ^= byte

        if calculated_checksum != received_checksum:
            self.packets_corrupted += 1
            return None

        # Update statistics
        self.packets_received += 1
        self.bytes_received += len(audio_data)

        return audio_data

    def process_audio(self, audio_data):
        """Process and play audio data"""
        if audio_data:
            # Play audio
            if self.audio_stream:
                try:
                    self.audio_stream.write(audio_data)
                except Exception as e:
                    print(f"Audio playback error: {e}")

            # Save to WAV file
            if self.wav_file:
                self.wav_file.writeframes(audio_data)

    def print_statistics(self):
        """Print reception statistics"""
        if self.start_time:
            elapsed = time.time() - self.start_time
            if elapsed > 0:
                packets_per_sec = self.packets_received / elapsed
                kb_per_sec = (self.bytes_received / 1024) / elapsed
                corruption_rate = (self.packets_corrupted / max(1, self.packets_received + self.packets_corrupted)) * 100

                print(f"\r📊 Packets: {self.packets_received} | "
                      f"Rate: {packets_per_sec:.1f} pkt/s | "
                      f"Throughput: {kb_per_sec:.1f} KB/s | "
                      f"Errors: {self.packets_corrupted} ({corruption_rate:.1f}%)",
                      end='', flush=True)

    def run(self):
        """Main receive loop"""
        if not self.open_serial():
            return False

        if not self.open_audio():
            self.cleanup()
            return False

        if not self.open_wav_file():
            self.cleanup()
            return False

        print("\n" + "="*60)
        print("🎤 USB Audio Streaming Active")
        print("="*60)
        print(f"Sample Rate: {SAMPLE_RATE} Hz")
        print(f"Format: 16-bit PCM Mono")
        print(f"Port: {self.port}")
        if self.save_file:
            print(f"Recording: {self.save_file}")
        print("\nPress Ctrl+C to stop\n")

        self.running = True
        self.start_time = time.time()
        stats_counter = 0

        try:
            while self.running:
                # Read packet
                audio_data = self.read_packet()

                if audio_data:
                    # Process audio
                    self.process_audio(audio_data)

                    # Print statistics every 10 packets
                    stats_counter += 1
                    if stats_counter >= 10:
                        self.print_statistics()
                        stats_counter = 0

        except KeyboardInterrupt:
            print("\n\nStopping...")
        except Exception as e:
            print(f"\nError: {e}")
        finally:
            self.cleanup()

        return True

    def cleanup(self):
        """Clean up resources"""
        self.running = False

        print("\n\n" + "="*60)
        print("📊 Final Statistics")
        print("="*60)
        print(f"Packets received: {self.packets_received}")
        print(f"Packets corrupted: {self.packets_corrupted}")
        print(f"Total data: {self.bytes_received / 1024:.2f} KB")

        if self.start_time:
            elapsed = time.time() - self.start_time
            print(f"Duration: {elapsed:.1f} seconds")
            if elapsed > 0:
                print(f"Average throughput: {(self.bytes_received / 1024) / elapsed:.1f} KB/s")

        if self.wav_file:
            self.wav_file.close()
            print(f"✅ Saved to: {self.save_file}")

        if self.audio_stream:
            self.audio_stream.stop_stream()
            self.audio_stream.close()

        if self.pyaudio_instance:
            self.pyaudio_instance.terminate()

        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()

        print("="*60 + "\n")

def list_serial_ports():
    """List available serial ports"""
    ports = serial.tools.list_ports.comports()

    if not ports:
        print("No serial ports found")
        return

    print("\n" + "="*60)
    print("Available Serial Ports:")
    print("="*60)

    for port in ports:
        print(f"\n📌 {port.device}")
        if port.description:
            print(f"   Description: {port.description}")
        if port.manufacturer:
            print(f"   Manufacturer: {port.manufacturer}")
        if port.serial_number:
            print(f"   Serial: {port.serial_number}")

    print("\n" + "="*60 + "\n")

def main():
    parser = argparse.ArgumentParser(
        description='USB Audio Receiver for ESP32-C3 INMP441 Microphone',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Play audio in real-time
  python3 usb_audio_receiver.py /dev/cu.usbmodem2101

  # Record to WAV file
  python3 usb_audio_receiver.py /dev/cu.usbmodem2101 --save recording.wav

  # List available serial ports
  python3 usb_audio_receiver.py --list
        """
    )

    parser.add_argument('port', nargs='?', help='Serial port (e.g., /dev/cu.usbmodem2101 or COM3)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('-s', '--save', metavar='FILE', help='Save audio to WAV file')
    parser.add_argument('-l', '--list', action='store_true', help='List available serial ports')
    parser.add_argument('-r', '--rate', type=int, default=16000, help='Sample rate in Hz (default: 16000)')

    args = parser.parse_args()

    # List ports mode
    if args.list:
        list_serial_ports()
        return 0

    # Normal mode - require port
    if not args.port:
        parser.print_help()
        print("\nError: Serial port required (use --list to see available ports)")
        return 1

    # Update global sample rate if specified
    global SAMPLE_RATE
    SAMPLE_RATE = args.rate

    # Run receiver
    receiver = AudioReceiver(args.port, args.baudrate, args.save)
    success = receiver.run()

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
