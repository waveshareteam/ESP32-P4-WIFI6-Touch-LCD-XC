/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#pragma once

/*
 * Phone stylesheet for the 4C board's 720x720 round panel.
 *
 * The visible area is a circle of diameter 720 centered at (360, 360). At a
 * distance d from the center, the visible chord is 2 * sqrt(360^2 - d^2) wide,
 * and the inscribed square is about 509 x 509 (x/y in 105..615). Every widget
 * below is sized so that its content stays inside that circle:
 *   - status bar:   0..60,    content centered (chord at y=30 is ~288 px)
 *   - icon table:   60..650,  510 px wide, 3 x 3 icons of 150 px
 *   - page spots:   668..700, centered (chord at y=684 is ~314 px)
 * Components that do not depend on the panel shape reuse the default dark data.
 */

#include "esp_brookesia.hpp"

namespace esp_brookesia::systems::phone {

constexpr int STYLESHEET_720_720_ROUND_DARK_SCREEN_SIZE = 720;
constexpr int STYLESHEET_720_720_ROUND_DARK_STATUS_BAR_HEIGHT = 60;

/* Core */
constexpr base::Context::Data STYLESHEET_720_720_ROUND_DARK_CORE_DATA = {
    .name = "720x720 Round Dark",
    .screen_size = gui::StyleSize::RECT(STYLESHEET_720_720_ROUND_DARK_SCREEN_SIZE,
                                        STYLESHEET_720_720_ROUND_DARK_SCREEN_SIZE),
    .display = STYLESHEET_DEFAULT_DARK_CORE_DISPLAY_DATA,
    .manager = STYLESHEET_DEFAULT_DARK_CORE_MANAGER_DATA,
};

/* Status bar */
constexpr StatusBar::Data STYLESHEET_720_720_ROUND_DARK_STATUS_BAR_DATA = {
    .main = {
        .size = gui::StyleSize::RECT_W_PERCENT(100, STYLESHEET_720_720_ROUND_DARK_STATUS_BAR_HEIGHT),
        // Opaque and the same color as the background: it blends into the round home screen, while the core still
        // shrinks app screens below it.
        .background_color = gui::StyleColor::COLOR(STYLESHEET_DEFAULT_DARK_CORE_DISPLAY_BG_COLOR),
        .text_font = gui::StyleFont::SIZE(24),
        .text_color = gui::StyleColor::COLOR(0xFFFFFF),
    },
    .area = {
        // A single centered area keeps every item inside the narrow chord at the top of the circle.
        .num = 1,
        .data = {
            {
                .size = gui::StyleSize::RECT_PERCENT(100, 100),
                .layout_column_align = StatusBar::AreaAlign::CENTER,
                .layout_column_start_offset = 0,
                .layout_column_pad = 6,
            },
        },
    },
    .icon_common_size = gui::StyleSize::SQUARE(32),
    .battery = {
        .area_index = 0,
        .icon_data = {
            .icon = {
                .image_num = 5,
                .images = {
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_battery_level1_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_battery_level2_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_battery_level3_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_battery_level4_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_battery_charge_36_36),
                },
            },
        },
    },
    .wifi = {
        .area_index = 0,
        .icon_data = {
            .icon = {
                .image_num = 4,
                .images = {
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_wifi_close_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_wifi_level1_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_wifi_level2_36_36),
                    gui::StyleImage::IMAGE_RECOLOR_WHITE(&esp_brookesia_image_large_status_bar_wifi_level3_36_36),
                },
            },
        },
    },
    .clock = {
        .area_index = 0,
    },
    .flags = {
        .enable_battery_icon = 1,
        .enable_battery_icon_common_size = 1,
        .enable_battery_label = 0,
        .enable_wifi_icon = 1,
        .enable_wifi_icon_common_size = 1,
        .enable_clock = 1,
    },
};

/* App launcher */
constexpr AppLauncherIcon::Data STYLESHEET_720_720_ROUND_DARK_APP_LAUNCHER_ICON_DATA = {
    .main = {
        .size = gui::StyleSize::SQUARE(150),
        .layout_row_pad = 8,
    },
    // Keep the default size equal to the 112 x 112 app icons: any other size makes LVGL scale them in software on
    // every redraw, which stutters page scrolling.
    .image = {
        .default_size = gui::StyleSize::SQUARE(112),
        .press_size = gui::StyleSize::SQUARE(100),
    },
    .label = {
        .text_font = gui::StyleFont::SIZE(20),
        .text_color = gui::StyleColor::COLOR(0xFFFFFF),
    },
};

