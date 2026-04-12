# hi public API reference

This document describes the public API exposed by headers in this folder:

- `io.hpp`
- `socket.hpp`
- `hi.hpp`
- `audiocontext.hpp`
- `gl_loader.hpp`

It is written as a practical reference for users integrating `io::`, `fs::`, and `hi::` in applications.

---

## 1. Integration Model

### 1.1 One implementation translation unit

Headers are designed to be included everywhere, but implementation must be emitted once:

```cpp
#define IO_IMPLEMENTATION
#include "hi/hi.hpp"
```

Rules:

- Define `IO_IMPLEMENTATION` in exactly one `.cpp` file.
- Other files include headers without that define.
- Works for hosted and freestanding-oriented build configs.

### 1.2 Main namespaces

- `io` - runtime primitives, containers, strings, memory/time/threading, file backend hooks.
- `fs` - filesystem helpers and RAII file/directory API.
- `hi` - windowing, OpenGL context setup, font atlas generation, immediate-mode GUI widgets.
- `ac` - audio device/mixer/scheduler API for music, ambient, SFX, UI.
- `gl` - OpenGL loader/wrapper API from `gl_loader.hpp`.

---

## 2. `io.hpp` API

### 2.1 Fundamental types and constants

`io` defines fixed-width aliases and utility constants used across the project:

- integer aliases: `u8/u16/u32/u64`, `i8/i16/i32/i64`
- `usize`, `isize`
- `npos`

### 2.2 Non-owning views

Primary type:

```cpp
template<typename T>
struct io::view;
```

Core behavior:

- non-owning pointer + length
- cheap copy, no allocation
- bounds are explicit

Common aliases:

- `io::char_view`
- `io::char_view_mut`
- `io::byte_view`
- `io::byte_view_mut`
- fixed-length wrappers: `view_n<T, N>`, `char_view_n<N>`, `byte_view_n<N>`

### 2.3 Strings

- `io::string` (UTF-8)
- `io::wstring` (wide, Windows interop)

Properties:

- NUL-terminated
- based on custom containers, not STL
- append/resize/split/join helpers
- direct `as_view()` integration

### 2.4 Containers

Public containers:

- `io::vector<T>`
- `io::deque<T>`
- `io::list<T>`

Design intent:

- explicit memory/lifetime control
- no hidden allocator objects
- stable low-level behavior suitable for freestanding builds

### 2.5 Output/Input helpers

Terminal/global streams:

- `io::out`
- `io::in`

Stack-based formatting/parsing helpers:

- `io::StackOut<N>`
- `io::StackIn<N>`

Example:

```cpp
io::StackOut<64> ss;
ss << "value = " << 42;
io::char_view msg = ss.view();
```

### 2.6 Atomics and synchronization

Key public primitives:

- `io::atomic<T>`
- `io::spin_mutex`
- `io::Backoff`

Threading API:

- `io::Thread`
- `io::ThreadPool`

### 2.7 Memory, timing, and process utilities

Core runtime calls:

- `void* io::alloc(io::usize bytes) noexcept`
- `void  io::free(void* ptr) noexcept`
- `void  io::sleep_ms(unsigned ms) noexcept`
- `io::u64 io::monotonic_ms() noexcept`
- `io::u64 io::monotonic_us() noexcept`
- `void  io::secure_zero(void* p, io::usize size) noexcept`
- `bool  io::os_entropy(void* dst, io::usize size) noexcept`

### 2.8 File backend and modes

Open/seek types:

- `io::OpenMode`
- `io::SeekWhence`

`OpenMode` flags:

- `Read`, `Write`, `Append`, `Truncate`, `Create`, `Binary`, `Text`

Native file backend symbols are available under `io::native` and used by `fs::File`.

---

## 3. `fs` API (from `io.hpp`)

### 3.1 Path helpers

- `fs::path_join(base, leaf, out)`

Overloads accept `io::char_view` and `io::string`.

### 3.2 RAII file object

```cpp
struct fs::File;
```

Key methods:

- `open(...)`, `close()`, `is_open()`
- status: `good()`, `fail()`, `eof()`, `clear()`
- I/O: `read(...)`, `write(...)`, `flush()`
- positioning: `seek(...)`, `tell()`, `size()`
- convenience: `read_line(...)`, `read_all(...)`, `write_line(...)`, `read_exact(...)`

### 3.3 Directory iteration

```cpp
struct fs::directory_iterator;
```

