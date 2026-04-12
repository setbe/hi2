#define IO_IMPLEMENTATION
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"
#include "../../hi/hi.hpp"

#if defined(_WIN32)
static inline void clear_thread_quit_message() noexcept {
    MSG msg{};
    while (::PeekMessageW(&msg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE)) {}
}
#else
static inline void clear_thread_quit_message() noexcept {}
#endif

struct interactive_state {
    bool ok = false;
    bool done = false;

    // event flags
    bool moved = false;
    bool resized = false;
    bool scroll_up = false;
    bool scroll_down = false;
    bool unfocus = false;
    bool focus = false;

    // for resize
    int last_w = 0;
    int last_h = 0;
};

struct TestWindow : public hi::Window<TestWindow> {
    interactive_state st{};

    TestWindow() noexcept {
        setTitle(IO_U8("Interactive test: press Y to start, N to fail"));
        st.last_w = width();
        st.last_h = height();
    }

    void onRender(float) noexcept override {
        static float red = 0.f;
        red += 0.01f;
        if (red > 1.f) red = 0.f;
        gl::ClearColor(red, 0.f, 0.f, 1.f);
        gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);
    }

    void onMouseMove(int, int) noexcept override {
        st.moved = true;
    }

    void onWindowResize(int w, int h) noexcept override {
        if (w != st.last_w || h != st.last_h) st.resized = true;
        st.last_w = w; st.last_h = h;
    }

    void onScroll(float dx, float dy) noexcept override {
        const float v = dx != 0.f ? dx : dy;
        if (v > 0.f) st.scroll_up = true;
        if (v < 0.f) st.scroll_down = true;
    }

    void onFocusChange(bool gained) noexcept override {
        if (gained) { st.focus = true;  st.unfocus = false; }
        else {        st.focus = false; st.unfocus = true; }
    }

    void onKeyDown(hi::Key k) noexcept override {
        if (k == hi::Key::Y) { st.ok = true; st.done = true; }
        if (k == hi::Key::N) { st.ok = false; st.done = true; }
    }

    void onError(hi::Error, hi::AboutError ae) noexcept override {
        setTitle(hi::what(ae));
        st.ok = false; st.done = true;
    }
};

static inline bool run_step(TestWindow& w, bool (*cond)(const interactive_state&) noexcept, io::char_view title, int max_frames = 60*15) noexcept {
    w.setTitle(title);
    int frames = 0;
    while (w.PollEvents()) {
        if (cond(w.st)) return true;
        if (++frames >= max_frames) return false;
    }
    return false;
}

TEST_CASE("hi interactive: mouse/resize/scroll/focus", "[hi][window][interactive]") {
    TestWindow w;

    // 1) red animation confirmation
    w.setTitle(IO_U8("Do you see red screen animation? Press Y/N"));
    while (w.PollEvents() && !w.st.done) {
        w.Render();
    }
    if (!w.st.done)
        w.st.ok = true; // user closed tests manually
    REQUIRE(w.st.ok);

    // 2) mouse move
    auto move = [](const interactive_state& s) noexcept { return s.moved; };
    REQUIRE(run_step(w, move, IO_U8("Please, move your mouse within window.")));

    // 3) resize
    auto resize = [](const interactive_state& s) noexcept { return s.resized; };
    REQUIRE(run_step(w, resize, IO_U8("Please, resize window.")));

    // 4) scroll up
    auto scroll_up = [](const interactive_state& s) noexcept { return s.scroll_up; };
    REQUIRE(run_step(w, scroll_up, IO_U8("Please, scroll up.")));

    // 5) scroll down
    auto scroll_down = [](const interactive_state& s) noexcept { return s.scroll_down; };
    REQUIRE(run_step(w, scroll_down, IO_U8("Please, scroll down.")));

    // 6) unfocus
    auto unfocus = [](const interactive_state& s) noexcept { return s.unfocus; };
    REQUIRE(run_step(w, unfocus, IO_U8("Please, unfocus window (Alt-Tab away).")));

    // 7) focus
    auto focus = [](const interactive_state& s) noexcept { return s.focus; };
    REQUIRE(run_step(w, focus, IO_U8("Please, focus window back.")));

    w.PollEvents();
    run_step(w, unfocus, IO_U8("Interactive test PASS. Close/unfocus window."));

}

struct SmokeWindow : public hi::Window<SmokeWindow> {
    bool gl_ok = true;
    SmokeWindow() noexcept {
        setTitle(IO_U8("GL smoke test..."));
        if (api() != hi::RendererApi::Opengl) gl_ok = false;
    }
    void onRender(float) noexcept override {}
    void onError(hi::Error, hi::AboutError ae) noexcept override {
        gl_ok = false;
        setTitle(hi::what(ae));
        io::sleep_ms(5000);
    }
};

