// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"

void SDL_SetRenderDrawColor(SDL_Renderer* r, const SDL_Color& col) {
	SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
}

void draw_text(TTF_Font* font, int x, int y, const SDL_Color& col, const char* str, int style) {
	if (font == NULL) {
		font = config.backup_font;
	}
	//SDL_Color col_white = { 255, 255, 255 };

	int old = TTF_GetFontStyle(font);
	TTF_SetFontStyle(font, style);
	//static wchar_t tmp[512];
	//mbstowcs(tmp, str, sizeof(tmp) / sizeof(wchar_t));
	SDL_Surface* src = TTF_RenderUTF8_Blended(font, str, col);
	//SDL_Surface * src = TTF_RenderUNICODE_Blended(font, (Uint16 *)tmp, col);
	TTF_SetFontStyle(font, old);
	if (src) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(config.mRenderer, src);
		if (tex) {

			SDL_Rect rc; //create a rect
			rc.x = x;  //controls the rect's x coordinate
			rc.y = y; // controls the rect's y coordinte
			rc.w = src->w; // controls the width of the rect
			rc.h = src->h; // controls the height of the rect

			SDL_RenderCopy(config.mRenderer, tex, NULL, &rc);

			SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(src);
	}
}

void draw_text(TTF_Font* font, int x, int y, const SDL_Color& col, const vector<string>& lines, int style) {
	if (font == NULL) {
		font = config.backup_font;
	}

	int cy = y;
	SDL_Point sz;
	for (size_t i = 0; i < lines.size(); i++) {
		auto& str = lines[i];
		TTF_SizeUTF8(font, str.c_str(), &sz.x, &sz.y);
		draw_text(font, x, cy, col, str.c_str(), style);
		cy += sz.y;
	}
}

void draw_text(TTF_Font* font, const SDL_Rect& rc, uint8 align, const SDL_Color& col, const string& str, int style) {
	if (font == NULL) {
		font = config.backup_font;
	}

	int old = TTF_GetFontStyle(font);
	TTF_SetFontStyle(font, style);

	//static wchar_t tmp[512];
	//mbstowcs(tmp, str, sizeof(tmp) / sizeof(wchar_t));
	/*
	if (style & TTF_STYLE_OUTLINE2) {
		TTF_SetFontOutline(font, 2);
	} else if (style & TTF_STYLE_OUTLINE) {
		TTF_SetFontOutline(font, 1);
	} else {
		TTF_SetFontOutline(font, 0);
	}
	*/
	SDL_Surface* src = TTF_RenderUTF8_Blended(font, str.c_str(), col);
	/*
	if (style & (TTF_STYLE_OUTLINE | TTF_STYLE_OUTLINE2)) {
		TTF_SetFontOutline(font, 0);
	}
	*/
	//SDL_Surface * src = TTF_RenderUNICODE_Blended(font, (Uint16 *)tmp, col);
	TTF_SetFontStyle(font, old);
	if (src) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(config.mRenderer, src);
		if (tex) {

			SDL_Rect rc2;
			rc2.y = rc.y;
			rc2.w = src->w;
			rc2.h = src->h;
			if (align & DTA_RIGHT) {
				rc2.x = rc.x + rc.w - src->w;
			} else if (align & DTA_CENTER) {
				rc2.y = rc.y;
				rc2.x = rc.x + ((rc.w - src->w) / 2);
			} else {
				rc2.x = rc.x;
			}
			if (align & DTA_MIDDLE) {
				rc2.y += (rc.h - src->h) / 2;
			} else if (align & DTA_BOTTOM) {
				rc2.y += rc.h - src->h - 1;
			}

			SDL_Rect clip = { 0,0,0,0 };
			SDL_RenderGetClipRect(config.mRenderer, &clip);
			SDL_RenderSetClipRect(config.mRenderer, &rc);
			SDL_RenderCopy(config.mRenderer, tex, NULL, &rc2);
			SDL_RenderSetClipRect(config.mRenderer, (clip.w || clip.h) ? &clip : NULL);

			SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(src);
	}
}

void draw_text_wrapped(TTF_Font* font, const SDL_Rect& rc, uint8 align, const SDL_Color& col, const string& str, int style) {
	if (font == NULL) {
		font = config.backup_font;
	}

	int old = TTF_GetFontStyle(font);
	TTF_SetFontStyle(font, style);
	int orig_wrap = TTF_GetFontWrappedAlign(font);
	if (align & DTA_CENTER) {
		TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
	} else if (align & DTA_RIGHT) {
		TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_RIGHT);
	} else {
		TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_LEFT);
	}
	SDL_Surface* src = TTF_RenderUTF8_Blended_Wrapped(font, str.c_str(), col, rc.w);
	TTF_SetFontWrappedAlign(font, orig_wrap);
	TTF_SetFontStyle(font, old);
	if (src) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(config.mRenderer, src);
		if (tex) {

			SDL_Rect rc2;
			rc2.y = rc.y;
			rc2.w = src->w;
			rc2.h = src->h;
			/*
			if (align & DTA_RIGHT) {
				rc2.x = rc.x + rc.w - src->w;
			} else if (align & DTA_CENTER) {
				rc2.y = rc.y;
				rc2.x = rc.x + ((rc.w - src->w) / 2);
			} else {
				rc2.x = rc.x;
			}
			*/
			rc2.x = rc.x;
			if (align & DTA_MIDDLE) {
				rc2.y += (rc.h - src->h) / 2;
			} else if (align & DTA_BOTTOM) {
				rc2.y += rc.h - src->h - 1;
			}

			SDL_Rect clip = { 0,0,0,0 };
			SDL_RenderGetClipRect(config.mRenderer, &clip);
			SDL_RenderSetClipRect(config.mRenderer, &rc);
			SDL_RenderCopy(config.mRenderer, tex, NULL, &rc2);
			SDL_RenderSetClipRect(config.mRenderer, (clip.w || clip.h) ? &clip : NULL);

			SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(src);
	}
}

