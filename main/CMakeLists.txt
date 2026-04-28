idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_https_ota nvs_flash esp_netif esp_event app_update
                    PRIV_REQUIRES driver)
