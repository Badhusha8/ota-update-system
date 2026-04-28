#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"

static const char *TAG = "OTA";

#define WIFI_SSID      CONFIG_WIFI_SSID
#define WIFI_PASSWORD  CONFIG_WIFI_PASSWORD
#define OTA_URL        "http://10.36.215.22:8070/esp32_ota.bin"
#define LED_PIN        GPIO_NUM_2

// ─── LED Blink Task ───────────────────────────────────────────────────────────

static void led_blink_task(void *pvParameter)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "LED blinking — firmware is running!");

    while (true) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ─── WiFi Event Handler ───────────────────────────────────────────────────────

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ─── WiFi Init ────────────────────────────────────────────────────────────────

static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        false, true, portMAX_DELAY);
}

// ─── OTA Task ─────────────────────────────────────────────────────────────────

static void ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA update from: %s", OTA_URL);

    esp_http_client_config_t http_config = {
        .url               = OTA_URL,
        .timeout_ms        = 10000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "OTA image size: %d bytes",
             esp_https_ota_get_image_size(ota_handle));

    // ── Download loop with progress ──
    while (true) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            int read  = esp_https_ota_get_image_len_read(ota_handle);
            int total = esp_https_ota_get_image_size(ota_handle);
            if (total > 0) {
                ESP_LOGI(TAG, "Progress: %d / %d bytes (%.1f%%)",
                         read, total, (float)read / total * 100.0f);
            }
        } else {
            break;
        }
    }

    if (!esp_https_ota_is_complete_data_received(ota_handle)) {
        ESP_LOGE(TAG, "OTA incomplete — download cut short");
        esp_https_ota_abort(ota_handle);
        vTaskDelete(NULL);
        return;
    }

    err = esp_https_ota_finish(ota_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA Success! Rebooting in 3 seconds...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
    }

    vTaskDelete(NULL);
}

// ─── App Main ─────────────────────────────────────────────────────────────────

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running from partition: %s", running->label);

    // Start LED immediately — proves this firmware is alive
    xTaskCreate(&led_blink_task, "led_blink_task", 2048, NULL, 5, NULL);

    // Always connect and check for OTA update
    wifi_init();
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
}
