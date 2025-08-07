#include <string.h>
#include <vmupro_sdk.h>

#include "sms.h"

extern "C" {
#include "smsplus/shared.h"
};

static const char *kLogSMSEmu = "[SMSPLUS]";

static constexpr size_t SMS_SCREEN_WIDTH   = 256;
static constexpr size_t SMS_VISIBLE_HEIGHT = 192;

static constexpr size_t GG_SCREEN_WIDTH   = 160;
static constexpr size_t GG_VISIBLE_HEIGHT = 144;

static char *launchfile             = nullptr;
static uint8_t *rombuffer           = nullptr;
static int frame_buffer_offset      = 48;  // for game gear apparently
static bool initialised             = false;
static uint8_t *sms_back_buffer     = nullptr;
volatile uint32_t *sms_audio_buffer = nullptr;
static uint8_t *sms_sram            = nullptr;
static uint8_t *sms_ram             = nullptr;
static uint8_t *sms_vdp_vram        = nullptr;
static uint8_t *pauseBuffer         = nullptr;

static bool emuRunning              = true;
static bool inOptionsMenu           = false;
static int frame_counter            = 0;
static int smsContextSelectionIndex = 0;
static int smsCurrentPaletteIndex   = 0;
static int renderFrame              = 1;
static uint32_t num_frames          = 0;
static uint64_t frame_time          = 0.0f;
static uint64_t frame_time_total    = 0.0f;
static uint64_t frame_time_max      = 0.0f;
static uint64_t frame_time_min      = 0.0f;
static float frame_time_avg         = 0.0f;

enum class EmulatorMenuState { EMULATOR_RUNNING, EMULATOR_MENU, EMULATOR_CONTEXT_MENU, EMULATOR_SETTINGS_MENU };

typedef enum ContextMenuEntryType {
  MENU_ACTION,
  MENU_OPTION_VOLUME,
  MENU_OPTION_BRIGHTNESS,
  MENU_OPTION_PALETTE,
  MENU_OPTION_SCALING,
  MENU_OPTION_STATE_SLOT,
  MENU_OPTION_NONE
};

typedef struct ContextMenuEntry_s {
  const char *title;
  bool enabled              = true;
  ContextMenuEntryType type = MENU_ACTION;
} ContextMenuEntry;

const ContextMenuEntry emuContextEntries[5] = {
    {.title = "Save & Continue", .enabled = true, .type = MENU_ACTION},
    {.title = "Load Game", .enabled = true, .type = MENU_ACTION},
    {.title = "Restart", .enabled = true, .type = MENU_ACTION},
    {.title = "Options", .enabled = true, .type = MENU_ACTION},
    {.title = "Quit", .enabled = true, .type = MENU_ACTION},
};

const ContextMenuEntry emuContextOptionEntries[5] = {
    {.title = "Volume", .enabled = true, .type = MENU_OPTION_VOLUME},
    {.title = "Brightness", .enabled = true, .type = MENU_OPTION_BRIGHTNESS},
    {.title = "Palette", .enabled = true, .type = MENU_OPTION_PALETTE},
    {.title = "State Slot", .enabled = true, .type = MENU_OPTION_STATE_SLOT},
    {.title = "", .enabled = false, .type = MENU_OPTION_NONE},
};

static const char *PaletteNames[]             = {"Nofrendo", "Composite", "Classic", "NTSC", "PVM", "Smooth"};
static EmulatorMenuState currentEmulatorState = EmulatorMenuState::EMULATOR_RUNNING;

void sms_frame(int skip_render);
void sms_init(void);
void sms_reset(void);

extern "C" void system_manage_sram(uint8_t *sram, int cartslot, int mode) {}

static void update_frame_time(uint64_t ftime) {
  num_frames++;
  frame_time       = ftime;
  frame_time_total = frame_time_total + frame_time;
}

static void reset_frame_time() {
  num_frames       = 0;
  frame_time       = 0;
  frame_time_total = 0;
  frame_time_max   = 0;
  frame_time_min   = 1000000;  // some large number
  frame_time_avg   = 0.0f;
}

static float get_fps() {
  if (frame_time_total == 0) {
    return 0.0f;
  }
  return num_frames / (frame_time_total / 1e6f);
}

