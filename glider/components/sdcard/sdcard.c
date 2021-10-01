#include "sdcard.h"

static const char *SDTAG = "sdcard";
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t* card;
const char mount_point[] = MOUNT_POINT;

void SdCardInit(){
    Queue = xQueueCreate(10, sizeof(Data_t));
    if(Queue == 0)
    {
        ESP_LOGE(SDTAG, "Failed to create queue");
    }
    esp_err_t ret;
        // Options for mounting the filesystem.
        // If format_if_mount_failed is set to true, SD card will be partitioned and
        // formatted in case when mounting fails.
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
            .format_if_mount_failed = true,
    #else
            .format_if_mount_failed = false,
    #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };
   
    ESP_LOGI(SDTAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(SDTAG, "Using SPI peripheral");

   
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(SDTAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);


    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SDTAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SDTAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }

    struct stat st;
    if (stat(MOUNT_POINT"/log.csv", &st) == 0) {
        // Delete it if it exists
        unlink(MOUNT_POINT"/log.csv");
    }
}

void SdCardUnmount(){
    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(SDTAG, "Card unmounted");
    //deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
}
void SdCardLog(uint8_t type, char* tag, char* text){
    Data_t buf;
    buf.timestamp = esp_log_timestamp();
    buf.type = type;
    snprintf(buf.tag,10, "%s", tag);
    snprintf(buf.text,20, "%s", text);
    if(xQueueSend(Queue, &buf, 0) != pdPASS){
        ESP_LOGW(SDTAG,"Failed to send data to queue!");
    }
}
void SdCardTask(void* arg){
    SdCardInit();
    Data_t buf;
    while(1){
        if( Queue != 0 )
        {
            ESP_LOGI(SDTAG, "Opening file");
            FILE* f = fopen(MOUNT_POINT"/log.csv", "a");
            if (f == NULL) {
                ESP_LOGE(SDTAG, "Failed to open file for writing");
                return;
            }

            for (size_t i = 0; i < 10 && uxQueueMessagesWaiting(Queue) != 0; i++)
                if( xQueueReceive( Queue, &buf, 0 ) )
                    fprintf(f, "%d,%d,%s,%s\n", buf.timestamp,buf.type,buf.tag,buf.text); 

            fclose(f);
            ESP_LOGI(SDTAG, "File written");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}