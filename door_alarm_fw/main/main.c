#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdio.h>
#include "esp_log.h"
#include <stdlib.h>
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include <string.h>
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "driver/rtc_io.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#include "mbedtls/ctr_drbg.h"
#include <mbedtls/entropy.h>

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "[AP_NAME]"
#define EXAMPLE_ESP_WIFI_PASS      "[PASSWORD]"
#define EXAMPLE_ESP_MAXIMUM_RETRY  3
#define CONFIG_ESP_WIFI_AUTH_WPA2_PSK 1

#define EXAMPLE_STATIC_IP_ADDR        "[STATIC_IP]"
#define EXAMPLE_STATIC_GW_ADDR        "[GATEWAY]"
#define EXAMPLE_STATIC_NETMASK_ADDR   "[NETMASK]"

#define HOST "us-east-1-1.aws.cloud2.influxdata.com"
#define PATH "/api/v2/write?orgID=[ID]a08ec8&bucket=[BUCKET_NAME]&precision=ns"
#define TOKEN "Token [your token]"


#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

// RTC_DATA_ATTR uint8_t prev_level = 0;
uint8_t prev_level = 0;

adc_cali_handle_t                       cali_handle;
adc_oneshot_unit_handle_t               adc_handle;
const uint8_t reed_en = GPIO_NUM_10;
const uint8_t door_sensor = GPIO_NUM_5;
const uint8_t meas_en = GPIO_NUM_6;
const uint8_t led_en = GPIO_NUM_14;


float voltage;
uint8_t level = 0;


static char       TAG[] = "MAIN";

void adc_init(adc_unit_t adcunit, adc_channel_t chan, adc_atten_t atten, adc_bitwidth_t bitwidth)
{
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = adcunit};

    esp_err_t ret                           = adc_oneshot_new_unit(&init_config, &adc_handle);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC unit initialization failed.");

        return;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = bitwidth,
        .atten    = atten,
    };

    ret = adc_oneshot_config_channel((adc_handle), (chan), &config);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC channel configuration failed.");

        return;
    }

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = adcunit,
        .atten    = atten,
        .bitwidth = bitwidth,
    };

    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &(cali_handle));

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC curve fitting failed.");

        return;
    }

    return;
}

// void post_event()
// {
//     esp_http_client_config_t config_post = {.host              = "maker.ifttt.com",
//                                             .path              = "/trigger/door_event/json/with/key/dJvviPf6PCdAXrUKria3fc",
//                                             .method            = HTTP_METHOD_POST,
//                                             .transport_type    = HTTP_TRANSPORT_OVER_SSL,
//                                             .crt_bundle_attach = esp_crt_bundle_attach};

//     esp_http_client_handle_t client      = esp_http_client_init(&config_post);

//     cJSON* root                          = cJSON_CreateObject();
//     cJSON_AddStringToObject(root, "door_state", "open");

//     char* jsonString = cJSON_Print(root);
//     printf("%s\n", jsonString);

//     // Set headers
//     esp_http_client_set_header(client, "Content-Type", "application/json");

//     // Set POST data
//     esp_http_client_set_post_field(client, jsonString, strlen(jsonString));
//     esp_http_client_perform(client);

//     esp_http_client_cleanup(client);

//     cJSON_Delete(root);
//     free(jsonString);

//     return;
// }

void post_event()
{
    esp_http_client_config_t config_post = {.host              = HOST,
                                            .path              = PATH,
                                            .method            = HTTP_METHOD_POST,
                                            .transport_type    = HTTP_TRANSPORT_OVER_SSL,
                                            .crt_bundle_attach = esp_crt_bundle_attach};

    esp_http_client_handle_t client      = esp_http_client_init(&config_post);

    cJSON* root                          = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "door_state", "open");

    char* jsonString = cJSON_Print(root);
    printf("%s\n", jsonString);

   // Prepare the data to be sent
    char post_data[100] = {0};
    // const char* post_data = "door_status charge=2.53,door_state=1";
    snprintf(post_data, sizeof(post_data), "door_status charge=%.2f,door_state=%d", voltage, level);
    ESP_LOGE(TAG, "Post data: %s", post_data);

    // Set headers
    esp_http_client_set_header(client, "Authorization", TOKEN);
    esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Set POST data
    // esp_http_client_set_post_field(client, jsonString, strlen(jsonString));
    esp_http_client_perform(client);

    esp_http_client_cleanup(client);

    cJSON_Delete(root);
    free(jsonString);

    return;
}

