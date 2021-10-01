#ifndef SDCARD_h
#define SDCARD_h

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "driver/sdmmc_host.h"

#define MOUNT_POINT "/sdcard"
#define SPI_DMA_CHAN    1

#define PIN_NUM_MISO 4
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

void SdCardInit();
void SdCardLogStatus(char* data);
//void SdCardLogData();
void SdCardUnmount();

#endif
