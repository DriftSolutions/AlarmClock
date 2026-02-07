// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

// Only runs on Raspberry Pi (Linux) using DSI screens

bool CONFIG::get_lcd_brightness(uint8& val) { // 0-255
    if (got_lcd_brightness) {
        //printf("Cached LCD brightness: %u\n", config.cur_lcd_brightness);
        val = cur_lcd_brightness;
        return true;
    }

    bool ret = false;
#ifndef WIN32
    FILE* fp = fopen(config.lcd_brightness_fn.c_str(), "rb");
    if (fp == NULL) {
        return false;
    }
    char buf[32] = { 0 };
    if (!feof(fp) && (fgets(buf, sizeof(buf), fp)) != NULL) {
        strtrim(buf);
        if (strspn(buf, "0123456789") == strlen(buf)) {
            int tmp = atoi(buf);
            if (tmp >= 0 && tmp <= 255) {
                config.cur_lcd_brightness = val = (uint8)tmp;
                config.got_lcd_brightness = true;
                ret = true;
                printf("Got LCD brightness: %u\n", val);
            }
        }
    }
    fclose(fp);
#endif
    return ret;
}

bool CONFIG::set_lcd_brightness(uint8 val) { // 0-255
    uint8 tmp;
    if (!get_lcd_brightness(tmp) || tmp != val) {
#ifndef WIN32
        FILE* fp = fopen(config.lcd_brightness_fn.c_str(), "wb");
        if (fp == NULL) {
            return false;
        }
        fprintf(fp, "%u", val);
        fclose(fp);
        printf("Set LCD brightness to: %u\n", val);
#endif
        got_lcd_brightness = true;
        cur_lcd_brightness = val;
    }
    return true;
}

void update_lcd_brightness() {
    static uint8 bright_level = (uint8)max((int64)0, min((int64)255, cfg->GetIntArg("-lcd_bright_level", 255)));
    static uint8 dim_level = (uint8)max((int64)0, min((int64)255, cfg->GetIntArg("-lcd_dim_level", 50)));

    if (config.is_dark && config.options.dim_when_dark) {
        config.set_lcd_brightness(dim_level);
    } else {
        config.set_lcd_brightness(bright_level);
    }
}

