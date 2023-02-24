#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

#include "ff.h"
#include "diskio.h"
extern "C" {
  #include <dirent.h>
}
#include "esp_vfs_fat.h"
#include "esp_task_wdt.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_spiffs.h"
#include "soc/efuse_reg.h"
#include "soc/rtc.h"
#include "esp_ipc.h"
#include "soc/adc_channel.h"

#include "fabutils.h"
#include "fabglconf.h"



#pragma GCC optimize ("O2")



namespace fabgl {




// derived from: https://linuxos.sk/blog/mirecove-dristy/detail/esp32-dynamicka-zmena-vzorkovacej-frekvencie-/
// A    : 1..63
// B    : 0..63
// N    : 2..254
// M    : 1..63
// ret: actual sample rate
int calcI2STimingParams(int sampleRate, int * outA, int * outB, int * outN, int * outM)
{
  *outM = 1;

  double N = (double)APB_CLK_FREQ / sampleRate;
  while (N >= 255) {
    ++(*outM);
    N = (double)APB_CLK_FREQ / sampleRate / *outM;
  }

  double min_error = 1.0;
  *outN = N;
  *outB = 0;
  *outA = 1;

  for (int a = 1; a < 64; ++a) {
    int b = (N - (double)(*outN)) * (double)a;
    if (b > 63)
      continue;

    double divisor = (double)(*outN) + (double)b / (double)a;
    double error = divisor > N ? divisor - N : N - divisor;
    if (error < min_error) {
      min_error = error;
      *outA = a;
      *outB = b;
    }

    ++b;
    if (b > 63)
      continue;
    divisor = (double)(*outN) + (double)b / (double)a;
    error = divisor > N ? divisor - N : N - divisor;
    if (error < min_error) {
      min_error = error;
      *outA = a;
      *outB = b;
    }
  }

  return APB_CLK_FREQ / ((double)(*outN) + (double)(*outB) / (*outA)) / *outM;
}

////////////////////////////////////////////////////////////////////////////////////////////
// esp_intr_alloc_pinnedToCore

struct esp_intr_alloc_args {
  int             source;
  int             flags;
  intr_handler_t  handler;
  void *          arg;
  intr_handle_t * ret_handle;
  TaskHandle_t    waitingTask;
};

int CoreUsage::s_busiestCore = FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE;

void esp_intr_alloc_pinnedToCore_call(void * arg)
{
  auto args = (esp_intr_alloc_args*) arg;
  esp_intr_alloc(args->source, args->flags, args->handler, args->arg, args->ret_handle);
}


void esp_intr_alloc_pinnedToCore(int source, int flags, intr_handler_t handler, void * arg, intr_handle_t * ret_handle, int core)
{
  esp_intr_alloc_args args = { source, flags, handler, arg, ret_handle, xTaskGetCurrentTaskHandle() };
  esp_ipc_call_blocking(core, esp_intr_alloc_pinnedToCore_call, &args);
}

}