IO_CONSTEXPR_VAR io::u8 GL_CORE_MAJOR = 3;
IO_CONSTEXPR_VAR io::u8 GL_CORE_MINOR = 3;

TEST_CASE("gl loader smoke: functions callable", "[gl][smoke]") {
    clear_thread_quit_message();
    SmokeWindow w{};
    REQUIRE(w.gl_ok);
    REQUIRE(w.api() == hi::RendererApi::Opengl);

    // one frame to ensure context current
    if (!w.PollEvents()) {
        clear_thread_quit_message();
    }
    REQUIRE(w.PollEvents());
    w.Render();

    const int real_maj = w.g.currentMajorVersion();
    const int real_min = w.g.currentMinorVersion();
    REQUIRE(gl::ver_ge(real_maj, real_min, GL_CORE_MAJOR, GL_CORE_MINOR));
    if (real_maj >= 3)
        REQUIRE(gl::native::pBindVertexArray != nullptr);
    
#define X(name, call_args, reqmaj) \
    do { if (real_maj >= (reqmaj)) { gl::name call_args; REQUIRE(gl::GetError() == 0); } } while (0);

// name, args to call (smoke), required_es_major
#define IO_GL_API_LIST(X) \
    X(GetError,              (),                        2) \
    X(CullFace,              (gl::Face::Back),          2) \
    X(ClearColor,            (0.f,0.f,0.f,1.f),         2) \
    X(Clear,                 (gl::buffer_bit.Color),    2) \
    X(Disable,               (gl::Capability::Blend),   2) \
    X(Enable,                (gl::Capability::Blend),   2) \
    X(BlendFunc,             (gl::BlendFactor::One, gl::BlendFactor::Zero), 2) \
    X(GetString,             (0x1F00u /*GL_VENDOR*/),   2) \
    X(Viewport,              (0,0,16,16),               2) \
    X(GenBuffers,            (0, (io::u32*)nullptr),    2) \
    X(GenTextures,           (0, (io::u32*)nullptr),    2) \
    X(ActiveTexture,         (gl::TexUnit::_0),     2) \
    /* ... add rest no state functions */ \
    X(BindVertexArray,       (0),                       3) \
    X(GenVertexArrays,       (0, (io::u32*)nullptr),    3)

    IO_GL_API_LIST(X);
#undef IO_GL_API_LIST
#undef X
}

TEST_CASE("gl high-level wrappers: shader/buffer/vao", "[gl][wrap]") {
    clear_thread_quit_message();
    SmokeWindow w;
    if (!w.PollEvents()) {
        clear_thread_quit_message();
    }
    REQUIRE(w.PollEvents());
    w.Render();

    // --- Shader: separated core/es versions ---
#if defined(__linux__) || defined(_WIN32)
    const char* vs =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "void main(){ gl_Position = vec4(aPos,1.0); }\n";
    const char* fs =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main(){ FragColor = vec4(1,0,0,1); }\n";
#else
    const char* vs =
        "#version 300 es\n"
        "layout(location=0) in vec3 aPos;\n"
        "void main(){ gl_Position = vec4(aPos,1.0); }\n";
    const char* fs =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 FragColor;\n"
        "void main(){ FragColor = vec4(1,0,0,1); }\n";
#endif

#if IO_GL_API_ES >= 20
    gl::Shader sh;
    REQUIRE(sh.Compile(vs, fs));
    REQUIRE(!sh.failed());
    sh.Use();
    REQUIRE(gl::GetError() == 0);

    // --- Buffer ---
    float tri[] = { 0.f,0.f,0.f,  1.f,0.f,0.f,  0.f,1.f,0.f };
    gl::Buffer vbo(gl::BufferTarget::ArrayBuffer);
    vbo.bind();
    vbo.data(tri, sizeof(tri), gl::BufferUsage::StaticDraw);
    REQUIRE(gl::GetError() == 0);
#endif

#if IO_GL_API_ES >= 30
    // --- VAO + MeshBinder ---
    gl::VertexArray vao;
    vao.bind();

    const gl::Attr attrs_arr[] = { gl::AttrOf<float>(3) };
    io::view<const gl::Attr> attrs(attrs_arr, 1);
    gl::MeshBinder::setup(vao, vbo, attrs);

    REQUIRE(gl::GetError() == 0);

    // optional draw call (no EBO)
    gl::DrawArrays(gl::PrimitiveMode::Triangles, 0, 3);
    REQUIRE(gl::GetError() == 0);
#endif
}
