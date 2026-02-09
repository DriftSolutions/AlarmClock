// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

class MenuItem_MainMenu_AlarmOnOff: public MenuItem {
public:
    MenuItem_MainMenu_AlarmOnOff() {
        updateText();
    }

    void updateText() {
        text = config.options.enable_alarm ? "Alarm On" : "Alarm Off";
        if (!config.options.enable_alarm && config.options.auto_enable_at_midnight) {
            footer = "Re-enables at Midnight";
        } else {
            footer.clear();
        }
    }

    void OnActivate() {
        updateText();
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.enable_alarm) {
            bgcol = colors.menu_on_bg;
            fgcol = colors.menu_on_text;
        } else {
            bgcol = colors.menu_off_bg;
            fgcol = colors.menu_off_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        config.options.enable_alarm = !config.options.enable_alarm;
        /*
        if (!config.options.enable_alarm) {
            config.ClearAlarm();
        }
        */
        updateText();
        save_dynamic_settings();
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_SetAlarmOneTime : public MenuItem {
public:
    MenuItem_MainMenu_SetAlarmOneTime() {
        updateText();
    }

    void updateText() {
        text = "Set One-Time Alarm Time";
        if (config.options.one_time_alarm.isValid()) {
            struct tm tm = { 0 };
            tm.tm_hour = config.options.one_time_alarm.hour;
            tm.tm_min = config.options.one_time_alarm.minute;
            footer = tm_to_str(tm);
        } else {
            footer.clear();
        }
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.one_time_alarm.isValid()) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        switch_to_menu_set_alarm(&config.options.one_time_alarm, "Set One-Time Alarm");
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_SetAlarmDaily : public MenuItem {
public:
    MenuItem_MainMenu_SetAlarmDaily() {
        text = "Set Daily Alarm Time";
        if (config.options.alarm_time.isValid()) {
            struct tm tm = { 0 };
            tm.tm_hour = config.options.alarm_time.hour;
            tm.tm_min = config.options.alarm_time.minute;
            //text += "\n\n";
            //text += tm_to_str(tm);
            footer = tm_to_str(tm);
        } else {
            footer.clear();
        }
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.alarm_time.isValid()) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        switch_to_menu_set_alarm(&config.options.alarm_time, "Set Daily Alarm");
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_SetAlarmDayOfWeek : public MenuItem {
private:
    size_t days_set = 0;
public:
    MenuItem_MainMenu_SetAlarmDayOfWeek() {
        text = "Set Day of Week Alarm Time";
        for (auto& d : config.options.daily_alarms) {
            if (d.isValid()) {
                days_set++;
            }
        }
        if (days_set == 1) {
            footer = "1 day set";
        } else if (days_set) {
            footer = mprintf("%zu days set", days_set);
        }
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (days_set) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        switch_to_menu_set_alarm_dow();
        return MCR_DO_NOTHING;
    }
};

class MenuItem_MainMenu_Settings : public MenuItem {
public:
    MenuItem_MainMenu_Settings() {
        text = "Settings";
        image = "settings.png";
    }

    virtual MENU_CLICK_RETURN OnPress() {
        switch_to_menu_settings();
        return MCR_DO_NOTHING;
    }
};

/*
class MenuItem_MainMenu_Back : public MenuItem {
public:
    MenuItem_MainMenu_Back() {
        text = "Back";
    }

    virtual MENU_CLICK_RETURN OnPress() {
        return MCR_BACK;
    }
};
*/

void switch_to_main_menu() {
    shared_ptr<Menu> m = make_shared<Menu>();
    m->title = "Main Menu";

    size_t btn_ind = 0;

    shared_ptr<MenuItem> i = make_shared<MenuItem_MainMenu_SetAlarmOneTime>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_SetAlarmDaily>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_SetAlarmDayOfWeek>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_AlarmOnOff>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);

    i = make_shared<MenuItem_MainMenu_Settings>();
    i->rc = config.menu_buttons.buttons[btn_ind++];
    m->items.push_back(i);
    
    switch_to_menu(m);
}
