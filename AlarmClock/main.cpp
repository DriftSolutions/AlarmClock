// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"
extern "C" {
    #include "sunriset.h"
}

#ifdef WIN32
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2_image.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_mixer.lib")
#ifdef DEBUG
#pragma comment(lib, "SDL2_gfx_d.lib")
#else
#pragma comment(lib, "SDL2_gfx.lib")
#endif
#endif

shared_ptr<AlarmConfig> cfg;
CONFIG config;
SDL_StatusBar status_bar;
shared_ptr<Page> page_clock;

// SDL2_mixer globals
Mix_Chunk* alarm_chunk = NULL;
int alarm_channel = -1;

const CONFIG_COLORS colors = {
    /* clock bg color */
    { 0x00, 0x00, 0x00, 0xFF },
    /* clock regular text */
    { 200, 240, 255, 0xFF },
    /* clock red text */
#ifdef WIN32
    { 0x60, 0, 0, 0xFF },
#else
    { 0xEE, 0, 0, 0xFF },
#endif

    /* menu page bg color */
    { 0x21, 0x28, 0x30, 0xFF },
    /* menu page text color */
    { 0xEE, 0xEE, 0xEE, 0xFF },

    /* normal */
    { 0x78, 0x88, 0x98, 0xFF }, // { 0x99, 0x99, 0x99, 0x99 }, // { 0x2E, 0x2E, 0x2E, 0xFF },
    { 0xEE, 0xEE, 0xEE, 0xFF },

    /* highlight */
    { 0x00, 0x80, 0xFF, 0xFF },
    { 0xEE, 0xEE, 0xEE, 0xFF },

    /* off */
    { 0xFF, 0x00, 0x00, 0x80 },
    { 0xEE, 0xEE, 0xEE, 0xFF },

    /* on */
    { 0x00, 0xC0, 0x00, 0x80 },
    { 0xEE, 0xEE, 0xEE, 0xFF },

    /* inactive */
    { 0x20, 0x20, 0x20, 0xFF },
    { 0x80, 0x80, 0x80, 0xFF },

    /* frame */
    { 0x42, 0x42, 0x42, 0xFF },
};

map<int, TTF_Font *> fonts;
TTF_Font* GetFontSize(int ptSize) {
    auto x = fonts.find(ptSize);
    if (x != fonts.end()) {
        return x->second;
    }

    auto font = TTF_OpenFont("resources/roboto.ttf", ptSize);
    if (font == NULL) {
        printf("Error opening font with point size %d: %s\n", ptSize, TTF_GetError());
        return NULL;
    }

    fonts[ptSize] = font;
    return font;
}

map<int, TTF_Font*> mono_fonts;
TTF_Font* GetMonoFontSize(int ptSize) {
    auto x = mono_fonts.find(ptSize);
    if (x != mono_fonts.end()) {
        return x->second;
    }

    auto font = TTF_OpenFont("resources/roboto_mono.ttf", ptSize);
    if (font == NULL) {
        printf("Error opening font with point size %d: %s\n", ptSize, TTF_GetError());
        return NULL;
    }

    mono_fonts[ptSize] = font;
    return font;
}

void shutdown(int err = 0) {
    printf("Shutting down...\n");
    config.shutdown_now = true;

    while (DSL_NumThreads()) {
        safe_sleep(1);
    }

    if (Mix_Init(0)) {
        Mix_HaltChannel(-1);
        if (alarm_chunk != NULL) {
            Mix_FreeChunk(alarm_chunk);
            alarm_chunk = NULL;
        }

        Mix_CloseAudio();
        Mix_Quit();
    }

    status_bar.Reset();

    for (auto& f : fonts) {
        TTF_CloseFont(f.second);
    }
    fonts.clear();
    for (auto& f : mono_fonts) {
        TTF_CloseFont(f.second);
    }
    mono_fonts.clear();
    if (config.backup_font != NULL) {
        TTF_CloseFont(config.backup_font);
        config.backup_font = NULL;
    }

    if (TTF_WasInit()) {
        TTF_Quit();
    }

    if (config.mWnd != NULL) {
        SDL_DestroyWindow(config.mWnd);
        config.mWnd = NULL;
    }
    if (config.mRenderer != NULL) {
        SDL_DestroyRenderer(config.mRenderer);
        config.mRenderer = NULL;
    }

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Quit();
    }

    cfg.reset();

    exit(err);
}

