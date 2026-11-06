#include "WlanPage.hpp"
#include "../Settings.hpp"
#include "SettingsUI.hpp"

#define ESP_UTILS_LOG_TAG "BS:WlanPage"
#include "esp_lib_utils.h"
#include "esp_wifi.h"

#include <algorithm>
#include <string.h>

LV_IMG_DECLARE(wifi_1);
LV_IMG_DECLARE(wifi_2);
LV_IMG_DECLARE(wifi_3);
LV_IMG_DECLARE(wifi_4);

namespace esp_brookesia::apps
{
    namespace
    {
        constexpr intptr_t DIALOG_ACTION_DISMISS = 0;
        constexpr intptr_t DIALOG_ACTION_FORGET = 1;
        constexpr lv_coord_t DIALOG_MAX_WIDTH = 560;
        constexpr lv_coord_t INLINE_SPINNER_SIZE = 24;

        lv_obj_t *create_inline_spinner(lv_obj_t *parent)
        {
            lv_obj_t *spinner = lv_spinner_create(parent);
            lv_obj_set_size(spinner, INLINE_SPINNER_SIZE, INLINE_SPINNER_SIZE);
            lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
            lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(spinner, lv_color_hex(settings_ui::COLOR_ACCENT), LV_PART_INDICATOR);
            return spinner;
        }

        // Dim a control and ignore touches. LV_STATE_DISABLED is avoided because the theme's disabled
        // recolor turns the dark rows light grey.
        void set_row_enabled(lv_obj_t *obj, bool enabled)
        {
            if (enabled) {
                lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_style_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            } else {
                lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_style_opa(obj, LV_OPA_40, LV_PART_MAIN);
            }
        }

        const lv_image_dsc_t *signal_icon(int8_t rssi)
        {
            if (rssi >= -55) {
                return &wifi_4;
            }
            if (rssi >= -67) {
                return &wifi_3;
            }
            if (rssi >= -78) {
                return &wifi_2;
            }
            return &wifi_1;
        }

        // Scanning while the station joins an AP makes the association fail.
        bool link_is_joining()
        {
            board_wifi_link_t link = {};
            board_wifi_service_get_link(&link);
            return link.state == BOARD_WIFI_LINK_CONNECTING || link.state == BOARD_WIFI_LINK_ASSOCIATED;
        }

        void copy_ssid(char *dst, size_t dst_size, const uint8_t *src, size_t src_len)
        {
            size_t len = std::min(src_len, dst_size - 1);
            memcpy(dst, src, len);
            dst[len] = '\0';
        }

        // Password form layout on the round panel.
        constexpr lv_coord_t PASSWORD_TITLE_HEIGHT = 30;
        constexpr lv_coord_t PASSWORD_TITLE_GAP = 46;
        constexpr lv_coord_t PASSWORD_TA_HEIGHT = 64;
        constexpr lv_coord_t PASSWORD_FORM_GAP = 16;
        constexpr lv_coord_t KB_PAD = 8;
        constexpr lv_coord_t KB_GAP = 6;
        constexpr int KB_ROWS = 4;
        constexpr int32_t KB_MIN_ROW_WEIGHT = 24;
        constexpr uint32_t KB_WIDTH_MASK = 0x000F;
        const uint32_t KB_FLAGS = LV_KEYBOARD_CTRL_BUTTON_FLAGS;
        constexpr uint32_t KB_CHECKED = LV_BUTTONMATRIX_CTRL_CHECKED;
        constexpr uint32_t KB_KEY = LV_BUTTONMATRIX_CTRL_POPOVER;

        struct KbKey {
            const char *text;
            uint32_t ctrl;    // lv_buttonmatrix_ctrl_t bits, including the relative width.
        };
        using KbLayout = std::vector<std::vector<KbKey>>;

