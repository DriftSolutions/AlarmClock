// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"
#include <assert.h>

/*
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
};
*/

void SDL_StatusBarSection::SetSizingPercent(double per, int min_pixels_wide) {
	if (per >= 0 && per <= 100) {
		memset(&sizing, 0, sizeof(sizing));
		sizing.is_percent = true;
		sizing.percent = per / 100;
		sizing.min_width = max(0, min_pixels_wide);
	}
}

void SDL_StatusBarSection::SetSizingFixed(int width) {
	if (width >= 0) {
		memset(&sizing, 0, sizeof(sizing));
		sizing.px = width;
	}
}

void SDL_StatusBarSection::SetText(const string& str) {
	if (str != text) {
		_text = str;
		clearCachedTexture();
	}
}
void SDL_StatusBarSection::Draw() {
	SDL_SetRenderDrawColor(config.mRenderer, colors.menu_normal_bg.r, colors.menu_normal_bg.g, colors.menu_normal_bg.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(config.mRenderer, &rc);
	SDL_SetRenderDrawColor(config.mRenderer, colors.menu_frame.r, colors.menu_frame.g, colors.menu_frame.b, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(config.mRenderer, &rc);
	/*
	SDL_SetRenderDrawColor(config.mRenderer, 0x33, 0x33, 0x33, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(config.mRenderer, rc.x, rc.y, rc.x + rc.w, rc.y);
	SDL_RenderDrawLine(config.mRenderer, rc.x, rc.y, rc.x, rc.y + rc.h);
	SDL_SetRenderDrawColor(config.mRenderer, 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(config.mRenderer, rc.x + 1, rc.y + rc.h, rc.x + rc.w - 1, rc.y + rc.h);
	SDL_RenderDrawLine(config.mRenderer, rc.x + rc.w, rc.y + 1, rc.x + rc.w, rc.y + rc.h - 1);
	*/

	if (progress_current && progress_max) {
		double per = double(progress_current) / double(progress_max);
		per = clamp<double>(per, 0, 1);
		SDL_Rect prc = {
			rc.x + 3, rc.y + 3,
			int(per * double(rc.w - 5)), rc.h - 5
		};
		SDL_SetRenderDrawColor(config.mRenderer, colors.menu_highlight_bg.r, colors.menu_highlight_bg.g, colors.menu_highlight_bg.b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(config.mRenderer, &prc);
	}

	if (text.empty()) { return; }

	SDL_Rect clip_rc = { rc.x + 3, rc.y + 2, rc.w - 6, rc.h - 4 };

	if (tex == NULL) {
		int w = INT_MAX;
		int ptsize = STATUS_FONT_SIZE;
		do {
			if (tex != NULL) {
				SDL_DestroyTexture(tex);
				tex = NULL;
			}
			auto font = GetFontSize(ptsize);
			auto src = TTF_RenderUTF8_Blended(font, text.c_str(), colors.menu_normal_text);
			if (src != NULL) {
				w = src->w;
				tex = SDL_CreateTextureFromSurface(config.mRenderer, src);
				tex_size = { src->w, src->h };
				SDL_FreeSurface(src);
				//TTF_SetFontSize
			}
		} while (w > clip_rc.w && ptsize-- > 10);
	}

	if (tex != NULL) {
		SDL_Rect orig_clip;
		SDL_RenderGetClipRect(config.mRenderer, &orig_clip);
		SDL_RenderSetClipRect(config.mRenderer, &clip_rc);

		SDL_Rect trc = { 0, 0, tex_size.w, tex_size.h };
		SDL_Rect drc;
		if (align == SBA_LEFT) {
			drc = { clip_rc.x, clip_rc.y, trc.w, trc.h };
		} else if (align == SBA_RIGHT) {
			drc = { clip_rc.x + clip_rc.w - trc.w, clip_rc.y, trc.w, trc.h };
		} else {
			drc = { clip_rc.x + ((clip_rc.w - trc.w) / 2), clip_rc.y, trc.w, trc.h };
		}
		drc.y = clip_rc.y + 1 + ((clip_rc.h - trc.h) / 2);
		SDL_RenderCopy(config.mRenderer, tex, &trc, &drc);

		if (orig_clip.w || orig_clip.h) {
			SDL_RenderSetClipRect(config.mRenderer, &orig_clip);
		} else {
			SDL_RenderSetClipRect(config.mRenderer, NULL);
		}
	}

}

int SDL_StatusBar::AddSection(shared_ptr<SDL_StatusBarSection>& sec) {
	sec = make_shared< SDL_StatusBarSection>();
	_sections.push_back(sec);
	return int(_sections.size()) - 1;
}
int SDL_StatusBar::AddSection() {
	shared_ptr<SDL_StatusBarSection> sec;
	return AddSection(sec);
}
bool SDL_StatusBar::GetSection(int ind, shared_ptr<SDL_StatusBarSection>& sec) {
	if (ind >= 0 && ind < _sections.size()) {
		sec = _sections[ind];
		return true;
	}
	return false;
}

bool SDL_StatusBar::SetSectionText(int ind, const string& str) {
	if (ind >= 0 && ind < _sections.size()) {
		_sections[ind]->SetText(str);
		return true;
	}
	return false;
}

void SDL_StatusBar::Draw() {
	SDL_SetRenderDrawColor(config.mRenderer, colors.menu_inactive_bg.r, colors.menu_inactive_bg.g, colors.menu_inactive_bg.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(config.mRenderer, &rc);
	for (auto& s : _sections) {
		s->Draw();
	}
}

bool SDL_StatusBar::GetStatusText(const SDL_Point& pt, string& str) {
	for (auto& s : _sections) {
		if (SDL_PointInRect(&pt, &s->rc)) {
			if (!s->status_text.empty()) {
				str = s->status_text;
				return true;
			}
			return false;
		}
	}
	return false;
}

void SDL_StatusBar::SetRect(const SDL_Rect& prc) {
	_rc = prc;
	CalcSections();
};

void SDL_StatusBar::CalcSections() {
	SDL_Rect usable = {
		rc.x + 3,
		rc.y + 3,
		rc.w - 6 - (3 * (int(sections.size()) - 1)),
		rc.h - 6 - 1
	};
	int curx = usable.x;
	int avail_for_percents = usable.w;
	double total_per = 0.0;
	// first pass, fixed-size sections
	for (auto& s : _sections) {
		if (!s->sizing.is_percent) {
			avail_for_percents -= s->sizing.px;
		} else {
			total_per += s->sizing.percent;
		}
	}
	assert(total_per <= 100);

	// second pass, assign sizes
	int left = usable.w;
	for (auto& s : _sections) {
		SDL_Rect src = { curx, usable.y, 0, usable.h };
		if (s->sizing.is_percent) {
			src.w = int(floor(double(avail_for_percents) * s->sizing.percent));
			left -= rc.w;
		} else {
			src.w = s->sizing.px;
			left -= rc.w;
		}
		s->rc = src;
		curx += src.w + 3;
	}
}

void SDL_StatusBar::Reset() {
	_rc = { 0,0 };
	_sections.clear();
}