void fatal_error(const string& msg, int err = 1) {
    printf("%s\n", msg.c_str());
#ifdef WIN32
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), config.mWnd);
#endif
    shutdown(err);
}

void on_resize();

bool init() {
    cfg->ReadConfigFile();

    config.use_sun = cfg->GetBoolArg("-use_sun", false);
    config.lcd_brightness_fn = cfg->GetArg("-lcd_brightness_fn", "/sys/class/backlight/11-0045/brightness");
    config.home_assistant.url = cfg->GetArg("-home_assistant_url");
    config.home_assistant.token = cfg->GetArg("-home_assistant_token");
    config.home_assistant.alarm_entity = cfg->GetArg("-home_assistant_alarming");
    config.home_assistant.alarm_enabled_entity = cfg->GetArg("-home_assistant_alarm_enabled");
    config.home_assistant.next_alarm_entity = cfg->GetArg("-home_assistant_next_alarm");

    load_dynamic_settings();

    //Initialize SDL
    printf("Init SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fatal_error(mprintf("SDL could not initialize! SDL Error: %s\n", SDL_GetError()));
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    printf("Init SDL_mixer...\n");
    if (Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_FLAC | MIX_INIT_OPUS) == 0) {
        fatal_error(mprintf("SDL_mixer could not initialize! Error: %s\n", Mix_GetError()));
    }

    printf("Opening sound device...\n");
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fatal_error(mprintf("Error opening audio! Error: %s\n", Mix_GetError()));
    }

    printf("Creating window...\n");
    config.win_size = {
        clamp<int>((int)cfg->GetIntArg("-width", WINDOW_WIDTH), 200, 4096),
        clamp<int>((int)cfg->GetIntArg("-height", WINDOW_HEIGHT), 200, 4096)
    };
    config.fullscreen = cfg->GetBoolArg("-fullscreen", true);

    //Create window
    config.mWnd = SDL_CreateWindow("Drift Alarm Clock", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.win_size.w, config.win_size.h, SDL_WINDOW_ALLOW_HIGHDPI | (config.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (config.mWnd == NULL) {
        fatal_error(mprintf("Window could not be created! SDL Error: %s\n", SDL_GetError()));
    }

    //SDL_SetWindowMinimumSize(config.mWnd, 400, 400);
    if (!config.fullscreen) {
        if (config.normal_win_position.x != INT_MAX && config.normal_win_position.y != INT_MAX) {
            SDL_SetWindowPosition(config.mWnd, config.normal_win_position.x, config.normal_win_position.y);
        }
    }

    config.mRenderer = SDL_CreateRenderer(config.mWnd, -1, SDL_RENDERER_ACCELERATED);
    if (config.mRenderer == NULL) {
        fatal_error(mprintf("Renderer could not be created! SDL Error: %s\n", SDL_GetError()));
    }

#ifndef WIN32
    SDL_ShowCursor(SDL_FALSE);
#endif

    /*
#ifdef WIN32
    int i;
    if ((i = SDL_GetWindowDisplayIndex(config.mWnd)) < 0) {
        i = 0;
    }
    float dpi_w, dpi_h;
    if (SDL_GetDisplayDPI(i, NULL, &dpi_w, &dpi_h) != 0 || dpi_w <= 0 || dpi_h <= 0) {
        dpi_w = dpi_h = 96;
    }
    //config.win_size.w, config.win_size.h
    int adj_w = int(dpi_w * 4.76f);
    int adj_h = int(dpi_h * 3.75f);
    SDL_SetWindowSize(config.mWnd, adj_w, adj_h);
    SDL_RenderSetLogicalSize(config.mRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif
*/

    printf("Init SDL_ttf...\n");
    if (TTF_Init() != 0) {
        fatal_error(mprintf("Error initializing SDL_ttf! TTF Error: %s\n", TTF_GetError()));
    }

    config.backup_font = TTF_OpenFont("resources/roboto.ttf", 14);
    if (config.backup_font == NULL) {
        fatal_error(mprintf("Error loading backup font! TTF Error: %s\n", TTF_GetError()));
    }

    {
        shared_ptr<SDL_StatusBarSection> sec;
        /*
        status_bar.AddSection(sec);
        sec->align = SBA_LEFT;
        sec->SetSizingPercent(100);
        */

        int posw, posh;
        TTF_SizeUTF8(GetFontSize(STATUS_FONT_SIZE), "FPS: 1000", &posw, &posh);

        status_bar.AddSection(sec);
        sec->align = SBA_LEFT;
        sec->SetSizingPercent(33);

        //shared_ptr<SDL_StatusBarSection> sec;
        //SB_MPOS
        status_bar.AddSection(sec);
        sec->align = SBA_LEFT;
        sec->SetSizingPercent(33);

        status_bar.AddSection(sec);
        sec->align = SBA_LEFT;
        sec->SetSizingPercent(33);

        /*
        //SB_GPOS
        status_bar.AddSection(sec);
        sec->align = SBA_CENTER;
        sec->SetSizingFixed(posw);
        sec->status_text = "Grid Position in Paper Units";

        //SB_PPOS
        status_bar.AddSection(sec);
        sec->align = SBA_CENTER;
        sec->SetSizingFixed(posw);
        sec->status_text = "Physical Mouse Position in Paper";

        //SB_STATUS
        status_bar.AddSection(sec);
        sec->align = SBA_LEFT;
        sec->SetSizingPercent(65);
        sec->status_text = "Status Message(s)";

        //SB_SELECTED
        status_bar.AddSection(sec);
        sec->align = SBA_LEFT;
        sec->SetSizingPercent(35, max(posw, 200));
        sec->status_text = "Selected Object(s) Info";
        */

#ifdef DEBUG_FPS
        //SB_FPS
        status_bar.AddSection(sec);
        sec->align = SBA_CENTER;
        sec->SetSizingFixed(posw);
#endif

        if (!config.home_assistant.isValid()) {            
            statusbar_set(SB_HOME_ASSISTANT_RECV, "HA: Disabled");
        }
        if (!config.home_assistant.needToSendUpdates()) {
            statusbar_set(SB_HOME_ASSISTANT_SEND, "HA: Disabled");
        }
    }

    {
        shared_ptr<Menu> m = make_shared<Menu>();
        m->title = "Sidebar Menu";

        shared_ptr<MenuItem> i = make_shared<MenuItem>();
        i->rc = config.menu_buttons.back;
        i->text = "Back";
        i->image = "back.png";
        i->on_press_return = MCR_BACK;
        m->items.push_back(i);

        i = make_shared<MenuItem>();
        i->rc = config.menu_buttons.home;
        i->text = "Clock";
        i->image = "clock.png";
        i->on_press_return = MCR_CLOCK;
        m->items.push_back(i);

        i = make_shared<MenuItem>();
        i->rc = config.menu_buttons.prev;
        i->text = "Prev";
        i->image = "up.png";
        i->on_press_return = MCR_PREV_PAGE;
        m->items.push_back(i);

        i = make_shared<MenuItem>();
        i->rc = config.menu_buttons.next;
        i->text = "Next";
        i->image = "down.png";
        i->on_press_return = MCR_NEXT_PAGE;
        m->items.push_back(i);

        config.side_menu = m;
    }

    on_resize();

    if (config.home_assistant.isValid()) {
        start_home_assistant();
    }

    return true;
}

void load_dynamic_settings() {
    string data;
    if (!file_get_contents(cfg->GetDataDirFile("settings.json"), data)) {
        return;
    }
    UniValue obj(UniValue::VOBJ);
    if (!obj.read(data)) {
        return;
    }

    config.options.FromUniValue(obj);
}

void save_dynamic_settings() {
    UniValue obj(UniValue::VOBJ);
    config.options.ToUniValue(obj);

    string fn = cfg->GetDataDirFile("settings.json");
    
    if (backup_file(fn)) {
        file_put_contents(fn, obj.write(1));
    }
}


#define BUTTON_SPACE_HOR 10
#define BUTTON_SPACE_VERT 10

void on_resize() {
    int ow, oh;
    SDL_GetWindowSize(config.mWnd, &ow, &oh);
    const int w = config.win_size.w = ow;
    const int h = config.win_size.h = oh;

    if (!config.fullscreen) {
        SDL_GetWindowPosition(config.mWnd, &config.normal_win_position.x, &config.normal_win_position.y);
    }

    //int txt_w = 0, txt_h = 0;
    //TTF_SizeUTF8(GetFontSize(TITLE_FONT_SIZE), "AYQGayqg|0", &txt_w, &txt_h);
    int txt_h = TTF_FontLineSkip(GetFontSize(TITLE_FONT_SIZE));
    config.title_area.x = 10;
    config.title_area.y = 0;
    config.title_area.w = w - 20;
    config.title_area.h = txt_h + 10;

    txt_h = TTF_FontLineSkip(GetFontSize(STATUS_FONT_SIZE));
    config.status_area.x = 0;
    config.status_area.w = w;
    config.status_area.h = txt_h + 12;
    config.status_area.y = h - config.status_area.h;
    status_bar.SetRect(config.status_area);

    config.main_area = { BUTTON_SPACE_HOR, config.title_area.y + config.title_area.h, w - (BUTTON_SPACE_HOR * 2), config.status_area.y - (config.title_area.y + config.title_area.h) };

    const int button_row_spacing = BUTTON_SPACE_VERT;
    const int button_col_spacing = BUTTON_SPACE_HOR;
    const int buttons_per_row = BUTTONS_PER_ROW;
    const int buttons_per_col = BUTTONS_PER_COL;
    int button_w = (config.main_area.w - (button_col_spacing * (buttons_per_row - 0))) / buttons_per_row;
    int button_h = (config.main_area.h - (button_row_spacing * buttons_per_col)) / buttons_per_col;

    config.menu_buttons.back = { config.main_area.x, config.main_area.y, button_w / 2, button_h };
    config.menu_buttons.home = { config.main_area.x, config.main_area.y + button_h + button_row_spacing, button_w / 2, button_h };
    config.menu_buttons.prev = { config.main_area.x + config.main_area.w - (button_w / 2), config.main_area.y, button_w / 2, button_h };
    config.menu_buttons.next = { config.main_area.x + config.main_area.w - (button_w / 2), config.main_area.y + button_h + button_row_spacing, button_w / 2, button_h };

    int start_x = config.main_area.x + (button_w / 2);
    int cx = start_x;
    int cy = 0;
    size_t btn_ind = 0;
    for (size_t i = 0; i < (BUTTONS_PER_ROW - 1) * BUTTONS_PER_COL; i++) {
        SDL_Rect btn;

        if (i > 0 && i % (buttons_per_row - 1) == 0) {
            cy += button_h + button_row_spacing;
            cx = start_x;
        }

        btn = {
            config.main_area.x + cx,
            config.main_area.y + cy,
            button_w,
            button_h
        };
        cx += button_w + button_col_spacing;
        
        config.menu_buttons.buttons[btn_ind++] = btn;
    }

    config.menu_area = {
        config.menu_buttons.buttons[0].x,
        config.menu_buttons.buttons[0].y,
        config.menu_buttons.buttons[NUM_BUTTONS - 1].x + config.menu_buttons.buttons[NUM_BUTTONS - 1].w - config.menu_buttons.buttons[0].x,
        config.menu_buttons.buttons[NUM_BUTTONS - 1].y + config.menu_buttons.buttons[NUM_BUTTONS - 1].h - config.menu_buttons.buttons[0].y,
    };
}

void render() {
    auto r = config.mRenderer;
    SDL_SetRenderDrawColor(r, colors.clock_bg);
    SDL_RenderClear(r);

    config.cur_page->Render();

    SDL_RenderPresent(config.mRenderer);
}

void update_mouse_position(int x, int y) {
    config.mpos.x = x;
    config.mpos.y = y;
}

void handle_keypresses(const SDL_Event& event) {
    if (event.key.keysym.sym == SDLK_ESCAPE) {
        config.shutdown_now = true;
    } else if (event.key.keysym.sym == SDLK_RETURN) {
        config.ClearAlarm();
    }
}

void set_fps_target(Uint32 fps) {
    config.fps = fps;
    config.fps_target = 1000 / fps;
}

void SetNextPage(shared_ptr<Page>& p) {
    if (config.cur_page) {
        config.next_page = p;
    } else {
        config.next_page.reset();
        config.cur_page = p;
        p->OnActivate();
    }
}

void SetPage(shared_ptr<Page> p) {
    config.next_page.reset();
    config.cur_page = p;
    p->OnActivate();
}

void get_page_clock(shared_ptr<Page>& page);
void switch_to_clock() {
    if (page_clock.get() == NULL) {
        get_page_clock(page_clock);
    }
    SetNextPage(page_clock);
}

bool GetAlarmFile(string& fn) {
    static vector<string> alarm_files;

    if (alarm_files.size() == 0) {
        int supported = Mix_Init(0);
        set<string> exts = { "wav" };
        if (supported & MIX_INIT_OGG) {
            exts.insert("ogg");
        }
        if (supported & MIX_INIT_MP3) {
            exts.insert("mp3");
        }
        if (supported & MIX_INIT_FLAC) {
            exts.insert("flac");
        }
        if (supported & MIX_INIT_OPUS) {
            exts.insert("opus");
        }
        Directory dir;
        dir.Open("resources/alarms");
        char buf[MAX_PATH];
        bool is_dir;
        while (dir.Read(buf, sizeof(buf), &is_dir)) {
            if (buf[0] == '.' || is_dir) { continue; }
            char* ext = strrchr(buf, '.');
            if (ext == NULL) { continue; }

            char* tmp = strdup(ext + 1);
            strlwr(tmp);
            auto x = exts.find(tmp);
            free(tmp);
            if (x == exts.end()) {
                continue;
            }

            string ffn = "resources/alarms" PATH_SEPS "";
            ffn += buf;
            alarm_files.push_back(ffn);
        }
    }
    if (alarm_files.size() == 0) {
        printf("No supported audio files in resources/alarms !\n");
        return false;
    }

    size_t ind = dsl_get_random<size_t>(0, alarm_files.size() - 1);
    fn = alarm_files[ind];
    return true;
}

void wipe_old_logs() {
    string base = cfg->GetDataDirFile("backups");
    if (access(base.c_str(), 0) != 0) {
        //doesn't exist
        return;
    }

    map<string, map<int64, string>> files;
    time_t cutoff = time(NULL) - (86400 * 30);

    Directory dir(base.c_str());
    char buf[MAX_PATH];
    while (dir.Read(buf, sizeof(buf))) {
        if (buf[0] == '.') { continue; }
        string ffn = base + PATH_SEPS + buf;
        struct stat64 st;
        if (stat64(ffn.c_str(), &st) != 0) { continue; }
        if (st.st_mtime > 0 && st.st_mtime < cutoff) {
            printf("Removing old backup file: %s\n", ffn.c_str());
            remove(ffn.c_str());
        }
    }
}

inline void tick_30s() {
    static Uint64 last = 0;
    if (last && SDL_GetTicks64() - last < 30000) {
        return;
    }

    if (config.use_sun) {
        if (!config.home_assistant.info.hadRecentUpdate()) {
            // fallback for if we haven't been able to get info from Home Assistant, or if we aren't using the Home Assistant integration
            double rise, set;
            int rs;

            //Default coordinates are Brockway, sister city of Ogdenville and North Haverbrook
            static double lat = atof(cfg->GetArg("-latitude", "41.24824681406314").c_str());
            static double lon = atof(cfg->GetArg("-longitude", "-78.79205590646596").c_str());

            time_t start = time(NULL);
            struct tm tm;
            localtime_r(&start, &tm);
            rs = sun_rise_set(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, lon, lat, &rise, &set);

            if (rs == 0) {
                double tzoffset = -5.00f;
                rise += tzoffset;
                set += tzoffset;
                //string str = mprintf("Sunrise: %02d:%02d UTC", (int)rise, (int)((rise - (int)rise) * 60));
                //string str2 = mprintf("Sunset:  %02d:%02d UTC", (int)set, (int)((set - (int)set) * 60));
                double timeofday = ((double(tm.tm_hour) * 3600.0f) + (double(tm.tm_min) * 60.f) + double(tm.tm_sec)) / 3600.0f;
                config.SetIsDark(timeofday < rise || timeofday > set);
            }
        }
    } else {
        //hardcoded dark hours, 10p - 10a
        time_t tme = time(NULL);
        struct tm cur;
        localtime_r(&tme, &cur);
        config.SetIsDark(cur.tm_hour >= 22 || cur.tm_hour < 10);
    }

    // just in case there was some error setting it previously
    update_lcd_brightness();

    last = SDL_GetTicks64();
}

inline void alarm_loop() {
    static time_t last = time(NULL);
    static time_t lastAlarmAudioTime = 0;

    time_t start = time(NULL);
    while (last < start) {
        bool is_whole_minute = (last % 60 == 0);
        if (!is_whole_minute) {
            last++;
            continue;
        }

        struct tm cur;
        localtime_r(&last, &cur);

        if (cur.tm_hour == 0 && cur.tm_min == 0) {
            wipe_old_logs();
            if (config.options.auto_enable_at_midnight && !config.options.enable_alarm) {
                printf("Re-enable alarm at midnight...\n");
                config.options.enable_alarm = true;
                save_dynamic_settings();
            }
        }

        if (config.options.enable_alarm && config.options.ShouldAlarmAt(last, &cur)) {
            config.Alarm();
        }

        last++;
    }

    if (config.alarming) {
        if (config.cur_page.get() != page_clock.get()) {
            switch_to_clock();
        }
        if (Mix_Playing(ALERT_CHANNEL) == 0) {
            if (alarm_chunk != NULL) {
                Mix_FreeChunk(alarm_chunk);
                alarm_chunk = NULL;
            }

            if (time(NULL) - lastAlarmAudioTime >= 5) {
                string fn;
                if (GetAlarmFile(fn)) {
                    printf("Attempting to play alarm file %s ...\n", fn.c_str());
                    alarm_chunk = Mix_LoadWAV(fn.c_str());
                    //Mix_Volume(ALERT_CHANNEL, int(MIX_MAX_VOLUME * 0.25));
                    if (alarm_chunk == NULL || Mix_PlayChannel(ALERT_CHANNEL, alarm_chunk, 0) == -1) {
                        mprintf("Error playing %s: %s\n", fn.c_str(), Mix_GetError());
#ifdef WIN32
                        PlaySoundA(fn.c_str(), NULL, SND_FILENAME | SND_ASYNC);
#else
                        string cmd = mprintf("aplay %s", escapeshellarg(fn).c_str());
                        printf("Falling back to: %s\n", cmd.c_str());
                        system(cmd.c_str());
#endif
                    }
                    lastAlarmAudioTime = time(NULL);
                } else {
                    printf("Could not find an alarm audio file!\n");
                }
            }
        }
    } else {
        if (Mix_Playing(ALERT_CHANNEL) != 0 && Mix_FadingChannel(ALERT_CHANNEL) == MIX_NO_FADING) {
            Mix_FadeOutChannel(ALERT_CHANNEL, 1000);
        }
    }
}

#ifdef WIN32
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
#define argc __argc
#define argv __argv
#else
int main(int argc, const char* argv[]) {
#endif
    dsl_init();
    atexit(dsl_cleanup);

    cfg = make_shared<AlarmConfig>();
    cfg->ParseParameters(argc, argv);

    if (!init()) {
        shutdown();
    }

    //Enable text input
    //SDL_StartTextInput();
    set_fps_target(30);

#ifdef DEBUG_FPS
    int64 cur_sec = 0;
    int cur_frame = 0;
#endif

    printf("Loaded and ready.\n");

    //While application is running
    SDL_Event event;
    while (!config.shutdown_now) {
        Uint64 frame_start = SDL_GetTicks64();

        alarm_loop();
        tick_30s();
        if (config.home_assistant.isValid()) {
            update_home_assistant();
        }

        if (config.next_page) {
            SetPage(config.next_page);
        }
        if (config.cur_page.get() == NULL) {
            switch_to_clock();
        }
        if (config.cur_page.get() == NULL) {
            continue;
        }

        //Handle events on queue
        while (SDL_PollEvent(&event) != 0) {
            //User requests quit
            if (event.type == SDL_QUIT) {
                config.shutdown_now = true;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    on_resize();
                } else if (event.window.event == SDL_WINDOWEVENT_MOVED) {
                    if (!config.fullscreen) {
                        config.normal_win_position.x = event.window.data1;
                        config.normal_win_position.y = event.window.data2;
                    }
                    on_resize();
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                update_mouse_position((int)event.motion.x, (int)event.motion.y);
                //handle_mouse_motion(event);
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                update_mouse_position((int)event.button.x, (int)event.button.y);
                if (event.button.button == 1) {
                    SDL_Point pt = { (int)event.button.x, (int)event.button.y };
                    config.cur_page->OnClick(pt);
                }
            } else if (event.type == SDL_KEYDOWN) {
                handle_keypresses(event);
            }
        }

        config.cur_page->Tick();

        render();

        Uint64 frame_end = SDL_GetTicks64();
        if (frame_end >= frame_start) {
            Uint64 took = frame_end - frame_start;
            if (took < config.fps_target) {
                SDL_Delay(config.fps_target - Uint32(took));
            }
        }
        config.cur_frame++;
#ifdef DEBUG_FPS
        cur_frame++;
        if (time(NULL) != cur_sec) {
            statusbar_set(SB_FPS, mprintf("FPS: %d", cur_frame));
            cur_frame = 0;
            cur_sec = time(NULL);
        }
#endif
    }

    //Disable text input
    //SDL_StopTextInput();
    if (!config.fullscreen && config.normal_win_position.x != INT_MAX && config.normal_win_position.y != INT_MAX) {
        //cfg->SetIntArg("-winx", config.normal_win_position.x);
        //cfg->SetIntArg("-winy", config.normal_win_position.y);
        save_dynamic_settings();
    }

    shutdown();
    return 0;
}

void CONFIG::Alarm() {
    if (!_alarming) {
        printf("Alarm!\n");
        _alarming = true;
    }
}

void CONFIG::ClearAlarm() {
    if (_alarming) {
        printf("Alarm acknowledged, shutting up now...\n");
        _alarming = false;
        if (config.options.auto_enable_at_midnight) {
            printf("Disabling alarm until midnight...\n");
            config.options.enable_alarm = false;
            save_dynamic_settings();
        }
    }
}

void CONFIG::SetIsDark(bool dark) {
    if (dark != is_dark) {
        if (dark) {
            printf("It is now dark outside...\n");
        } else {
            printf("It is now light outside...\n");
        }
        _is_dark = dark;
        if (config.cur_page) {
            config.cur_page->OnDarkChanged();
        }
        update_lcd_brightness();
    }
}

void statusbar_set(SB_SECTIONS sec, const string& str) {
    status_bar.SetSectionText(sec, str);
}

void statusbar_set_progress(SB_SECTIONS sec, size_t cur, size_t max) {
    shared_ptr<SDL_StatusBarSection> s;
    if (status_bar.GetSection(sec, s)) {
        s->progress_current = cur;
        s->progress_max = max;
    }
}

bool ConfigOptions::ShouldAlarmAt(time_t ts, tm* ptm) {
    struct tm* tm = ptm;
    if (ptm == NULL) {
        static struct tm _tm;
        localtime_r(&ts, &_tm);
        tm = &_tm;
    }

    vector<ALARM_TIME> opts;
    if (one_time_alarm.isValid()) {
        opts.push_back(one_time_alarm);
    }
    if (daily_alarms[tm->tm_wday].isValid()) {
        opts.push_back(daily_alarms[tm->tm_wday]);
    } else if (alarm_time.isValid()) {
        opts.push_back(alarm_time);
    }

    for(auto& o : opts) {
        if (o.hour == tm->tm_hour && o.minute == tm->tm_min) {
            if (o.hour == one_time_alarm.hour && o.minute == one_time_alarm.minute) {
                one_time_alarm.hour = -1;
                one_time_alarm.minute = -1;
                save_dynamic_settings();
            }
            return true;
        }
    }
    return false;
}

time_t ConfigOptions::GetNextAlarmTime(time_t start) const {
    struct tm tm;
    localtime_r(&start, &tm);
    tm.tm_sec = 0;
    return GetNextAlarmTime(tm);
}

time_t ConfigOptions::GetNextAlarmTime(const struct tm& start) const {
    assert(start.tm_wday >= 0 && start.tm_wday <= 6);
    assert(start.tm_sec == 0);

    vector<ALARM_TIME> opts;
    if (one_time_alarm.isValid()) {
        opts.push_back(one_time_alarm);
    }
    if (daily_alarms[start.tm_wday].isValid()) {
        // If there is a day of week alarm time for this day, then use it
        opts.push_back(daily_alarms[start.tm_wday]);
    } else if (alarm_time.isValid()) {
        // Otherwise, use the regular daily alarm time
        opts.push_back(alarm_time);
    }

    time_t ret = 0;
    for (auto& o : opts) {
        if (o.hour > start.tm_hour || (o.hour == start.tm_hour && o.minute >= start.tm_min)) {
            struct tm tm2 = start;
            tm2.tm_hour = o.hour;
            tm2.tm_min = o.minute;
            time_t ts = mktime(&tm2);
            if (ts >= 0 && (ret == 0 || ts < ret)) {
                ret = ts;
            }
        }
    }

    return ret;
}

time_t ConfigOptions::GetNextAlarmTime() const {
    time_t start = time(NULL);
    time_t ret = GetNextAlarmTime(start);
    int days = 0;
    while (ret <= 0 && days++ < 7) {
        start += 86400;
        struct tm tm;
        localtime_r(&start, &tm);
        tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
        ret = GetNextAlarmTime(tm);
    }
    return ret;
}

static void AlarmTimeToUniValue(const ALARM_TIME& at, UniValue& obj) {
    obj.setObject();
    obj.pushKV("hour", at.hour);
    obj.pushKV("minute", at.minute);
}

static bool AlarmTimeFromUniValue(ALARM_TIME& at, const UniValue& obj) {
    if (!obj.isObject()) {
        return false;
    }
    if (obj.exists("hour") && obj["hour"].isNum()) {
        at.hour = obj["hour"].get_int();
    }
    if (obj.exists("minute") && obj["minute"].isNum()) {
        at.minute = obj["minute"].get_int();
    }
    return true;
}

static UniValue PointToUniValue(const SDL_Point& at) {
    UniValue obj(UniValue::VARR);
    obj.push_back(at.x);
    obj.push_back(at.y);
    return obj;
}

static bool PointFromUniValue(SDL_Point& at, const UniValue& obj) {
    if (!obj.isArray() || obj.size() != 2) {
        return false;
    }
    if (!obj[0].isNum() || !obj[1].isNum()) {
        return false;
    }
    at.x = obj[0].get_int();
    at.y = obj[1].get_int();
    return true;
}

void ConfigOptions::ToUniValue(UniValue& obj) {
    obj.setObject();
    obj.pushKV("enable_alarm", enable_alarm);
    obj.pushKV("dim_when_dark", dim_when_dark);
    obj.pushKV("auto_enable_at_midnight", auto_enable_at_midnight);
    obj.pushKV("flip_clock_style", flip_clock_style);
    obj.pushKV("screen_timeout", screen_timeout / 1000);

    UniValue ota(UniValue::VOBJ);
    AlarmTimeToUniValue(one_time_alarm, ota);
    obj.pushKV("one_time_alarm", ota);

    UniValue at(UniValue::VOBJ);
    AlarmTimeToUniValue(alarm_time, at);
    obj.pushKV("alarm_time", at);

    UniValue da(UniValue::VARR);
    for (int i = 0; i < 7; i++) {
        UniValue day(UniValue::VOBJ);
        AlarmTimeToUniValue(daily_alarms[i], day);
        da.push_back(day);
    }
    obj.pushKV("daily_alarms", da);

    if (!config.fullscreen) {
        obj.pushKV("window_pos", PointToUniValue(config.normal_win_position));
    }
}

bool ConfigOptions::FromUniValue(const UniValue& obj) {
    if (!obj.isObject()) {
        return false;
    }

    if (obj.exists("enable_alarm") && obj["enable_alarm"].isBool()) {
        enable_alarm = obj["enable_alarm"].getBool();
    }
    if (obj.exists("dim_when_dark") && obj["dim_when_dark"].isBool()) {
        dim_when_dark = obj["dim_when_dark"].getBool();
    }
    if (obj.exists("auto_enable_at_midnight") && obj["auto_enable_at_midnight"].isBool()) {
        auto_enable_at_midnight = obj["auto_enable_at_midnight"].getBool();
    }
    if (obj.exists("flip_clock_style") && obj["flip_clock_style"].isBool()) {
        flip_clock_style = obj["flip_clock_style"].getBool();
    }    
    if (obj.exists("screen_timeout") && obj["screen_timeout"].isNum()) {
        screen_timeout = (uint64)max((int64_t)1, obj["screen_timeout"].get_int64()) * 1000;
    }

    if (obj.exists("one_time_alarm")) {
        AlarmTimeFromUniValue(one_time_alarm, obj["one_time_alarm"]);
    }
    if (obj.exists("alarm_time")) {
        AlarmTimeFromUniValue(alarm_time, obj["alarm_time"]);
    }
    if (obj.exists("daily_alarms") && obj["daily_alarms"].isArray()) {
        const UniValue& arr = obj["daily_alarms"];
        for (size_t i = 0; i < arr.size() && i < 7; i++) {
            AlarmTimeFromUniValue(daily_alarms[i], arr[i]);
        }
    }

    if (obj.exists("window_pos") && obj["window_pos"].isArray()) {
        PointFromUniValue(config.normal_win_position, obj["window_pos"]);
    }

    return true;
}

DSL_Mutex outputMutex;
#undef printf
void ac_printf(const char* fmt, ...) {
    AutoMutex(outputMutex);

    // print to console
    va_list va;
    va_start(va, fmt);
    char* tmp = dsl_vmprintf(fmt, va);
    va_end(va);

    char buf[128];
    time_t tme = time(NULL);
    struct tm tm;
    localtime_r(&tme, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

    printf("[%s] %s", buf, tmp);

    if (config.log_fp == NULL) {
        config.log_fp = fopen(cfg->GetDataDirFile("debug.log").c_str(), "w");
        if (config.log_fp != NULL) {
            setbuf(config.log_fp, NULL); // unbuffered
        }
    }
    if (config.log_fp != NULL) {
        fprintf(config.log_fp, "[%s] %s", buf, tmp);
    }

    dsl_free(tmp);
}