        // Same layouts as LVGL's default text keyboards (lv_keyboard.c). The mode-switch labels
        // ("abc", "ABC", "1#") must match LV_KEYBOARD_CTRL_BUTTON_MODE_* there.
        const KbLayout &kb_layout(lv_keyboard_mode_t mode)
        {
            static const KbLayout lower = {
                {{"1#", KB_FLAGS | 5}, {"q", KB_KEY | 4}, {"w", KB_KEY | 4}, {"e", KB_KEY | 4}, {"r", KB_KEY | 4},
                 {"t", KB_KEY | 4}, {"y", KB_KEY | 4}, {"u", KB_KEY | 4}, {"i", KB_KEY | 4}, {"o", KB_KEY | 4},
                 {"p", KB_KEY | 4}, {LV_SYMBOL_BACKSPACE, KB_CHECKED | 7}},
                {{"ABC", KB_FLAGS | 6}, {"a", KB_KEY | 3}, {"s", KB_KEY | 3}, {"d", KB_KEY | 3}, {"f", KB_KEY | 3},
                 {"g", KB_KEY | 3}, {"h", KB_KEY | 3}, {"j", KB_KEY | 3}, {"k", KB_KEY | 3}, {"l", KB_KEY | 3},
                 {LV_SYMBOL_NEW_LINE, KB_CHECKED | 7}},
                {{"_", KB_CHECKED | KB_KEY | 1}, {"-", KB_CHECKED | KB_KEY | 1}, {"z", KB_KEY | 1}, {"x", KB_KEY | 1},
                 {"c", KB_KEY | 1}, {"v", KB_KEY | 1}, {"b", KB_KEY | 1}, {"n", KB_KEY | 1}, {"m", KB_KEY | 1},
                 {".", KB_CHECKED | KB_KEY | 1}, {",", KB_CHECKED | KB_KEY | 1}, {":", KB_CHECKED | KB_KEY | 1}},
                {{LV_SYMBOL_KEYBOARD, KB_FLAGS | 2}, {LV_SYMBOL_LEFT, KB_CHECKED | 2}, {" ", 6},
                 {LV_SYMBOL_RIGHT, KB_CHECKED | 2}, {LV_SYMBOL_OK, KB_FLAGS | 2}},
            };
            static const KbLayout upper = {
                {{"1#", KB_FLAGS | 5}, {"Q", KB_KEY | 4}, {"W", KB_KEY | 4}, {"E", KB_KEY | 4}, {"R", KB_KEY | 4},
                 {"T", KB_KEY | 4}, {"Y", KB_KEY | 4}, {"U", KB_KEY | 4}, {"I", KB_KEY | 4}, {"O", KB_KEY | 4},
                 {"P", KB_KEY | 4}, {LV_SYMBOL_BACKSPACE, KB_CHECKED | 7}},
                {{"abc", KB_FLAGS | 6}, {"A", KB_KEY | 3}, {"S", KB_KEY | 3}, {"D", KB_KEY | 3}, {"F", KB_KEY | 3},
                 {"G", KB_KEY | 3}, {"H", KB_KEY | 3}, {"J", KB_KEY | 3}, {"K", KB_KEY | 3}, {"L", KB_KEY | 3},
                 {LV_SYMBOL_NEW_LINE, KB_CHECKED | 7}},
                {{"_", KB_CHECKED | KB_KEY | 1}, {"-", KB_CHECKED | KB_KEY | 1}, {"Z", KB_KEY | 1}, {"X", KB_KEY | 1},
                 {"C", KB_KEY | 1}, {"V", KB_KEY | 1}, {"B", KB_KEY | 1}, {"N", KB_KEY | 1}, {"M", KB_KEY | 1},
                 {".", KB_CHECKED | KB_KEY | 1}, {",", KB_CHECKED | KB_KEY | 1}, {":", KB_CHECKED | KB_KEY | 1}},
                {{LV_SYMBOL_CLOSE, KB_FLAGS | 2}, {LV_SYMBOL_LEFT, KB_CHECKED | 2}, {" ", 6},
                 {LV_SYMBOL_RIGHT, KB_CHECKED | 2}, {LV_SYMBOL_OK, KB_FLAGS | 2}},
            };
            static const KbLayout special = {
                {{"1", KB_KEY | 1}, {"2", KB_KEY | 1}, {"3", KB_KEY | 1}, {"4", KB_KEY | 1}, {"5", KB_KEY | 1},
                 {"6", KB_KEY | 1}, {"7", KB_KEY | 1}, {"8", KB_KEY | 1}, {"9", KB_KEY | 1}, {"0", KB_KEY | 1},
                 {LV_SYMBOL_BACKSPACE, KB_CHECKED | 2}},
                {{"abc", KB_FLAGS | 2}, {"+", KB_KEY | 1}, {"&", KB_KEY | 1}, {"/", KB_KEY | 1}, {"*", KB_KEY | 1},
                 {"=", KB_KEY | 1}, {"%", KB_KEY | 1}, {"!", KB_KEY | 1}, {"?", KB_KEY | 1}, {"#", KB_KEY | 1},
                 {"<", KB_KEY | 1}, {">", KB_KEY | 1}},
                {{"\\", KB_KEY | 1}, {"@", KB_KEY | 1}, {"$", KB_KEY | 1}, {"(", KB_KEY | 1}, {")", KB_KEY | 1},
                 {"{", KB_KEY | 1}, {"}", KB_KEY | 1}, {"[", KB_KEY | 1}, {"]", KB_KEY | 1}, {";", KB_KEY | 1},
                 {"\"", KB_KEY | 1}, {"'", KB_KEY | 1}},
                {{LV_SYMBOL_KEYBOARD, KB_FLAGS | 2}, {LV_SYMBOL_LEFT, KB_CHECKED | 2}, {" ", 6},
                 {LV_SYMBOL_RIGHT, KB_CHECKED | 2}, {LV_SYMBOL_OK, KB_FLAGS | 2}},
            };
            switch (mode) {
            case LV_KEYBOARD_MODE_TEXT_UPPER:
                return upper;
            case LV_KEYBOARD_MODE_SPECIAL:
                return special;
            default:
                return lower;
            }
        }