constexpr AppLauncherData STYLESHEET_720_720_ROUND_DARK_APP_LAUNCHER_DATA = {
    // `main` is replaced by the flex size (below the status bar, down to the screen bottom).
    .main = {
        .y_start = 0,
        .size = gui::StyleSize::RECT_PERCENT(100, 100),
    },
    // The launcher derives the row count from table height / icon height. Keep it below four cells.
    // Three rows have 35 px vertical padding, centering the grid near the screen center.
    .table = {
        .default_num = 3,
        .size = gui::StyleSize::RECT(510, 590),
    },
    .indicator = {
        .main_size = gui::StyleSize::RECT(240, 32),
        .main_layout_column_pad = 10,
        .main_layout_bottom_offset = 20,
        .spot_inactive_size = gui::StyleSize::SQUARE(12),
        .spot_active_size = gui::StyleSize::RECT(30, 12),
        .spot_inactive_background_color = gui::StyleColor::COLOR(0xC6C6C6),
        .spot_active_background_color = gui::StyleColor::COLOR(0xFFFFFF),
    },
    .icon = STYLESHEET_720_720_ROUND_DARK_APP_LAUNCHER_ICON_DATA,
    .flags = {
        .enable_table_scroll_anim = 1,
    },
};

/* Recents screen */
constexpr RecentsScreenSnapshot::Data STYLESHEET_720_720_ROUND_DARK_RECENTS_SCREEN_SNAPSHOT_DATA = {
    .main_size = gui::StyleSize::RECT(300, 340),
    .title = {
        .main_size = gui::StyleSize::RECT_W_PERCENT(100, 40),
        .main_layout_column_pad = 10,
        .icon_size = gui::StyleSize::SQUARE_PERCENT(100),
        .text_font = gui::StyleFont::HEIGHT_PERCENT(70),
        .text_color = gui::StyleColor::COLOR(0xFFFFFF),
    },
    .image = {
        .main_size = gui::StyleSize::RECT(300, 290),
        .radius = 20,
    },
    .flags = {
        .enable_all_main_size_refer_screen = 0,
    },
};

constexpr RecentsScreen::Data STYLESHEET_720_720_ROUND_DARK_RECENTS_SCREEN_DATA = {
    .main = {
        .y_start = 0,
        .size = gui::StyleSize::RECT_PERCENT(100, 100),
        .layout_row_pad = 10,
        // Keep the memory label and the trash icon away from the top and bottom arcs.
        .layout_top_pad = 60,
        .layout_bottom_pad = 40,
        .background_color = gui::StyleColor::COLOR(0x1A1A1A),
    },
    .memory = {
        .main_size = gui::StyleSize::RECT_W_PERCENT(100, 24),
        // The label is right-aligned; this offset moves it next to the vertical center line.
        .main_layout_x_right_offset = 230,
        .label_text_font = gui::StyleFont::HEIGHT_PERCENT(100),
        .label_text_color = gui::StyleColor::COLOR(0xFFFFFF),
        .label_unit_text = "KB",
    },
    .snapshot_table = {
        .main_size = gui::StyleSize::RECT_PERCENT(100, 100),
        .main_layout_column_pad = 30,
        .snapshot = STYLESHEET_720_720_ROUND_DARK_RECENTS_SCREEN_SNAPSHOT_DATA,
    },
    .trash_icon = {
        .default_size = gui::StyleSize::SQUARE(56),
        .press_size = gui::StyleSize::SQUARE(50),
        .image = gui::StyleImage::IMAGE(&esp_brookesia_image_large_recents_screen_trash_64_64),
    },
    .flags = {
        .enable_memory = 1,
        .enable_table_height_flex = 1,
        .enable_table_snapshot_use_icon_image = 0,
        .enable_table_scroll_anim = 0,
    },
};

