// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

class PageClock : public Page {
public:
	CachedText txtTime, txtDate, txtAlarm, txtWeather;
	int64 last_time = 0;
	bool have_weather = false;
	SDL_Color text_col = colors.clock_text;

	void OnActivate();
	void OnClick(const SDL_Point& pt);
	void Tick();
	void Render();

	void OnDarkChanged();
};

void PageClock::OnDarkChanged() {
	if (config.is_dark && config.options.dim_when_dark) {
		text_col = colors.clock_red_text;
	} else {
		text_col = colors.clock_text;
	}
	txtTime.col = text_col;
	txtTime.free_texture();
	txtDate.col = text_col;
	txtDate.free_texture();
	txtAlarm.col = text_col;
	txtAlarm.free_texture();
	txtWeather.col = text_col;
	txtWeather.free_texture();
}

void PageClock::OnActivate() {
	txtTime.font = GetFontSize(TIME_FONT_SIZE);
	txtTime.rc = { 0, 0, config.win_size.w, config.win_size.h }; // config.main_area;
	txtTime.align = DTA_CENTER | DTA_MIDDLE;

	txtDate.font = GetFontSize(TITLE_FONT_SIZE);
	txtDate.rc = { 0, 0, config.win_size.w, config.win_size.h - 10 };
	txtDate.align = DTA_CENTER | DTA_BOTTOM;

	txtAlarm.font = GetFontSize(TITLE_FONT_SIZE);
	txtAlarm.rc = config.title_area;
	txtAlarm.align = DTA_LEFT | DTA_MIDDLE;
	txtAlarm.setText("Alarm On");

	txtWeather.font = GetFontSize(TITLE_FONT_SIZE);
	txtWeather.rc = config.title_area;
	txtWeather.align = DTA_RIGHT | DTA_MIDDLE;

	OnDarkChanged();
}

void PageClock::Tick() {
	time_t cur_time = time(NULL);
	if (last_time == 0 || (cur_time != last_time && cur_time % 60 == 0)) {
		struct tm tm;
		localtime_r(&cur_time, &tm);
		char buf[128] = { 0 };
		strftime(buf, sizeof(buf), "%A, %B %e, %Y", &tm);
		txtDate.setText(buf);

		txtTime.setText(tm_to_str(tm));
	}

	auto& ha = config.home_assistant.info;
	if (ha.hadRecentUpdate() && !ha.weather.empty()) {
		string str = ha.weather;
		if (ha.temp > DBL_MIN && !ha.weather.empty()) {
			str += mprintf(", %d%s", int(ha.temp), ha.temp_unit.c_str());
		}
		txtWeather.setText(str);
	} else {
		txtWeather.setText("");
	}
}

void PageClock::OnClick(const SDL_Point& pt) {
	if (config.alarming) {
		config.ClearAlarm();
	} else {
		switch_to_main_menu();
	}
}

void PageClock::Render() {
	if (config.options.enable_alarm) {
		txtAlarm.Draw();
	}
	if (!txtWeather.str.empty()) {
		txtWeather.Draw();
	}
	txtDate.Draw();
	if (config.alarming) {
		draw_text_wrapped(GetFontSize(ALARM_FONT_SIZE), config.main_area, DTA_CENTER | DTA_MIDDLE, colors.clock_text, "Time to Wake Up!");
	} else {
		txtTime.Draw();
	}

#ifdef DEBUG_FPS
	status_bar.Draw();
#endif

	/*
	{
		//SDL_Color col = { 0xFF, 0,0, 0xFF };
		//SDL_SetRenderDrawColor(config.mRenderer, col);
		//SDL_RenderDrawRect(config.mRenderer, &config.title_area);
		//SDL_SetRenderDrawColor(config.mRenderer, { 0, 0xFF, 0, 0xFF });
		//SDL_RenderDrawRect(config.mRenderer, &config.main_area);
		SDL_SetRenderDrawColor(config.mRenderer, { 0, 0, 0xFF, 0xFF });
		SDL_RenderDrawRect(config.mRenderer, &config.menu_area);
		//SDL_SetRenderDrawColor(config.mRenderer, { 0xFF, 0, 0xFF, 0xFF });
		//SDL_RenderDrawRect(config.mRenderer, &config.status_area);

		SDL_SetRenderDrawColor(config.mRenderer, { 0, 0xFF, 0, 0xFF });
		SDL_RenderDrawRect(config.mRenderer, &config.menu_buttons.back);
		SDL_RenderDrawRect(config.mRenderer, &config.menu_buttons.home);
		SDL_SetRenderDrawColor(config.mRenderer, { 0xFF, 0, 0, 0xFF });
		for (auto& b : config.menu_buttons) {
			SDL_RenderDrawRect(config.mRenderer, &b);
		}
	}
	*/
	/*
	SDL_SetRenderDrawColor(config.mRenderer, { 0, 0xFF, 0, 0xFF });
	SDL_RenderDrawRect(config.mRenderer, &config.menu_buttons.back);
	SDL_RenderDrawRect(config.mRenderer, &config.menu_buttons.home);
	SDL_SetRenderDrawColor(config.mRenderer, { 0xFF, 0, 0, 0xFF });
	for (auto& b : config.menu_buttons.buttons) {
		SDL_RenderDrawRect(config.mRenderer, &b);
	}
	SDL_SetRenderDrawColor(config.mRenderer, { 0, 0, 0xFF, 0xFF });
	SDL_RenderDrawRect(config.mRenderer, &config.menu_buttons.prev);
	SDL_RenderDrawRect(config.mRenderer, &config.menu_buttons.next);
	*/
}


void get_page_clock(shared_ptr<Page>& page) {
	page = make_shared<PageClock>();
}
