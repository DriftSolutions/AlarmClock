// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"
#include <list>

class PageMenu : public Page {
private:
	Uint64 lastActivity = SDL_GetTicks64();
public:
	list<shared_ptr<Menu>> stack;
	shared_ptr<Menu> menu;
	shared_ptr<MenuItem> back_button, prev_button, next_button;

	PageMenu(shared_ptr<Menu>& pmenu) {
		menu = pmenu;
	}

	void OnActivate();
	void OnClick(const SDL_Point& pt);
	void Tick();
	void Render();
};

void PageMenu::OnActivate() {
	lastActivity = SDL_GetTicks64();
	if (config.side_menu) {
		config.side_menu->items[0]->rc = config.menu_buttons.back;
		config.side_menu->items[1]->rc = config.menu_buttons.home;
		config.side_menu->items[2]->rc = config.menu_buttons.prev;
		config.side_menu->items[3]->rc = config.menu_buttons.next;

		back_button = config.side_menu->items[0];
		prev_button = config.side_menu->items[2];
		next_button = config.side_menu->items[3];
	}
}

string get_relative_day_string(const struct tm& target) {
	time_t ts = time(NULL);
	struct tm tm;
	localtime_r(&ts, &tm);
	if (tm.tm_year == target.tm_year && tm.tm_mon == target.tm_mon && tm.tm_mday == target.tm_mday) {
		return "Today";
	} else if (
		(tm.tm_year == target.tm_year && tm.tm_mon == target.tm_mon && tm.tm_mday + 1 == target.tm_mday) || // next day in same month
		(tm.tm_year == target.tm_year && tm.tm_mon + 1 == target.tm_mon && target.tm_mday == 1) || // first day of next month
		(tm.tm_year + 1 == target.tm_year && target.tm_mon == 0 && target.tm_mday == 1) // first day of next year
		) {
		return "Tomorrow";
	}
	char buf[16] = { 0 };
	strftime(buf, sizeof(buf), "%a", &tm);
	return buf;
};

void PageMenu::Tick() {
	if (menu && SDL_GetTicks64() - lastActivity > menu->GetMenuTimeout()) {
		switch_to_clock();
	}
	if (config.next_menu) {
		if (menu) {
			stack.push_back(menu);
		}
		menu = config.next_menu;
		menu->RecalcRects();
		config.next_menu.reset();
	}
	back_button->enabled = (stack.size() != 0);
	if (prev_button) {
		prev_button->enabled = (menu && menu->first_ind);
	}
	if (next_button) {
		next_button->enabled = (menu && menu->first_ind + NUM_BUTTONS < menu->items.size());
	}

	if (config.options.enable_alarm) {
		time_t ts = config.options.GetNextAlarmTime();
		if (ts > 0) {
			struct tm tm;
			localtime_r(&ts, &tm);
			statusbar_set(SB_NEXT_ALARM, mprintf("Next alarm: %s %s", get_relative_day_string(tm).c_str(), tm_to_str(tm).c_str()));
			//statusbar_set(SB_NEXT_ALARM, mprintf("Next alarm_chunk: %d/%d %s", tm.tm_mon + 1, tm.tm_mday, tm_to_str(tm).c_str()));
		} else {
			statusbar_set(SB_NEXT_ALARM, "No Times Set");
		}
	} else {
		statusbar_set(SB_NEXT_ALARM, "Alarm Off");
	}
}

void PageMenu::OnClick(const SDL_Point& pt) {
	lastActivity = SDL_GetTicks64();

	MENU_CLICK_RETURN ret = MCR_NO_MATCH;
	if (config.side_menu) {
		ret = config.side_menu->OnClick(pt);
	}
	if (ret == MCR_NO_MATCH && menu) {
		ret = menu->OnClick(pt);
	}

	if (ret == MCR_CLOCK) {
		switch_to_clock();
	} else if (ret == MCR_HOME) {
		switch_to_main_menu();
	} else if (ret == MCR_BACK) {
		if (stack.size()) {
			menu = *stack.rbegin();
			stack.pop_back();
		} else {
			switch_to_clock();
		}
	} else if (ret == MCR_PREV_PAGE && menu) {
		menu->PrevPage();
	} else if (ret == MCR_NEXT_PAGE && menu) {
		menu->NextPage();
	}
}

void PageMenu::Render() {
	SDL_SetRenderDrawColor(config.mRenderer, colors.menu_page_bg);
	SDL_RenderClear(config.mRenderer);

	if (config.side_menu) {
		config.side_menu->Draw();
	}
	if (menu) {
		draw_text(GetFontSize(TITLE_FONT_SIZE), config.title_area, DTA_CENTER | DTA_MIDDLE, colors.menu_normal_text, menu->title);
		menu->Draw();
	}
	status_bar.Draw();
}

void create_page_menu(shared_ptr<Page>& page, shared_ptr<Menu>& menu) {
	page = make_shared<PageMenu>(menu);
}