/* Gesture */
constexpr Gesture::IndicatorBarData STYLESHEET_720_720_ROUND_DARK_GESTURE_BOTTOM_INDICATOR_BAR_DATA = {
    .main = {
        .size_min = gui::StyleSize::RECT(0, 10),
        // The chord near the bottom of the circle is narrow, so cap the bar width.
        .size_max = gui::StyleSize::RECT(160, 10),
        .radius = 5,
        .layout_pad_all = 2,
        .color = gui::StyleColor::COLOR(0x1A1A1A),
    },
    .indicator = {
        .radius = 5,
        .color = gui::StyleColor::COLOR(0xFFFFFF),
    },
    .animation = {
        .scale_back_path_type = gui::StyleAnimation::ANIM_PATH_TYPE_BOUNCE,
        .scale_back_time_ms = 500,
    },
};

constexpr Gesture::Data STYLESHEET_720_720_ROUND_DARK_GESTURE_DATA = {
    .detect_period_ms = 20,
    .threshold = {
        .direction_vertical = 50,
        .direction_horizon = 50,
        .direction_angle = 60,
        // The edges of a round panel are only touchable near their middle; wider edge bands make edge swipes
        // reachable (the bottom band then spans ~366 px at y=670).
        .horizontal_edge = 40,
        .vertical_edge = 50,
        .duration_short_ms = 800,
        .speed_slow_px_per_ms = 0.1,
    },
    .indicator_bars = {
        [static_cast<int>(Gesture::IndicatorBarType::LEFT)] =
        STYLESHEET_DEFAULT_DARK_GESTURE_LEFT_RIGHT_INDICATOR_BAR_DATA,
        [static_cast<int>(Gesture::IndicatorBarType::RIGHT)] =
        STYLESHEET_DEFAULT_DARK_GESTURE_LEFT_RIGHT_INDICATOR_BAR_DATA,
        [static_cast<int>(Gesture::IndicatorBarType::BOTTOM)] =
        STYLESHEET_720_720_ROUND_DARK_GESTURE_BOTTOM_INDICATOR_BAR_DATA,
    },
    .flags = {
        .enable_indicator_bars = {
            [static_cast<int>(Gesture::IndicatorBarType::LEFT)] = 0,
            [static_cast<int>(Gesture::IndicatorBarType::RIGHT)] = 0,
            [static_cast<int>(Gesture::IndicatorBarType::BOTTOM)] = 1,
        },
    },
};

/* Display */
constexpr Display::Data STYLESHEET_720_720_ROUND_DARK_DISPLAY_DATA = {
    .status_bar = {
        .data = STYLESHEET_720_720_ROUND_DARK_STATUS_BAR_DATA,
        .visual_mode = StatusBar::VisualMode::SHOW_FIXED,
    },
    .navigation_bar = {
        .data = STYLESHEET_DEFAULT_DARK_NAVIGATION_BAR_DATA,
        .visual_mode = NavigationBar::VisualMode::HIDE,
    },
    .app_launcher = {
        .data = STYLESHEET_720_720_ROUND_DARK_APP_LAUNCHER_DATA,
        .default_image = gui::StyleImage::IMAGE(&esp_brookesia_image_small_app_launcher_default_98_98),
    },
    .recents_screen = {
        .data = STYLESHEET_720_720_ROUND_DARK_RECENTS_SCREEN_DATA,
        .status_bar_visual_mode = StatusBar::VisualMode::HIDE,
        .navigation_bar_visual_mode = NavigationBar::VisualMode::HIDE,
    },
    .flags = {
        .enable_status_bar = 1,
        .enable_navigation_bar = 1,
        .enable_app_launcher_flex_size = 1,
        .enable_recents_screen = 1,
        .enable_recents_screen_flex_size = 1,
    },
};

/* Stylesheet */
constexpr Stylesheet STYLESHEET_720_720_ROUND_DARK = {
    .core = STYLESHEET_720_720_ROUND_DARK_CORE_DATA,
    .display = STYLESHEET_720_720_ROUND_DARK_DISPLAY_DATA,
    .manager = STYLESHEET_DEFAULT_DARK_MANAGER_DATA,
};

} // namespace esp_brookesia::systems::phone

#define ESP_BROOKESIA_PHONE_720_720_ROUND_DARK_STYLESHEET() esp_brookesia::systems::phone::STYLESHEET_720_720_ROUND_DARK