static bool savaStateHandler(const char *filename) {
  // Save to a file named after the game + state (.ggstate)
  char filepath[512];
  vmupro_snprintf(filepath, 512, "/sdcard/roms/%s/%sstate", "MasterSystem", filename);
  FILE *f = fopen(filepath, "w");
  if (f) {
    system_save_state(f);
    fclose(f);
    return true;
  }

  return false;
}

static bool loadStateHandler(const char *filename) {
  char filepath[512];
  vmupro_snprintf(filepath, 512, "/sdcard/roms/%s/%sstate", "MasterSystem", filename);

  FILE *f = fopen(filepath, "r");
  if (f) {
    system_load_state(f);
    fclose(f);
    return true;
  }

  system_reset();
  return false;
}

void Tick() {
  static constexpr uint64_t max_frame_time = 1000000 / 60.0f;  // microseconds per frame
  vmupro_display_clear(VMUPRO_COLOR_BLACK);
  vmupro_display_refresh();

  int64_t next_frame_time = vmupro_get_time_us();
  int64_t lastTime        = next_frame_time;
  int64_t accumulated_us  = 0;

  while (emuRunning) {
    int64_t currentTime = vmupro_get_time_us();
    float fps_now       = get_fps();

    vmupro_btn_read();

    if (currentEmulatorState == EmulatorMenuState::EMULATOR_MENU) {
      vmupro_display_clear(VMUPRO_COLOR_BLACK);
      // vmupro_blit_buffer_dithered(pauseBuffer, 0, 0, 240, 240, 2);
      vmupro_blit_buffer_blended(pauseBuffer, 0, 0, 240, 240, 150);
      // We'll only use our front buffer for this to simplify things
      int startY = 50;
      vmupro_draw_fill_rect(40, 37, 200, 170, VMUPRO_COLOR_NAVY);

      vmupro_set_font(VMUPRO_FONT_OPEN_SANS_15x18);
      for (int x = 0; x < 5; ++x) {
        uint16_t fgColor = smsContextSelectionIndex == x ? VMUPRO_COLOR_NAVY : VMUPRO_COLOR_WHITE;
        uint16_t bgColor = smsContextSelectionIndex == x ? VMUPRO_COLOR_WHITE : VMUPRO_COLOR_NAVY;
        if (smsContextSelectionIndex == x) {
          vmupro_draw_fill_rect(50, startY + (x * 22), 190, (startY + (x * 22) + 20), VMUPRO_COLOR_WHITE);
        }
        if (inOptionsMenu) {
          if (!emuContextOptionEntries[x].enabled) fgColor = VMUPRO_COLOR_GREY;
          vmupro_draw_text(emuContextOptionEntries[x].title, 60, startY + (x * 22), fgColor, bgColor);
          // For options, we need to draw the current option value right aligned within the selection boundary
          switch (emuContextOptionEntries[x].type) {
            case MENU_OPTION_BRIGHTNESS: {
              char currentBrightness[5] = "0%";
              vmupro_snprintf(currentBrightness, 6, "%d%%", (vmupro_get_global_brightness() * 10) + 10);
              int tlen = vmupro_calc_text_length(currentBrightness);
              vmupro_draw_text(currentBrightness, 190 - tlen - 5, startY + (x * 22), fgColor, bgColor);
            } break;
            case MENU_OPTION_VOLUME: {
              char currentVolume[5] = "0%";
              vmupro_snprintf(currentVolume, 6, "%d%%", (vmupro_get_global_volume() * 10) + 10);
              int tlen = vmupro_calc_text_length(currentVolume);
              vmupro_draw_text(currentVolume, 190 - tlen - 5, startY + (x * 22), fgColor, bgColor);
            } break;
            case MENU_OPTION_PALETTE: {
              int tlen = vmupro_calc_text_length(PaletteNames[smsCurrentPaletteIndex]);
              vmupro_draw_text(
                  PaletteNames[smsCurrentPaletteIndex], 190 - tlen - 5, startY + (x * 22), fgColor, bgColor
              );
            } break;
            default:
              break;
          }
        }
        else {
          if (!emuContextEntries[x].enabled) fgColor = VMUPRO_COLOR_GREY;
          vmupro_draw_text(emuContextEntries[x].title, 60, startY + (x * 22), fgColor, bgColor);
        }
      }
      vmupro_display_refresh();

      if (vmupro_btn_pressed(Btn_B)) {
        // Exit the context menu, resume execution
        // Reset our frame counters otherwise the emulation
        // will run at full blast :)
        if (inOptionsMenu) {
          inOptionsMenu = false;
        }
        else {
          if (smsContextSelectionIndex != 4) {
            vmupro_resume_double_buffer_renderer();
          }
          reset_frame_time();
          lastTime             = vmupro_get_time_us();
          accumulated_us       = 0;
          currentEmulatorState = EmulatorMenuState::EMULATOR_RUNNING;
        }
      }
      else if (vmupro_btn_pressed(Btn_A) && !inOptionsMenu) {
        // Get the selection index. What are we supposed to do?
        if (smsContextSelectionIndex == 0) {
          vmupro_resume_double_buffer_renderer();
          // Save in both cases
          savaStateHandler((const char *)launchfile);

          // Close the modal
          reset_frame_time();
          lastTime             = vmupro_get_time_us();
          accumulated_us       = 0;
          currentEmulatorState = EmulatorMenuState::EMULATOR_RUNNING;
        }
        else if (smsContextSelectionIndex == 1) {
          vmupro_resume_double_buffer_renderer();
          loadStateHandler((const char *)launchfile);

          // Close the modal
          reset_frame_time();
          lastTime             = vmupro_get_time_us();
          accumulated_us       = 0;
          currentEmulatorState = EmulatorMenuState::EMULATOR_RUNNING;
        }
        else if (smsContextSelectionIndex == 2) {  // Reset
          vmupro_resume_double_buffer_renderer();
          reset_frame_time();
          lastTime       = vmupro_get_time_us();
          accumulated_us = 0;
          system_reset();
          currentEmulatorState = EmulatorMenuState::EMULATOR_RUNNING;
        }
        else if (smsContextSelectionIndex == 3) {  // Options
          // let's change a flag, let the rendered in the next iteration re-render the menu
          inOptionsMenu = true;
        }
        else if (smsContextSelectionIndex == 4) {  // Quit
          emuRunning = false;
          system_shutdown();
        }
      }
      else if (vmupro_btn_pressed(DPad_Down)) {
        if (smsContextSelectionIndex == 4)
          smsContextSelectionIndex = 0;
        else
          smsContextSelectionIndex++;
      }
      else if (vmupro_btn_pressed(DPad_Up)) {
        if (smsContextSelectionIndex == 0)
          smsContextSelectionIndex = 4;
        else
          smsContextSelectionIndex--;
      }
      else if ((vmupro_btn_pressed(DPad_Right) || vmupro_btn_pressed(DPad_Left)) && inOptionsMenu) {
        // Adjust the setting
        switch (emuContextOptionEntries[smsContextSelectionIndex].type) {
          case MENU_OPTION_BRIGHTNESS: {
            uint8_t currentBrightness = vmupro_get_global_brightness();
            uint8_t newBrightness     = 1;
            if (vmupro_btn_pressed(DPad_Right)) {
              newBrightness = currentBrightness < 10 ? currentBrightness + 1 : 10;
            }
            else {
              newBrightness = currentBrightness > 0 ? currentBrightness - 1 : 0;
            }
            // settings->brightness = newBrightness;
            vmupro_set_global_brightness(newBrightness);
            // OLEDDisplay::getInstance()->setBrightness(newBrightness, true, false);
          } break;
          case MENU_OPTION_VOLUME: {
            uint8_t currentVolume = vmupro_get_global_volume();
            uint8_t newVolume     = 1;
            if (vmupro_btn_pressed(DPad_Right)) {
              newVolume = currentVolume < 10 ? currentVolume + 1 : 10;
            }
            else {
              newVolume = currentVolume > 0 ? currentVolume - 1 : 0;
            }
            vmupro_set_global_volume(newVolume);
          } break;
          case MENU_OPTION_PALETTE:
            break;
          default:
            break;
        }
      }
    }
    else {
      input.pad[0] = 0;

      input.pad[0] |= vmupro_btn_held(DPad_Up) ? INPUT_UP : 0;
      input.pad[0] |= vmupro_btn_held(DPad_Down) ? INPUT_DOWN : 0;
      input.pad[0] |= vmupro_btn_held(DPad_Left) ? INPUT_LEFT : 0;
      input.pad[0] |= vmupro_btn_held(DPad_Right) ? INPUT_RIGHT : 0;
      input.pad[0] |= vmupro_btn_held(Btn_B) ? INPUT_BUTTON2 : 0;
      input.pad[0] |= vmupro_btn_held(Btn_A) ? INPUT_BUTTON1 : 0;

      // pad[1] is player 1
      input.pad[1] = 0;

      // system is where we input the start button, as well as soft reset
      input.system = 0;
      input.system |= vmupro_btn_held(Btn_Mode) ? INPUT_START : 0;
      input.system |= vmupro_btn_held(Btn_Power) ? INPUT_PAUSE : 0;  // Master System Only

      // The bottom button is for bringing up the menu overlay!
      if (vmupro_btn_pressed(Btn_Bottom)) {
        // Copy the rendered framebuffer to our pause buffer
        vmupro_delay_ms(16);

        // get the last rendered buffer
        if (vmupro_get_last_blitted_fb_side() == 0) {
          memcpy(pauseBuffer, vmupro_get_front_fb(), 115200);
        }
        else {
          memcpy(pauseBuffer, vmupro_get_back_fb(), 115200);
        }
        vmupro_pause_double_buffer_renderer();

        // Set the state to show the overlay
        currentEmulatorState = EmulatorMenuState::EMULATOR_MENU;

        // Delay by one frame to allow the display to finish refreshing
        vmupro_delay_ms(16);
        continue;
      }

      if (renderFrame) {
        bitmap.data = vmupro_get_back_buffer();
        system_frame(0);
        vmupro_push_double_buffer_frame();
      }
      else {
        system_frame(1);
        renderFrame = 1;
      }

      int16_t *buff16 = (int16_t *)sms_audio_buffer;
      for (int x = 0; x < sms_snd.sample_count; ++x) {
        buff16[x * 2]     = sms_snd.output[0][x];  // Left
        buff16[x * 2 + 1] = sms_snd.output[1][x];  // Right
      }


      vmupro_audio_add_stream_samples(
          (int16_t *)sms_audio_buffer, sms_snd.sample_count * 2, vmupro_stereo_mode_t::VMUPRO_AUDIO_STEREO, true
      );

      ++frame_counter;

      int64_t elapsed_us = vmupro_get_time_us() - currentTime;
      int64_t sleep_us   = max_frame_time - elapsed_us + accumulated_us;

      vmupro_log(
          VMUPRO_LOG_INFO,
          kLogSMSEmu,
          "loop %i, fps: %.2f, elapsed: %lli, sleep: %lli",
          frame_counter,
          fps_now,
          elapsed_us,
          sleep_us
      );

      if (sleep_us > 360) {
        vmupro_delay_us(sleep_us - 360);  // subtract 360 micros for jitter
        accumulated_us = 0;
      }
      else if (sleep_us < 0) {
        renderFrame    = 0;
        accumulated_us = sleep_us;
      }

      update_frame_time(currentTime - lastTime);
      lastTime = currentTime;
    }
  }
}