Usage pattern:

- construct with directory path
- increment with `operator++`
- check `is_end()`
- access `directory_entry` via `*` or `->`

### 3.4 Filesystem queries

Utility functions:

- `fs::exists(...)`
- `fs::status(...)`
- `fs::is_directory(...)`
- `fs::is_regular_file(...)`
- `fs::file_size(...)`
- `fs::create_directory(...)`
- `fs::remove(...)`
- `fs::rename(...)`
- `fs::current_directory(...)`

---

## 4. `socket.hpp` API

Important: networking symbols are in namespace `io`.

### 4.1 Protocol constants and message types

Public constants:

- `UDP_MAGIC`
- `UDP_VERSION`
- `DEFAULT_MTU`, `MIN_MTU`, `MAX_MTU`
- `HANDSHAKE_TIMEOUT_MS`, `PEER_TIMEOUT_MS`, `KEEPALIVE_INTERVAL_MS`
- `MAX_PEERS`

Control message IDs:

- `MSG_HELLO`
- `MSG_COOKIE`
- `MSG_HELLO2`
- `MSG_WELCOME`
- `MSG_KEEPALIVE`
- `MSG_DISCONNECT`

### 4.2 Addressing types

```cpp
struct io::IP;
struct io::Endpoint;
```

Highlights:

- `IP::from_string("x.y.z.w") -> u32` in network byte order
- `Endpoint { addr_be, port_be }`
- endian helpers: `h2ns/h2nl/h2nll`, `n2hs/n2hl/n2hll`

### 4.3 Socket API

```cpp
struct io::Socket;
```

Primary methods:

- `open(Protocol)`
- `close()`
- `set_blocking(bool)`
- `bind(Endpoint)`
- `send_to(Endpoint, byte_view)`
- `recv_from(Endpoint&, byte_view_mut)`

Status methods:

- `is_open()`
- `protocol()`
- `native()`
- `error()`
- `error_str()`

Enums:

- `io::Protocol { TCP, UDP }`
- `io::Error { None, WouldBlock, Again, Closed, Generic, ConnReset }`

### 4.4 UDP framing and reliability helpers

Public types:

- `io::UdpChan { Unreliable, Reliable }`
- `io::UdpHeader`
- `io::SeqWindow128`
- `io::AckState`

Helper functions:

- `udp_header_host_to_wire(...)`
- `udp_header_wire_to_host(...)`
- `make_ack_state(...)`
- `is_acked_by(...)`

### 4.5 Disconnect and drop callbacks

Enums:

- `io::DisconnectReason`
- `io::DropReason`

Callback bundle:

```cpp
struct io::UdpCallbacks {
    void (*on_packet)(...);
    void (*on_drop)(...);
    void (*on_established)(...);
    void (*on_disconnect)(...);
    void* ud;
};
```

### 4.6 Event loop

```cpp
template<u16 MaxPayload = 1200u, usize MaxPackets = 2048u>
struct io::EventLoop;
```

High-level lifecycle:

- `init(...)`
- `run_udp(...)`
- `stop()`

Connection helpers:

- `start_client_handshake(server, mtu, features, now_ms)`
- `find_peer(...)`
- `get_peer_create(...)`
- `disconnect_peer(...)`
- `send_to_peer(...)`

Handshake model:

- client: `HELLO -> COOKIE -> HELLO2 -> WELCOME`
- cookie validation protects against spoofed source addresses
- negotiated MTU/features become peer session parameters

---

## 5. `hi.hpp` API

### 5.1 Core enums

Key enums:

- `hi::Key`
- `hi::RendererApi`
- `hi::WindowBackend`
- `hi::Cursor`
- `hi::CursorState`
- `hi::Error`
- `hi::AboutError`

### 5.2 Window callback interface

```cpp
struct hi::IWindow;
```

Main callbacks:

- `onRender(float dt)`
- `onError(Error, AboutError)`
- `onScroll(float, float)`
- `onWindowResize(int, int)`
- `onMouseMove(int, int)`
- `onKeyDown(Key)`
- `onKeyUp(Key)`
- `onFocusChange(bool)`

Input bridge callbacks:

- `onNativeKeyEvent(Key, bool)`
- `onTextInput(io::char_view utf8)`

Library-managed methods include `Render()`, `onGeometryChange(...)`, cursor state reset, and backend accessors.

### 5.3 CRTP window class

