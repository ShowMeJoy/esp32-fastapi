extern "C" {
    #include "ssd1306.h"
    #include "esp_timer.h"

    #include "esp_http_client.h"
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "esp_log.h"
    #include "nvs_flash.h"

    #include "esp_netif.h"
    #include "lwip/ip4_addr.h"

    #include "secrets.h"
}
#include "DHT.hpp"
static const char TAG[] = "logger";

void wifi_init_sta(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = { 0 };
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "WiFi started, waiting for connection...");
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK;
}

void send_data(float temp, float hum) {
    char json[128];
    snprintf(json, sizeof(json), 
            "{\"temperature\":%.2f, \"humidity\":%.2f,\"timestamp\":\"%lld\"}",
            temp, hum, esp_timer_get_time()/1000);

    esp_http_client_config_t config = {
        .url = URL,
        .event_handler = _http_event_handler};

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json, strlen(json));

    esp_err_t err = esp_http_client_perform(client);

    ESP_LOGI("HTTP", "POST status = %d, err = %d", esp_http_client_get_status_code(client), err);
    esp_http_client_cleanup(client);
}

void print_ip(void) {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_get_ip_info(netif, &ip_info);
    ESP_LOGI(TAG, "IP: %s", ip4addr_ntoa((const ip4_addr_t*)&ip_info.ip));
}

extern "C" void app_main(void) {
    char buf[32];
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "esp_wifi_connect done");
    wifi_init_sta();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    print_ip();

    init_ssd1306();
    DHT dht;
    dht.setDHTgpio(GPIO_NUM_18);
    ssd1306_print_str(18, 0, "Starting DHT!\n", false);

    while(1)
    {   
        int ret = dht.readDHT();
        dht.errorHandler(ret);

        float temp = dht.getTemperature();
        float hum = dht.getHumidity();
        snprintf(buf, sizeof(buf), "T:%.2f\nH:%.2f", temp, hum);
        ssd1306_print_str(0, 17, buf, false);

        send_data(temp, hum);

        ssd1306_display();
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}