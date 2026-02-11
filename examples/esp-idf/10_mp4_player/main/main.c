/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <string.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_private/esp_cache_private.h"

#include "esp_codec_dev.h"
#include "driver/gpio.h"

#include "bsp/esp-bsp.h"
#include "app_stream_adapter.h"
#include "sdkconfig.h"

static const char *TAG = "main";

/* ===================== Display ===================== */

#define DISPLAY_BUFFER_SIZE (BSP_LCD_H_RES * BSP_LCD_V_RES * 2)

static esp_lcd_panel_handle_t lcd_panel;
static esp_lcd_panel_io_handle_t lcd_io;
static void *lcd_buffer[CONFIG_BSP_LCD_DPI_BUFFER_NUMS];
static SemaphoreHandle_t trans_sem;

/* ===================== Audio ===================== */

static esp_codec_dev_handle_t g_audio_dev = NULL;

/* ===================== Stream ===================== */

static app_stream_adapter_handle_t stream_adapter;

/* ===================== Playlist ===================== */

#define MAX_MP4_FILES   16
#define MAX_PATH_LEN   128

static char mp4_list[MAX_MP4_FILES][MAX_PATH_LEN];
static int mp4_count = 0;

/* ===================== DPI Callback ===================== */

static bool flush_dpi_panel_ready_callback(
        esp_lcd_panel_handle_t panel,
        esp_lcd_dpi_panel_event_data_t *edata,
        void *user_ctx)
{
    BaseType_t taskAwake = pdFALSE;
    if (trans_sem) {
        xSemaphoreGiveFromISR(trans_sem, &taskAwake);
    }
    return false;
}

/* ===================== Frame Callback ===================== */

static esp_err_t display_decoded_frame(uint8_t *buffer,
                                       uint32_t buffer_size,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t buffer_index,
                                       void *user_data)
{
    esp_lcd_panel_draw_bitmap(lcd_panel, 0, 0, width, height, buffer);

    xSemaphoreTake(trans_sem, 0);
    xSemaphoreTake(trans_sem, portMAX_DELAY);

    return ESP_OK;
}

/* ===================== Scan MP4 Files ===================== */

static void scan_mp4_files(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", dir_path);
        return;
    }

    struct dirent *entry;
    mp4_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        const char *name = entry->d_name;
        int len = strlen(name);

        if (len > 4 && strcasecmp(&name[len - 4], ".mp4") == 0) {
            snprintf(mp4_list[mp4_count],
                     MAX_PATH_LEN,
                     "%s/%s",
                     dir_path,
                     name);

            ESP_LOGI(TAG, "Found MP4: %s", mp4_list[mp4_count]);

            mp4_count++;
            if (mp4_count >= MAX_MP4_FILES) {
                break;
            }
        }
    }

    closedir(dir);
    ESP_LOGI(TAG, "Total MP4 files: %d", mp4_count);
}

/* ===================== Playlist Playback ===================== */

static void play_media_playlist(void)
{
    if (mp4_count == 0) {
        ESP_LOGW(TAG, "No MP4 files to play");
        return;
    }

    uint32_t loop_round = 0;

    while (1) {
        ESP_LOGI(TAG, "========== Playlist Loop #%lu ==========", ++loop_round);

        for (int i = 0; i < mp4_count; i++) {
            const char *filename = mp4_list[i];
            ESP_LOGI(TAG, "Playing [%d/%d]: %s", i + 1, mp4_count, filename);

            app_stream_adapter_set_file(stream_adapter,
                                        filename,
                                        g_audio_dev != NULL);

            esp_err_t ret = app_stream_adapter_start(stream_adapter);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to start %s", filename);
                continue;
            }

            uint32_t stable_count = 0;
            uint32_t last_frames = 0;

            while (1) {
                app_stream_stats_t stats;
                ret = app_stream_adapter_get_stats(stream_adapter, &stats);

                if (ret == ESP_OK) {
                    if (stats.frames_processed == last_frames) {
                        stable_count++;
                        if (stable_count >= 3) {
                            ESP_LOGI(TAG,
                                     "Finished %s (%" PRIu32 " frames)",
                                     filename,
                                     stats.frames_processed);
                            break;
                        }
                    } else {
                        stable_count = 0;
                        last_frames = stats.frames_processed;
                    }
                } else {
                    ESP_LOGW(TAG, "Stats error, stop playback");
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(500));
            }

            app_stream_adapter_stop(stream_adapter);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

/* ===================== app_main ===================== */

void app_main(void)
{
    ESP_LOGI(TAG, "HDMI MP4 Playlist Player Start");

    /* ---------- Display ---------- */

    ESP_ERROR_CHECK(bsp_display_new(NULL, &lcd_panel, &lcd_io));
    bsp_display_backlight_on();

    trans_sem = xSemaphoreCreateBinary();

    esp_lcd_dpi_panel_event_callbacks_t callbacks = {
        .on_refresh_done = flush_dpi_panel_ready_callback,
    };
    esp_lcd_dpi_panel_register_event_callbacks(lcd_panel, &callbacks, NULL);

    ESP_ERROR_CHECK(
        esp_lcd_dpi_panel_get_frame_buffer(
            lcd_panel,
            CONFIG_BSP_LCD_DPI_BUFFER_NUMS,
            &lcd_buffer[0],
            &lcd_buffer[1])
    );

    /* ---------- SD Card ---------- */

    ESP_ERROR_CHECK(bsp_sdcard_mount());

    /* ---------- Audio ---------- */

    ESP_LOGI(TAG, "Init audio codec");
    g_audio_dev = bsp_audio_codec_speaker_init();
    if (g_audio_dev) {
        esp_codec_dev_set_out_vol(g_audio_dev, 80);
    }

    /* ---------- Stream Adapter ---------- */

    app_stream_adapter_config_t adapter_config = {
        .frame_cb      = display_decoded_frame,
        .user_data     = NULL,
        .decode_buffers = lcd_buffer,
        .buffer_count  = CONFIG_BSP_LCD_DPI_BUFFER_NUMS,
        .buffer_size   = DISPLAY_BUFFER_SIZE,
        .audio_dev     = g_audio_dev,
        .jpeg_config   = APP_STREAM_JPEG_CONFIG_DEFAULT_RGB565()
    };

    ESP_ERROR_CHECK(
        app_stream_adapter_init(&adapter_config, &stream_adapter)
    );

    /* ---------- Scan & Play ---------- */

    scan_mp4_files(BSP_SD_MOUNT_POINT);

    play_media_playlist();
}
