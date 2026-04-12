#define IO_IMPLEMENTATION
#include "../hi/hi.hpp"
#include "../3rd_party/stb/stb_truetype_stream/codepoints/stbtt_codepoints_stream.hpp"

static const char* FONT_DIR{ "../resources/noto/" };
static const char* FONT_ALT_DIR{ "../../resources/noto/" };
static const char* FONT_FILENAME_WORLD{ "NotoSans-Regular.ttf" };
static const char* FONT_FILENAME_JAPANESE{ "NotoSansJP-Regular.ttf" };
static const char* FONT_FILENAME_KOREAN{ "NotoSansKR-Regular.ttf" };
static const char* FONT_FILENAME_TRADITIONAL_CHINESE{ "NotoSansTC-Regular.ttf" };
static const char* FONT_FILENAME_SIMPLIFIED_CHINESE{ "NotoSansSC-Regular.ttf" };

struct MainWindow : public hi::Window<MainWindow> {
    float text_scale{ 1.f };
    float move_offset_x{ 10.f };
    float move_offset_y{ 5.f };
    float bg_speed{ 0.0001f };
    float accent_alpha{ 0.8f };
    float ui_scale_mul{ 1.0f };

    hi::AtlasId world_atlas{ -1 };
    hi::AtlasId jp_atlas{ -1 };
    hi::AtlasId kr_atlas{ -1 };
    hi::AtlasId tc_atlas{ -1 };
    hi::AtlasId sc_atlas{ -1 };

    char tf_search_buf[96]{ "find: button and sliders" };
    char tf_username_buf[64]{ "player_01" };
    char tf_note_buf[160]{ "drag to select, Ctrl+C / Ctrl+V, type to replace selection" };
    io::usize tf_search_len{ 0 };
    io::usize tf_username_len{ 0 };
    io::usize tf_note_len{ 0 };

    MainWindow() noexcept {
        this->setTitle(IO_U8("Hello World! Привіт Світе!"));
        this->setVSyncEnable(false);
        tf_search_len = io::len(tf_search_buf);
        tf_username_len = io::len(tf_username_buf);
        tf_note_len = io::len(tf_note_buf);
    }

    void onError(hi::Error err, hi::AboutError ae) noexcept override {
        setTitle(hi::what(ae));
    }

    void onKeyDown(hi::Key k) noexcept override {
        using K = hi::Key;
        switch (k) {
        case K::Right: move_offset_x += text_scale; break;
        case K::Left:  move_offset_x -= text_scale; break;
        case K::Down:  move_offset_y += text_scale; break;
        case K::Up:    move_offset_y -= text_scale; break;
        }
    }

    void onKeyUp(hi::Key k) noexcept override {
        using K = hi::Key;
        switch (k) {
        case K::_1: setCursorVisible(!isCursorVisible()); break;
        case K::_2: setFullscreen(!isFullscreen()); break;
        }
    }

    void onScroll(float deltaX, float deltaY) noexcept override {
        text_scale += deltaX;
        text_scale += deltaY;
        io::StackOut<64> ss;
        ss << "Current scale is " << text_scale << " times";
        setTitle(ss.view());
    }

    void onRender(float dt) noexcept override {
        static float red = 0.f;
        (void)dt;
        red += bg_speed;
        if (red > 1.f) red = 0.f;
        gl::ClearColor(red, 0.f, 0.f, 0.f);
        gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);

        hi::ButtonDraw btn{};
        btn.atlas = world_atlas;
        btn.dock = hi::TextDock::TopC;
        btn.x = 0.f; btn.y = 50.f;
        btn.scale = text_scale;
        btn.text = IO_U8("Click me");
        btn.style.normal = hi::TextStyle{1.f,  1.f, 1.f,  1.f, false };
        btn.style.hover = hi::TextStyle{ 1.f,  1.f, 0.7f, 1.f, true, 0.f, 0.f, 0.f, 1.f, /*.outline_px*/1.2f, /*.softness_px*/0.9f };
        btn.style.active = hi::TextStyle{0.7f, 1.f, 0.7f, 1.f, true, 0.f, 0.f, 0.f, 1.f, /*.outline_px*/2.0f, /*.softness_px*/0.9f };