void switch_to_menu(shared_ptr<Menu>& menu) {
	bool is_current_page_menu = (config.cur_page.get() != NULL && dynamic_cast<PageMenu*>(config.cur_page.get()) != NULL);
	if (!is_current_page_menu) {
		shared_ptr<Page> p;
		create_page_menu(p, menu);
		SetNextPage(p);
	} else {
		config.next_menu = menu;
	}
}

void MenuItem::Draw() {
	int rad = 10;
	SDL_Color bg = enabled ? colors.menu_normal_bg : colors.menu_inactive_bg;
	SDL_Color fg = enabled ? colors.menu_normal_text : colors.menu_inactive_text;
	getColors(bg, fg);

	roundedBoxRGBA(config.mRenderer, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h, rad, bg.r, bg.g, bg.b, bg.a);

	if (line_skip <= 0) {
		auto ffont = GetFontSize(FOOTER_FONT_SIZE);
		line_skip = TTF_FontLineSkip(ffont);
	}

	if (!image.empty()) {
		if (img_tex == NULL && (img_lastTry == 0 || SDL_GetTicks64() - img_lastTry >= 10000)) {
			img_rc = { 0 };
			string fn = "resources/images/" + image;
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			auto src = IMG_Load(fn.c_str());
			if (src != NULL) {
				img_tex = SDL_CreateTextureFromSurface(config.mRenderer, src);
				if (img_tex != NULL) {
					img_rc = { 0, 0, src->w, src->h };
				}
				SDL_FreeSurface(src);
			} else {
				int x = 1;
			}
			img_lastTry = SDL_GetTicks64();
		}

		if (img_tex != NULL) {
			SDL_Rect padded = {
				rc.x + 5,
				rc.y + 5,
				rc.w - 10,
				rc.h - 10
			};
			auto rc2 = ScaleRectToFit(img_rc, padded);
			SDL_RenderCopy(config.mRenderer, img_tex, NULL, &rc2);
			return;
		}
	}
	/*
	{
		SDL_Color frame = bg;
		Uint8 amount = 0x10;
		Uint8* channels[3] = { &frame.r,&frame.g,&frame.b };
		for (auto& c : channels) {
			if (*c > amount) {
				*c -= amount;
			} else {
				*c = 0;
			}
		}
		if (!memcmp(&bg, &colors.menu_normal_bg, sizeof(SDL_Color))) {
			int x = 1;
		}
		//roundedRectangleRGBA(config.mRenderer, rc.x, rc.y, rc.x + rc.w, rc.y + rc.h, rad, frame.r, frame.g, frame.b, frame.a);
		roundedRectangleRGBA(config.mRenderer, rc.x-1, rc.y-1, rc.x + rc.w + 1, rc.y + rc.h + 1, rad, frame.r, frame.g, frame.b, frame.a);
	}
	*/

	//draw_text(GetFontSize(MENU_FONT_SIZE), rc, DTA_CENTER | DTA_MIDDLE, colors.menu_normal_text, text.c_str());
	if (footer.empty()) {
		draw_text_wrapped(MENU_FONT_SIZE, rc, DTA_CENTER | DTA_MIDDLE, fg, text);
	} else {
		SDL_Rect rc3 = {
			rc.x,
			rc.y + rc.h - line_skip - 2,
			rc.w,
			line_skip
		};
		thickLineRGBA(config.mRenderer, rc3.x, rc3.y, rc3.x + rc3.w, rc3.y, 2, colors.menu_frame.r, colors.menu_frame.g, colors.menu_frame.b, colors.menu_frame.a);
		draw_text_wrapped(GetFontSize(FOOTER_FONT_SIZE), rc, DTA_CENTER | DTA_BOTTOM, fg, footer);

		SDL_Rect rc2 = rc;
		rc2.h -= line_skip;
		draw_text_wrapped(MENU_FONT_SIZE, rc2, DTA_CENTER | DTA_MIDDLE, fg, text);
	}
}

void Menu::RecalcRects() {
	size_t btn_ind = 0;
	for (size_t i = 0; i < NUM_BUTTONS && (i + first_ind) < items.size(); i++) {
		auto& itm = items[i + first_ind];
		itm->rc = config.menu_buttons.buttons[btn_ind++];
	}
}

MENU_CLICK_RETURN Menu::OnClick(const SDL_Point& pt, MENU_CLICK_RETURN def) {
	
	for (size_t i = 0; i < NUM_BUTTONS && (i + first_ind) < items.size(); i++) {
		auto& itm = items[i + first_ind];
		if (itm->enabled && SDL_PointInRect(&pt, &itm->rc)) {
			return itm->OnPress();
		}
	}
	return def;
}

uint64 Menu::GetMenuTimeout() {
	return config.options.screen_timeout;
	// If there is no activity, it will return to the clock display after this long
}