static void example_set_static_ip(esp_netif_t *netif)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(EXAMPLE_STATIC_IP_ADDR);
    ip.netmask.addr = ipaddr_addr(EXAMPLE_STATIC_NETMASK_ADDR);
    ip.gw.addr = ipaddr_addr(EXAMPLE_STATIC_GW_ADDR);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }
    ESP_LOGD(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", EXAMPLE_STATIC_IP_ADDR, EXAMPLE_STATIC_NETMASK_ADDR, EXAMPLE_STATIC_GW_ADDR);
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        example_set_static_ip(arg);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *my_sta = esp_netif_create_default_wifi_sta();
    assert(my_sta);

    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            // .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            // .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    post_event();
}

void app_main(void)
{
    uint16_t adc_raw;
    uint16_t vbus_V;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

     switch (esp_sleep_get_wakeup_cause()) 
     {
        case ESP_SLEEP_WAKEUP_EXT0:
        {
            ESP_LOGI(TAG, "Wake up from ext0");
            break;
        }
        case ESP_SLEEP_WAKEUP_EXT1: {
            uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
            if (wakeup_pin_mask != 0) {
                int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
                ESP_LOGI(TAG, "Wake up from GPIO %d\n", pin);
            } else {
                ESP_LOGI(TAG, "Wake up from GPIO\n");
            }
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER: 
        {
            ESP_LOGI(TAG, "Wake up from timer. Time spent in deep sleep\n");
            break;
        }
        default:
        break;
     }


    gpio_reset_pin(led_en);
    gpio_set_direction(led_en, GPIO_MODE_OUTPUT);
    gpio_set_level(led_en, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    adc_init(ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_11, ADC_BITWIDTH_DEFAULT);
    vTaskDelay(pdMS_TO_TICKS(1000));

    adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &adc_raw);
    adc_cali_raw_to_voltage(cali_handle, adc_raw, &vbus_V);

    gpio_set_level(led_en, 0);

    voltage = (25.1/5.1)*((float)vbus_V/1000);

    ESP_LOGI("MAIN", "Voltage level: %f", voltage);
    ESP_LOGI("MAIN", "Voltage level raw: %d", vbus_V);

    rtc_gpio_hold_dis(reed_en);
    rtc_gpio_init(reed_en);
    rtc_gpio_set_direction(reed_en, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(reed_en, 0);

    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_reset_pin(door_sensor);
    gpio_set_direction(door_sensor, GPIO_MODE_INPUT);
    level = 0;
    level = gpio_get_level(door_sensor);
    ESP_LOGI("MAIN", "Door sensor state: %d", level);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    if(level == 0)
    {

        esp_sleep_enable_ext0_wakeup(door_sensor, 1);

        rtc_gpio_pullup_en(door_sensor);
        rtc_gpio_set_level(reed_en, 0);
        rtc_gpio_hold_en(reed_en); 
        ESP_LOGI("MAIN", "Open door");

    }
    else
    {
        esp_sleep_enable_ext0_wakeup(door_sensor, 0);

        rtc_gpio_pullup_en(door_sensor);
        rtc_gpio_set_level(reed_en, 0);
        rtc_gpio_hold_en(reed_en); 
        ESP_LOGI("MAIN", "Closed door");
    }
    prev_level = level;

    wifi_init_sta();
    esp_wifi_stop();
    esp_deep_sleep_start();

    return;
}