        auto st = Button(btn);
        if (st.clicked) {
            setTitle(IO_U8("Clicked!"));
        }

        // --- TextFields (different styles) ---
        hi::TextFieldDraw tf_search{};
        tf_search.atlas = world_atlas;
        tf_search.dock = hi::TextDock::TopL;
        tf_search.x = 24.f;
        tf_search.y = 52.f;
        tf_search.scale = 0.75f;
        tf_search.text = io::char_view_mut{ tf_search_buf, sizeof(tf_search_buf) };
        tf_search.text_len = &tf_search_len;
        tf_search.id = 101;
        tf_search.style.placeholder = IO_U8("Search...");
        tf_search.style.min_width = 320.f;
        tf_search.style.box.border = true;
        tf_search.style.box.border_radius = 12.f;
        tf_search.style.box.pad_top = 8.f;
        tf_search.style.box.pad_bottom = 8.f;
        tf_search.style.box.pad_left = 12.f;
        tf_search.style.box.pad_right = 12.f;
        tf_search.style.normal = hi::TextStyle{ 0.95f, 0.95f, 1.f, 1.f, true, 0.2f, 0.35f, 0.9f, accent_alpha, 1.2f, 0.9f };
        tf_search.style.hover  = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 0.3f, 0.5f, 1.f, accent_alpha, 1.3f, 0.9f };
        tf_search.style.active = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 0.45f, 0.65f, 1.f, accent_alpha, 1.5f, 0.9f };
        (void)TextField(tf_search);

        hi::TextFieldDraw tf_username{};
        tf_username.atlas = world_atlas;
        tf_username.dock = hi::TextDock::TopL;
        tf_username.x = 24.f;
        tf_username.y = 104.f;
        tf_username.scale = 0.72f;
        tf_username.text = io::char_view_mut{ tf_username_buf, sizeof(tf_username_buf) };
        tf_username.text_len = &tf_username_len;
        tf_username.id = 102;
        tf_username.style.placeholder = IO_U8("Username");
        tf_username.style.min_width = 230.f;
        tf_username.style.box.border = true;
        tf_username.style.box.border_radius = 3.f;
        tf_username.style.box.underscored = true;
        tf_username.style.box.pad_top = 7.f;
        tf_username.style.box.pad_bottom = 7.f;
        tf_username.style.box.pad_left = 10.f;
        tf_username.style.box.pad_right = 10.f;
        tf_username.style.normal = hi::TextStyle{ 1.f, 1.f, 0.85f, 1.f, true, 0.65f, 0.55f, 0.1f, accent_alpha, 1.0f, 0.9f };
        tf_username.style.hover  = hi::TextStyle{ 1.f, 1.f, 0.9f, 1.f, true, 0.8f, 0.65f, 0.2f, accent_alpha, 1.2f, 0.9f };
        tf_username.style.active = hi::TextStyle{ 1.f, 1.f, 0.95f, 1.f, true, 0.95f, 0.8f, 0.25f, accent_alpha, 1.4f, 0.9f };
        (void)TextField(tf_username);

        hi::TextFieldDraw tf_note{};
        tf_note.atlas = world_atlas;
        tf_note.dock = hi::TextDock::TopL;
        tf_note.x = 24.f;
        tf_note.y = 154.f;
        tf_note.scale = 0.70f;
        tf_note.text = io::char_view_mut{ tf_note_buf, sizeof(tf_note_buf) };
        tf_note.text_len = &tf_note_len;
        tf_note.id = 103;
        tf_note.style.placeholder = IO_U8("Notes");
        tf_note.style.min_width = 520.f;
        tf_note.style.box.border = true;
        tf_note.style.box.border_radius = 18.f;
        tf_note.style.box.pad_top = 9.f;
        tf_note.style.box.pad_bottom = 9.f;
        tf_note.style.box.pad_left = 16.f;
        tf_note.style.box.pad_right = 16.f;
        tf_note.style.normal = hi::TextStyle{ 0.9f, 1.f, 0.95f, 1.f, true, 0.1f, 0.55f, 0.35f, accent_alpha, 1.0f, 0.9f };
        tf_note.style.hover  = hi::TextStyle{ 0.95f, 1.f, 0.98f, 1.f, true, 0.2f, 0.75f, 0.5f, accent_alpha, 1.2f, 0.9f };
        tf_note.style.active = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 0.28f, 0.9f, 0.65f, accent_alpha, 1.4f, 0.9f };
        (void)TextField(tf_note);

        // --- Sliders (different styles) ---
        io::StackOut<96> sl1_text;
        sl1_text << "Background speed: " << bg_speed;
        hi::SliderDraw s1{};
        s1.atlas = world_atlas;
        s1.dock = hi::TextDock::TopL;
        s1.x = 24.f;
        s1.y = 238.f;
        s1.scale = 0.72f;
        s1.text = sl1_text.view();
        s1.value = &bg_speed;
        s1.min_value = 0.00002f;
        s1.max_value = 0.0030f;
        s1.step = 0.00002f;
        s1.id = 201;
        s1.style.min_width = 330.f;
        s1.style.track_height = 5.f;
        s1.style.track_gap = 8.f;
        s1.style.handle_size = 12.f;
        s1.style.box.border = true;
        s1.style.box.border_radius = 10.f;
        s1.style.box.pad_top = 8.f;
        s1.style.box.pad_bottom = 7.f;
        s1.style.box.pad_left = 12.f;
        s1.style.box.pad_right = 12.f;
        s1.style.normal = hi::TextStyle{ 1.f, 0.9f, 0.9f, 1.f, true, 0.85f, 0.25f, 0.25f, accent_alpha, 1.0f, 0.9f };
        s1.style.hover  = hi::TextStyle{ 1.f, 0.95f, 0.95f, 1.f, true, 1.f, 0.35f, 0.35f, accent_alpha, 1.2f, 0.9f };
        s1.style.active = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 1.f, 0.5f, 0.5f, accent_alpha, 1.4f, 0.9f };
        (void)Slider(s1);

        io::StackOut<96> sl3_text;
        sl3_text << "UI scale mul: " << ui_scale_mul;
        hi::SliderDraw s3{};
        s3.atlas = world_atlas;
        s3.dock = hi::TextDock::TopL;
        s3.x = 24.f;
        s3.y = 312.f;
        s3.text = sl3_text.view();
        s3.value = &ui_scale_mul;
        s3.min_value = 0.5f;
        s3.max_value = 2.0f;
        s3.step = 0.01f;
        s3.id = 203;
        s3.style.min_width = 330.f;
        s3.style.track_height = 6.f;
        s3.style.track_gap = 8.f;
        s3.style.handle_size = 14.f;
        s3.style.box.border = true;
        s3.style.box.border_radius = 18.f;
        s3.style.box.pad_top = 8.f;
        s3.style.box.pad_bottom = 8.f;
        s3.style.box.pad_left = 14.f;
        s3.style.box.pad_right = 14.f;
        s3.style.normal = hi::TextStyle{ 0.9f, 0.95f, 1.f, 1.f, true, 0.3f, 0.45f, 0.95f, accent_alpha, 1.0f, 0.9f };
        s3.style.hover  = hi::TextStyle{ 0.95f, 0.97f, 1.f, 1.f, true, 0.4f, 0.58f, 1.f, accent_alpha, 1.2f, 0.9f };
        s3.style.active = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 0.55f, 0.7f, 1.f, accent_alpha, 1.4f, 0.9f };
        (void)Slider(s3);

        io::StackOut<96> sl2_text;
        sl2_text << "Accent alpha: " << accent_alpha;
        hi::SliderDraw s2{};
        s2.atlas = world_atlas;
        s2.dock = hi::TextDock::BottomR;
        s2.x = -15.f;
        s2.y = -15.f;
        s2.text = sl2_text.view();
        s2.value = &accent_alpha;
        s2.min_value = 0.2f;
        s2.max_value = 1.0f;
        s2.step = 0.01f;
        s2.id = 202;
        s2.style.min_width = 330.f;
        s2.style.track_height = 4.f;
        s2.style.track_gap = 7.f;
        s2.style.handle_size = 10.f;
        s2.style.box.border = true;
        s2.style.box.border_radius = 2.f;
        s2.style.box.underscored = true;
        s2.style.box.pad_top = 7.f;
        s2.style.box.pad_bottom = 6.f;
        s2.style.box.pad_left = 10.f;
        s2.style.box.pad_right = 10.f;
        s2.style.normal = hi::TextStyle{ 0.95f, 1.f, 0.95f, 1.f, true, 0.2f, 0.7f, 0.3f, accent_alpha, 1.0f, 0.9f };
        s2.style.hover = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 0.25f, 0.85f, 0.35f, accent_alpha, 1.2f, 0.9f };
        s2.style.active = hi::TextStyle{ 1.f, 1.f, 1.f, 1.f, true, 0.4f, 1.f, 0.55f, accent_alpha, 1.4f, 0.9f };
        (void)Slider(s2);

        setElementScale(ui_scale_mul);
        

        hi::TextStyle ts{};
        ts.r = red;
        ts.b = red;
        ts.softness_px = 0.f;
        ts.outline = true;
        ts.outline_px = text_scale * 0.05f;

        hi::TextDraw td{};
        td.style = ts;

        td.scale = text_scale;
        td.space_between = -0.18f;

        const float x0 = move_offset_x;
        float y = move_offset_y;

        // 1) WORLD: Latin + Cyrillic (UI / mixed text)
        td.atlas = world_atlas;

        /*td.x = x0; td.y = y;
        td.text = IO_U8("[WORLD] Hello World!  Привіт Світе!  Greek? (won't)  عربى? (won't)");
        this->DrawText(td);*/

        td.x = td.y = 0.f;
        td.dock = hi::TextDock::TopL;    td.text = "[top-left]"; this->DrawText(td);
        td.dock = hi::TextDock::TopC;    td.text = "[top-center]"; this->DrawText(td);
        td.dock = hi::TextDock::TopR;    td.text = "[top-right]"; this->DrawText(td);
        //td.dock = hi::TextDock::LeftC;   td.text = "[left-center]"; this->DrawText(td);
        td.dock = hi::TextDock::RightC;  td.text = "[right-center]"; this->DrawText(td);
        td.dock = hi::TextDock::BottomL; td.text = "[bottom-left]"; this->DrawText(td);
        td.dock = hi::TextDock::BottomC; td.text = "[bottom-center]"; this->DrawText(td);
        //td.dock = hi::TextDock::BottomR; td.text = "[bottom-right]"; this->DrawText(td);

        td.dock = hi::TextDock::TopL;

        // td.x = x0; td.y = y;
        //y += 90.f;

        //// 2) Han-string with the same glyphs, but with different atlases JP/SC/TC/KR.
        //io::char_view han_line = IO_U8("漢字對照: 直 骨 令 青 海 國 龍 風 體");

        //td.x = x0; td.y = y;

        //io::out.reset();

        //td.atlas = jp_atlas;
        //io::out << "[JP atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 70.f;

        //io::out.reset();

        //td.atlas = sc_atlas;
        //td.x = x0; td.y = y;
        //io::out << "[SC atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 70.f;

        //io::out.reset();

        //td.atlas = tc_atlas;
        //td.x = x0; td.y = y;
        //io::out << "[TC atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 70.f;

        //io::out.reset();

        //td.atlas = kr_atlas;
        //td.x = x0; td.y = y;
        //io::out << "[KR atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 90.f;

        //// 3) Lang examples (to ensure Kana/Hangul works)
        //td.atlas = jp_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[JP] 日本語テスト: こんにちは世界  カタカナ: アイウエオ  漢字: 東京 大学 日本");
        //this->DrawText(td);
        //y += 70.f;

        //td.atlas = kr_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[KR] 한국어 테스트: 안녕하세요 세계  한글 + 漢字 혼용: 國語 漢字");
        //this->DrawText(td);
        //y += 70.f;

        //td.atlas = sc_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[SC] 你好，世界  简体中文示例: 汉字 语言 共和国 龙 风 体");
        //this->DrawText(td);
        //y += 70.f;

        //td.atlas = tc_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[TC] 你好，世界  繁體中文示例: 漢字 語言 共和國 龍 風 體");
        //this->DrawText(td);

        this->FlushText();
    }

    bool LoadResources() noexcept {
        hi::FontAtlasDesc desc{};
        desc.mode = hi::FontAtlasMode::SDF;
        desc.pixel_height = 48;
        desc.spread_px = 4.f;

        io::string full_font_path;
        {
            io::string cwd;
            if (!fs::current_directory(cwd)) return false;
            io::out << "current dir is: " << cwd << '\n';

            // try "../resources/noto"
            if (!fs::path_join(cwd, FONT_DIR, full_font_path)) return false;
            io::out << "searching in dir: " << full_font_path << '\n';

            // try alt "../../resources/noto"
            if (!fs::exists(full_font_path)) {
                if (!fs::path_join(cwd, FONT_ALT_DIR, full_font_path)) return false;
                io::out << "searching in dir: " << full_font_path << '\n';
            }
            if (!fs::exists(full_font_path)) {
                io::out << "Couldn't find font in FONT_DIR and FONT_ALT_DIR\n";
            }
        }

        io::string current_font;

        auto load_atlas = [&](const char* filename, hi::AtlasId& out_atlas, auto... scripts) -> bool {
            fs::path_join(full_font_path, filename, current_font);
            hi::FontId font_id = this->LoadFont(current_font.as_view());
            if (font_id < 0) return false;
            out_atlas = this->GenerateFontAtlas(font_id, desc, scripts...);
            return out_atlas >= 0;
        };

        // WORLD: Latin/Cyrillic/Greek/Arabic/...
        if (!load_atlas(FONT_FILENAME_WORLD, world_atlas,
            stbtt_codepoints::Script::Latin,
            stbtt_codepoints::Script::Cyrillic)) return false;
        io::out << "WORLD atlas side: " << getAtlasSideSize(world_atlas) << '\n';

        //// JP
        //if (!load_atlas(FONT_FILENAME_JAPANESE, jp_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::Kana,
        //    stbtt_codepoints::Script::JouyouKanji,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "JP atlas side: " << getAtlasSideSize(jp_atlas) << '\n';

        //// KR
        //// TODO: Script::Hangul
        //if (!load_atlas(FONT_FILENAME_KOREAN, kr_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "KR atlas side: " << getAtlasSideSize(kr_atlas) << '\n';

        //// TC
        //if (!load_atlas(FONT_FILENAME_TRADITIONAL_CHINESE, tc_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "TC atlas side: " << getAtlasSideSize(tc_atlas) << '\n';

        //// SC
        //if (!load_atlas(FONT_FILENAME_SIMPLIFIED_CHINESE, sc_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "SC atlas side: " << getAtlasSideSize(sc_atlas) << '\n';

        return true;
    }
}; // struct MainWindow

int main() {
    MainWindow win{};
    if (!win.LoadResources()) io::exit_process(-1);

    while (win.PollEvents()) {
        win.Render();
    }
    io::exit_process(0);
}