void Exit() {}

void app_main(void) {
  vmupro_log(VMUPRO_LOG_INFO, kLogSMSEmu, "Starting SMSPLUS", option.version);

  vmupro_emubrowser_settings_t emuSettings = {
      .title = "Master System", .rootPath = "/sdcard/roms/MasterSystem", .filterExtension = ".sms"
  };
  vmupro_emubrowser_init(emuSettings);

  launchfile = (char *)malloc(512);
  memset(launchfile, 0x00, 512);
  vmupro_emubrowser_render_contents(launchfile);
  if (strlen(launchfile) == 0) {
    vmupro_log(VMUPRO_LOG_ERROR, kLogSMSEmu, "We somehow exited the browser with no file to show!");
    return;
  }

  char launchPath[512 + 22];
  vmupro_snprintf(launchPath, (512 + 22), "/sdcard/roms/MasterSystem/%s", launchfile);

  size_t romfilesize = 0;
  if (vmupro_file_exists(launchPath)) {
    romfilesize = vmupro_get_file_size(launchPath);
    if (romfilesize > 0) {
      rombuffer = (uint8_t *)malloc(romfilesize);
      bool res  = vmupro_read_file_complete(launchPath, rombuffer, &romfilesize);
      if (!res) {
        vmupro_log(VMUPRO_LOG_ERROR, kLogSMSEmu, "Error reading file %s", launchPath);
        free(rombuffer);
        rombuffer = nullptr;
      }
    }
    else {
      vmupro_log(VMUPRO_LOG_ERROR, kLogSMSEmu, "File %s is empty!", launchPath);
      return;
    }
  }
  else {
    vmupro_log(VMUPRO_LOG_ERROR, kLogSMSEmu, "File %s does not exist!", launchPath);
    return;
  }

  load_rom_data(rombuffer, romfilesize);

  if (sms.console == CONSOLE_GGMS || sms.console <= CONSOLE_SMS2) {
    sms.console                   = CONSOLE_SMS;
    sms.display                   = DISPLAY_NTSC;
    sms.territory                 = TERRITORY_DOMESTIC;
    bitmap.vmpro_output.cropped   = 1;
    bitmap.vmpro_output.fill      = 0;
    bitmap.vmpro_output.xStart    = 8;
    bitmap.vmpro_output.fboffset  = 0;
    bitmap.vmpro_output.yOffset   = 24;
    bitmap.vmpro_output.outWidth  = SMS_SCREEN_WIDTH;
    bitmap.vmpro_output.outHeight = SMS_VISIBLE_HEIGHT;
    frame_buffer_offset           = 0;
  }
  else {
    sms.console                   = CONSOLE_GG;
    sms.display                   = DISPLAY_NTSC;
    sms.territory                 = TERRITORY_DOMESTIC;
    bitmap.vmpro_output.cropped   = 0;
    bitmap.vmpro_output.fill      = 1;
    bitmap.vmpro_output.xStart    = 0;
    bitmap.vmpro_output.yOffset   = 12;
    bitmap.vmpro_output.fboffset  = 48;
    bitmap.vmpro_output.outWidth  = GG_SCREEN_WIDTH;
    bitmap.vmpro_output.outHeight = GG_VISIBLE_HEIGHT;
    frame_buffer_offset           = 48;
  }

  if (!initialised) {
    vmupro_log(VMUPRO_LOG_INFO, kLogSMSEmu, "Initialising renderer and RAM");

    vmupro_start_double_buffer_renderer();
    sms_back_buffer = vmupro_get_back_buffer();

    // We need to allocate some memory for SRAM, RAM, VDP RAM and the audio buffer
    sms_sram     = (uint8_t *)malloc(0x8000);
    sms_ram      = (uint8_t *)malloc(0x2000);
    sms_vdp_vram = (uint8_t *)malloc(0x4000);
    pauseBuffer  = (uint8_t *)malloc(115200);

    // We need a maximum of 736 stereo 16-bit samples at 44.1khz at 60fps. This works out to
    // 736 x 4 = 2944 bytes
    // We also zero out the allocated buffer to prevent any cracks during startup
    sms_audio_buffer = (uint32_t *)malloc(736 * sizeof(uint32_t));
    memset((void *)sms_audio_buffer, 0x00, 736 * sizeof(uint32_t));

    // Now check everything was allocated successfully
    if (!sms_sram || !sms_ram || !sms_vdp_vram || !sms_audio_buffer) {
      vmupro_log(VMUPRO_LOG_ERROR, kLogSMSEmu, "Error initialising memory");
      return;
    }
  }

  // Now setup the options for the emulator
  bitmap.width  = SMS_SCREEN_WIDTH;
  bitmap.height = SMS_VISIBLE_HEIGHT;
  bitmap.pitch  = bitmap.width;
  bitmap.data   = sms_back_buffer;

  cart.sram  = sms_sram;
  sms.wram   = sms_ram;
  sms.use_fm = 0;
  vdp.vram   = sms_vdp_vram;

  set_option_defaults();

  option.sndrate  = 44100;
  option.overscan = 0;
  option.extra_gg = 0;

  system_init2();
  system_reset();

  frame_counter = 0;
  initialised   = true;

  vmupro_audio_start_listen_mode();

  Tick();

  Exit();

  vmupro_log(VMUPRO_LOG_INFO, kLogSMSEmu, "SMSPlus %s Init Done", option.version);
}
