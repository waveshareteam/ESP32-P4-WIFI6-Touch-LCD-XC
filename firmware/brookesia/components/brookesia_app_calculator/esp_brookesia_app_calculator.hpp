/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps
{

    class Calculator : public systems::phone::App
    {
    public:
        static Calculator *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~Calculator();

        using systems::phone::App::endRecordResource;
        using systems::phone::App::startRecordResource;

    protected:
        Calculator(bool use_status_bar, bool use_navigation_bar);

        // Core app interface methods
        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;

        // Calculator logic methods
        bool isStartZero(void);
        bool isStartNum(void);
        bool isStartPercent(void);
        bool isLegalDot(void);
        double calculate(const char *input);

        // Layout builders: the round one keeps every widget inside the visible circle.
        void createRectLayout(void);
        void createRoundLayout(int32_t panel_size, const lv_area_t &screen_area);
        void alignRoundLabel(lv_obj_t *label);

        int formula_len;
        lv_obj_t *keyboard;
        lv_obj_t *history_label;
        lv_obj_t *formula_label;
        lv_obj_t *result_label;
        uint16_t _height;
        uint16_t _width;
        bool _round_layout = false;
        const lv_font_t *_font_big = nullptr;   // Font of the active line (formula or result)
        const lv_font_t *_font_small = nullptr; // Font of the inactive line and the history

        bool isStarting(void) const
        {
            return _is_starting;
        }
        bool isStopping(void) const
        {
            return _is_stopping;
        }

    private:
        static Calculator *_instance;
        static void keyboard_event_cb(lv_event_t *e);

        std::atomic<bool> _is_starting = false;
        std::atomic<bool> _is_stopping = false;
    };

} // namespace esp_brookesia::apps
