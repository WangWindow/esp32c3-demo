// LED 与 WiFi 演示
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define WIFI_SSID "CMCC-888"     // 修改为你的路由器SSID
#define WIFI_PASS "wyghbq800325" // 修改为你的路由器密码
#define WIFI_MAX_RETRY 5

#define LED_GPIO GPIO_NUM_8
#define BLINK_DELAY_MS 500

static const char* TAG = "APP";

static EventGroupHandle_t s_wifi_event_group;
static int                s_retry_num = 0;

enum {
    WIFI_CONNECTED_BIT = BIT0,
    WIFI_FAIL_BIT      = BIT1,
};

static void print_wifi_info(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        esp_netif_t*        netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_netif_ip_info_t ip_info;
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            uint8_t mac[6];
            esp_wifi_get_mac(WIFI_IF_STA, mac);
            ESP_LOGI(TAG, "Connected SSID: %s", (char*)ap_info.ssid);
            ESP_LOGI(TAG, "Channel: %d  RSSI: %d dBm", ap_info.primary, ap_info.rssi);
            ESP_LOGI(TAG,
                     "IP: %d.%d.%d.%d  GW: %d.%d.%d.%d  MASK: %d.%d.%d.%d",
                     IP2STR(&ip_info.ip),
                     IP2STR(&ip_info.gw),
                     IP2STR(&ip_info.netmask));
            ESP_LOGI(TAG,
                     "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0],
                     mac[1],
                     mac[2],
                     mac[3],
                     mac[4],
                     mac[5]);
            const char* hostname = NULL;
            esp_netif_get_hostname(netif, &hostname);
            if (hostname)
                ESP_LOGI(TAG, "Hostname: %s", hostname);
            ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        }
    }
}

static void wifi_event_handler(void*            arg,
                               esp_event_base_t event_base,
                               int32_t          event_id,
                               void*            event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retry to connect to the AP (%d/%d)", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {0};
    strlcpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable    = true;
    wifi_config.sta.pmf_cfg.required   = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi start STA, connecting to: %s", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(15000));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
        print_wifi_info(); // 只打印一次
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "WiFi connect timeout");
    }
}

void app_main() {
    // 初始化 NVS (WiFi 需要)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta(); // 连接 WiFi 并打印一次信息

    // LED GPIO 配置
    gpio_config_t io_conf = {.pin_bit_mask = 1ULL << LED_GPIO,
                             .mode         = GPIO_MODE_OUTPUT,
                             .pull_up_en   = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type    = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    int level = 0;
    while (1) {
        level = !level;
        gpio_set_level(LED_GPIO, level);
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY_MS));
    }
}