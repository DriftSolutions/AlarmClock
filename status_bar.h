// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#pragma once

enum STATUS_BAR_ALIGN {
	SBA_LEFT,
	SBA_CENTER,
	SBA_RIGHT
};

struct STATUS_BAR_SECTION_SIZE {
	union {
		int px;
		double percent;
	};
	bool is_percent;
	int min_width; // only honored if is_percent == true
};

class SDL_StatusBar;

class SDL_StatusBarSection {
private:
	SDL_Texture * tex = NULL;
	SDL_Size tex_size = { 0,0 };
	string _text;
protected:
	friend class SDL_StatusBar;
	SDL_Rect rc = { 0 };
	STATUS_BAR_SECTION_SIZE sizing = { 0 };

	inline void clearCachedTexture() {
		if (tex != NULL) {
			SDL_DestroyTexture(tex);
			tex = NULL;
		}
	}
public:
	//const STATUS_BAR_SECTION_SIZE& sizing = _sizing;
	STATUS_BAR_ALIGN align = SBA_LEFT;
	const string& text = _text;
	string status_text;
	size_t progress_max = 0;
	size_t progress_current = 0;

	void SetText(const string& str);
	void SetSizingPercent(double per, int min_pixels_wide = 0);
	void SetSizingFixed(int width);
	void Draw();

	~SDL_StatusBarSection() {
		clearCachedTexture();
	}
};

class SDL_StatusBar {
private:
	vector<shared_ptr<SDL_StatusBarSection>> _sections;
	SDL_Rect _rc = { 0 };
	SDL_StatusBarSection * getSection(int ind);
public:
	const SDL_Rect& rc = _rc;
	const vector<shared_ptr<SDL_StatusBarSection>>& sections = _sections;

	int AddSection(shared_ptr<SDL_StatusBarSection>& sec); // Adds a section returning, it's number index. Stores pointer in sec.
	int AddSection(); // Adds a section, returning it's number index.
	bool GetSection(int ind, shared_ptr<SDL_StatusBarSection>& sec); // Stores section pointer in sec if ind is in range.

	bool SetSectionText(int ind, const string& str);
	bool GetStatusText(const SDL_Point& pt, string& str);

	void Draw();
	void CalcSections(); // if you modify section sizing, call this or SetRect() to recalculate them
	void SetRect(const SDL_Rect& rc);
	void Reset();
	void ClearCachedTextures() {
		for (auto& x : _sections) {
			x->clearCachedTexture();
		}
	}
};