```cpp
template<typename Derived>
struct hi::Window : hi::IWindow;
```

Runtime control:

- `PollEvents()`
- `Render()`
- `SwapBuffers()`
- `setTargetFps(...)`
- `setVSyncEnable(...)`

Window control:

- `setShow(...)`
- `setTitle(...)`
- `setFullscreen(...)`
- `setCursor(...)`
- `setCursorVisible(...)`
- `setElementScale(...)`

State query:

- `width()`, `height()`
- `mouseX()`, `mouseY()`
- `UiScale()`
- `isMouseDown()`, `isMouseReleased()`
- `isVSync()`, `isFullscreen()`, `isShown()`

### 5.4 Font and atlas API

Types:

- `hi::FontId`
- `hi::AtlasId`
- `hi::FontAtlasMode { SDF, MTSDF, DebugR, DebugRGBA }`
- `hi::FontAtlasDesc`

Methods:

- `LoadFont(ttf_path)`
- `GenerateFontAtlas(font_id, desc)`
- `GenerateFontAtlas(font_id, desc, scripts...)`
- `getAtlasSideSize(atlas_id)`

### 5.5 Text rendering API

Types:

- `hi::TextStyle`
- `hi::TextDock`
- `hi::TextDraw`

Methods:

- `DrawText(const TextDraw&)`
- `FlushText()`

Rendering is batched; call `FlushText()` once near the end of frame rendering.

### 5.6 Immediate-mode GUI API

### Button

Types:

- `hi::ButtonStyle`
- `hi::ButtonDraw`
- `hi::ButtonState`

Method:

- `ButtonState Button(const ButtonDraw&)`

Returns hover/held/click state and final rectangle.

### Shared box styling

`hi::UiBoxStyle` fields:

- `border`
- `border_radius`
- `underscored`
- `pad_top`
- `pad_left`
- `pad_right`
- `pad_bottom`

### TextField

Types:

- `hi::TextFieldStyle`
- `hi::TextFieldDraw`
- `hi::TextFieldState`

Method:

- `TextFieldState TextField(const TextFieldDraw&)`

Supports:

- UTF-8 text input
- mouse selection
- copy/cut/paste
- replace-selection-on-type behavior
- cursor/selection feedback in immediate-mode rendering

`TextFieldDraw` uses mutable buffer input:

- `io::char_view_mut text`
- optional `io::usize* text_len`
- optional stable `id`

### Slider

Types:

- `hi::SliderStyle`
- `hi::SliderDraw`
- `hi::SliderState`

Method:

- `SliderState Slider(const SliderDraw&)`

Behavior:

- binds to `float* value`
- supports min/max/step
- value editing by dragging in widget bounds
- text and slider state share the same styled box pass

### 5.7 Event loop model

Typical frame:

1. `while (win.PollEvents())`
2. `win.Render()`

Inside `onRender(dt)`:

1. clear/backdrop drawing
2. widget calls (`Button`, `TextField`, `Slider`, `DrawText`)
3. `FlushText()`

---

## 6. `audiocontext.hpp` API

### 6.1 Core model

Main enums:

- `ac::MusicState` - `Stillness`, `Wander`, `Strain`, `Threat`, `Unreality`
- `ac::Category` - all music states + `Ambient`, `SFX`, `UI`

Audio context owns:

- output device (`DeviceOptions`)
- asset registry (directory scan + direct register)
- mixer with fixed voice budget (64 voices)
- scheduler with silence/cooldown/transition/filter/duck policies

### 6.2 Initialization and loading

Primary flow:

- `AudioContext::Init(options, audio_root, fades, fade_count)`
- `RegisterAudioRoot(...)` or `RegisterDefaultAudioRoots()`
- category directories are auto-mapped:
- `stillness/`, `wander/`, `strain/`, `threat/`, `unreality/`, `ambient/`, `sfx/`, `ui/`

Streaming policy:

- music + ambient stream from disk
- `SFX` + `UI` decode to memory

### 6.3 High-level game-facing API

Mood and callbacks:

- `SetMood(const ac::Mood&)`
- `CurrentMood()`
- `SetGameCallbacks(const ac::GameCallbacks&)`
- `ClearGameCallbacks()`
- `Update(elapsed_ms)` (scheduler tick)

Playback helpers:

