// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

class MenuItem_SetAlarm_Time : public MenuItem {
public:
    ALARM_TIME* target = NULL;
    int hour, min;

    MenuItem_SetAlarm_Time(ALARM_TIME* ptarget, int phour, int pmin) {
        target = ptarget;
        hour = phour;
        min = pmin;
        if (phour >= 0) {
            struct tm tm = { 0 };
            tm.tm_hour = hour;
            tm.tm_min = min;
            text = tm_to_str(tm);
        } else {
            text = "Clear Time";
        }
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (target->hour == hour && target->minute == min) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        target->hour = hour;
        target->minute = min;
        //cfg->WriteConfigFile();
        save_dynamic_settings();
        return MCR_HOME;
    }
};

void switch_to_menu_set_alarm(ALARM_TIME* target, const string& title) {
    shared_ptr<Menu> m = make_shared<Menu>();

    m->title = title.empty() ? "Set Alarm" : title;

    shared_ptr<MenuItem> i = make_shared<MenuItem_SetAlarm_Time>(target, -1, -1);
    m->items.push_back(i);

#if 1
    // def DEBUG
    {
        time_t tme = time(NULL);
        struct tm tm;
        localtime_r(&tme, &tm);
        tm.tm_min++;
        if (tm.tm_min > 59) {
            tm.tm_min = 0;
            tm.tm_hour++;
            if (tm.tm_hour > 23) {
                tm.tm_hour = 0;
            }
        }

        i = make_shared<MenuItem_SetAlarm_Time>(target, tm.tm_hour, tm.tm_min);
        m->items.push_back(i);
    }
#endif

    i = make_shared<MenuItem_SetAlarm_Time>(target, 10, 0);
    m->items.push_back(i);

    for (int h = 9; h >= 0; h--) {
        for (int min = 45; min >= 0; min -= 15) {
            shared_ptr<MenuItem> i = make_shared<MenuItem_SetAlarm_Time>(target, h, min);
            m->items.push_back(i);
        }
    }
    for (int h = 23; h >= 10; h--) {
        for (int min = 45; min >= 0; min -= 15) {
            if (h == 10 && min == 0) { break; }
            shared_ptr<MenuItem> i = make_shared<MenuItem_SetAlarm_Time>(target, h, min);
            m->items.push_back(i);
        }
    }

    /*
    for (int h = 7; h <= 10; h++) {
        for (int min = 0; min <= 45; min += 15) {
            shared_ptr<MenuItem> i = make_shared<MenuItem_SetAlarm_Time>(target, h, min);
            m->items.push_back(i);
        }
    }
    */

    m->RecalcRects();
    switch_to_menu(m);
}

class MenuItem_SetAlarm_DayOfWeek : public MenuItem {
public:
    int dow;
    MenuItem_SetAlarm_DayOfWeek(int pdow, const string& str) {
        dow = pdow;
        text = str;
        if (config.options.daily_alarms[dow].isValid()) {
            struct tm tm = { 0 };
            tm.tm_hour = config.options.daily_alarms[dow].hour;
            tm.tm_min = config.options.daily_alarms[dow].minute;
            footer = tm_to_str(tm);
        }
    }

    void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {
        if (config.options.daily_alarms[dow].isValid()) {
            bgcol = colors.menu_highlight_bg;
            fgcol = colors.menu_highlight_text;
        }
    }

    virtual MENU_CLICK_RETURN OnPress() {
        switch_to_menu_set_alarm(&config.options.daily_alarms[dow], mprintf("Set %s Alarm", text.c_str()));
        return MCR_DO_NOTHING;
    }
};

void switch_to_menu_set_alarm_dow() {
    shared_ptr<Menu> m = make_shared<Menu>();

    m->title = "Set Day of Week Alarm Time";

    shared_ptr<MenuItem> i = make_shared<MenuItem_SetAlarm_DayOfWeek>(0, "Sun");
    m->items.push_back(i);
    i = make_shared<MenuItem_SetAlarm_DayOfWeek>(1, "Mon");
    m->items.push_back(i);
    i = make_shared<MenuItem_SetAlarm_DayOfWeek>(2, "Tue");
    m->items.push_back(i);
    i = make_shared<MenuItem_SetAlarm_DayOfWeek>(3, "Wed");
    m->items.push_back(i);
    i = make_shared<MenuItem_SetAlarm_DayOfWeek>(4, "Thu");
    m->items.push_back(i);
    i = make_shared<MenuItem_SetAlarm_DayOfWeek>(5, "Fri");
    m->items.push_back(i);
    i = make_shared<MenuItem_SetAlarm_DayOfWeek>(6, "Sat");
    m->items.push_back(i);

    m->RecalcRects();
    switch_to_menu(m);
}
