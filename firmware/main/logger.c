#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

#define DHT_GPIO 4

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


void app_main(void) {
    nvs_flash_init();
    wifi_connect();
    setup_time();

    float temperature = 0;
    float humidity = 0;

    while (1) {
        char body[256];
        esp_err_t res = dht_read_float_data(DHT_TYPE_AM2301, DHT_GPIO, &humidity, &temperature);

        time_t now;
        time(&now);

        snprintf(body, sizeof(body),
            "{"
            "\"device_id\":\"esp32-1\", "
            "\"temperature\":%.1f, "
            "\"humidity\":%.1f, "
            "\"timestamp\": %llu"
            "}",
            temperature, humidity, now
        );

        esp_http_client_config_t config = { .url = URL };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, body, strlen(body));
        esp_http_client_perform(client);
        esp_http_client_cleanup(client);

        if (res == ESP_OK) {
            ESP_LOGI(
                "DHT", "Temp: %.1f °C, Humidity: %.1f %%, Timestamp: %llu",
                temperature, humidity, now);
        } else {
            ESP_LOGE("DHT", "Error reading data: %s", esp_err_to_name(res));
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}