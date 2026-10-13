#include "wifi_time.h"

#include "esp_log.h"

static const char *TAG = "wifi_time";
static volatile bool synced;

bool wifi_time_synced(void)
{
    return synced;
}

#if __has_include("secrets.h")

#include <string.h>
#include <sys/time.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "secrets.h"

static void on_time(struct timeval *tv)
{
    (void)tv;
    synced = true;
    ESP_LOGI(TAG, "clock set from the internet");
}

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi not connected; trying again");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi connected");
    }
}

/* Log and give up on Wi-Fi if a set-up call fails. */
#define TRY(call)                                                        \
    do {                                                                 \
        esp_err_t err_ = (call);                                         \
        if (err_ != ESP_OK) {                                            \
            ESP_LOGW(TAG, "%s failed (%s): no Wi-Fi time", #call, esp_err_to_name(err_)); \
            return;                                                      \
        }                                                                \
    } while (0)

void wifi_time_start(void)
{
    esp_err_t nvs = nvs_flash_init();
    if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TRY(nvs_flash_erase());
        nvs = nvs_flash_init();
    }
    TRY(nvs);
    TRY(esp_netif_init());
    TRY(esp_event_loop_create_default());
    if (esp_netif_create_default_wifi_sta() == NULL) {
        ESP_LOGW(TAG, "cannot create the Wi-Fi interface: no Wi-Fi time");
        return;
    }

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    TRY(esp_wifi_init(&init));
    TRY(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL, NULL));
    TRY(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL, NULL));

    wifi_config_t config = {0};
    strncpy((char *)config.sta.ssid, WIFI_SSID, sizeof(config.sta.ssid) - 1);
    strncpy((char *)config.sta.password, WIFI_PASSWORD, sizeof(config.sta.password) - 1);
    TRY(esp_wifi_set_mode(WIFI_MODE_STA));
    TRY(esp_wifi_set_config(WIFI_IF_STA, &config));

    /* Ask a time server once connected, and again every hour (the default). */
    esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    sntp.sync_cb = on_time;
    TRY(esp_netif_sntp_init(&sntp));

    TRY(esp_wifi_start());
    ESP_LOGI(TAG, "joining Wi-Fi in the background");
}

#else

void wifi_time_start(void)
{
    ESP_LOGW(TAG, "no secrets.h: Wi-Fi is off and the clock runs from the build time");
}

#endif