        // Builds a keyboard map whose rows are narrowed by hidden spacer buttons at both ends, so
        // each row stays inside row_widths[i] of the full_width keyboard content area.
        void build_round_kb_map(const KbLayout &layout, const lv_coord_t *row_widths, lv_coord_t full_width,
                                std::vector<const char *> &map, std::vector<lv_buttonmatrix_ctrl_t> &ctrl)
        {
            map.clear();
            ctrl.clear();
            for (size_t i = 0; i < layout.size(); ++i) {
                const auto &row = layout[i];
                int32_t max_weight = 1;
                int32_t row_weight = 0;
                for (const auto &key : row) {
                    const int32_t weight = static_cast<int32_t>(key.ctrl & KB_WIDTH_MASK);
                    max_weight = LV_MAX(max_weight, weight);
                    row_weight += weight;
                }
                // Scale light rows up so the spacer weight has finer steps.
                const int32_t scale = LV_MAX(1, LV_MIN(15 / max_weight,
                                                       (KB_MIN_ROW_WEIGHT + row_weight - 1) / row_weight));
                row_weight *= scale;

                // With spacer weight s on each side the keys get
                // (full - (n + 1) * gap) * W / (W + 2s) + (n - 1) * gap pixels; solve for s, rounding up.
                const int32_t keys = static_cast<int32_t>(row.size());
                const int32_t key_px = row_widths[i] - (keys - 1) * KB_GAP;
                const int32_t avail = full_width - (keys + 1) * KB_GAP;
                int32_t spacer = 0;
                if (row_widths[i] < full_width && key_px > 0 && avail > key_px) {
                    spacer = (row_weight * (avail - key_px) + 2 * key_px - 1) / (2 * key_px);
                    spacer = LV_MIN(spacer, 15);
                }

                if (spacer > 0) {
                    map.push_back(" ");
                    ctrl.push_back(static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_HIDDEN | spacer));
                }
                for (const auto &key : row) {
                    map.push_back(key.text);
                    ctrl.push_back(static_cast<lv_buttonmatrix_ctrl_t>((key.ctrl & ~KB_WIDTH_MASK) |
                                                                         ((key.ctrl & KB_WIDTH_MASK) * scale)));
                }
                if (spacer > 0) {
                    map.push_back(" ");
                    ctrl.push_back(static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_HIDDEN | spacer));
                }
                map.push_back((i + 1 < layout.size()) ? "\n" : "");
            }
        }
    } // namespace

    WlanPage *WlanPage::_instance = nullptr;

    WlanPage *WlanPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new WlanPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    WlanPage::WlanPage(bool use_status_bar, bool use_navigation_bar)
        : App("WLAN", nullptr, true, use_status_bar, use_navigation_bar)
    {
    }

    WlanPage::~WlanPage() {}

    bool WlanPage::run()
    {
        ESP_UTILS_LOGI("WlanPage Run");

        if (!board_wifi_service_is_ready()) {
            ESP_UTILS_LOGE("Wi-Fi service is not available");
        }

        subscribeEvents();
        createUi();

        ui_timer = lv_timer_create(ui_timer_cb, UI_POLL_PERIOD_MS, this);
        if (ui_timer == nullptr) {
            ESP_UTILS_LOGE("Create WLAN UI timer failed");
        }

        if (board_wifi_service_is_enabled()) {
            setUiState(UiState::Scanning);
            requestScan();
        } else {
            setUiState(UiState::Off);
        }
        return true;
    }

    bool WlanPage::back()
    {
        ESP_UTILS_LOGI("WlanPage Back");

        Settings::requestInstance()->showRootPage();
        return true;
    }

    bool WlanPage::close()
    {
        ESP_UTILS_LOGI("WlanPage Close");

        if (ui_timer != nullptr) {
            lv_timer_delete(ui_timer);
            ui_timer = nullptr;
        }

        esp_wifi_service_scan_handle_t scan_handle = board_wifi_service_get_scan_handle();
        if (scan_handle != nullptr) {
            esp_wifi_service_scan_remove_cb(scan_handle, wifi_scan_done_cb);
        }

        destroyUi();
        memset(wifi_pwd, 0, sizeof(wifi_pwd));
        memset(&previous_profile, 0, sizeof(previous_profile));
        ui_state = UiState::Off;
        scan_deferred = false;
        return true;
    }

    void WlanPage::subscribeEvents()
    {
        if (events_subscribed) {
            return;
        }

        // Subscribed for the lifetime of the page object: unsubscribing by event id would also drop
        // the subscriptions of other apps.
        esp_err_t ret = board_wifi_service_subscribe(BOARD_WIFI_EVENT_ANY, wifi_service_event_cb, this);
        if (ret != ESP_OK) {
            ESP_UTILS_LOGE("Subscribe Wi-Fi service events failed: %s", esp_err_to_name(ret));
            return;
        }
        events_subscribed = true;
    }

    void WlanPage::createUi()
    {
        destroyUi();
        lv_obj_clean(lv_scr_act());

        page_root = settings_ui::create_page(lv_scr_act(), Settings::requestInstance()->getContentTopInset());
        lv_obj_t *back_btn = settings_ui::create_header(page_root, "WLAN", [](lv_event_t *e) {
            (void)e;
            lv_async_call([](void *param) {
                (void)param;
                Settings::requestInstance()->showRootPage();
            }, nullptr);
        });

        lv_obj_t *header = (back_btn != nullptr) ? lv_obj_get_parent(back_btn) : nullptr;
        if (header != nullptr) {
            refresh_btn = lv_button_create(header);
            lv_obj_set_size(refresh_btn, settings_ui::HEADER_BUTTON_SIZE, settings_ui::HEADER_BUTTON_SIZE);
            lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, -settings_ui::header_button_inset(page_root), 0);
            lv_obj_set_style_radius(refresh_btn, 8, LV_PART_MAIN);
            lv_obj_set_style_border_width(refresh_btn, 0, LV_PART_MAIN);
            lv_obj_set_style_shadow_width(refresh_btn, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(settings_ui::COLOR_SURFACE_PRESSED), LV_STATE_PRESSED);
            lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, LV_STATE_PRESSED);
            lv_obj_add_event_cb(refresh_btn, refresh_event_cb, LV_EVENT_CLICKED, this);

            lv_obj_t *refresh_icon = lv_label_create(refresh_btn);
            lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
            lv_obj_set_style_text_font(refresh_icon, &lv_font_montserrat_26, LV_PART_MAIN);
            lv_obj_set_style_text_color(refresh_icon, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
            lv_obj_center(refresh_icon);
        }

        settings_ui::init_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);

        list1 = settings_ui::create_content_list(page_root);
        lv_obj_add_style(list1, &style_list, LV_PART_MAIN);

        settings_ui::add_section(list1, "Network", style_list_text);

        lv_obj_t *wlan_btn = lv_list_add_button(list1, nullptr, "Wi-Fi");
        lv_obj_add_style(wlan_btn, &style_list_btn, LV_PART_MAIN);
        settings_ui::use_ellipsis_for_button_label(wlan_btn);

        wlan_switch = lv_switch_create(wlan_btn);
        lv_obj_set_size(wlan_switch, 64, 36);
        lv_obj_add_event_cb(wlan_switch, switch_event_cb, LV_EVENT_VALUE_CHANGED, this);
        // Only the switch is interactive. LV_STATE_DISABLED would also grey out the switch because
        // the theme's disabled recolor is applied to children.
        lv_obj_remove_flag(wlan_btn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_update_layout(page_root);
        lv_coord_t content_width = lv_obj_get_width(page_root) - settings_ui::PAGE_HORIZONTAL_MARGIN * 2;
        lv_coord_t keyboard_height = lv_obj_get_height(page_root) * 38 / 100;
        if (keyboard_height > 420) {
            keyboard_height = 420;
        } else if (keyboard_height < 260) {
            keyboard_height = 260;
        }
        if (content_width < 1) {
            content_width = 1;
        }

        // The round panel is too narrow near the bottom for a full keyboard, so the password form
        // sits around the screen centre and every element is clipped to its visible chord.
        lv_area_t page_area;
        lv_obj_get_coords(page_root, &page_area);
        int32_t panel_hor = 0;
        int32_t panel_ver = 0;
        settings_ui::get_panel_size(page_root, panel_hor, panel_ver);
        const lv_coord_t screen_mid = panel_ver / 2;
        lv_coord_t kb_top = screen_mid - keyboard_height / 4;
        lv_coord_t ta_top = kb_top - PASSWORD_FORM_GAP - PASSWORD_TA_HEIGHT;
        lv_coord_t title_top = ta_top - PASSWORD_TITLE_GAP;
        const lv_coord_t min_title_top = page_area.y1 + settings_ui::PAGE_HEADER_HEIGHT + 16;
        if (title_top < min_title_top) {
            const lv_coord_t shift = min_title_top - title_top;
            kb_top += shift;
            ta_top += shift;
            title_top += shift;
        }
        auto fit_width = [&](lv_coord_t top, lv_coord_t height) {
            const lv_coord_t width =
                2 * (settings_ui::round_screen_half_width(page_root, top, top + height) - settings_ui::ROUND_EDGE_MARGIN);
            return LV_MAX(static_cast<lv_coord_t>(1), LV_MIN(width, content_width));
        };

        password_title = lv_label_create(page_root);
        lv_label_set_text(password_title, "Connect to network");
        lv_label_set_long_mode(password_title, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(password_title, fit_width(title_top, PASSWORD_TITLE_HEIGHT));
        lv_obj_set_style_text_font(password_title, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(password_title, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_align(password_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(password_title, LV_ALIGN_TOP_MID, 0, title_top - page_area.y1);
        lv_obj_add_flag(password_title, LV_OBJ_FLAG_HIDDEN);

        ta = lv_textarea_create(page_root);
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(ta, fit_width(ta_top, PASSWORD_TA_HEIGHT), PASSWORD_TA_HEIGHT);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, ta_top - page_area.y1);

        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_password_mode(ta, true);
        lv_textarea_set_max_length(ta, sizeof(wifi_pwd) - 1);
        lv_textarea_set_password_show_time(ta, 1500);
        lv_textarea_set_placeholder_text(ta, "Enter password...");
        lv_obj_set_style_bg_color(ta, lv_color_hex(settings_ui::COLOR_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(ta, lv_color_hex(settings_ui::COLOR_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(ta, 6, LV_PART_MAIN);
        lv_obj_set_style_text_color(ta, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_set_style_text_color(ta, lv_color_hex(settings_ui::COLOR_SECONDARY_TEXT), LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_set_style_bg_color(ta, lv_color_white(), LV_PART_CURSOR);
        lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_CURSOR);

        kb = lv_keyboard_create(page_root);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(kb, content_width, keyboard_height);
        lv_obj_align(kb, LV_ALIGN_TOP_MID, 0, kb_top - page_area.y1);
        lv_obj_set_style_pad_all(kb, KB_PAD, LV_PART_MAIN);
        lv_obj_set_style_pad_row(kb, KB_GAP, LV_PART_MAIN);
        lv_obj_set_style_pad_column(kb, KB_GAP, LV_PART_MAIN);
        // A rectangular background would show clipped corners on the round panel; only keys are drawn.
        lv_obj_set_style_bg_opa(kb, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(kb, 0, LV_PART_MAIN);

        const lv_coord_t kb_inner_width = content_width - KB_PAD * 2;
        const lv_coord_t kb_row_height = (keyboard_height - KB_PAD * 2 - KB_GAP * (KB_ROWS - 1)) / KB_ROWS;
        lv_coord_t kb_row_widths[KB_ROWS];
        for (int i = 0; i < KB_ROWS; ++i) {
            const lv_coord_t row_top = kb_top + KB_PAD + i * (kb_row_height + KB_GAP);
            kb_row_widths[i] = LV_MIN(fit_width(row_top, kb_row_height), kb_inner_width);
        }
        const lv_keyboard_mode_t kb_modes[KB_MAP_COUNT] = {
            LV_KEYBOARD_MODE_TEXT_LOWER, LV_KEYBOARD_MODE_TEXT_UPPER, LV_KEYBOARD_MODE_SPECIAL,
        };
        for (size_t i = 0; i < KB_MAP_COUNT; ++i) {
            build_round_kb_map(kb_layout(kb_modes[i]), kb_row_widths, kb_inner_width, kb_map[i], kb_ctrl[i]);
            lv_keyboard_set_map(kb, kb_modes[i], kb_map[i].data(), kb_ctrl[i].data());
        }
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_set_style_bg_color(kb, lv_color_hex(0x333333), LV_PART_ITEMS);
        lv_obj_set_style_text_color(kb, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_ITEMS);
        lv_obj_set_style_text_font(kb, &lv_font_montserrat_18, LV_PART_ITEMS);
        lv_obj_set_style_border_color(kb, lv_color_hex(0x555555), LV_PART_ITEMS);
        lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
        lv_obj_set_style_radius(kb, 4, LV_PART_ITEMS);
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, this);
    }

    void WlanPage::destroyUi()
    {
        if (page_root == nullptr) {
            return;
        }

        lv_obj_del(page_root);
        page_root = nullptr;
        list1 = nullptr;
        wlan_switch = nullptr;
        refresh_btn = nullptr;
        dialog = nullptr;
        dialog_footer = nullptr;
        password_title = nullptr;
        ta = nullptr;
        kb = nullptr;
        connected_text = nullptr;
        conn_btn = nullptr;
        available_text = nullptr;
        wifi_btns.clear();
        settings_ui::reset_list_styles(style_list, style_list_btn, style_list_text, style_list_btn_pressed);
    }

    void WlanPage::setUiState(UiState state)
    {
        ui_state = state;
        if (state != UiState::Scanning) {
            scan_deferred = false;
        }
        if (page_root == nullptr) {
            return;
        }

        if (state != UiState::Password) {
            hidePasswordInput();
        }

        if (state != UiState::Off) {
            lv_obj_add_state(wlan_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(wlan_switch, LV_STATE_CHECKED);
        }
        if (state == UiState::Connecting) {
            lv_obj_add_state(wlan_switch, LV_STATE_DISABLED);
        } else {
            lv_obj_remove_state(wlan_switch, LV_STATE_DISABLED);
        }

        if (refresh_btn != nullptr) {
            if (state == UiState::Idle || state == UiState::Scanning) {
                lv_obj_remove_flag(refresh_btn, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(refresh_btn, LV_OBJ_FLAG_HIDDEN);
            }
            if (state == UiState::Scanning) {
                lv_obj_add_state(refresh_btn, LV_STATE_DISABLED);
            } else {
                lv_obj_remove_state(refresh_btn, LV_STATE_DISABLED);
            }
        }

        // Connection progress is shown inline, so only the password editor hides the list.
        if (state == UiState::Password) {
            lv_obj_add_flag(list1, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(list1, LV_OBJ_FLAG_HIDDEN);
            rebuildList();
        }
    }

    void WlanPage::clearListRows()
    {
        if (connected_text != nullptr) {
            lv_obj_del(connected_text);
            connected_text = nullptr;
        }
        if (conn_btn != nullptr) {
            lv_obj_del(conn_btn);
            conn_btn = nullptr;
        }
        if (available_text != nullptr) {
            lv_obj_del(available_text);
            available_text = nullptr;
        }
        for (auto row : wifi_btns) {
            lv_obj_del(row);
        }
        wifi_btns.clear();
    }

    lv_obj_t *WlanPage::addListRow(const char *text)
    {
        lv_obj_t *row = lv_list_add_button(list1, nullptr, text);
        lv_obj_add_style(row, &style_list_btn, LV_PART_MAIN);
        lv_obj_add_style(row, &style_list_btn_pressed, LV_STATE_PRESSED);
        settings_ui::use_ellipsis_for_button_label(row);
        return row;
    }

    void WlanPage::addRowProgress(lv_obj_t *row, const char *text)
    {
        lv_obj_t *status = lv_label_create(row);
        lv_label_set_text(status, text);
        lv_obj_set_style_text_font(status, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(status, lv_color_hex(settings_ui::COLOR_SECONDARY_TEXT), LV_PART_MAIN);
        create_inline_spinner(row);
    }

    void WlanPage::rebuildList()
    {
        if (list1 == nullptr) {
            return;
        }

        const int32_t scroll_y = lv_obj_get_scroll_y(list1);
        clearListRows();
        board_wifi_link_t link = {};
        board_wifi_service_get_link(&link);
        rendered_link = link;
        if (ui_state == UiState::Off) {
            return;
        }

        const bool connecting = ui_state == UiState::Connecting;
        const bool scanning = ui_state == UiState::Scanning;

        // The board tracks the link from boot, so an automatic connection that started before the
        // page opened is listed too.
        const char *link_ssid = connecting ? connecting_ssid : link.ssid;
        if (connecting || link.state != BOARD_WIFI_LINK_IDLE) {
            connected_text = settings_ui::add_section(list1, "Connected WLAN", style_list_text);
            conn_btn = addListRow(link_ssid);
            if (connecting || link.state == BOARD_WIFI_LINK_CONNECTING) {
                lv_obj_remove_flag(conn_btn, LV_OBJ_FLAG_CLICKABLE);
                addRowProgress(conn_btn, "Connecting...");
            } else {
                lv_obj_add_event_cb(conn_btn, conn_btn_event_cb, LV_EVENT_CLICKED, this);
                if (link.state == BOARD_WIFI_LINK_GOT_IP) {
                    lv_obj_t *state_icon = lv_label_create(conn_btn);
                    lv_label_set_text(state_icon, LV_SYMBOL_OK);
                    lv_obj_set_style_text_font(state_icon, &lv_font_montserrat_26, LV_PART_MAIN);
                } else {
                    // Associated but still waiting for an address.
                    addRowProgress(conn_btn, "Obtaining IP...");
                }
            }
        }

        available_text = settings_ui::add_section(list1, "Available WLAN", style_list_text);

        // Previous results stay visible while a new scan runs, so say that one is in progress.
        if (scanning) {
            lv_obj_t *hint = lv_list_add_text(list1, scan_deferred ? "Waiting for connection to finish..." : "Scanning...");
            lv_obj_add_style(hint, &style_list_text, LV_PART_MAIN);
            wifi_btns.push_back(hint);
        }

        size_t shown = 0;
        for (size_t i = 0; i < ap_list.size(); ++i) {
            const ApInfo &ap = ap_list[i];
            if (strcmp(ap.ssid, link_ssid) == 0) {
                continue;
            }

            lv_obj_t *wifi_btn = addListRow(ap.ssid);
            lv_obj_set_user_data(wifi_btn, reinterpret_cast<void *>(i));
            lv_obj_add_event_cb(wifi_btn, ap_btn_event_cb, LV_EVENT_CLICKED, this);

            lv_obj_t *icon = lv_image_create(wifi_btn);
            lv_image_set_src(icon, signal_icon(ap.rssi));

            // Other networks cannot be picked while a connection started from this page is running.
            set_row_enabled(wifi_btn, !connecting);
            wifi_btns.push_back(wifi_btn);
            shown++;
        }

        if (shown == 0 && !scanning) {
            lv_obj_t *hint = lv_list_add_text(list1, connecting ? "Connecting to network..." : "No networks found");
            lv_obj_add_style(hint, &style_list_text, LV_PART_MAIN);
            wifi_btns.push_back(hint);
        }

        // Rebuilding recreates every row; keep the user's place in the list.
        lv_obj_update_layout(list1);
        lv_obj_scroll_to_y(list1, scroll_y, LV_ANIM_OFF);
    }

    void WlanPage::showPasswordInput(const ApInfo &ap)
    {
        selected_ap = ap;
        setUiState(UiState::Password);

        lv_label_set_text_fmt(password_title, "Connect to %s", ap.ssid);
        lv_obj_remove_flag(password_title, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(ta, "");
        lv_obj_remove_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(ta, LV_STATE_FOCUSED);
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    void WlanPage::hidePasswordInput()
    {
        if (password_title != nullptr) {
            lv_obj_add_flag(password_title, LV_OBJ_FLAG_HIDDEN);
        }
        if (ta != nullptr) {
            lv_textarea_set_text(ta, "");
            lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        }
        if (kb != nullptr) {
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void WlanPage::createDialog(const char *title, const char *text)
    {
        closeDialog(false);
        if (page_root == nullptr) {
            return;
        }

        // Owned by the page (not lv_layer_top) so it never shows over other apps and is deleted with
        // the page. The backdrop swallows touches meant for the page behind it.
        dialog = lv_obj_create(page_root);
        lv_obj_remove_style_all(dialog);
        lv_obj_set_size(dialog, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(dialog, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dialog, LV_OPA_60, LV_PART_MAIN);
        lv_obj_add_flag(dialog, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

        lv_coord_t width = lv_obj_get_width(page_root) - settings_ui::PAGE_HORIZONTAL_MARGIN * 2;
        if (width > DIALOG_MAX_WIDTH) {
            width = DIALOG_MAX_WIDTH;
        }

        lv_obj_t *panel = lv_obj_create(dialog);
        lv_obj_remove_style_all(panel);
        lv_obj_set_size(panel, width, LV_SIZE_CONTENT);
        lv_obj_center(panel);
        lv_obj_set_style_bg_color(panel, lv_color_hex(settings_ui::COLOR_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(panel, lv_color_hex(settings_ui::COLOR_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(panel, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(panel, 28, LV_PART_MAIN);
        lv_obj_set_style_pad_row(panel, 16, LV_PART_MAIN);
        lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *title_label = lv_label_create(panel);
        lv_label_set_text(title_label, title);
        lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(title_label, lv_pct(100));
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_26, LV_PART_MAIN);
        lv_obj_set_style_text_color(title_label, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);

        lv_obj_t *text_label = lv_label_create(panel);
        lv_label_set_text(text_label, text);
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_width(text_label, lv_pct(100));
        lv_obj_set_style_text_font(text_label, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(text_label, lv_color_hex(settings_ui::COLOR_SECONDARY_TEXT), LV_PART_MAIN);

        dialog_footer = lv_obj_create(panel);
        lv_obj_remove_style_all(dialog_footer);
        lv_obj_set_size(dialog_footer, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_top(dialog_footer, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_column(dialog_footer, 12, LV_PART_MAIN);
        lv_obj_set_flex_flow(dialog_footer, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(dialog_footer, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(dialog_footer, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_move_foreground(dialog);
    }

    void WlanPage::addDialogButton(const char *text, intptr_t action, bool primary)
    {
        if (dialog_footer == nullptr) {
            return;
        }

        lv_obj_t *btn = lv_button_create(dialog_footer);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, 56);
        lv_obj_set_style_pad_hor(btn, 28, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(primary ? settings_ui::COLOR_ACCENT : settings_ui::COLOR_SURFACE_PRESSED),
                                  LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(settings_ui::COLOR_BORDER), LV_STATE_PRESSED);
        lv_obj_set_user_data(btn, reinterpret_cast<void *>(action));
        lv_obj_add_event_cb(btn, dialog_btn_event_cb, LV_EVENT_CLICKED, this);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(settings_ui::COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_center(label);
    }

    void WlanPage::closeDialog(bool async)
    {
        if (dialog == nullptr) {
            return;
        }
        if (async) {
            // Called from a dialog button: the button must outlive its own event.
            lv_obj_delete_async(dialog);
        } else {
            lv_obj_delete(dialog);
        }
        dialog = nullptr;
        dialog_footer = nullptr;
    }

    void WlanPage::showMessage(const char *title, const char *text)
    {
        createDialog(title, text);
        addDialogButton("OK", DIALOG_ACTION_DISMISS, true);
    }

    void WlanPage::showForgetConfirm()
    {
        board_wifi_link_t link = {};
        board_wifi_service_get_link(&link);
        if (link.state != BOARD_WIFI_LINK_ASSOCIATED && link.state != BOARD_WIFI_LINK_GOT_IP) {
            return;
        }

        createDialog(link.ssid, "Forget this network? The saved password will be removed.");
        addDialogButton("Cancel", DIALOG_ACTION_DISMISS, false);
        addDialogButton("Forget", DIALOG_ACTION_FORGET, true);
    }

    void WlanPage::requestScan()
    {
        if (link_is_joining()) {
            if (!scan_deferred) {
                ESP_UTILS_LOGI("Wi-Fi is connecting, defer scan until the attempt finishes");
                scan_deferred = true;
                if (ui_state == UiState::Scanning) {
                    rebuildList();  // Show the waiting hint instead of "Scanning..."
                }
            }
            return;
        }
        scan_deferred = false;

        esp_wifi_service_scan_handle_t scan_handle = board_wifi_service_get_scan_handle();
        esp_err_t ret = ESP_ERR_INVALID_STATE;
        if (scan_handle != nullptr) {
            ret = esp_wifi_service_scan_request(scan_handle, nullptr, wifi_scan_done_cb, this);
        }
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("Request Wi-Fi scan failed: %s", esp_err_to_name(ret));
            if (ui_state == UiState::Scanning) {
                setUiState(UiState::Idle);
            }
        }
    }

    void WlanPage::startConnect(const ApInfo &ap, const char *password)
    {
        esp_wifi_service_profile_mgr_t profile_mgr = board_wifi_service_get_profile_manager();
        if (profile_mgr == nullptr) {
            showMessage("Wi-Fi unavailable", "The Wi-Fi service is not running.");
            setUiState(UiState::Idle);
            return;
        }

        strlcpy(connecting_ssid, ap.ssid, sizeof(connecting_ssid));
        strlcpy(wifi_pwd, password ? password : "", sizeof(wifi_pwd));

        // Remember an existing profile so a failed attempt with a new password can be rolled back.
        restore_profile_on_fail =
            esp_wifi_service_profile_mgr_get(profile_mgr, connecting_ssid, &previous_profile) == ESP_OK;
        {
            board_wifi_link_t link = {};
            board_wifi_service_get_link(&link);
            // Reconnecting to the current SSID first disconnects from it; that event is not a failure.
            ignore_next_disconnect = (link.state == BOARD_WIFI_LINK_ASSOCIATED ||
                                      link.state == BOARD_WIFI_LINK_GOT_IP) &&
                                     strcmp(link.ssid, connecting_ssid) == 0;
        }
        pending_events.fetch_and(~(PENDING_CONNECTED | PENDING_DISCONNECTED | PENDING_GOT_IP | PENDING_CONNECT_FAILED));

        ESP_UTILS_LOGI("Connect to SSID: %s", connecting_ssid);
        setUiState(UiState::Connecting);
        connect_start_tick = lv_tick_get();

        esp_err_t ret = board_wifi_service_request_connect(connecting_ssid, wifi_pwd, CONNECT_PRIORITY, 0);
        memset(wifi_pwd, 0, sizeof(wifi_pwd));
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("Request Wi-Fi connect failed: %s", esp_err_to_name(ret));
            finishConnect(false);
        }
    }

    void WlanPage::finishConnect(bool success, bool abort_attempt)
    {
        if (success) {
            ESP_UTILS_LOGI("Connected to SSID: %s", connecting_ssid);
        } else {
            ESP_UTILS_LOGW("Connect to SSID %s failed", connecting_ssid);

            esp_wifi_service_profile_mgr_t profile_mgr = board_wifi_service_get_profile_manager();
            if (profile_mgr != nullptr) {
                // Do not keep retrying credentials that never worked.
                if (restore_profile_on_fail) {
                    (void)esp_wifi_service_profile_mgr_add(profile_mgr, &previous_profile);
                } else {
                    (void)esp_wifi_service_profile_mgr_delete(profile_mgr, connecting_ssid);
                }
            }
            if (abort_attempt) {
                // The driver may still be trying the old credentials.
                (void)esp_wifi_disconnect();
            }
            // Let the selector fall back to other saved networks.
            esp_err_t ret = board_wifi_service_request_reeval();
            if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
                ESP_UTILS_LOGW("Request Wi-Fi re-evaluation failed: %s", esp_err_to_name(ret));
            }

            char text[96];
            snprintf(text, sizeof(text), "Unable to connect to %s. Check the password and try again.", connecting_ssid);
            showMessage("Connection failed", text);
        }

        memset(&previous_profile, 0, sizeof(previous_profile));
        restore_profile_on_fail = false;
        ignore_next_disconnect = false;
        connecting_ssid[0] = '\0';
        setUiState(UiState::Idle);
    }

    void WlanPage::forgetConnectedNetwork()
    {
        board_wifi_link_t link = {};
        board_wifi_service_get_link(&link);
        if (link.state != BOARD_WIFI_LINK_ASSOCIATED && link.state != BOARD_WIFI_LINK_GOT_IP) {
            return;
        }
        const char *current_ssid = link.ssid;

        esp_wifi_service_profile_mgr_t profile_mgr = board_wifi_service_get_profile_manager();
        if (profile_mgr != nullptr) {
            esp_err_t ret = esp_wifi_service_profile_mgr_delete(profile_mgr, current_ssid);
            if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
                ESP_UTILS_LOGW("Delete Wi-Fi profile failed: %s", esp_err_to_name(ret));
            }
        }
        ESP_UTILS_LOGI("Forget SSID: %s", current_ssid);
        // The selector re-evaluates the remaining profiles after the disconnect event.
        (void)esp_wifi_disconnect();
    }

    void WlanPage::applyScanRecords()
    {
        std::vector<wifi_ap_record_t> records;
        esp_err_t scan_err = ESP_OK;
        {
            std::lock_guard<std::mutex> lock(shared_mutex);
            records.swap(scan_records);
            scan_err = scan_error;
        }
        if (scan_err != ESP_OK) {
            ESP_UTILS_LOGW("Wi-Fi scan failed: %s", esp_err_to_name(scan_err));
        }

        // Keep the strongest entry per SSID.
        std::vector<ApInfo> results;
        for (const wifi_ap_record_t &record : records) {
            if (record.ssid[0] == '\0') {
                continue;
            }
            ApInfo ap = {};
            copy_ssid(ap.ssid, sizeof(ap.ssid), record.ssid, strnlen((const char *)record.ssid, sizeof(record.ssid)));
            ap.rssi = record.rssi;
            ap.authmode = record.authmode;

            auto it = std::find_if(results.begin(), results.end(), [&ap](const ApInfo &item) {
                return strcmp(item.ssid, ap.ssid) == 0;
            });
            if (it == results.end()) {
                results.push_back(ap);
            } else if (ap.rssi > it->rssi) {
                *it = ap;
            }
        }
        std::sort(results.begin(), results.end(), [](const ApInfo &a, const ApInfo &b) {
            return a.rssi > b.rssi;
        });
        if (results.size() > MAX_AP_NUM) {
            results.resize(MAX_AP_NUM);
        }
        ESP_UTILS_LOGI("Wi-Fi scan done: %u records, %u networks",
                       static_cast<unsigned>(records.size()), static_cast<unsigned>(results.size()));
        ap_list.swap(results);
    }

    void WlanPage::processPendingEvents()
    {
        const uint32_t events = pending_events.exchange(0);

        if (scan_deferred && ui_state == UiState::Scanning && !link_is_joining()) {
            requestScan();
            // The hint changes from "Waiting..." to "Scanning..."; the link check below may not rebuild.
            rebuildList();
        }

        if (events & PENDING_SCAN_DONE) {
            applyScanRecords();
            if (ui_state == UiState::Scanning) {
                setUiState(UiState::Idle);
            } else if (ui_state == UiState::Idle) {
                rebuildList();
            }
        }

        if (ui_state == UiState::Connecting) {
            board_wifi_link_t link = {};
            board_wifi_service_get_link(&link);
            const bool target_connected = link.state == BOARD_WIFI_LINK_GOT_IP &&
                                          strcmp(link.ssid, connecting_ssid) == 0;
            bool target_disconnected = false;
            {
                std::lock_guard<std::mutex> lock(shared_mutex);
                target_disconnected = strcmp(disconnected_ssid, connecting_ssid) == 0;
            }

            if ((events & PENDING_GOT_IP) && target_connected) {
                finishConnect(true);
            } else if ((events & PENDING_DISCONNECTED) && target_disconnected) {
                if (ignore_next_disconnect) {
                    ignore_next_disconnect = false;
                } else {
                    finishConnect(false);
                }
            } else if (events & PENDING_CONNECT_FAILED) {
                finishConnect(false);
            } else if (lv_tick_elaps(connect_start_tick) > CONNECT_TIMEOUT_MS) {
                finishConnect(false, true);
            }
            return;
        }

        // Compare against the board link snapshot each tick; this also covers attempts the selector
        // starts on its own.
        if (ui_state == UiState::Idle || ui_state == UiState::Scanning) {
            board_wifi_link_t link = {};
            board_wifi_service_get_link(&link);
            if (link.state != rendered_link.state || strcmp(link.ssid, rendered_link.ssid) != 0) {
                rebuildList();
            }
        }
    }

    void WlanPage::wifi_service_event_cb(const adf_event_t *event, void *ctx)
    {
        WlanPage *self = static_cast<WlanPage *>(ctx);
        if (self == nullptr || event == nullptr) {
            return;
        }

        uint32_t pending = 0;
        switch (event->event_id) {
        case BOARD_WIFI_EVENT_CONNECTED:
            pending = PENDING_CONNECTED;
            break;
        case BOARD_WIFI_EVENT_DISCONNECTED: {
            std::lock_guard<std::mutex> lock(self->shared_mutex);
            self->disconnected_ssid[0] = '\0';
            if (event->payload != nullptr && event->payload_len >= sizeof(wifi_event_sta_disconnected_t)) {
                auto *info = static_cast<const wifi_event_sta_disconnected_t *>(event->payload);
                copy_ssid(self->disconnected_ssid, sizeof(self->disconnected_ssid), info->ssid, info->ssid_len);
            }
            pending = PENDING_DISCONNECTED;
            break;
        }
        case BOARD_WIFI_EVENT_STA_GOT_IP:
            pending = PENDING_GOT_IP;
            break;
        case BOARD_WIFI_EVENT_STA_LOST_IP:
            pending = PENDING_LOST_IP;
            break;
        case BOARD_WIFI_EVENT_SELECTOR_SWITCH_FAILED:
            pending = PENDING_CONNECT_FAILED;
            break;
        default:
            return;
        }
        self->pending_events.fetch_or(pending);
    }

    // Runs in the event loop task: copy the raw records only, the LVGL timer does the rest.
    void WlanPage::wifi_scan_done_cb(esp_err_t scan_err, uint16_t ap_count, const wifi_ap_record_t *ap_records, void *ctx)
    {
        WlanPage *self = static_cast<WlanPage *>(ctx);
        if (self == nullptr) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(self->shared_mutex);
            self->scan_error = scan_err;
            if (scan_err == ESP_OK && ap_records != nullptr) {
                self->scan_records.assign(ap_records, ap_records + ap_count);
            } else {
                self->scan_records.clear();
            }
        }
        self->pending_events.fetch_or(PENDING_SCAN_DONE);
    }

    void WlanPage::ui_timer_cb(lv_timer_t *timer)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_timer_get_user_data(timer));
        if (self != nullptr) {
            self->processPendingEvents();
        }
    }

    void WlanPage::switch_event_cb(lv_event_t *e)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_event_get_user_data(e));
        lv_obj_t *sw = static_cast<lv_obj_t *>(lv_event_get_target(e));
        if (self == nullptr || sw == nullptr) {
            return;
        }

        const bool enable = lv_obj_has_state(sw, LV_STATE_CHECKED);
        esp_err_t ret = board_wifi_service_set_enabled(enable);
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("Set Wi-Fi enabled=%d failed: %s", enable, esp_err_to_name(ret));
        }

        if (enable && ret == ESP_OK) {
            self->setUiState(UiState::Scanning);
            self->requestScan();
        } else {
            self->setUiState(UiState::Off);
            if (enable) {
                self->showMessage("Wi-Fi unavailable", "Unable to turn on Wi-Fi.");
            }
        }
    }

    void WlanPage::refresh_event_cb(lv_event_t *e)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_event_get_user_data(e));
        if (self == nullptr || self->ui_state != UiState::Idle) {
            return;
        }
        // The button is rebuilt by the scan state change; do not delete it inside its own event.
        lv_async_call([](void *param) {
            WlanPage *page = static_cast<WlanPage *>(param);
            if (page->page_root == nullptr || page->ui_state != UiState::Idle) {
                return;
            }
            page->setUiState(UiState::Scanning);
            page->requestScan();
        }, self);
    }

    void WlanPage::ap_btn_event_cb(lv_event_t *e)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_event_get_user_data(e));
        lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
        if (self == nullptr || btn == nullptr || self->ui_state != UiState::Idle) {
            return;
        }

        const size_t index = reinterpret_cast<size_t>(lv_obj_get_user_data(btn));
        if (index >= self->ap_list.size()) {
            return;
        }

        self->selected_ap = self->ap_list[index];
        ESP_UTILS_LOGI("Selected SSID: %s", self->selected_ap.ssid);
        // Connecting rebuilds the list, which would delete this row inside its own event.
        lv_async_call([](void *param) {
            WlanPage *page = static_cast<WlanPage *>(param);
            if (page->page_root == nullptr || page->ui_state != UiState::Idle) {
                return;
            }
            if (page->selected_ap.authmode == WIFI_AUTH_OPEN) {
                page->startConnect(page->selected_ap, "");
            } else {
                page->showPasswordInput(page->selected_ap);
            }
        }, self);
    }

    void WlanPage::conn_btn_event_cb(lv_event_t *e)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_event_get_user_data(e));
        if (self != nullptr && self->ui_state == UiState::Idle) {
            self->showForgetConfirm();
        }
    }

    // Keyboard READY submits the password, CANCEL returns to the network list.
    void WlanPage::kb_event_cb(lv_event_t *e)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_event_get_user_data(e));
        lv_event_code_t code = lv_event_get_code(e);
        if (self == nullptr || self->ui_state != UiState::Password) {
            return;
        }

        if (code == LV_EVENT_CANCEL) {
            self->setUiState(UiState::Idle);
            return;
        }

        if (code == LV_EVENT_READY) {
            const char *password = lv_textarea_get_text(self->ta);
            const size_t password_len = strlen(password);
            ESP_UTILS_LOGI("Wi-Fi password length: %u", static_cast<unsigned>(password_len));
            if (password_len < 8) {
                lv_label_set_text(self->password_title, "Password must be at least 8 characters");
                return;
            }
            self->startConnect(self->selected_ap, password);
        }
    }

    void WlanPage::dialog_btn_event_cb(lv_event_t *e)
    {
        WlanPage *self = static_cast<WlanPage *>(lv_event_get_user_data(e));
        lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
        if (self == nullptr || btn == nullptr || self->dialog == nullptr) {
            return;
        }

        const intptr_t action = reinterpret_cast<intptr_t>(lv_obj_get_user_data(btn));
        self->closeDialog(true);

        if (action == DIALOG_ACTION_FORGET) {
            self->forgetConnectedNetwork();
        }
    }
} // namespace esp_brookesia::apps
