#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bmp180.h"
#include "esp_log.h"
#include "dht.h"
#include <time.h>
#include <esp_sntp.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"

#include "secrets.h"

#define REFERENCE_PRESSURE 101325

#define DHT_GPIO 4
#define I2C_PIN_SDA 21
#define I2C_PIN_SCL 22

typedef struct {
    char device_id[32];
    float temperature;
    float humidity;
    uint32_t pressure;
    float altitude;
    time_t time;
} sensor_data_t;

volatile sensor_data_t sensor_data;

void dht22_task(void *pvParameter) {
    while(1) {
        float temp, hum;
        if (dht_read_float_data(DHT_TYPE_AM2301, DHT_GPIO, &hum, &temp) == ESP_OK) {
            sensor_data.temperature = temp;
            sensor_data.humidity = hum;
            ESP_LOGI("DHT22", "Temp: %.1f C Hum: %.1f%%", temp, hum);
        } 
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void bmp180_task(void *pvParameter) {


    while(1) {
        uint32_t pressure;
        float altitude;
        float temperature;
        if (
            bmp180_read_pressure(&pressure) == ESP_OK &&
            bmp180_read_altitude(REFERENCE_PRESSURE, &altitude) == ESP_OK &&
            bmp180_read_temperature(&temperature) == ESP_OK
            ) {
                sensor_data.pressure = pressure;
                sensor_data.altitude = altitude;
                ESP_LOGI("BMP180", "Pres: %.1d Pa Alt: %.1f m Temp: %.1f C", pressure, altitude, temperature);
            }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void wifi_connect() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS } };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

void setup_time() {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    while (timeinfo.tm_year < (2020 - 1900)) {
        time(&now);
        localtime_r(&now, &timeinfo);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void send_post_request(char body[256]) {
    esp_http_client_config_t config = {
        .url = URL,
        .timeout_ms = 5000
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE("HTTP", "Failed to init HTTP client");
        return;
    }
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI("HTTP", "POST succeed, status = %d", status);
    }
    else {
        ESP_LOGI("HTTP", "POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void http_sender_task(void *pvParameter) {
    while (1) {
        char body[256];
        time_t now;
        time(&now);
        snprintf(body, sizeof(body),
            "{"
            "\"device_id\":\"esp32-1\", "
            "\"temperature\":%.1f, "
            "\"humidity\":%.1f, "
            "\"pressure\":%.ld, "
            "\"altitude\":%.f, "
            "\"timestamp\": %llu"
            "}",
            sensor_data.temperature, sensor_data.humidity, sensor_data.pressure, sensor_data.altitude, now
        );

        send_post_request(body);

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


void app_main(void) {
    nvs_flash_init();
    wifi_connect();
    setup_time();
    esp_err_t err = bmp180_init(I2C_PIN_SDA, I2C_PIN_SCL);
    if (err != ESP_OK) {
        ESP_LOGI("BMP180", "BMP180 init failed with error = %d", err);
    }

    xTaskCreate(&bmp180_task, "bmp180_task", 1024*4, NULL, 5, NULL);
    xTaskCreate(&dht22_task, "dht22_task", 1024*4, NULL, 5, NULL);
    xTaskCreate(&http_sender_task, "http_sender_task", 1024*4, NULL, 5, NULL);
}