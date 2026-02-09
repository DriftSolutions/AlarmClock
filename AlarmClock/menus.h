// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

typedef void(*menu_item_clicked)(void* uData);

enum MENU_CLICK_RETURN {
	MCR_NO_MATCH, // coordinates didn't match a button
	MCR_CLOCK, // Close the menu and go back to the clock display
	MCR_HOME, // Return to the main menu
	MCR_BACK, // Return to the last menu
	MCR_PREV_PAGE,
	MCR_NEXT_PAGE,
	MCR_DO_NOTHING
};

class MenuItem {
private:
	SDL_Texture* img_tex = NULL;
	SDL_Rect img_rc = { 0 };
	Uint64 img_lastTry = 0;
	int line_skip = -1;
public:
	string text;
	string footer;
	string image;
	bool enabled = true;
	//bool selected = false;
	//void* uData = NULL;
	SDL_Rect rc = { 0 };

	MENU_CLICK_RETURN on_press_return = MCR_DO_NOTHING;
	virtual MENU_CLICK_RETURN OnPress() {
		return on_press_return;
	}

	virtual void OnActivate() {}

	virtual void getColors(SDL_Color& bgcol, SDL_Color& fgcol) {} // the default will be the stock background/foreground colors, you don't need to do anything to keep them

	virtual void Draw();
};

class Menu {
public:
	string title;
	vector<shared_ptr<MenuItem>> items;
	size_t first_ind = 0;

	virtual void Draw() {
		for (size_t i = 0; i < NUM_BUTTONS && (i + first_ind) < items.size(); i++) {
			auto& itm = items[i + first_ind];
			itm->Draw();
		}
	}

	virtual void OnActivate() {
		for (auto& itm : items) {
			itm->OnActivate();
		}
	}

	virtual void RecalcRects();

	virtual void PrevPage() {
		if (first_ind >= NUM_BUTTONS) {
			first_ind -= NUM_BUTTONS;
			RecalcRects();
		}
	}

	virtual void NextPage() {
		if (first_ind + NUM_BUTTONS < items.size()) {
			first_ind += NUM_BUTTONS;
			RecalcRects();
		}
	}

	MENU_CLICK_RETURN OnClick(const SDL_Point& pt, MENU_CLICK_RETURN def = MCR_NO_MATCH);

	virtual uint64 GetMenuTimeout();
};

/*
bool init_menus();
void reload_menu_images();
void quit_menus();
void draw_menus();
void menu_on_click(int x, int y, int button);
MENU_ITEM* menu_get_item_at(const SDL_Point& pt);
bool menu_keypress(const SDL_KeyCode key, MENU_KEYPRESS_MOD mod);
MENU_ITEM* add_menu_item(MENU* m, const string& title, menu_item_clicked cb, void* cbData = NULL, menu_item_is_sel is_selected = NULL);
MENU_ITEM* add_menu_item(MENU* m, const char8_t* title, menu_item_clicked cb, void* cbData = NULL, menu_item_is_sel is_selected = NULL);
void calc_menu_items(MENU* x);
void add_menu_about_lines(vector<string>& keys, vector<string>& funcs); #pragma once
*/

struct ALARM_TIME;

void switch_to_main_menu();
void switch_to_menu_set_alarm(ALARM_TIME* target, const string& title);
void switch_to_menu_set_alarm_dow();
