# VMUPro SMS Plus Emulator

A Sega Master System and Game Gear emulator for the VMUPro handheld device, based on the SMS Plus GX emulator core.

## Overview

This project ports the SMS Plus GX emulator to run on the VMUPro platform, allowing you to play classic Master System and Game Gear games on your VMUPro device. The emulator provides automatic console detection and accurate emulation with authentic sound and graphics.

## Features

- Full Master System and Game Gear emulation using SMS Plus GX core
- Automatic console detection (SMS vs Game Gear)
- Sound emulation with PSG and FM support
- Video Display Processor (VDP) emulation for accurate graphics
- Frame rate monitoring and performance statistics
- Built-in ROM browser and launcher
- Save state functionality
- Pause menu with options and settings
- Controller input support via VMUPro buttons

## Supported Systems

The emulator supports:

- Sega Master System (.sms files)
- Sega Game Gear (.gg files)
- Automatic detection and configuration based on ROM type

## Requirements

- VMUPro device
- VMUPro SDK (included as submodule)
- ESP-IDF development framework
- SMS ROM files (.sms format) or Game Gear ROM files (.gg format)

## Building

### Prerequisites

1. Install ESP-IDF and set up your development environment
2. Clone this repository with submodules:
   ```bash
   git clone --recursive https://github.com/8BitMods/vmupro-smsplus.git
   cd vmupro-smsplus/src
   ```

### Build Process

1. Build the project:

   ```bash
   idf.py build
   ```

2. Package the application:

   ```bash
   ./pack.sh
   ```

   Or on Windows:

   ```powershell
   ./pack.ps1
   ```

3. Deploy to VMUPro:
   ```bash
   ./send.sh
   ```
   Or on Windows:
   ```powershell
   ./send.ps1
   ```

## Usage

### ROM Setup

1. Place SMS/Game Gear ROM files on the VMUPro device in `/sdcard/roms/SMSGG/`
2. Supported formats: `.sms` (Master System) and `.gg` (Game Gear)
3. The file browser supports sub-folders, so you can organise files as you'd like

### Controls

- **D-pad**: SMS/GG D-pad
- **A button**: SMS/GG Button 1
- **B button**: SMS/GG Button 2
- **Mode button**: Start/Pause
- **Power button**: Reset (Master System only)
- **Bottom button**: Open pause menu

### Gameplay

1. Launch SMS Plus emulator from VMUPro menu
2. Select a ROM file from the browser
3. Game will automatically detect console type and configure accordingly
4. Use pause menu (Bottom button) for save states and options

## Pause Menu Features

- **Save & Continue**: Save current state and resume
- **Load Game**: Load previously saved state
- **Restart**: Reset the current game
- **Options**: Adjust volume, brightness, and palette settings
- **Quit**: Exit to ROM browser

## Technical Details

- **Platform**: ESP32-based VMUPro
- **CPU**: Z80 emulation
- **Video**: VDP emulation with SMS (256x192) and Game Gear (160x144) support
- **Audio**: PSG and FM sound synthesis at 44.1kHz
- **Memory**: Automatic SRAM handling for battery saves
- **Performance**: 60 FPS gameplay with frame skipping when needed

## File Structure

```
src/
├── main/
│   ├── main.cpp              # Main emulator entry point
│   └── smsplus/              # SMS Plus GX emulator core
│       ├── cpu/              # Z80 CPU emulation
│       ├── sound/            # PSG and FM sound emulation
│       └── unzip/            # ROM archive support
├── metadata.json             # VMUPro app metadata
├── icon.bmp                  # Application icon
├── pack.sh/pack.ps1         # Packaging scripts
└── send.sh/send.ps1         # Deployment scripts
```

## Credits

- **SMS Plus GX Emulator**: Original Master System/Game Gear emulator core
- **8BitMods**: VMUPro porting and optimization
- **VMUPro SDK**: Platform-specific libraries and tools

## Performance

The emulator includes:

- Real-time FPS counter and frame time monitoring
- Frame time averaging and performance statistics
- Performance optimization for VMUPro hardware
- Automatic frame skipping when needed to maintain 60 FPS

## License

The project is licensed under GNU General Public License v2.0, with the SMS Plus GX emulator core maintaining its original licensing terms.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests to help improve the emulator.

## Support

For support:

- Check VMUPro documentation
- Report issues on the project repository
- Join the 8BitMods community

---

Enjoy playing classic Master System and Game Gear games on your VMUPro device!
