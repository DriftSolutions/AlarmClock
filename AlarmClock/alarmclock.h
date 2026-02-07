// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#pragma once
#define SDL_MAIN_HANDLED
//#define DEBUG_FPS
#ifndef DSL_STATIC
#define DSL_STATIC
#endif
#ifndef ENABLE_CURL
#define ENABLE_CURL
#endif
/*
#ifndef ENABLE_SQLITE
#define ENABLE_SQLITE
#endif
#ifndef ENABLE_LIBEVENT
#define ENABLE_LIBEVENT
#endif
*/
#include <drift/dsl.h>
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_ttf.h>
#include <univalue.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 480

#define BUTTONS_PER_ROW 4
#define BUTTONS_PER_COL 2
#define NUM_BUTTONS ((BUTTONS_PER_ROW - 1) * BUTTONS_PER_COL)

#define TITLE_FONT_SIZE 42
#define TIME_FONT_SIZE 200
#define FLIP_TIME_FONT_SIZE 100
#define ALARM_FONT_SIZE 96
#define MENU_FONT_SIZE 36
#define FOOTER_FONT_SIZE 24
#define STATUS_FONT_SIZE 18

#define ALERT_CHANNEL 0

#define printf ac_printf
void ac_printf(const char* pszFormat, ...);

typedef struct SDL_Size {
	int w;
	int h;
} SDL_Size;

#include "menus.h"
#include "status_bar.h"
#include "config.h"
extern shared_ptr<AlarmConfig> cfg;

struct CONFIG_COLORS {
	SDL_Color clock_bg;
	SDL_Color clock_text;
	SDL_Color clock_red_text;

	SDL_Color menu_page_bg;
	SDL_Color menu_page_text;
	SDL_Color menu_normal_bg;
	SDL_Color menu_normal_text;
	SDL_Color menu_highlight_bg;
	SDL_Color menu_highlight_text;
	SDL_Color menu_off_bg;
	SDL_Color menu_off_text;
	SDL_Color menu_on_bg;
	SDL_Color menu_on_text;
	SDL_Color menu_inactive_bg;
	SDL_Color menu_inactive_text;
	SDL_Color menu_frame;
};
extern const CONFIG_COLORS colors;

class Page {
public:
	virtual void Tick() {};

	virtual void OnActivate() {};
	virtual void OnDarkChanged() {}
	virtual void OnClick(const SDL_Point& pt) {}

	virtual void Render() = 0;
};

void SetNextPage(shared_ptr<Page>& p);
void switch_to_clock();
void switch_to_menu_settings();
void switch_to_menu(shared_ptr<Menu>& menu);

struct ALARM_TIME {
	int hour = -1;
	int minute = -1;

	bool isValid() const {
		return (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59);
	}
};

class ConfigOptions {
public:
	bool enable_alarm = false;
	bool dim_when_dark = true;
	bool auto_enable_at_midnight = false;
	bool flip_clock_style = false;
	// If there is no activity, it will return to the clock display after this long
#ifdef DEBUG
	uint64 screen_timeout = 30000;
#else
	uint64 screen_timeout = 15000;
#endif

	ALARM_TIME one_time_alarm;
	ALARM_TIME alarm_time;
	ALARM_TIME daily_alarms[7];

	void ToUniValue(UniValue& obj);
	bool FromUniValue(const UniValue& obj);

	bool ShouldAlarmAt(time_t ts, struct tm* ptm = NULL);
	time_t GetNextAlarmTime() const;
	time_t GetNextAlarmTime(time_t tme) const;
	time_t GetNextAlarmTime(const struct tm& tm) const;
};

class HomeAssistantInfo {
public:
	int64 last_update = 0;

	bool is_dark = false;
	string weather;
	double temp = DBL_MIN;
	string temp_unit;

	bool hadRecentUpdate() const {
		return (last_update >= time(NULL) - 1800);
	}
};
bool GetHomeAssistantInfo(HomeAssistantInfo& info);

void load_dynamic_settings();
void save_dynamic_settings();
void update_lcd_brightness();

class CONFIG {
private:
	bool _alarming = false;
	bool _is_dark = false;

	bool got_lcd_brightness = false;
	uint8 cur_lcd_brightness = 0;
public:
	bool shutdown_now = false;
	FILE* log_fp = NULL;

	ConfigOptions options;

	const bool& alarming = _alarming;
	void Alarm();
	void ClearAlarm();

	bool use_sun = false;
	const bool& is_dark = _is_dark;
	void SetIsDark(bool dark);

