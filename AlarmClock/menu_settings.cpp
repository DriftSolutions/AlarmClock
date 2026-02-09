// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

class MenuItem_MainMenu_DimWhenDark : public MenuItem {
public:
    MenuItem_MainMenu_DimWhenDark() {
        text = "Dim When Dark";
        updateFooter();
    }

    void updateFooter() {
        footer = config.options.dim_when_dark ? "On" : "Off";
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.dim_when_dark) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        } else {
            bgcol = colors.menu_normal_bg;
            fgcol = colors.menu_normal_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        config.options.dim_when_dark = !config.options.dim_when_dark;
        updateFooter();
        update_lcd_brightness();
        if (config.cur_page) {
            config.cur_page->OnDarkChanged();
        }
        save_dynamic_settings();
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_AutoEnableMidnight : public MenuItem {
public:
    MenuItem_MainMenu_AutoEnableMidnight() {
        text = "Auto Enable Alarm at Midnight";
        updateFooter();
    }

    void updateFooter() {
        footer = config.options.auto_enable_at_midnight ? "On" : "Off";
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.auto_enable_at_midnight) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        } else {
            bgcol = colors.menu_normal_bg;
            fgcol = colors.menu_normal_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        config.options.auto_enable_at_midnight = !config.options.auto_enable_at_midnight;
        updateFooter();
        save_dynamic_settings();
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_FlipClockStyle : public MenuItem {
public:
    MenuItem_MainMenu_FlipClockStyle() {
        text = "Flip Clock Style";
        updateFooter();
    }

    void updateFooter() {
        footer = config.options.flip_clock_style ? "On" : "Off";
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.flip_clock_style) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        } else {
            bgcol = colors.menu_normal_bg;
            fgcol = colors.menu_normal_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        config.options.flip_clock_style = !config.options.flip_clock_style;
        updateFooter();
        save_dynamic_settings();
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_ScreenTimeout : public MenuItem {
public:
    set<uint64> options = { 15000, 30000, 45000, 60000 };

    MenuItem_MainMenu_ScreenTimeout() {
        text = "Screen Timeout";
        updateFooter();
    }

    void updateFooter() {
        footer = mprintf("%llus", config.options.screen_timeout / 1000);
    }

    virtual MENU_CLICK_RETURN OnPress() {
        auto x = options.find(config.options.screen_timeout);
        if (x != options.end() && ++x != options.end()) {
            config.options.screen_timeout = *x;
        } else {
            config.options.screen_timeout = *options.begin();
        }
        updateFooter();
        save_dynamic_settings();
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_Exit : public MenuItem {
public:
    MenuItem_MainMenu_Exit() {
        text = "Exit Clock";
    }

    virtual MENU_CLICK_RETURN OnPress() {
        config.shutdown_now = true;
        return MCR_DO_NOTHING;
    }
};

void switch_to_menu_settings() {
    shared_ptr<Menu> m = make_shared<Menu>();
    m->title = "Settings";

    size_t btn_ind = 0;

    shared_ptr<MenuItem> i = make_shared<MenuItem_MainMenu_DimWhenDark>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_AutoEnableMidnight>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_ScreenTimeout>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_FlipClockStyle>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);    

    i = make_shared<MenuItem_MainMenu_Exit>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);    

    switch_to_menu(m);
}
