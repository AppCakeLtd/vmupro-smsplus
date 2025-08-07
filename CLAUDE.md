# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
VMUPro SMSPlus is a Sega Master System and Game Gear emulator built for the VMUPro device using the VMUPro SDK. This is an ESP32-based application that runs SMS Plus GX emulation core with a VMUPro-specific frontend.

## Project Structure
- **src/** - Main application source code
  - **main/** - ESP-IDF main component containing the emulator frontend
    - **main.cpp** - Main application entry point and emulator loop
    - **smsplus/** - SMS Plus GX emulation core (unmodified from upstream)
  - **CMakeLists.txt** - ESP-IDF project configuration
  - **metadata.json** - VMUPro application metadata
- **vmupro-sdk/** - VMUPro SDK submodule providing hardware APIs

## Development Commands
**Prerequisites:** ESP-IDF environment must be configured (typically `. /opt/idfvmusdk/export.sh`)

- **Build:** `cd src && idf.py build`
- **Package:** `cd src && ./pack.sh` (creates smsplus.vmupack)
- **Send to device:** `cd src && ./send.sh [port]` (uploads and runs on VMUPro)
- **Clean build:** `cd src && idf.py clean`

## Architecture Overview
The emulator integrates SMS Plus GX core with VMUPro hardware:

**Emulation Core (smsplus/):**
- Unmodified SMS Plus GX emulator
- Handles Z80 CPU, VDP, sound, and game logic
- Provides standard emulation APIs

**VMUPro Frontend (main.cpp):**
- Bridges emulator core with VMUPro SDK
- Manages display rendering, audio streaming, and input handling
- Implements pause menu system with save states and options
- Handles file selection through emubrowser API

**Key Integration Points:**
- `bitmap.data = vmupro_get_back_buffer()` - Direct framebuffer rendering
- `vmupro_audio_add_stream_samples()` - Real-time audio streaming
- `vmupro_emubrowser_render_contents()` - ROM file selection
- Input mapping from VMUPro buttons to SMS controller

## Configuration
- Console detection automatically sets SMS vs Game Gear mode
- Audio configured for 44.1kHz stereo output
- Display supports both SMS (256x192) and Game Gear (160x144) resolutions
- Save states stored in `/sdcard/roms/MasterSystem/` directory

## Memory Management
The application allocates specific buffers for emulation:
- SRAM: 32KB for cartridge battery backup
- Work RAM: 8KB for system RAM
- VRAM: 16KB for video memory
- Audio buffer: ~3KB for stereo samples
- Pause buffer: 115KB for menu background

## VMUPro SDK Integration
This project depends on the VMUPro SDK located at `vmupro-sdk/`. Key APIs used:
- Display: Double-buffered rendering, graphics primitives
- Audio: Real-time streaming with ring buffer
- Input: Button state management
- File: ROM loading and save state management
- Utilities: Timing, logging, emulator browser