	string lcd_brightness_fn;
	bool get_lcd_brightness(uint8& val);// 0-255
	bool set_lcd_brightness(uint8 val);

	struct {
		string url, token;
		bool isValid() const {
			return (!url.empty() && !token.empty());
		}

		string alarm_entity, next_alarm_entity;
		bool needToSendUpdates() const { 
			if (isValid()) {
				return (!alarm_entity.empty() || !next_alarm_entity.empty());
			}
			return false;
		}
		HomeAssistantInfo info;
	} home_assistant;

	Uint32 fps = 0;
	Uint32 fps_target = 0;
	uint32 cur_frame = 0;

	bool fullscreen = false;
	SDL_Window* mWnd = NULL;
	SDL_Renderer* mRenderer = NULL;

	SDL_Point normal_win_position = { INT_MAX, INT_MAX };
	SDL_Size win_size = { 0 };
	SDL_Rect title_area = { 0 };
	SDL_Rect main_area = { 0 };
	SDL_Rect status_area = { 0 };

	TTF_Font* backup_font = NULL;

	SDL_Point mpos = { 0,0 }; //mouse/finger pos in window pixels

	SDL_Rect menu_area = { 0 };
	struct {
		SDL_Rect back = { 0 };
		SDL_Rect home = { 0 };
		SDL_Rect prev = { 0 };
		SDL_Rect next = { 0 };
		SDL_Rect buttons[NUM_BUTTONS] = { 0 };
	} menu_buttons;

	shared_ptr<Menu> side_menu;
	shared_ptr<Menu> next_menu;

	shared_ptr<Page> cur_page;
	shared_ptr<Page> next_page;
};
extern CONFIG config;

enum SB_SECTIONS {
	SB_NEXT_ALARM,
	SB_HOME_ASSISTANT_SEND,
	SB_HOME_ASSISTANT_RECV,
#ifdef DEBUG_FPS
	SB_FPS,
#endif
};
void statusbar_set(SB_SECTIONS sec, const string& str);
void statusbar_set_progress(SB_SECTIONS sec, size_t cur, size_t max);
extern SDL_StatusBar status_bar;

TTF_Font* GetFontSize(int ptSize);
TTF_Font* GetMonoFontSize(int ptSize);

void draw_text(TTF_Font* font, int x, int y, const SDL_Color& col, const char* str, int style = TTF_STYLE_NORMAL);
void draw_text(TTF_Font* font, int x, int y, const SDL_Color& col, const vector<string>& lines, int style = TTF_STYLE_NORMAL);
enum DRAW_TEXT_ALIGNMENTS {
	DTA_LEFT = 1,
	DTA_CENTER = 2,
	DTA_RIGHT = 4,
	DTA_TOP = 8,
	DTA_MIDDLE = 16,
	DTA_BOTTOM = 32
};
void draw_text(TTF_Font* font, const SDL_Rect& rc, uint8 align, const SDL_Color& col, const string& str, int style = TTF_STYLE_NORMAL); // align = DRAW_TEXT_ALIGNMENTS
void draw_text_wrapped(TTF_Font* font, const SDL_Rect& rc, uint8 align, const SDL_Color& col, const string& str, int style = TTF_STYLE_NORMAL); // doesn't size down font to fit rc
void draw_text_wrapped(int ptSize, const SDL_Rect& rc, uint8 align, const SDL_Color& col, const string& str, int style = TTF_STYLE_NORMAL); // sizes down font if needed to fit rc

class CachedText {
private:
	string _str;
	SDL_Texture* tex = NULL;
public:
	const string& str = _str;

	TTF_Font* font = NULL;
	SDL_Rect rc = { 0 };
	uint8 align = DTA_LEFT | DTA_TOP;
	SDL_Color col = { 0 };
	int style = TTF_STYLE_NORMAL;

	void setText(const string& pstr) {
		if (pstr != str) {
			_str = pstr;
			free_texture();
		}
	}
	void Draw();

	void free_texture() {
		if (tex != NULL) {
			SDL_DestroyTexture(tex);
			tex = NULL;
		}
	}
	~CachedText() {
		free_texture();
	}
};

void SDL_SetRenderDrawColor(SDL_Renderer* r, const SDL_Color& col);

string ts_to_str(time_t ts);
string tm_to_str(const tm& tm);
string FormatMinutes(int64 secs);

void start_home_assistant();
void update_home_assistant();
bool backup_file(const string& fn);

SDL_Rect ScaleRectToFit(const SDL_Rect& source, const SDL_Rect& container);
