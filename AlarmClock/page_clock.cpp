// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

class FlipClockDigit {
public:
	SDL_Rect rc = { 0 };
	shared_ptr<CachedText> txt = make_shared<CachedText>();
};

class FlipClock {
private:
	string str;
	vector<FlipClockDigit> digits;

	const int hor_padding = 30;
	const int vert_padding = 10;
	const int hor_spacing = 10;
	const int border_radius = 10;

	SDL_Color text_col = { 0 };
public:
	~FlipClock() {
		Reset();
	}

	void SetString(const string& pstr) {
		if (str != pstr) {
			Reset();
			str = pstr;
		}
	}

	void Render() {
		if (str.empty()) { return; }

		auto font = GetMonoFontSize(FLIP_TIME_FONT_SIZE);
		//str = "12:55p";

		digits.clear();
		if (digits.size() == 0) {
			SDL_Size max_size = { 0 };
			TTF_SizeUTF8(font, "X", &max_size.w, &max_size.h);

			const int cell_w = (max_size.w + (hor_padding * 2));
			const int total_w = (cell_w * (int)str.length()) + (hor_spacing * (int)(str.length() - 1));
			const int cell_h = (max_size.h + (vert_padding * 2));

			int cx = config.main_area.x + ((config.main_area.w - total_w) / 2);
			int cy = config.main_area.y + ((config.main_area.h - cell_h) / 2);

			for (size_t i = 0; i < str.length(); i++) {
				FlipClockDigit d;
				d.rc = d.txt->rc = { cx, cy, cell_w, cell_h };
				d.txt->align = DTA_CENTER | DTA_MIDDLE;
				d.txt->font_size = FLIP_TIME_FONT_SIZE;
				d.txt->col = text_col;
				d.txt->setText(str.substr(i, 1));

				digits.push_back(std::move(d));

				cx += cell_w + hor_spacing;
			}
		}

		int hinge_h = 23;
		int hinge_w = hor_padding / 2;

		SDL_SetRenderDrawColor(config.mRenderer, { 0xff,0,0,0xff });
		for (auto& d : digits) {
			const auto& rc = d.rc;
			const auto& border = text_col;
			//SDL_Rect rc = { config.main_area.x, config.main_area.y, total_w, cell_h };
			//SDL_RenderDrawRect(config.mRenderer, &d.rc);
			if (!config.is_dark) {
				const auto& bg = colors.menu_page_bg;
				roundedBoxRGBA(config.mRenderer, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h, border_radius, bg.r, bg.g, bg.b, bg.a);
			}
			roundedRectangleRGBA(config.mRenderer, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h, border_radius, border.r, border.g, border.b, border.a);
			roundedRectangleRGBA(config.mRenderer, rc.x, rc.y + 1, rc.x + rc.w, rc.y + rc.h + 1, border_radius, border.r, border.g, border.b, border.a);
			roundedRectangleRGBA(config.mRenderer, rc.x, rc.y - 1, rc.x + rc.w, rc.y + rc.h - 1, border_radius, border.r, border.g, border.b, border.a);

			rectangleRGBA(config.mRenderer, rc.x, rc.y + (rc.h / 2) - (hinge_h / 2), rc.x + hinge_w, rc.y + (rc.h / 2) + (hinge_h / 2), border.r, border.g, border.b, border.a);
			rectangleRGBA(config.mRenderer, rc.x + rc.w - hinge_w, rc.y + (rc.h / 2) - (hinge_h / 2), rc.x + rc.w + 1, rc.y + (rc.h / 2) + (hinge_h / 2), border.r, border.g, border.b, border.a);

			d.txt->Draw();

			thickLineRGBA(config.mRenderer, rc.x + hinge_w, rc.y + (rc.h / 2) - 1, rc.x + rc.w - hinge_w - 1, rc.y + (rc.h / 2) - 1, 3, colors.clock_bg.r, colors.clock_bg.g, colors.clock_bg.b, colors.clock_bg.a);
		}
	}

	void Reset() {
		digits.clear();
	}

	void setTextColor(const SDL_Color& col) {
		text_col = col;
		for (auto& d : digits) {
			d.txt->col = col;
			d.txt->clearCache();
		}
	}
};

class PageClock : public Page {
public:
	CachedText txtTime, txtDate, txtAlarm, txtWeather;
	FlipClock flip;

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
	txtTime.clearCache();
	txtDate.col = text_col;
	txtDate.clearCache();
	txtAlarm.col = text_col;
	txtAlarm.clearCache();
	txtWeather.col = text_col;
	txtWeather.clearCache();
	flip.setTextColor(text_col);
}

void PageClock::OnActivate() {
	txtTime.font_size = TIME_FONT_SIZE;
	txtTime.rc = { 0, 0, config.win_size.w, config.win_size.h }; // config.main_area;
	txtTime.align = DTA_CENTER | DTA_MIDDLE;

	txtDate.font_size = TITLE_FONT_SIZE;
	txtDate.rc = { 0, 0, config.win_size.w, config.win_size.h - 10 };
	txtDate.align = DTA_CENTER | DTA_BOTTOM;

	txtAlarm.font_size = TITLE_FONT_SIZE;
	txtAlarm.rc = config.title_area;
	txtAlarm.align = DTA_LEFT | DTA_MIDDLE;
	txtAlarm.setText("Alarm On");

	txtWeather.font_size = TITLE_FONT_SIZE;
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
		flip.SetString(tm_to_str(tm));
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
	} else if (config.options.flip_clock_style) {
		flip.Render();
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
