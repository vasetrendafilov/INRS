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
#include "freertos/queue.h"

#define MOUNT_POINT "/sdcard"
#define SPI_DMA_CHAN    1

#define PIN_NUM_MISO 4
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13


QueueHandle_t Queue;
typedef struct
{
  uint32_t timestamp;
  uint8_t type;
  char tag[10];
  char text[20];  
} Data_t;

void SdCardInit();
void SdCardLog(uint8_t type, char* tag, char* text);
void SdCardTask(void* arg);
void SdCardUnmount();

#endif