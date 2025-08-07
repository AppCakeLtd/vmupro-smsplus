#include <string.h>
#include <vmupro_sdk.h>

#include "sms.h"

extern "C" {
#include "smsplus/shared.h"
};

void sms_frame(int skip_render);
void sms_init(void);
void sms_reset(void);

static const char *kLogSMSEmu = "[SMSPLUS]";

static constexpr size_t SMS_SCREEN_WIDTH   = 256;
static constexpr size_t SMS_VISIBLE_HEIGHT = 192;

static constexpr size_t GG_SCREEN_WIDTH   = 160;
static constexpr size_t GG_VISIBLE_HEIGHT = 144;

static char *launchfile        = nullptr;
static uint8_t *rombuffer      = nullptr;
static int frame_buffer_offset = 48;  // for game gear apparently
static bool initialised        = false;

void app_main(void) {
  set_option_defaults();
  vmupro_log(VMUPRO_LOG_INFO, kLogSMSEmu, "Starting %s", option.version);

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
    }
}
