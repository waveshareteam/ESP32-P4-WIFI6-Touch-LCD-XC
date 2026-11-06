#pragma once

#include "lvgl.h"
#include "sdkconfig.h"

namespace esp_brookesia::apps::settings_ui {

#if CONFIG_SETTINGS_UI_ROUND_DISPLAY
static constexpr bool ROUND_DISPLAY = true;
#else
static constexpr bool ROUND_DISPLAY = false;
#endif

static constexpr lv_coord_t PAGE_HEADER_HEIGHT = 72;
static constexpr lv_coord_t HEADER_BUTTON_SIZE = 48;
static constexpr lv_coord_t HEADER_BUTTON_EDGE_MARGIN = 20;
static constexpr lv_coord_t HEADER_TITLE_HEIGHT = 36;
static constexpr lv_coord_t HEADER_TITLE_GAP = 12;
static constexpr lv_coord_t ROUND_EDGE_MARGIN = 16;
static constexpr lv_coord_t PAGE_HORIZONTAL_MARGIN = 32;
static constexpr lv_coord_t CONTENT_TOP_GAP = 12;
static constexpr lv_coord_t CONTENT_BOTTOM_MARGIN = 20;
static constexpr lv_coord_t ROW_HEIGHT = 84;
static constexpr lv_coord_t CONTROL_PANEL_HEIGHT = 140;

static constexpr uint32_t COLOR_BACKGROUND = 0x000000;
static constexpr uint32_t COLOR_SURFACE = 0x242426;
static constexpr uint32_t COLOR_SURFACE_PRESSED = 0x343438;
static constexpr uint32_t COLOR_BORDER = 0x3A3A3C;
static constexpr uint32_t COLOR_PRIMARY_TEXT = 0xFFFFFF;
static constexpr uint32_t COLOR_SECONDARY_TEXT = 0xA7A7AD;
static constexpr uint32_t COLOR_ACCENT = 0x00BFFF;

/*
 * Size of the physical panel. While the core runs an app it shrinks the display resolution
 * to the app's visual area (e.g. below an opaque status bar), so the panel is taken as the
 * larger of the display resolution and the far edge of the object's screen.
 */
inline void get_panel_size(lv_obj_t *obj, int32_t &hor, int32_t &ver)
{
    lv_display_t *disp = lv_obj_get_display(obj);
    lv_obj_t *screen = lv_obj_get_screen(obj);
    lv_obj_update_layout(screen);
    lv_area_t screen_area;
    lv_obj_get_coords(screen, &screen_area);
    hor = LV_MAX(lv_display_get_horizontal_resolution(disp), screen_area.x2 + 1);
    ver = LV_MAX(lv_display_get_vertical_resolution(disp), screen_area.y2 + 1);
}

/*
 * With CONFIG_SETTINGS_UI_ROUND_DISPLAY, returns half of the visible width of the
 * circular panel for the band [abs_y_top, abs_y_bottom] (screen coordinates), i.e.
 * the chord at the band edge farthest from the centre. Otherwise, and on non-square
 * displays, the panel is a rectangle and half of the screen width is returned.
 */
inline lv_coord_t round_screen_half_width(lv_obj_t *obj, lv_coord_t abs_y_top, lv_coord_t abs_y_bottom)
{
    int32_t hor = 0;
    int32_t ver = 0;
    get_panel_size(obj, hor, ver);
    if (!ROUND_DISPLAY || (hor != ver)) {
        return hor / 2;
    }

    const int32_t radius = hor / 2;
    const int32_t dy_top = LV_ABS(abs_y_top - radius);
    const int32_t dy_bottom = LV_ABS(abs_y_bottom - radius);
    const int32_t dy = LV_MIN(LV_MAX(dy_top, dy_bottom), radius);
    return lv_sqrt32(static_cast<uint32_t>(radius * radius - dy * dy));
}

/*
 * Distance from the left or right edge of a full-width page to a header button, so the
 * button stays inside the visible area. On a rectangular panel this is the plain margin.
 */
inline lv_coord_t header_button_inset(lv_obj_t *page)
{
    lv_obj_update_layout(page);
    lv_area_t page_area;
    lv_obj_get_coords(page, &page_area);
    const lv_coord_t top = page_area.y1 + (PAGE_HEADER_HEIGHT - HEADER_BUTTON_SIZE) / 2;
    const lv_coord_t half_width = round_screen_half_width(page, top, top + HEADER_BUTTON_SIZE);
    int32_t panel_hor = 0;
    int32_t panel_ver = 0;
    get_panel_size(page, panel_hor, panel_ver);
    const lv_coord_t screen_half = panel_hor / 2;
    return LV_MAX(HEADER_BUTTON_EDGE_MARGIN, screen_half - half_width + ROUND_EDGE_MARGIN);
}

/*
 * top_inset reserves space at the top of the screen, e.g. for a status bar
 * that floats over the app. The page and everything sized in percent inside
 * it then fit below that space.
 */
inline lv_obj_t *create_page(lv_obj_t *screen, lv_coord_t top_inset = 0)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_top(screen, top_inset, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *page = lv_obj_create(screen);
    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, lv_pct(100), lv_pct(100));
    lv_obj_center(page);
    lv_obj_set_style_bg_color(page, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

inline lv_obj_t *create_header(
    lv_obj_t *page, const char *title, lv_event_cb_t back_cb = nullptr, void *user_data = nullptr)
{
    lv_obj_t *header = lv_obj_create(page);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), PAGE_HEADER_HEIGHT);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(header, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    const lv_coord_t button_inset = header_button_inset(page);
    lv_obj_t *back_button = nullptr;
    if (back_cb != nullptr) {
        back_button = lv_button_create(header);
        lv_obj_set_size(back_button, HEADER_BUTTON_SIZE, HEADER_BUTTON_SIZE);
        lv_obj_align(back_button, LV_ALIGN_LEFT_MID, button_inset, 0);
        lv_obj_set_style_radius(back_button, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(back_button, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(back_button, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(back_button, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_color(back_button, lv_color_hex(COLOR_SURFACE_PRESSED), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(back_button, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_add_event_cb(back_button, back_cb, LV_EVENT_CLICKED, user_data);

        lv_obj_t *back_icon = lv_label_create(back_button);
        lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(back_icon, &lv_font_montserrat_30, LV_PART_MAIN);
        lv_obj_set_style_text_color(back_icon, lv_color_hex(COLOR_PRIMARY_TEXT), LV_PART_MAIN);
        lv_obj_center(back_icon);
    }

    lv_obj_t *title_label = lv_label_create(header);
    lv_label_set_text(title_label, title);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_PRIMARY_TEXT), LV_PART_MAIN);

    const lv_coord_t page_width = lv_obj_get_width(page);
    lv_coord_t title_width = 0;
    if (ROUND_DISPLAY) {
        // Centre the title in the chord of its band, leaving room on both sides for the back
        // button and a page action button (e.g. WLAN refresh) placed with header_button_inset().
        lv_area_t page_area;
        lv_obj_get_coords(page, &page_area);
        const lv_coord_t title_top = page_area.y1 + (PAGE_HEADER_HEIGHT - HEADER_TITLE_HEIGHT) / 2;
        title_width = 2 * (round_screen_half_width(page, title_top, title_top + HEADER_TITLE_HEIGHT) -
                           ROUND_EDGE_MARGIN);
        if (back_button != nullptr) {
            title_width = LV_MIN(title_width,
                                 page_width - 2 * (button_inset + HEADER_BUTTON_SIZE + HEADER_TITLE_GAP));
        }
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_width(title_label, LV_MAX(static_cast<lv_coord_t>(1), title_width));
        lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
    } else {
        const lv_coord_t title_x = (back_button != nullptr) ?
                                   (button_inset + HEADER_BUTTON_SIZE + 16) : PAGE_HORIZONTAL_MARGIN;
        title_width = page_width - title_x - PAGE_HORIZONTAL_MARGIN;
        lv_obj_set_width(title_label, LV_MAX(static_cast<lv_coord_t>(1), title_width));
        lv_obj_align(title_label, LV_ALIGN_LEFT_MID, title_x, 0);
    }

    return back_button;
}

inline lv_obj_t *create_content_list(lv_obj_t *page)
{
    lv_obj_update_layout(page);
    lv_coord_t content_width = lv_obj_get_width(page) - PAGE_HORIZONTAL_MARGIN * 2;
    lv_coord_t content_height = lv_obj_get_height(page) - PAGE_HEADER_HEIGHT -
                                CONTENT_TOP_GAP - CONTENT_BOTTOM_MARGIN;
    if (content_width < 1) {
        content_width = 1;
    }
    if (content_height < 1) {
        content_height = 1;
    }

    int32_t screen_width = 0;
    int32_t screen_height = 0;
    get_panel_size(page, screen_width, screen_height);
    const bool round_layout = ROUND_DISPLAY && (screen_width == screen_height);
    if (round_layout) {
        // Fit the rows to the width of the square inscribed in the circle.
        const int32_t radius = screen_width / 2;
        const lv_coord_t inscribed_width = 2 * lv_sqrt32(static_cast<uint32_t>(radius * radius / 2));
        content_width = LV_MIN(content_width, inscribed_width);
    }

    lv_obj_t *list = lv_list_create(page);
    lv_obj_set_size(list, content_width, content_height);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, PAGE_HEADER_HEIGHT + CONTENT_TOP_GAP);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    if (round_layout) {
        // Rows at the top and bottom of the list would be clipped by the arcs. Pad the list so the
        // first and last rows can scroll into the band where the circle is as wide as the list.
        // Local styles win over the list style the pages add afterwards.
        lv_obj_update_layout(list);
        lv_area_t list_area;
        lv_obj_get_coords(list, &list_area);
        const int32_t radius = screen_width / 2;
        const int32_t half_width = LV_MIN(content_width / 2 + ROUND_EDGE_MARGIN, radius);
        const lv_coord_t fit_dy = lv_sqrt32(static_cast<uint32_t>(radius * radius - half_width * half_width));
        lv_obj_set_style_pad_top(list, LV_MAX(0, (radius - fit_dy) - list_area.y1), LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(list, LV_MAX(0, list_area.y2 - (radius + fit_dy)), LV_PART_MAIN);
    }
    return list;
}

inline void init_list_styles(
    lv_style_t &list, lv_style_t &row, lv_style_t &section, lv_style_t &pressed)
{
    lv_style_init(&list);
    lv_style_set_pad_all(&list, 0);
    lv_style_set_pad_row(&list, 6);
    lv_style_set_border_width(&list, 0);
    lv_style_set_radius(&list, 0);
    lv_style_set_bg_color(&list, lv_color_hex(COLOR_BACKGROUND));
    lv_style_set_bg_opa(&list, LV_OPA_TRANSP);

    lv_style_init(&row);
    lv_style_set_height(&row, ROW_HEIGHT);
    lv_style_set_pad_hor(&row, 18);
    lv_style_set_pad_ver(&row, 12);
    lv_style_set_pad_column(&row, 14);
    lv_style_set_border_width(&row, 0);
    lv_style_set_radius(&row, 6);
    lv_style_set_bg_color(&row, lv_color_hex(COLOR_SURFACE));
    lv_style_set_bg_opa(&row, LV_OPA_COVER);
    lv_style_set_text_font(&row, &lv_font_montserrat_26);
    lv_style_set_text_color(&row, lv_color_hex(COLOR_PRIMARY_TEXT));

    lv_style_init(&section);
    lv_style_set_text_font(&section, &lv_font_montserrat_20);
    lv_style_set_text_color(&section, lv_color_hex(COLOR_SECONDARY_TEXT));
    lv_style_set_bg_color(&section, lv_color_hex(COLOR_BACKGROUND));
    lv_style_set_bg_opa(&section, LV_OPA_TRANSP);
    lv_style_set_pad_left(&section, 4);
    lv_style_set_pad_right(&section, 4);
    lv_style_set_pad_top(&section, 16);
    lv_style_set_pad_bottom(&section, 4);

    lv_style_init(&pressed);
    lv_style_set_bg_color(&pressed, lv_color_hex(COLOR_SURFACE_PRESSED));
    lv_style_set_bg_opa(&pressed, LV_OPA_COVER);
    lv_style_set_text_color(&pressed, lv_color_hex(COLOR_PRIMARY_TEXT));
}

inline void reset_list_styles(
    lv_style_t &list, lv_style_t &row, lv_style_t &section, lv_style_t &pressed)
{
    lv_style_reset(&list);
    lv_style_reset(&row);
    lv_style_reset(&section);
    lv_style_reset(&pressed);
}

inline lv_obj_t *add_section(lv_obj_t *list, const char *text, lv_style_t &section_style)
{
    lv_obj_t *section = lv_list_add_text(list, text);
    lv_obj_add_style(section, &section_style, LV_PART_MAIN);
    return section;
}

inline lv_obj_t *get_button_label(lv_obj_t *button)
{
    const uint32_t child_count = lv_obj_get_child_count(button);
    for (uint32_t i = 0; i < child_count; ++i) {
        lv_obj_t *child = lv_obj_get_child(button, i);
        if (lv_obj_has_class(child, &lv_label_class)) {
            return child;
        }
    }
    return nullptr;
}

inline void use_ellipsis_for_button_label(lv_obj_t *button)
{
    lv_obj_t *label = get_button_label(button);
    if (label != nullptr) {
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_flex_grow(label, 1);
    }
}

inline lv_obj_t *add_info_row(
    lv_obj_t *list, const void *icon, const char *name, const char *value, lv_style_t &row_style)
{
    lv_obj_t *row = lv_list_add_button(list, icon, nullptr);
    lv_obj_add_style(row, &row_style, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_flex_grow(name_label, 1);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_PRIMARY_TEXT), LV_PART_MAIN);

    lv_obj_t *value_label = lv_label_create(row);
    lv_label_set_text(value_label, value);
    lv_label_set_long_mode(value_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(value_label, lv_pct(52));
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(value_label, lv_color_hex(COLOR_SECONDARY_TEXT), LV_PART_MAIN);
    return row;
}

} // namespace esp_brookesia::apps::settings_ui