void draw_text_wrapped(int ptSize, const SDL_Rect& rc, uint8 align, const SDL_Color& col, const string& str, int style) {
	int h = rc.h + 10;

	SDL_Surface* src = NULL;
	while (h > rc.h && ptSize > 10) {
		auto font = GetFontSize(ptSize);

		int old = TTF_GetFontStyle(font);
		TTF_SetFontStyle(font, style);
		int orig_wrap = TTF_GetFontWrappedAlign(font);
		if (align & DTA_CENTER) {
			TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_CENTER);
		} else if (align & DTA_RIGHT) {
			TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_RIGHT);
		} else {
			TTF_SetFontWrappedAlign(font, TTF_WRAPPED_ALIGN_LEFT);
		}

		if (src != NULL) {
			SDL_FreeSurface(src);
		}
		src = TTF_RenderUTF8_Blended_Wrapped(font, str.c_str(), col, rc.w);
		if (src != NULL) {
			h = src->h;
		}

		TTF_SetFontWrappedAlign(font, orig_wrap);
		TTF_SetFontStyle(font, old);
		ptSize -= 2;
	}

	if (src) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(config.mRenderer, src);
		if (tex) {

			SDL_Rect rc2;
			rc2.y = rc.y;
			rc2.w = src->w;
			rc2.h = src->h;
			/*
			if (align & DTA_RIGHT) {
				rc2.x = rc.x + rc.w - src->w;
			} else if (align & DTA_CENTER) {
				rc2.y = rc.y;
				rc2.x = rc.x + ((rc.w - src->w) / 2);
			} else {
				rc2.x = rc.x;
			}
			*/
			rc2.x = rc.x;
			if (align & DTA_MIDDLE) {
				rc2.y += (rc.h - src->h) / 2;
			} else if (align & DTA_BOTTOM) {
				rc2.y += rc.h - src->h - 1;
			}

			SDL_Rect clip = { 0,0,0,0 };
			SDL_RenderGetClipRect(config.mRenderer, &clip);
			SDL_RenderSetClipRect(config.mRenderer, &rc);
			SDL_RenderCopy(config.mRenderer, tex, NULL, &rc2);
			SDL_RenderSetClipRect(config.mRenderer, (clip.w || clip.h) ? &clip : NULL);

			SDL_DestroyTexture(tex);
		}
		SDL_FreeSurface(src);
	}
}

void CachedText::Draw() {
	draw_text(font, rc, align, col, str, style);
}

string ts_to_str(time_t ts) {
	struct tm tm;
	localtime_r(&ts, &tm);
	return tm_to_str(tm);
}

string tm_to_str(const tm& tm) {
	string hour, min, post;
	if (tm.tm_hour == 0) {
		hour = "12";
		post = "a";
	} else if (tm.tm_hour < 12) {
		hour = mprintf("%d", tm.tm_hour);
		post = "a";
	} else if (tm.tm_hour == 12) {
		hour = "12";
		post = "p";
	} else {
		hour = mprintf("%d", tm.tm_hour - 12);
		post = "p";
	}
	return mprintf("%s:%02d%s", hour.c_str(), tm.tm_min, post.c_str());
}

string FormatMinutes(int64 secs) {
	bool is_neg = (secs < 0);
	if (is_neg) {
		secs *= -1;
	}
	int64 days = secs / 86400;
	if (days) {
		secs -= days * 86400;
	}
	int64 hours = secs / 3600;
	if (hours) {
		secs -= hours * 3600;
	}
	int64 mins = secs / 60;
	if (mins) {
		secs -= mins * 60;
	}
	if (secs >= 30) {
		mins++;
	}

	stringstream sstr;
	if (is_neg) {
		sstr << "-";
	}
	if (days) {
		sstr << days << "d ";
	}
	if (hours) {
		sstr << hours << "h ";
	}
	sstr << mins << "m";
	return sstr.str();
}

bool backup_file(const string& fn) {
	if (access(fn.c_str(), 0) == 0) {
		time_t ts = time(NULL);
		struct tm tm;
		localtime_r(&ts, &tm);
		int ind = 0;
		string base = cfg->GetDataDirFile("backups");
		if (access(base.c_str(), 0) != 0) {
			dsl_mkdir(base.c_str(), 0700);
		}
		base += "/";
		base += nopathA(fn.c_str());
		string bfn = mprintf("%s.%04d%02d%02d%02d", base.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, ind++);
		while (access(bfn.c_str(), 0) == 0) {
			bfn = mprintf("%s.%04d%02d%02d%02d", base.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, ind++);
		}
		return copy_file(fn, bfn);
	}
	return true;
}

SDL_Rect ScaleRectToFit(const SDL_Rect& source, const SDL_Rect& container) {
	SDL_Rect result;

	// Calculate aspect ratios
	float sourceAspect = static_cast<float>(source.w) / static_cast<float>(source.h);
	float containerAspect = static_cast<float>(container.w) / static_cast<float>(container.h);

	// Determine which dimension is the limiting factor
	if (sourceAspect > containerAspect) {
		// Source is wider - fit to width
		result.w = container.w;
		result.h = static_cast<int>(container.w / sourceAspect);
	} else {
		// Source is taller - fit to height
		result.h = container.h;
		result.w = static_cast<int>(container.h * sourceAspect);
	}

	// Center within container
	result.x = container.x + (container.w - result.w) / 2;
	result.y = container.y + (container.h - result.h) / 2;

	return result;
}