- `PlayMusic(...)`, `PlayMusicAsset(...)`
- `PlayAmbient(...)`, `PlayAmbientAsset(...)`
- `PlaySfx(...)`, `PlayUi(...)`
- `PlayRandom(...)`, `PlayAsset(...)`
- force variants: `ForceMusicState(...)`, `ForceMusicAsset(...)`, `ForceAmbient(...)`, `ForceAmbientAsset(...)`

Control and diagnostics:

- `StopMusic()`, `StopCategory(...)`, `StopAll()`
- `SetSchedulerPolicy(...)`
- `CollectRuntimeSnapshot(...)`
- `ActiveVoiceCount()`, `ScheduledEventCount()`

### 6.4 Scheduler callback layers

Legacy callbacks (`SchedulerCallbacks`) stay available for compatibility.

Preferred callbacks (`GameCallbacks`) receive `const ac::Mood&` directly:

- music selector
- ambient selector
- music filter
- ambient filter
- directive callback

The scheduler still enforces internal rules:

- allowed music transition graph
- hold/cooldown checks
- silence windows
- ducking/filter smoothing

---

## 7. Detailed Usage Examples

### 7.1 Minimal setup

```cpp
#define IO_IMPLEMENTATION
#include "hi/audiocontext.hpp"

int main() {
    ac::AudioContext audio{};
    const ac::Fade fades[] = {
        { ac::MusicState::Stillness, 10, 10 },
        { ac::MusicState::Wander, 10, 10 },
        { ac::MusicState::Strain, 10, 10 },
        { ac::MusicState::Threat, 10, 10 },
        { ac::MusicState::Unreality, 10, 10 }
    };
    if (!audio.Init(ac::DeviceOptions{}, io::char_view{ "resources/audio" }, fades, 5))
        return 1;
    audio.SetMasterGain(1.0f);
    return 0;
}
```

### 7.2 Mood-driven scheduler usage

```cpp
struct GameAudioDriver {
    ac::AudioContext* audio = nullptr;
    ac::Mood mood{};
};

static void MusicSelect(void* user, io::u32 elapsed_ms, const ac::Mood& mood, ac::MusicSelection& out) noexcept {
    (void)elapsed_ms;
    (void)user;
    out.request_play = true;
    out.state = (mood.threat > 0.5f) ? ac::MusicState::Threat : ac::MusicState::Stillness;
    out.activation = (mood.threat > 0.5f) ? mood.threat : mood.stillness;
}

int TickAudio(GameAudioDriver& d, io::u32 elapsed_ms) {
    d.audio->SetMood(d.mood);
    return d.audio->Update(elapsed_ms).music_started ? 1 : 0;
}
```

### 7.3 One-shot SFX and UI

```cpp
audio.PlaySfx(io::char_view{ "sfx1" }, 0.8f, -0.2f, 1.0f, 0.00);
audio.PlaySfx(io::char_view{ "sfx2" }, 0.8f,  0.2f, 1.0f, 0.12);
audio.PlayUi(io::char_view{ "click" }, 1.0f, 0.0f, 1.0f, 0.00);
```

### 7.4 Audio tests and what they verify

Implemented example tests in `examples/tests/`:

- `test_audio` - basic `AudioContext` lifecycle, gains clamp, init/shutdown sanity.
- `test_wav` - simple playback smoke test (`SFX` + one music stream).
- `test_fade_in_out` - verifies fade-in/fade-out and explicit music crossfade overlap behavior.
- `test_audio_mixer` - parallel playback layers: music + ambient + delayed SFX scheduling.
- `test_audio_scheduler` - long scenario test with `Mood`, transition-graph behavior, silence windows, callbacks, runtime state logging.

---

## 8. `gl_loader.hpp` API

`gl_loader.hpp` provides:

- OpenGL enum wrappers
- dynamic function loading
- thin wrappers around GL calls
- utility RAII classes:
- `gl::Shader`
- `gl::Buffer`
- `gl::VertexArray`
- `gl::MeshBinder`

This layer is designed to stay close to raw OpenGL while keeping API names scoped and type-checked.

---

## 9. Error Handling and ABI Expectations

- No exception-based error flow is required.
- APIs mostly report status via booleans/enums/return codes.
- Lifetime and ownership are explicit.
- Freestanding configurations disable RTTI/exceptions by design.

---

## 10. Compatibility Notes

- Windows and Linux are active targets.
- API is under active development; behavior is stable, names can evolve.
- When upgrading, re-check:
- `CMakePresets.json`
- callback signatures (for example `onRender(float dt)`)
- widget struct field defaults
