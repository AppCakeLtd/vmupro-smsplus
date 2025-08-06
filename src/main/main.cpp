#include <string.h>
#include <vmupro_sdk.h>

#include "sms.h"

extern "C" {
#include "smsplus/shared.h"
};

static const char *kLogNESEmu = "[SMSPLUS]";

void app_main(void) { vmupro_log(VMUPRO_LOG_INFO, kLogNESEmu, "Starting %s", option.version); }
