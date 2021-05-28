/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
 // hx711
QueueHandle_t queue;
typedef struct
{
  uint32_t timestamp;
  long weight;  
} Data_t;

#include "HX711.h"
#define AVG_SAMPLES   1
#define GPIO_DATA   GPIO_NUM_19
#define GPIO_SCLK   GPIO_NUM_18
float scale =  447.878;
long weight_arr[30];
int i=0;
TaskHandle_t loadcellHandle = NULL;
//end

static void weight_reading_task(void* arg);

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

// load cell handelers ------------------------------------------------------------------------------------------------
static esp_err_t loadcell_init_get_handler(httpd_req_t *req)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_22) | (1ULL<<GPIO_NUM_23);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    HX711_init(GPIO_DATA,GPIO_SCLK,eGAIN_128); 
    HX711_set_scale(scale);
    queue = xQueueCreate(20, sizeof(Data_t));
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "scale", scale);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
static esp_err_t loadcell_tare_get_handler(httpd_req_t *req)
{
    ESP_LOGI(REST_TAG, "****************** Initialing weight sensor ***************");
    bool created;
    created = xTaskCreatePinnedToCore(weight_reading_task, "weight_reading_task", 2048, NULL, 1, &loadcellHandle,1); 
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "created", created);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
/*static esp_err_t loadcell_calibrate_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
       
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
          
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int weight = cJSON_GetObjectItem(root, "weight")->valueint; 
    cJSON_Delete(root);
    // bi rekol delay nes da ne se bugne ili predvreme da go stopneme
    if( loadcellHandle != NULL )
        vTaskSuspend(loadcellHandle);
    long reading = HX711_get_units(10);
    scale = reading / (float)weight;
    HX711_set_scale(scale);
    if( loadcellHandle != NULL )
        vTaskResume(loadcellHandle);

    httpd_resp_set_type(req, "application/json");
    cJSON *root1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(root1, "scale", scale);
    const char *sys_info = cJSON_Print(root1);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root1);
    return ESP_OK;
}*/
static esp_err_t loadcell_resume_get_handler(httpd_req_t *req)
{
    if( loadcellHandle != NULL )
        vTaskResume(loadcellHandle);
    httpd_resp_sendstr(req, "Loadcell Resumed");
    return ESP_OK;
}
static esp_err_t loadcell_pause_get_handler(httpd_req_t *req)
{
    if( loadcellHandle != NULL )
        vTaskSuspend(loadcellHandle);
    httpd_resp_sendstr(req, "Loadcell Paused");
    return ESP_OK;
}
static esp_err_t staticfire_get_handler(httpd_req_t *req)
{   
    gpio_set_level(GPIO_NUM_22,1);
    gpio_set_level(GPIO_NUM_23,0);
    httpd_resp_sendstr(req, "Static Fire!!");
    return ESP_OK;
}
/* Simple handler for getting temperature data */
static esp_err_t loadcell_data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    
    char str[128];
    Data_t buf;
    int j=0;
    int index = 0;
    for (j=0; j<6; j++){
        xQueueReceive(queue, &buf, 0);
        index += snprintf(&str[index], 128-index, "%d ", buf.timestamp);
        index += snprintf(&str[index], 128-index, "%ld ", buf.weight);
    }
    cJSON_AddStringToObject(root,"weight",str);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
static void weight_reading_task(void* arg)
{
    Data_t data;
    HX711_tare();

    while(1)
    {
        data.weight = HX711_get_units(AVG_SAMPLES);
        data.timestamp = esp_log_timestamp();
        ESP_LOGI(REST_TAG, "%ld \n", data.weight);
        xQueueSend(queue, &data, 0);
        //vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    // URI handelers for loadcell
    httpd_uri_t loadcell_init_get_uri = {
        .uri = "/api/v1/loadcell/init",
        .method = HTTP_GET,
        .handler = loadcell_init_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &loadcell_init_get_uri);

    httpd_uri_t loadcell_tare_get_uri = {
        .uri = "/api/v1/loadcell/tare",
        .method = HTTP_GET,
        .handler = loadcell_tare_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &loadcell_tare_get_uri);

    /*httpd_uri_t loadcell_calibrate_post_uri = {
        .uri = "/api/v1/loadcell/calibrate",
        .method = HTTP_POST,
        .handler = loadcell_calibrate_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &loadcell_calibrate_post_uri);
    */
    httpd_uri_t loadcell_resume_get_uri = {
        .uri = "/api/v1/loadcell/resume",
        .method = HTTP_GET,
        .handler = loadcell_resume_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &loadcell_resume_get_uri);

    httpd_uri_t loadcell_pause_get_uri = {
        .uri = "/api/v1/loadcell/pause",
        .method = HTTP_GET,
        .handler = loadcell_pause_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &loadcell_pause_get_uri);

    /* URI handler for fetching loadcell data */
    httpd_uri_t loadcell_data_get_uri = {
        .uri = "/api/v1/loadcell/data",
        .method = HTTP_GET,
        .handler = loadcell_data_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &loadcell_data_get_uri);

      httpd_uri_t staticfire_get_uri = {
        .uri = "/api/v1/staticfire",
        .method = HTTP_GET,
        .handler = staticfire_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &staticfire_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
