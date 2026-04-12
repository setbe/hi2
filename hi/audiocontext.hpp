
#ifndef AUDIOCONTEXT_HPP
#define AUDIOCONTEXT_HPP

#include "io.hpp"

#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
#   include <mmsystem.h>
#endif

namespace ac {

enum class MusicState : io::u8 {
    Stillness = 0,
    Wander,
    Strain,
    Threat,
    Unreality,
    Count
};

enum class Category : io::u8 {
    Stillness = 0,
    Wander,
    Strain,
    Threat,
    Unreality,
    Ambient,
    SFX,
    UI,
    Count
};

static IO_CONSTEXPR Category ToCategory(MusicState state) noexcept {
    return static_cast<Category>(static_cast<io::u8>(state));
}

struct DeviceOptions {
    io::u32 sample_rate = 48000;
    io::u32 channels = 2;
    io::u32 period_size_in_frames = 0;
    io::u32 periods = 0;
};

struct Fade {
    MusicState state = MusicState::Stillness;
    io::u32 in_secs = 0;
    io::u32 out_secs = 0;
};

struct OwnedAssetBytes {
    io::string id{};
    io::vector<io::u8> bytes{};
};

struct CategoryAssetGroup {
    Category category = Category::SFX;
    io::vector<OwnedAssetBytes> assets{};
};

struct PlayRequest {
    Category category = Category::SFX;
    io::char_view asset_id{};
    float gain = 1.0f;
    float pan = 0.0f;
    float pitch = 1.0f;
    double delay_seconds = 0.0;
};

struct FilterParams {
    float gain = 1.0f;
    float low_pass = 1.0f;
    float mid_pass = 1.0f;
    float high_pass = 1.0f;
};

struct MusicSelection {
    bool request_play = false;
    bool force = false;
    bool bypass_silence = false;
    MusicState state = MusicState::Stillness;
    float activation = 1.0f;
    io::char_view asset_id{};
    float gain = 1.0f;
    float pan = 0.0f;
    float pitch = 1.0f;
};

struct AmbientSelection {
    bool request_play = false;
    bool allow_overlap = false;
    bool bypass_silence = false;
    float activation = 1.0f;
    io::char_view asset_id{};
    float gain = 1.0f;
    float pan = 0.0f;
    float pitch = 1.0f;
};

struct SchedulerPolicy {
    bool use_music_transition_graph = true;
    io::u32 music_min_hold_ms = 900;

    io::u32 music_asset_cooldown_ms = 1200;
    io::u32 ambient_asset_cooldown_ms = 2000;

    io::u32 music_silence_secs = 0;
    io::u32 music_rand_silence_secs = 0;
    io::u32 ambient_silence_secs = 0;
    io::u32 ambient_rand_silence_secs = 0;

    float music_enter_threshold[static_cast<io::usize>(MusicState::Count)] = { 0.60f, 0.60f, 0.60f, 0.60f, 0.60f };
    float music_exit_threshold[static_cast<io::usize>(MusicState::Count)] = { 0.45f, 0.45f, 0.45f, 0.45f, 0.45f };

    float filter_smoothing = 0.08f;
    float low_alpha = 0.06f;
    float high_alpha = 0.88f;

    float music_duck_by_sfx = 0.88f;
    float music_duck_by_ui = 0.72f;
    float ambient_duck_by_sfx = 0.80f;
    float ambient_duck_by_ui = 0.60f;
    float duck_smoothing = 0.10f;
};

enum class DirectiveType : io::u8 {
    None = 0,
    ForceMusicState,
    ForceMusicAsset,
    ForceAmbient,
    ForceAmbientAsset,
    StopMusic,
    StopAmbient,
    SetMusicSilenceMs,
    SetAmbientSilenceMs
};

struct Directive {
    DirectiveType type = DirectiveType::None;
    MusicState music_state = MusicState::Stillness;
    io::char_view asset_id{};
    bool force = true;
    bool allow_ambient_overlap = true;
    bool bypass_silence = true;
    float gain = 1.0f;
    float pan = 0.0f;
    float pitch = 1.0f;
    io::u32 silence_ms = 0;
    io::u32 hold_ms = 0;
};

struct SchedulerTickResult {
    bool directive_consumed = false;
    bool directive_applied = false;
    bool music_started = false;
    bool ambient_started = false;
};

struct RuntimeSnapshot {
    io::u32 active_voices = 0;
    io::u32 voice_capacity = 0;
    io::u32 scheduled_events = 0;
    io::u32 by_category[static_cast<io::usize>(Category::Count)]{};
    bool has_active_music_state = false;
    MusicState active_music_state = MusicState::Stillness;
    bool music_silence_active = false;
    bool ambient_silence_active = false;
};

struct Mood {
    float stillness = 0.0f;
    float wander = 0.0f;
    float strain = 0.0f;
    float threat = 0.0f;
    float unreality = 0.0f;
    float ambient = 0.0f;
};

using MusicSelectionCallback = void(*)(void* user, io::u32 elapsed_ms, MusicSelection& out);
using AmbientSelectionCallback = void(*)(void* user, io::u32 elapsed_ms, AmbientSelection& out);
using MusicFilterCallback = void(*)(void* user, io::u32 elapsed_ms, MusicState state, FilterParams& out);
using AmbientFilterCallback = void(*)(void* user, io::u32 elapsed_ms, FilterParams& out);
using DirectiveCallback = void(*)(void* user, io::u32 elapsed_ms, Directive& out);
using DebugVoiceVisitor = void(*)(void* user, Category category, io::char_view asset_id, io::u64 cursor_frames, io::u64 total_frames, bool streamed);
using GameMusicSelectionCallback = void(*)(void* user, io::u32 elapsed_ms, const Mood& mood, MusicSelection& out);
using GameAmbientSelectionCallback = void(*)(void* user, io::u32 elapsed_ms, const Mood& mood, AmbientSelection& out);
using GameMusicFilterCallback = void(*)(void* user, io::u32 elapsed_ms, const Mood& mood, MusicState state, FilterParams& out);
using GameAmbientFilterCallback = void(*)(void* user, io::u32 elapsed_ms, const Mood& mood, FilterParams& out);
using GameDirectiveCallback = void(*)(void* user, io::u32 elapsed_ms, const Mood& mood, Directive& out);

struct SchedulerCallbacks {
    void* user = nullptr;
    MusicSelectionCallback music_selection = nullptr;
    AmbientSelectionCallback ambient_selection = nullptr;
    MusicFilterCallback music_filter = nullptr;
    AmbientFilterCallback ambient_filter = nullptr;
    DirectiveCallback directive = nullptr;
};

struct GameCallbacks {
    void* user = nullptr;
    GameMusicSelectionCallback music_selection = nullptr;
    GameAmbientSelectionCallback ambient_selection = nullptr;
    GameMusicFilterCallback music_filter = nullptr;
    GameAmbientFilterCallback ambient_filter = nullptr;
    GameDirectiveCallback directive = nullptr;
};

namespace detail {

struct DeviceConfig {
    io::u32 sample_rate = 48000;
    io::u32 channels = 2;
    io::u32 frames_per_buffer = 512;
    io::u32 buffer_count = 3;
};

class Device {
public:
    using DataCallback = void(*)(void* user, float* output, io::u32 frames);

    Device() noexcept = default;
    ~Device() noexcept { Uninit(); }

    Device(const Device&) noexcept = delete;
    Device& operator=(const Device&) noexcept = delete;

    bool Init(const DeviceConfig& cfg, DataCallback cb, void* user) noexcept {
        if (_initialized) return true;
        if (cb == nullptr) return false;
        DeviceConfig safe = cfg;
        if (safe.sample_rate == 0) safe.sample_rate = 48000;
        if (safe.channels == 0) safe.channels = 2;
        if (safe.frames_per_buffer == 0) safe.frames_per_buffer = 512;
        if (safe.buffer_count < 2) safe.buffer_count = 3;

        const io::usize sample_count = static_cast<io::usize>(safe.frames_per_buffer) * safe.channels;
        if (!_mix_buffer.resize(sample_count))
            return false;

        _cfg = safe;
        _callback = cb;
        _user = user;

        if (!BackendInit()) {
            _mix_buffer.clear();
            _callback = nullptr; _user = nullptr;
            return false;
        }
        _initialized = true;
        return true;
    }

    bool Start() noexcept {
        if (!_initialized) return false;
        if (_running.load(io::memory_order_acquire) != 0)
            return true;
        _running.store(1, io::memory_order_release);

        if (!_thread.start(&Device::ThreadEntry, this)) {
            _running.store(0, io::memory_order_release);
            return false;
        }

        return true;
    }

    void Stop() noexcept {
        if (!_initialized) return;
        _running.store(0, io::memory_order_release);
        if (_thread.running())
            (void)_thread.join();
        BackendStop();
    }

    void Uninit() noexcept {
        if (!_initialized) return;
        Stop(); BackendUninit();
        _mix_buffer.clear();
        _callback = nullptr; _user = nullptr; _initialized = false;
    }

    bool IsInitialized() const noexcept {
        return _initialized;
    }

    io::u32 SampleRate() const noexcept {
        return _cfg.sample_rate;
    }

    io::u32 Channels() const noexcept {
        return _cfg.channels;
    }

private:
    static io::u32 PeriodMs(const DeviceConfig& cfg) noexcept {
        if (cfg.sample_rate == 0)
            return 10;
        const io::u64 numer = io::mul_u32(cfg.frames_per_buffer, 1000u);
        io::u32 ms = io::div_u64_u32(numer, cfg.sample_rate);
        if (ms == 0)
            ms = 1;
        return ms;
    }

    static void ThreadEntry(void* p) noexcept {
        Device* self = reinterpret_cast<Device*>(p);
        if (self != nullptr)
            self->ThreadLoop();
    }

    void ThreadLoop() noexcept {
        const io::usize sample_count = static_cast<io::usize>(_cfg.frames_per_buffer) * _cfg.channels;

        while (_running.load(io::memory_order_acquire) != 0) {
            for (io::usize i=0; i<sample_count; ++i)
                _mix_buffer[i] = 0.0f;

            if (_callback != nullptr)
                _callback(_user, _mix_buffer.data(), _cfg.frames_per_buffer);

            if (!BackendSubmit(_mix_buffer))
                io::sleep_ms(PeriodMs(_cfg));
        }
    }

#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
    struct WinBuffer {
        io::vector<io::i16> pcm{};
        WAVEHDR hdr{};
        bool prepared = false;
        bool queued = false;
    };
#endif

    bool BackendInit() noexcept {
#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
        if (_is_backend_ready)
            return true;

        WAVEFORMATEX fmt{};
        fmt.wFormatTag = WAVE_FORMAT_PCM;
        fmt.nChannels = static_cast<WORD>(_cfg.channels);
        fmt.nSamplesPerSec = static_cast<DWORD>(_cfg.sample_rate);
        fmt.wBitsPerSample = 16;
        fmt.nBlockAlign = static_cast<WORD>((fmt.nChannels * fmt.wBitsPerSample) / 8);
        fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
        fmt.cbSize = 0;

        MMRESULT res = waveOutOpen(&_wave_out, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
        if (res != MMSYSERR_NOERROR) {
            _wave_out = nullptr;
            return false;
        }

        if (!_win_buffers.resize(_cfg.buffer_count)) {
            waveOutClose(_wave_out);
            _wave_out = nullptr;
            return false;
        }

        const io::usize sample_count = static_cast<io::usize>(_cfg.frames_per_buffer) * _cfg.channels;

        for (io::usize i=0; i<_win_buffers.size(); ++i) {
            WinBuffer& b = _win_buffers[i];
            if (!b.pcm.resize(sample_count)) {
                BackendUninit();
                return false;
            }

            b.hdr = {};
            b.hdr.lpData = reinterpret_cast<LPSTR>(b.pcm.data());
            b.hdr.dwBufferLength = static_cast<DWORD>(sample_count * sizeof(io::i16));
            b.hdr.dwFlags = 0;
            b.hdr.dwLoops = 0;

            res = waveOutPrepareHeader(_wave_out, &b.hdr, sizeof(WAVEHDR));
            if (res != MMSYSERR_NOERROR) {
                BackendUninit();
                return false;
            }

            b.prepared = true;
            b.queued = false;
        }

        _next_win_buff = 0;
        _is_backend_ready = true;
#endif
        return true;
    }

    void BackendStop() noexcept {
#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
        if (!_is_backend_ready || _wave_out == nullptr)
            return;

        (void)waveOutReset(_wave_out);

        for (io::usize i=0; i<_win_buffers.size(); ++i) {
            WinBuffer& b = _win_buffers[i];
            if (b.prepared && b.queued) {
                for (;;) {
                    if ((b.hdr.dwFlags & WHDR_DONE) != 0)
                        break;
                    io::sleep_ms(1);
                }
            }
            b.queued = false;
        }
#endif
    }

    void BackendUninit() noexcept {
#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
        if (!_is_backend_ready)
            return;

        if (_wave_out != nullptr)
            (void)waveOutReset(_wave_out);

        for (io::usize i=0; i<_win_buffers.size(); ++i) {
            WinBuffer& b = _win_buffers[i];
            if (_wave_out != nullptr && b.prepared)
                (void)waveOutUnprepareHeader(_wave_out, &b.hdr, sizeof(WAVEHDR));
            b.prepared = false;
            b.queued = false;
            b.pcm.clear();
        }

        _win_buffers.clear();

        if (_wave_out != nullptr) {
            (void)waveOutClose(_wave_out);
            _wave_out = nullptr;
        }

        _next_win_buff = 0;
        _is_backend_ready = false;
#endif
    }

    static io::i16 FloatToI16(float v) noexcept {
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;

        io::i32 q = static_cast<io::i32>(v * 32767.0f);
        if (q > 32767) q = 32767;
        if (q < -32768) q = -32768;
        return static_cast<io::i16>(q);
    }

    bool BackendSubmit(const io::vector<float>& interleaved) noexcept {
#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
        if (!_is_backend_ready || _wave_out == nullptr || _win_buffers.empty())
            return false;

        WinBuffer& b = _win_buffers[_next_win_buff];

        if (b.queued) {
            while (_running.load(io::memory_order_acquire) != 0) {
                if ((b.hdr.dwFlags & WHDR_DONE) != 0)
                    break;
                io::sleep_ms(1);
            }

            if ((b.hdr.dwFlags & WHDR_DONE) == 0)
                return false;

            b.queued = false;
        }

        const io::usize sample_count = b.pcm.size();
        if (interleaved.size() < sample_count)
            return false;

        for (io::usize i=0; i<sample_count; ++i)
            b.pcm[i] = FloatToI16(interleaved[i]);

        b.hdr.dwBufferLength = static_cast<DWORD>(sample_count * sizeof(io::i16));
        b.hdr.dwFlags &= ~WHDR_DONE;

        const MMRESULT res = waveOutWrite(_wave_out, &b.hdr, sizeof(WAVEHDR));
        if (res != MMSYSERR_NOERROR)
            return false;

        b.queued = true;

        ++_next_win_buff;
        if (_next_win_buff >= _win_buffers.size())
            _next_win_buff = 0;

        return true;
#else
        (void)interleaved;
        return false;
#endif
    }

private:
    DeviceConfig _cfg{};
    DataCallback _callback = nullptr;
    void* _user = nullptr;

    io::atomic<io::u32> _running{ 0 };
    io::Thread _thread{};

    io::vector<float> _mix_buffer{};

#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
    HWAVEOUT _wave_out = nullptr;
    io::vector<WinBuffer> _win_buffers{};
    io::usize _next_win_buff = 0;
    bool _is_backend_ready = false;
#endif

    bool _initialized = false;
};

} // namespace detail

class AudioContext {
public:
    AudioContext() noexcept
        : _initialized(false),
          _out_sample_rate(48000),
          _out_channels(2),
          _master_gain(1.0f),
          _frames_mixed(0),
          _schedule_seq(0) {
        for (io::usize i=0; i<CATEGORY_COUNT; ++i) {
            _bus_gains[i] = 1.0f;
            _filter_gains[i] = 1.0f;
            _filter_gain_current[i] = 1.0f;
            _filter_low_targets[i] = 1.0f;
            _filter_mid_targets[i] = 1.0f;
            _filter_high_targets[i] = 1.0f;
            _filter_low_current[i] = 1.0f;
            _filter_mid_current[i] = 1.0f;
            _filter_high_current[i] = 1.0f;
            _round_robin_index[i] = 0;
            _last_started_asset[i] = nullptr;
            _voice_end_counter[i] = 0;
            _voice_end_seen_counter[i] = 0;
            _voice_end_played_frames[i] = 0;
            _voice_end_natural[i] = 0;
        }
        for (io::usize i=0; i<MUSIC_STATE_COUNT; ++i) {
            _music_fade_in_secs[i] = 0;
            _music_fade_out_secs[i] = 0;
        }
        ClampSchedulerPolicyLocked();
    }

    ~AudioContext() noexcept {
        Shutdown();
    }

    AudioContext(const AudioContext&) noexcept = delete;
    AudioContext& operator=(const AudioContext&) noexcept = delete;

    bool Init(const DeviceOptions& options = DeviceOptions(), io::char_view audio_root = io::char_view{}, const Fade* fades = nullptr, io::usize fade_count = 0) noexcept {
        if (_initialized)
            return true;
        ApplyMusicFades(fades, fade_count);

        detail::DeviceConfig cfg{};
        cfg.sample_rate = (options.sample_rate > 0) ?
                          options.sample_rate : 48000;
        cfg.channels = (options.channels > 0) ?
                        options.channels : 2;
        cfg.frames_per_buffer = (options.period_size_in_frames > 0) ?
                               options.period_size_in_frames : 512;
        cfg.buffer_count = (options.periods > 0) ?
                           options.periods : 3;

        if (!_device.Init(cfg, &AudioContext::DeviceCallback, this))
            return false;

        _out_sample_rate = _device.SampleRate();
        _out_channels = _device.Channels();

        if (!_device.Start()) {
            _device.Uninit();
            return false;
        }

        _initialized = true;

        if (audio_root.empty()) {
            (void)RegisterDefaultAudioRoots();
        } else {
            (void)RegisterAudioRoot(audio_root);
        }

        return true;
    }

    void Shutdown() noexcept {
        if (!_initialized)
            return;

        _device.Stop();

        {
            MutexGuard lock(&_mutex);
            _voices.clear();
            _events.clear();
            for (io::usize i=0; i<CATEGORY_COUNT; ++i) {
                _assets[i].clear();
                _round_robin_index[i] = 0;
                _filter_gains[i] = 1.0f;
                _filter_gain_current[i] = 1.0f;
                _filter_low_targets[i] = 1.0f;
                _filter_mid_targets[i] = 1.0f;
                _filter_high_targets[i] = 1.0f;
                _filter_low_current[i] = 1.0f;
                _filter_mid_current[i] = 1.0f;
                _filter_high_current[i] = 1.0f;
                _last_started_asset[i] = nullptr;
                _voice_end_counter[i] = 0;
                _voice_end_seen_counter[i] = 0;
                _voice_end_played_frames[i] = 0;
                _voice_end_natural[i] = 0;
            }
            _scheduler_callbacks = SchedulerCallbacks{};
            _game_callbacks = GameCallbacks{};
            _mood = Mood{};
            _last_music_state = MusicState::Stillness;
            _scheduler_has_music_state = false;
        }

        _device.Uninit();
        _music_silence_until_frame.store(0, io::memory_order_release);
        _ambient_silence_until_frame.store(0, io::memory_order_release);
        _directive_hold_until_frame.store(0, io::memory_order_release);
        _music_state_hold_until_frame = 0;
        _music_duck_current = 1.0f;
        _ambient_duck_current = 1.0f;
        _initialized = false;
    }

    bool IsInitialized() const noexcept {
        return _initialized;
    }

    bool RegisterAudioRoot(io::char_view audio_root) noexcept {
        if (!_initialized || audio_root.empty())
            return false;

        bool any_found = false;
        for (io::u32 i = 0; i < static_cast<io::u32>(CATEGORY_COUNT); ++i) {
            const Category category = static_cast<Category>(i);
            io::string category_path{};
            if (!fs::path_join(audio_root, CategoryDirectoryName(category), category_path))
                continue;

            if (RegisterCategoryDirectory(category, category_path.as_view()))
                any_found = true;
        }
        return any_found;
    }

    bool RegisterDefaultAudioRoots() noexcept {
        if (!_initialized)
            return false;

        const io::char_view roots[] = {
            io::char_view{ "resources/audio" },
            io::char_view{ "../resources/audio" },
            io::char_view{ "../../resources/audio" },
            io::char_view{ "../../../resources/audio" },
            io::char_view{ "../../../../resources/audio" }
        };

        for (io::usize i=0; i<(sizeof(roots) / sizeof(roots[0])); ++i) {
            if (!fs::is_directory(roots[i]))
                continue;
            return RegisterAudioRoot(roots[i]);
        }

#ifdef _DEBUG
        io::char_view current_category_search{ "resources/audio" };
        io::out << "Coudn't find any audio for category" << current_category_search << '\n';
#endif
        return false;
    }

    bool RegisterCategoryDirectory(Category category, io::char_view directory) noexcept {
        if (!_initialized || directory.empty())
            return false;

        bool found_any = false;
        bool all_ok = true;

        io::vector<io::string> pending{};
        io::string root{};
        if (!root.append(directory))
            return false;
        if (!pending.push_back(io::move(root)))
            return false;

        while (!pending.empty()) {
            io::string current = io::move(pending.back());
            pending.pop_back();

            for (fs::directory_iterator it{ current.as_view() }; !it.is_end(); ++it) {
                const fs::directory_entry& entry = *it;
                if (entry.type == fs::file_type::directory) {
                    io::string next{};
                    if (!next.append(entry.path)) {
                        all_ok = false;
                        continue;
                    }
                    if (!pending.push_back(io::move(next)))
                        all_ok = false;
                    continue;
                }
                if (entry.type != fs::file_type::regular)
                    continue;
                if (!HasWavExtension(entry.path))
                    continue;

                io::string asset_id{};
                if (!BuildAssetIdFromPath(entry.path, asset_id)) {
                    all_ok = false;
                    continue;
                }

                found_any = true;
                if (!RegisterAssetFile(category, asset_id.as_view(), entry.path))
                    all_ok = false;
            }
        }

#ifdef _DEBUG
        if (!found_any) {
            io::char_view current_category_search = directory;
            io::out << "Coudn't find any audio for category" << current_category_search << '\n';
        }
#endif
        return found_any && all_ok;
    }

    bool RegisterAssetFile(Category category, io::char_view asset_id, io::char_view path) noexcept {
        if (!_initialized || asset_id.empty() || path.empty())
            return false;

        Asset decoded{};
        if (!decoded.id.append(asset_id))
            return false;
        if (!decoded.path.append(path))
            return false;

        if (CategoryStreamsFromDisk(category)) {
            if (!LoadWavMetadataFromFile(path, decoded)) return false;
            decoded.storage = AssetStorage::Stream;
        } else {
            if (!DecodeWavFromFile(path, decoded)) return false;
            decoded.storage = AssetStorage::Memory;
        }

        MutexGuard lock(&_mutex);
        return UpsertAssetLocked(CategoryIndex(category), decoded);
    }

    bool RegisterAssetBytes(Category category, io::char_view asset_id, const void* bytes, io::usize byte_count) noexcept {
        if (!_initialized || asset_id.empty() || bytes==nullptr || byte_count==0)
            return false;
        Asset decoded{};
        if (!decoded.id.append(asset_id)) return false;
        decoded.storage = AssetStorage::Memory;

        if (!DecodeWavFromMemory(bytes, byte_count, decoded)) return false;

        MutexGuard lock(&_mutex);
        return UpsertAssetLocked(CategoryIndex(category), decoded);
    }

    bool RegisterAssetBytes(Category category, const io::string& asset_id, const void* bytes, io::usize byte_count) noexcept {
        return RegisterAssetBytes(category, asset_id.as_view(), bytes, byte_count);
    }

    bool RegisterAssetBytes(Category category, const OwnedAssetBytes& asset) noexcept {
        if (asset.bytes.empty()) return false;
        return RegisterAssetBytes(category, asset.id.as_view(), asset.bytes.data(), asset.bytes.size());
    }

    bool RegisterAssetGroup(Category category, const io::vector<OwnedAssetBytes>& assets) noexcept {
        bool all_ok = true;
        for (io::usize i=0; i<assets.size(); ++i)
            if (!RegisterAssetBytes(category, assets[i]))
                all_ok = false;
        return all_ok;
    }

    bool RegisterGroups(const io::vector<CategoryAssetGroup>& groups) noexcept {
        bool all_ok = true;
        for (io::usize i=0; i<groups.size(); ++i)
            if (!RegisterAssetGroup(groups[i].category, groups[i].assets))
                all_ok = false;
        return all_ok;
    }

    void SetMood(const Mood& mood) noexcept {
        MutexGuard lock(&_mutex);
        _mood = mood;
        ClampMoodLocked(_mood);
    }

    Mood CurrentMood() noexcept {
        MutexGuard lock(&_mutex);
        return _mood;
    }

    void SetGameCallbacks(const GameCallbacks& callbacks) noexcept {
        MutexGuard lock(&_mutex);
        _game_callbacks = callbacks;
    }

    void ClearGameCallbacks() noexcept {
        MutexGuard lock(&_mutex);
        _game_callbacks = GameCallbacks{};
    }

    bool PlayMusic(MusicState state, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool force = false, bool bypass_silence = false) noexcept {
        return TryPlayMusic(state, force, bypass_silence, gain, pan, pitch);
    }

    bool PlayMusicAsset(MusicState state, io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool force = false, bool bypass_silence = false) noexcept {
        return TryPlayMusicAsset(state, asset_id, force, bypass_silence, gain, pan, pitch);
    }

    bool PlayAmbient(float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool allow_overlap = false, bool bypass_silence = false) noexcept {
        return TryPlayAmbient(allow_overlap, bypass_silence, gain, pan, pitch);
    }

    bool PlayAmbientAsset(io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool allow_overlap = false, bool bypass_silence = false) noexcept {
        return TryPlayAmbientAsset(asset_id, allow_overlap, bypass_silence, gain, pan, pitch);
    }

    bool PlaySfx(io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, double delay_seconds = 0.0) noexcept {
        PlayRequest request{};
        request.category = Category::SFX;
        request.asset_id = asset_id;
        request.gain = gain;
        request.pan = pan;
        request.pitch = pitch;
        request.delay_seconds = delay_seconds;
        return SchedulePlay(request);
    }

    bool PlayUi(io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, double delay_seconds = 0.0) noexcept {
        PlayRequest request{};
        request.category = Category::UI;
        request.asset_id = asset_id;
        request.gain = gain;
        request.pan = pan;
        request.pitch = pitch;
        request.delay_seconds = delay_seconds;
        return SchedulePlay(request);
    }

    bool PlayRandom(Category category, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        return PlayAny(category, gain, pan, pitch);
    }

    bool PlayAsset(Category category, io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, double delay_seconds = 0.0) noexcept {
        PlayRequest request{};
        request.category = category;
        request.asset_id = asset_id;
        request.gain = gain;
        request.pan = pan;
        request.pitch = pitch;
        request.delay_seconds = delay_seconds;
        return SchedulePlay(request);
    }

    bool ForceMusicState(MusicState state, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        return TryPlayMusic(state, true, true, gain, pan, pitch);
    }

    bool ForceMusicAsset(MusicState state, io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        return TryPlayMusicAsset(state, asset_id, true, true, gain, pan, pitch);
    }

    bool ForceAmbient(float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool allow_overlap = true) noexcept {
        return TryPlayAmbient(allow_overlap, true, gain, pan, pitch);
    }

    bool ForceAmbientAsset(io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f, bool allow_overlap = true) noexcept {
        return TryPlayAmbientAsset(asset_id, allow_overlap, true, gain, pan, pitch);
    }

    bool PlayNow(Category category, io::char_view asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        PlayRequest request{};
        request.category = category;
        request.asset_id = asset_id;
        request.gain = gain;
        request.pan = pan;
        request.pitch = pitch;
        request.delay_seconds = 0.0;
        return SchedulePlay(request);
    }

    bool PlayNow(Category category, const io::string& asset_id, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        return PlayNow(category, asset_id.as_view(), gain, pan, pitch);
    }

    bool PlayAny(Category category, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        if (!_initialized) return false;
        MutexGuard lock(&_mutex);

        const io::usize index = CategoryIndex(category);
        if (index >= CATEGORY_COUNT || _assets[index].empty()) return false;

        io::u32 rr = _round_robin_index[index];
        if (rr >= _assets[index].size()) rr = 0;
        Asset* asset = SelectAssetForPlayLocked(category, rr, _frames_mixed.load(io::memory_order_relaxed));
        if (asset == nullptr) return false;
        _round_robin_index[index] = rr;

        ScheduledEvent event{};
        event.start_frame = _frames_mixed.load(io::memory_order_relaxed);
        event.category = category;
        event.asset = asset;
        event.gain = Clamp(gain, 0.0f, 4.0f);
        event.pan = Clamp(pan, -1.0f, 1.0f);
        event.pitch = Clamp(pitch, 0.25f, 4.0f);
        event.sequence = ++_schedule_seq;
        return InsertEventLocked(event);
    }

    bool SchedulePlay(const PlayRequest& request) noexcept {
        if (!_initialized || request.asset_id.empty()) return false;
        MutexGuard lock(&_mutex);

        const io::usize index = CategoryIndex(request.category);
        Asset* asset = FindAssetLocked(index, request.asset_id);
        if (asset == nullptr)
            return false;

        io::u64 delay_frames = 0;
#if defined(IO_HAS_STD)
        const double safe_delay = request.delay_seconds < 0.0 ? 0.0 : request.delay_seconds;
        if (safe_delay > 0.0)
            delay_frames = static_cast<io::u64>(safe_delay * static_cast<double>(_out_sample_rate));
#endif
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);

        ScheduledEvent event{};
        event.start_frame = now_frame + delay_frames;
        event.category = request.category;
        event.asset = asset;
        event.gain = Clamp(request.gain, 0.0f, 4.0f);
        event.pan = Clamp(request.pan, -1.0f, 1.0f);
        event.pitch = Clamp(request.pitch, 0.25f, 4.0f);
        event.sequence = ++_schedule_seq;

        return InsertEventLocked(event);
    }

    SchedulerTickResult Update(io::u32 elapsed_ms) noexcept { return TickScheduler(elapsed_ms); }

    void SetSchedulerCallbacks(const SchedulerCallbacks& callbacks) noexcept {
        MutexGuard lock(&_mutex);
        _scheduler_callbacks = callbacks;
    }

    void SetSchedulerPolicy(const SchedulerPolicy& policy) noexcept {
        MutexGuard lock(&_mutex);
        _scheduler_policy = policy;
        ClampSchedulerPolicyLocked();
    }

    void SetMusicSilenceMs(io::u32 ms) noexcept {
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 add = MsToFrames(ms);
        _music_silence_until_frame.store(now_frame + add, io::memory_order_release);
    }

    void SetAmbientSilenceMs(io::u32 ms) noexcept {
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 add = MsToFrames(ms);
        _ambient_silence_until_frame.store(now_frame + add, io::memory_order_release);
    }

    bool IsMusicSilenceActive() const noexcept {
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 until_frame = _music_silence_until_frame.load(io::memory_order_acquire);
        return now_frame < until_frame;
    }

    bool IsAmbientSilenceActive() const noexcept {
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 until_frame = _ambient_silence_until_frame.load(io::memory_order_acquire);
        return now_frame < until_frame;
    }

    bool TryPlayMusic(MusicState state, bool force = false, bool bypass_silence = false, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        if (!_initialized) return false;
        if (!bypass_silence && IsMusicSilenceActive()) return false;
        if (force) StopMusic();
        else {
            MutexGuard lock(&_mutex);
            if (HasPendingMusicLocked()) return false;
        }
        const bool started = PlayAny(ToCategory(state), gain, pan, pitch);
        if (started) {
            _last_music_state = state;
            _scheduler_music_state = state;
            _scheduler_has_music_state = true;
            _music_state_hold_until_frame = _frames_mixed.load(io::memory_order_relaxed) + MsToFrames(_scheduler_policy.music_min_hold_ms);
        }
        return started;
    }

    bool TryPlayMusicAsset(MusicState state, io::char_view asset_id, bool force = false, bool bypass_silence = false, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        if (!_initialized || asset_id.empty()) return false;
        if (!bypass_silence && IsMusicSilenceActive()) return false;
        if (force) StopMusic();
        else {
            MutexGuard lock(&_mutex);
            if (HasPendingMusicLocked()) return false;
        }
        const bool started = PlayNow(ToCategory(state), asset_id, gain, pan, pitch);
        if (started) {
            _last_music_state = state;
            _scheduler_music_state = state;
            _scheduler_has_music_state = true;
            _music_state_hold_until_frame = _frames_mixed.load(io::memory_order_relaxed) + MsToFrames(_scheduler_policy.music_min_hold_ms);
        }
        return started;
    }

    bool TryPlayAmbient(bool allow_overlap = false, bool bypass_silence = false, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        if (!_initialized) return false;
        if (!bypass_silence && IsAmbientSilenceActive()) return false;
        if (!allow_overlap) {
            MutexGuard lock(&_mutex);
            if (HasPendingCategoryLocked(Category::Ambient)) return false;
        }
        return PlayAny(Category::Ambient, gain, pan, pitch);
    }

    bool TryPlayAmbientAsset(io::char_view asset_id, bool allow_overlap = false, bool bypass_silence = false, float gain = 1.0f, float pan = 0.0f, float pitch = 1.0f) noexcept {
        if (!_initialized || asset_id.empty()) return false;
        if (!bypass_silence && IsAmbientSilenceActive()) return false;
        if (!allow_overlap) {
            MutexGuard lock(&_mutex);
            if (HasPendingCategoryLocked(Category::Ambient)) return false;
        }
        return PlayNow(Category::Ambient, asset_id, gain, pan, pitch);
    }

    void StopMusic() noexcept {
        StopCategory(Category::Stillness);
        StopCategory(Category::Wander);
        StopCategory(Category::Strain);
        StopCategory(Category::Threat);
        StopCategory(Category::Unreality);
    }

    SchedulerTickResult TickScheduler(io::u32 elapsed_ms) noexcept {
        SchedulerTickResult result{};
        if (!_initialized) return result;

        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        SchedulerCallbacks callbacks{};
        GameCallbacks game_callbacks{};
        Mood mood{};
        MusicState active_music_state = _last_music_state;
        {
            MutexGuard lock(&_mutex);
            callbacks = _scheduler_callbacks;
            game_callbacks = _game_callbacks;
            mood = _mood;
            ConsumeEndedVoicesAndUpdateSilenceLocked(now_frame);
            if (!HasPendingMusicLocked())
                _scheduler_has_music_state = false;

            if (_scheduler_has_music_state) {
                active_music_state = _scheduler_music_state;
            } else {
                MusicState current_state{};
                if (TryGetActiveMusicStateLocked(current_state)) {
                    active_music_state = current_state;
                    _scheduler_music_state = current_state;
                    _scheduler_has_music_state = true;
                }
            }
        }

        Directive directive{};
        if (game_callbacks.directive != nullptr || callbacks.directive != nullptr) {
            if (game_callbacks.directive != nullptr)
                game_callbacks.directive(game_callbacks.user, elapsed_ms, mood, directive);
            else
                callbacks.directive(callbacks.user, elapsed_ms, directive);
            if (directive.type != DirectiveType::None) {
                result.directive_consumed = true;
                result.directive_applied = ApplyDirective(directive);
            }
            if (directive.hold_ms > 0)
                SetDirectiveHoldMs(directive.hold_ms);
        }

        if (IsDirectiveHoldActive())
            return result;

        if (game_callbacks.music_filter != nullptr || callbacks.music_filter != nullptr) {
            FilterParams filter{};
            if (game_callbacks.music_filter != nullptr)
                game_callbacks.music_filter(game_callbacks.user, elapsed_ms, mood, active_music_state, filter);
            else
                callbacks.music_filter(callbacks.user, elapsed_ms, active_music_state, filter);
            SetCategoryFilterParams(ToCategory(active_music_state), filter);
        }

        if (game_callbacks.ambient_filter != nullptr || callbacks.ambient_filter != nullptr) {
            FilterParams filter{};
            if (game_callbacks.ambient_filter != nullptr)
                game_callbacks.ambient_filter(game_callbacks.user, elapsed_ms, mood, filter);
            else
                callbacks.ambient_filter(callbacks.user, elapsed_ms, filter);
            SetCategoryFilterParams(Category::Ambient, filter);
        }

        if (game_callbacks.music_selection != nullptr || callbacks.music_selection != nullptr) {
            MusicSelection selection{};
            if (game_callbacks.music_selection != nullptr)
                game_callbacks.music_selection(game_callbacks.user, elapsed_ms, mood, selection);
            else
                callbacks.music_selection(callbacks.user, elapsed_ms, selection);
            if (selection.request_play) {
                MusicState target_state = selection.state;
                bool can_start = true;
                bool force_start = selection.force;

                if (!selection.force && selection.activation < EnterThreshold(target_state))
                    can_start = false;

                if (can_start && _scheduler_has_music_state) {
                    const MusicState current_state = _scheduler_music_state;
                    if (target_state != current_state && _scheduler_policy.use_music_transition_graph)
                        target_state = ResolveMusicTransitionStep(current_state, target_state);
                    if (!selection.force && target_state != current_state) {
                        if (now_frame < _music_state_hold_until_frame)
                            can_start = false;
                        else
                            force_start = true;
                    }
                }

                if (selection.asset_id.empty())
                    result.music_started = can_start && TryPlayMusic(target_state, force_start, selection.bypass_silence, selection.gain, selection.pan, selection.pitch);
                else
                    result.music_started = can_start && TryPlayMusicAsset(target_state, selection.asset_id, force_start, selection.bypass_silence, selection.gain, selection.pan, selection.pitch);

                if (result.music_started) {
                    _scheduler_music_state = target_state;
                    _scheduler_has_music_state = true;
                    _music_state_hold_until_frame = now_frame + MsToFrames(_scheduler_policy.music_min_hold_ms);
                }
            }
        }

        if (game_callbacks.ambient_selection != nullptr || callbacks.ambient_selection != nullptr) {
            AmbientSelection selection{};
            if (game_callbacks.ambient_selection != nullptr)
                game_callbacks.ambient_selection(game_callbacks.user, elapsed_ms, mood, selection);
            else
                callbacks.ambient_selection(callbacks.user, elapsed_ms, selection);
            if (selection.request_play) {
                const bool can_start = selection.activation >= 0.20f;
                if (selection.asset_id.empty())
                    result.ambient_started = can_start && TryPlayAmbient(selection.allow_overlap, selection.bypass_silence, selection.gain, selection.pan, selection.pitch);
                else
                    result.ambient_started = can_start && TryPlayAmbientAsset(selection.asset_id, selection.allow_overlap, selection.bypass_silence, selection.gain, selection.pan, selection.pitch);
            }
        }
        return result;
    }

    void StopAll() noexcept {
        if (!_initialized) return;
        MutexGuard lock(&_mutex);
        _voices.clear();
        _events.clear();
    }

    void StopCategory(Category category) noexcept {
        if (!_initialized) return;
        MutexGuard lock(&_mutex);

        for (io::usize i=0; i<_voices.size(); ++i)
            if (_voices[i].active && _voices[i].category == category)
                if (!RequestMusicFadeOut(_voices[i]))
                    EndVoiceLocked(_voices[i], false);

        io::usize dst = 0;
        for (io::usize i=0; i<_events.size(); ++i) {
            if (_events[i].category == category) continue;
            if (dst != i) _events[dst] = _events[i];
            ++dst;
        }
        (void)_events.resize(dst);
    }
    void SetMasterGain(float gain) noexcept { _master_gain = Clamp(gain, 0.0f, 4.0f); }
    float MasterGain() const noexcept { return _master_gain; }
    void SetCategoryGain(Category category, float gain) noexcept {
        const io::usize index = CategoryIndex(category);
        if (index < CATEGORY_COUNT)
            _bus_gains[index] = Clamp(gain, 0.0f, 4.0f);
    }

    void SetCategoryFilterGain(Category category, float gain) noexcept {
        const io::usize index = CategoryIndex(category);
        if (index < CATEGORY_COUNT)
            _filter_gains[index] = Clamp(gain, 0.0f, 4.0f);
    }

    void SetCategoryFilterParams(Category category, const FilterParams& params) noexcept {
        const io::usize index = CategoryIndex(category);
        if (index >= CATEGORY_COUNT) return;
        _filter_gains[index] = Clamp(params.gain, 0.0f, 4.0f);
        _filter_low_targets[index] = Clamp(params.low_pass, 0.0f, 4.0f);
        _filter_mid_targets[index] = Clamp(params.mid_pass, 0.0f, 4.0f);
        _filter_high_targets[index] = Clamp(params.high_pass, 0.0f, 4.0f);
    }

    float CategoryGain(Category category) const noexcept {
        const io::usize index = CategoryIndex(category);
        if (index < CATEGORY_COUNT) return _bus_gains[index];
        return 1.0f;
    }

    float CategoryFilterGain(Category category) const noexcept {
        const io::usize index = CategoryIndex(category);
        if (index < CATEGORY_COUNT) return _filter_gains[index];
        return 1.0f;
    }
    io::u32 OutputSampleRate() const noexcept { return _out_sample_rate; }
    io::u32 OutputChannels() const noexcept { return _out_channels; }
    io::u32 VoiceCapacity() const noexcept { return static_cast<io::u32>(MAX_VOICES); }
    io::u32 ActiveVoiceCount() noexcept {
        if (!_initialized) return 0;
        MutexGuard lock(&_mutex);
        io::u32 count = 0;
        for (io::usize i=0; i<_voices.size(); ++i)
            if (_voices[i].active)
                ++count;
        return count;
    }

    io::u32 ActiveVoiceCount(Category category) noexcept {
        if (!_initialized) return 0;
        MutexGuard lock(&_mutex);
        io::u32 count = 0;
        for (io::usize i=0; i<_voices.size(); ++i)
            if (_voices[i].active && _voices[i].category == category)
                ++count;
        return count;
    }

    io::u32 ScheduledEventCount() noexcept {
        if (!_initialized) return 0;
        MutexGuard lock(&_mutex);
        return static_cast<io::u32>(_events.size());
    }

    bool TryGetActiveMusicState(MusicState& out_state) noexcept {
        if (!_initialized) return false;
        MutexGuard lock(&_mutex);
        return TryGetActiveMusicStateLocked(out_state);
    }

    void DebugVisitActiveVoices(DebugVoiceVisitor visitor, void* user = nullptr) noexcept {
        if (!_initialized || visitor == nullptr) return;
        MutexGuard lock(&_mutex);
        for (io::usize i=0; i<_voices.size(); ++i) {
            const Voice& voice = _voices[i];
            if (!voice.active) continue;
            io::char_view id{};
            io::u64 total_frames = 0;
            bool streamed = false;
            if (voice.asset != nullptr) {
                id = voice.asset->id.as_view();
                total_frames = voice.asset->frame_count;
                streamed = voice.asset->storage == AssetStorage::Stream;
            }
            const io::u64 cursor_frames = voice.cursor_fp >> CURSOR_FRAC_BITS;
            visitor(user, voice.category, id, cursor_frames, total_frames, streamed);
        }
    }

    void CollectRuntimeSnapshot(RuntimeSnapshot& out, DebugVoiceVisitor visitor = nullptr, void* user = nullptr) noexcept {
        out = RuntimeSnapshot{};
        out.voice_capacity = VoiceCapacity();
        out.music_silence_active = IsMusicSilenceActive();
        out.ambient_silence_active = IsAmbientSilenceActive();
        if (!_initialized) return;

        MutexGuard lock(&_mutex);
        out.scheduled_events = static_cast<io::u32>(_events.size());
        for (io::usize i=0; i<_voices.size(); ++i) {
            const Voice& voice = _voices[i];
            if (!voice.active) continue;

            ++out.active_voices;
            const io::usize category_index = CategoryIndex(voice.category);
            if (category_index < CATEGORY_COUNT)
                ++out.by_category[category_index];

            if (!out.has_active_music_state && IsMusicCategory(voice.category)) {
                out.active_music_state = CategoryAsMusicState(voice.category);
                out.has_active_music_state = true;
            }

            if (visitor == nullptr) continue;
            io::char_view id{};
            io::u64 total_frames = 0;
            bool streamed = false;
            if (voice.asset != nullptr) {
                id = voice.asset->id.as_view();
                total_frames = voice.asset->frame_count;
                streamed = voice.asset->storage == AssetStorage::Stream;
            }
            const io::u64 cursor_frames = voice.cursor_fp >> CURSOR_FRAC_BITS;
            visitor(user, voice.category, id, cursor_frames, total_frames, streamed);
        }
    }

    double AudioTimeSeconds() const noexcept {
#if !defined(IO_HAS_STD)
        return 0.0;
#else
        if (_out_sample_rate == 0)
            return 0.0;
        return static_cast<double>(_frames_mixed.load(io::memory_order_relaxed)) / static_cast<double>(_out_sample_rate);
#endif
    }

private:
    static const io::usize CATEGORY_COUNT = static_cast<io::usize>(Category::Count);
    static const io::usize MUSIC_STATE_COUNT = static_cast<io::usize>(MusicState::Count);
    static const io::usize MAX_VOICES = 64;
    static const io::u32 CURSOR_FRAC_BITS = 12;
    static const io::u32 CURSOR_FRAC_ONE = (1u << CURSOR_FRAC_BITS);
    static const io::u32 STREAM_CHUNK_FRAMES = 1024;
    static const io::u32 DECODE_CHUNK_FRAMES = 2048;

    enum class AssetStorage : io::u8 {
        Memory = 0,
        Stream
    };

    struct Asset {
        io::string id{};
        io::string path{};
        AssetStorage storage = AssetStorage::Memory;
        bool has_last_start = false;
        io::u64 last_start_frame = 0;

        io::u16 audio_format = 0;
        io::u16 bits_per_sample = 0;
        io::u16 channels = 0;
        io::u16 block_align = 0;
        io::u32 sample_rate = 0;
        io::u64 frame_count = 0;
        io::u64 data_offset = 0;

        io::vector<float> pcm{};
    };

    struct Voice {
        bool active = false;
        Category category = Category::SFX;
        Asset* asset = nullptr;
        io::u64 spawn_sequence = 0;
        io::u64 cursor_fp = 0;
        io::u32 step_fp = CURSOR_FRAC_ONE;
        float left_gain = 1.0f;
        float right_gain = 1.0f;
        io::u32 startup_delay_frames = 0;
        bool apply_music_fade = false;
        bool fade_out_started = false;
        io::u32 fade_in_src_frames = 0;
        io::u32 fade_out_src_frames = 0;
        io::u32 fade_out_total_out_frames = 0;
        io::u32 fade_out_progress_out_frames = 0;

        fs::File stream_file{};
        io::vector<io::u8> stream_raw{};
        io::vector<float> stream_cache{};
        io::u64 stream_cache_start_frame = 0;
        io::u32 stream_cache_frames = 0;
        float eq_low_state[2] = { 0.0f, 0.0f };
        float eq_high_state[2] = { 0.0f, 0.0f };
        float eq_high_prev_in[2] = { 0.0f, 0.0f };
    };

    struct ScheduledEvent {
        io::u64 start_frame = 0;
        Category category = Category::SFX;
        Asset* asset = nullptr;
        float gain = 1.0f;
        float pan = 0.0f;
        float pitch = 1.0f;
        io::u64 sequence = 0;
    };

    struct WavFormat {
        io::u16 audio_format = 0;
        io::u16 channels = 0;
        io::u32 sample_rate = 0;
        io::u16 bits_per_sample = 0;
        io::u16 block_align = 0;
    };

    struct WavInfo {
        WavFormat fmt{};
        io::u64 data_offset = 0;
        io::u32 data_size = 0;
    };

    class MutexGuard {
    public:
        explicit MutexGuard(io::spin_mutex* mutex) noexcept
            : _mutex(mutex) {
            if (_mutex != nullptr)
                _mutex->lock();
        }

        ~MutexGuard() noexcept {
            if (_mutex != nullptr)
                _mutex->unlock();
        }

        MutexGuard(const MutexGuard&) noexcept = delete;
        MutexGuard& operator=(const MutexGuard&) noexcept = delete;

    private:
        io::spin_mutex* _mutex;
    };

    static io::usize CategoryIndex(Category category) noexcept {
        return static_cast<io::usize>(category);
    }

    static io::usize MusicStateIndex(MusicState state) noexcept {
        return static_cast<io::usize>(state);
    }

    static bool IsMusicCategory(Category category) noexcept {
        return static_cast<io::u8>(category) <= static_cast<io::u8>(Category::Unreality);
    }

    static MusicState CategoryAsMusicState(Category category) noexcept {
        return static_cast<MusicState>(static_cast<io::u8>(category));
    }

    static io::char_view CategoryDirectoryName(Category category) noexcept {
        switch (category) {
        case Category::Stillness: return io::char_view{ "stillness" };
        case Category::Wander:    return io::char_view{ "wander" };
        case Category::Strain:    return io::char_view{ "strain" };
        case Category::Threat:    return io::char_view{ "threat" };
        case Category::Unreality: return io::char_view{ "unreality" };
        case Category::Ambient:   return io::char_view{ "ambient" };
        case Category::SFX:       return io::char_view{ "sfx" };
        case Category::UI:        return io::char_view{ "ui" };
        default:                  return io::char_view{ "" };
        }
    }

    static bool CategoryStreamsFromDisk(Category category) noexcept {
        return category != Category::SFX && category != Category::UI;
    }

    void ApplyMusicFades(const Fade* fades, io::usize fade_count) noexcept {
        for (io::usize i=0; i<MUSIC_STATE_COUNT; ++i) {
            _music_fade_in_secs[i] = 0;
            _music_fade_out_secs[i] = 0;
        }
        if (fades == nullptr || fade_count == 0) return;
        for (io::usize i=0; i<fade_count; ++i) {
            const io::usize index = MusicStateIndex(fades[i].state);
            if (index >= MUSIC_STATE_COUNT) continue;
            _music_fade_in_secs[index] = fades[i].in_secs;
            _music_fade_out_secs[index] = fades[i].out_secs;
        }
    }

    io::u32 MusicFadeInSecs(Category category) const noexcept {
        if (!IsMusicCategory(category)) return 0;
        const io::usize index = MusicStateIndex(CategoryAsMusicState(category));
        if (index >= MUSIC_STATE_COUNT) return 0;
        return _music_fade_in_secs[index];
    }

    io::u32 MusicFadeOutSecs(Category category) const noexcept {
        if (!IsMusicCategory(category)) return 0;
        const io::usize index = MusicStateIndex(CategoryAsMusicState(category));
        if (index >= MUSIC_STATE_COUNT) return 0;
        return _music_fade_out_secs[index];
    }

    io::u64 MsToFrames(io::u32 ms) const noexcept {
        if (ms == 0 || _out_sample_rate == 0) return 0;
        const io::u64 numer = io::mul_u32(_out_sample_rate, ms);
        io::u64 frames = io::div_u64_u32(numer, 1000u);
        if (frames == 0) frames = 1;
        return frames;
    }

    void SetDirectiveHoldMs(io::u32 ms) noexcept {
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 add = MsToFrames(ms);
        _directive_hold_until_frame.store(now_frame + add, io::memory_order_release);
    }

    bool IsDirectiveHoldActive() const noexcept {
        const io::u64 now_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 hold_until = _directive_hold_until_frame.load(io::memory_order_acquire);
        return now_frame < hold_until;
    }

    bool HasActiveCategoryLocked(Category category) const noexcept {
        for (io::usize i=0; i<_voices.size(); ++i)
            if (_voices[i].active && _voices[i].category == category)
                return true;
        return false;
    }

    bool HasScheduledCategoryLocked(Category category) const noexcept {
        for (io::usize i=0; i<_events.size(); ++i)
            if (_events[i].category == category)
                return true;
        return false;
    }

    bool HasPendingCategoryLocked(Category category) const noexcept {
        if (HasActiveCategoryLocked(category)) return true;
        return HasScheduledCategoryLocked(category);
    }

    bool HasPendingMusicLocked() const noexcept {
        for (io::usize i=0; i<_voices.size(); ++i)
            if (_voices[i].active && IsMusicCategory(_voices[i].category))
                return true;
        for (io::usize i=0; i<_events.size(); ++i)
            if (IsMusicCategory(_events[i].category))
                return true;
        return false;
    }

    bool TryGetActiveMusicStateLocked(MusicState& out_state) const noexcept {
        for (io::usize i=0; i<_voices.size(); ++i) {
            if (!_voices[i].active) continue;
            if (!IsMusicCategory(_voices[i].category)) continue;
            out_state = CategoryAsMusicState(_voices[i].category);
            return true;
        }
        return false;
    }

    bool ApplyDirective(const Directive& directive) noexcept {
        switch (directive.type) {
        case DirectiveType::None:
            return false;
        case DirectiveType::ForceMusicState:
            return TryPlayMusic(directive.music_state, directive.force, directive.bypass_silence, directive.gain, directive.pan, directive.pitch);
        case DirectiveType::ForceMusicAsset:
            return TryPlayMusicAsset(directive.music_state, directive.asset_id, directive.force, directive.bypass_silence, directive.gain, directive.pan, directive.pitch);
        case DirectiveType::ForceAmbient:
            return TryPlayAmbient(directive.allow_ambient_overlap, directive.bypass_silence, directive.gain, directive.pan, directive.pitch);
        case DirectiveType::ForceAmbientAsset:
            return TryPlayAmbientAsset(directive.asset_id, directive.allow_ambient_overlap, directive.bypass_silence, directive.gain, directive.pan, directive.pitch);
        case DirectiveType::StopMusic:
            StopMusic();
            return true;
        case DirectiveType::StopAmbient:
            StopCategory(Category::Ambient);
            return true;
        case DirectiveType::SetMusicSilenceMs:
            SetMusicSilenceMs(directive.silence_ms);
            return true;
        case DirectiveType::SetAmbientSilenceMs:
            SetAmbientSilenceMs(directive.silence_ms);
            return true;
        default:
            return false;
        }
    }

    static io::u32 VoicePriority(Category category) noexcept {
        if (IsMusicCategory(category)) return 4;
        if (category == Category::UI) return 3;
        if (category == Category::Ambient) return 2;
        return 1;
    }

    static float VoiceActivityLevel(const Voice& voice) noexcept {
        float level = voice.left_gain + voice.right_gain;
        if (level < 0.0f) level = 0.0f;
        return level;
    }

    Voice* SelectVoiceForReplacementLocked(Category incoming) noexcept {
        const io::u32 incoming_priority = VoicePriority(incoming);
        const io::usize invalid_index = static_cast<io::usize>(-1);
        io::usize best_index = invalid_index;
        io::u32 best_priority = 0;
        float best_level = 0.0f;
        io::u64 best_sequence = 0;

        for (io::usize i=0; i<_voices.size(); ++i) {
            Voice& voice = _voices[i];
            if (!voice.active) continue;
            const io::u32 voice_priority = VoicePriority(voice.category);
            if (voice_priority >= 4) continue;
            if (voice_priority > incoming_priority) continue;

            const float level = VoiceActivityLevel(voice);
            if (best_index == invalid_index) {
                best_index = i;
                best_priority = voice_priority;
                best_level = level;
                best_sequence = voice.spawn_sequence;
                continue;
            }

            bool better = false;
            if (voice_priority < best_priority) better = true;
            else if (voice_priority == best_priority) {
                if (level < best_level) better = true;
                else if (level == best_level && voice.spawn_sequence < best_sequence) better = true;
            }

            if (better) {
                best_index = i;
                best_priority = voice_priority;
                best_level = level;
                best_sequence = voice.spawn_sequence;
            }
        }

        if (best_index == invalid_index) return nullptr;
        return &_voices[best_index];
    }

    void ClampSchedulerPolicyLocked() noexcept {
        _scheduler_policy.filter_smoothing = Clamp(_scheduler_policy.filter_smoothing, 0.0f, 1.0f);
        _scheduler_policy.duck_smoothing = Clamp(_scheduler_policy.duck_smoothing, 0.0f, 1.0f);
        _scheduler_policy.low_alpha = Clamp(_scheduler_policy.low_alpha, 0.0f, 1.0f);
        _scheduler_policy.high_alpha = Clamp(_scheduler_policy.high_alpha, 0.0f, 1.0f);
        _scheduler_policy.music_duck_by_sfx = Clamp(_scheduler_policy.music_duck_by_sfx, 0.0f, 1.0f);
        _scheduler_policy.music_duck_by_ui = Clamp(_scheduler_policy.music_duck_by_ui, 0.0f, 1.0f);
        _scheduler_policy.ambient_duck_by_sfx = Clamp(_scheduler_policy.ambient_duck_by_sfx, 0.0f, 1.0f);
        _scheduler_policy.ambient_duck_by_ui = Clamp(_scheduler_policy.ambient_duck_by_ui, 0.0f, 1.0f);
        for (io::usize i=0; i<MUSIC_STATE_COUNT; ++i) {
            _scheduler_policy.music_enter_threshold[i] = Clamp(_scheduler_policy.music_enter_threshold[i], 0.0f, 1.0f);
            _scheduler_policy.music_exit_threshold[i] = Clamp(_scheduler_policy.music_exit_threshold[i], 0.0f, 1.0f);
        }
    }

    float EnterThreshold(MusicState state) const noexcept {
        const io::usize index = MusicStateIndex(state);
        if (index >= MUSIC_STATE_COUNT) return 0.0f;
        return _scheduler_policy.music_enter_threshold[index];
    }

    float ExitThreshold(MusicState state) const noexcept {
        const io::usize index = MusicStateIndex(state);
        if (index >= MUSIC_STATE_COUNT) return 0.0f;
        return _scheduler_policy.music_exit_threshold[index];
    }

    io::u32 NextRandomU32() noexcept {
        _rand_state = (_rand_state * 1664525u) + 1013904223u;
        return _rand_state;
    }

    io::u64 CooldownFramesForCategory(Category category) const noexcept {
        io::u32 cooldown_ms = 0;
        if (IsMusicCategory(category))
            cooldown_ms = _scheduler_policy.music_asset_cooldown_ms;
        else if (category == Category::Ambient)
            cooldown_ms = _scheduler_policy.ambient_asset_cooldown_ms;
        return MsToFrames(cooldown_ms);
    }

    static bool IsMusicEdgeAllowed(MusicState from, MusicState to) noexcept {
        if (from == to) return true;
        if (from == MusicState::Stillness && to == MusicState::Wander) return true;
        if (from == MusicState::Wander && (to == MusicState::Stillness || to == MusicState::Strain || to == MusicState::Unreality)) return true;
        if (from == MusicState::Strain && (to == MusicState::Wander || to == MusicState::Threat || to == MusicState::Unreality)) return true;
        if (from == MusicState::Threat && to == MusicState::Strain) return true;
        if (from == MusicState::Unreality && (to == MusicState::Strain || to == MusicState::Wander)) return true;
        return false;
    }

    static MusicState ResolveMusicTransitionStep(MusicState from, MusicState to) noexcept {
        if (from == to) return to;
        if (IsMusicEdgeAllowed(from, to)) return to;

        const io::u32 INVALID = 0xFFFFFFFFu;
        io::u32 queue[MUSIC_STATE_COUNT]{};
        io::u32 head = 0, tail = 0;
        io::u32 prev[MUSIC_STATE_COUNT]{};
        for (io::u32 i=0; i<MUSIC_STATE_COUNT; ++i)
            prev[i] = INVALID;

        const io::u32 from_i = static_cast<io::u32>(MusicStateIndex(from));
        const io::u32 to_i = static_cast<io::u32>(MusicStateIndex(to));
        queue[tail++] = from_i;
        prev[from_i] = from_i;

        while (head < tail) {
            const io::u32 u = queue[head++];
            if (u == to_i) break;
            for (io::u32 v=0; v<MUSIC_STATE_COUNT; ++v) {
                if (prev[v] != INVALID) continue;
                if (!IsMusicEdgeAllowed(static_cast<MusicState>(u), static_cast<MusicState>(v))) continue;
                prev[v] = u;
                queue[tail++] = v;
            }
        }

        if (prev[to_i] == INVALID) return from;
        io::u32 step = to_i;
        while (prev[step] != from_i && step != from_i)
            step = prev[step];
        return static_cast<MusicState>(step);
    }

    Asset* SelectAssetForPlayLocked(Category category, io::u32& round_robin, io::u64 now_frame) noexcept {
        const io::usize index = CategoryIndex(category);
        if (index >= CATEGORY_COUNT || _assets[index].empty()) return nullptr;

        const io::u64 cooldown_frames = CooldownFramesForCategory(category);
        const io::usize count = _assets[index].size();
        if (round_robin >= count) round_robin = 0;
        const Asset* last_asset = _last_started_asset[index];

        io::usize selected = static_cast<io::usize>(-1);
        for (io::usize pass=0; pass<2 && selected == static_cast<io::usize>(-1); ++pass) {
            for (io::usize i=0; i<count; ++i) {
                const io::usize pos = (round_robin + i) % count;
                Asset* candidate = _assets[index][pos].get();
                if (candidate == nullptr) continue;

                if (pass == 0 && count > 1 && candidate == last_asset)
                    continue;

                if (pass == 0 && cooldown_frames > 0 && candidate->has_last_start) {
                    const io::u64 elapsed = now_frame > candidate->last_start_frame ? (now_frame - candidate->last_start_frame) : 0;
                    if (elapsed < cooldown_frames)
                        continue;
                }

                selected = pos;
                break;
            }
        }

        if (selected == static_cast<io::usize>(-1))
            selected = round_robin;

        round_robin = static_cast<io::u32>((selected + 1) % count);
        return _assets[index][selected].get();
    }

    void ConsumeEndedVoicesAndUpdateSilenceLocked(io::u64 now_frame) noexcept {
        for (io::usize i=0; i<CATEGORY_COUNT; ++i) {
            if (_voice_end_seen_counter[i] == _voice_end_counter[i])
                continue;

            _voice_end_seen_counter[i] = _voice_end_counter[i];
            const Category category = static_cast<Category>(i);

            if (category == Category::Ambient) {
                if (HasPendingCategoryLocked(Category::Ambient))
                    continue;
                if (_scheduler_policy.ambient_silence_secs == 0)
                    continue;

                io::u32 rand_secs = 0;
                if (_scheduler_policy.ambient_rand_silence_secs > 0)
                    rand_secs = NextRandomU32() % (_scheduler_policy.ambient_rand_silence_secs + 1u);

                const io::u64 wait_secs = static_cast<io::u64>(_scheduler_policy.ambient_silence_secs + rand_secs);
                const io::u32 wait_secs32 = wait_secs > static_cast<io::u64>(static_cast<io::u32>(-1)) ? static_cast<io::u32>(-1) : static_cast<io::u32>(wait_secs);
                _ambient_silence_until_frame.store(now_frame + io::mul_u32(wait_secs32, _out_sample_rate), io::memory_order_release);
                continue;
            }

            if (!IsMusicCategory(category))
                continue;
            if (HasPendingMusicLocked())
                continue;
            if (_scheduler_policy.music_silence_secs == 0)
                continue;

            io::u32 rand_secs = 0;
            if (_scheduler_policy.music_rand_silence_secs > 0)
                rand_secs = NextRandomU32() % (_scheduler_policy.music_rand_silence_secs + 1u);

            const io::u64 wait_secs = static_cast<io::u64>(_scheduler_policy.music_silence_secs + rand_secs);
            const io::u32 wait_secs32 = wait_secs > static_cast<io::u64>(static_cast<io::u32>(-1)) ? static_cast<io::u32>(-1) : static_cast<io::u32>(wait_secs);
            _music_silence_until_frame.store(now_frame + io::mul_u32(wait_secs32, _out_sample_rate), io::memory_order_release);
        }
    }

    void UpdateSmoothedFiltersAndDuckingLocked(io::u32 frame_count) noexcept {
        (void)frame_count;
        const float f = _scheduler_policy.filter_smoothing;
        for (io::usize i=0; i<CATEGORY_COUNT; ++i) {
            _filter_gain_current[i] += (_filter_gains[i] - _filter_gain_current[i]) * f;
            _filter_low_current[i] += (_filter_low_targets[i] - _filter_low_current[i]) * f;
            _filter_mid_current[i] += (_filter_mid_targets[i] - _filter_mid_current[i]) * f;
            _filter_high_current[i] += (_filter_high_targets[i] - _filter_high_current[i]) * f;
        }

        io::u32 sfx_active = 0;
        io::u32 ui_active = 0;
        for (io::usize i=0; i<_voices.size(); ++i) {
            if (!_voices[i].active) continue;
            if (_voices[i].category == Category::SFX) ++sfx_active;
            if (_voices[i].category == Category::UI) ++ui_active;
        }

        float music_target = 1.0f;
        float ambient_target = 1.0f;
        if (sfx_active > 0) {
            music_target *= _scheduler_policy.music_duck_by_sfx;
            ambient_target *= _scheduler_policy.ambient_duck_by_sfx;
        }
        if (ui_active > 0) {
            music_target *= _scheduler_policy.music_duck_by_ui;
            ambient_target *= _scheduler_policy.ambient_duck_by_ui;
        }

        const float d = _scheduler_policy.duck_smoothing;
        _music_duck_current += (music_target - _music_duck_current) * d;
        _ambient_duck_current += (ambient_target - _ambient_duck_current) * d;
    }

    float ApplyVoiceEq(Voice& voice, io::u32 output_channel, float sample, Category category) const noexcept {
        const io::usize category_index = CategoryIndex(category);
        if (category_index >= CATEGORY_COUNT) return sample;

        const io::u32 ch = output_channel > 0 ? 1u : 0u;
        const float low_alpha = _scheduler_policy.low_alpha;
        const float high_alpha = _scheduler_policy.high_alpha;

        const float low_prev = voice.eq_low_state[ch];
        const float low = low_prev + low_alpha * (sample - low_prev);
        voice.eq_low_state[ch] = low;

        const float prev_in = voice.eq_high_prev_in[ch];
        const float high_prev = voice.eq_high_state[ch];
        const float high = high_alpha * (high_prev + sample - prev_in);
        voice.eq_high_state[ch] = high;
        voice.eq_high_prev_in[ch] = sample;

        const float mid = sample - low - high;
        return low * _filter_low_current[category_index] +
               mid * _filter_mid_current[category_index] +
               high * _filter_high_current[category_index];
    }

    static bool IsSameId(io::char_view lhs, io::char_view rhs) noexcept {
        if (lhs.size() != rhs.size())
            return false;
        for (io::usize i=0; i<lhs.size(); ++i) {
            if (lhs[i] != rhs[i])
                return false;
        }
        return true;
    }

    static bool HasTag(const io::u8* p, io::u8 a, io::u8 b, io::u8 c, io::u8 d) noexcept {
        return p[0] == a && p[1] == b && p[2] == c && p[3] == d;
    }

    static io::u16 LeU16(const io::u8* p) noexcept {
        return static_cast<io::u16>(
            static_cast<io::u16>(p[0]) |
            (static_cast<io::u16>(p[1]) << 8));
    }

    static io::u32 LeU32(const io::u8* p) noexcept {
        return static_cast<io::u32>(
            static_cast<io::u32>(p[0]) |
            (static_cast<io::u32>(p[1]) << 8) |
            (static_cast<io::u32>(p[2]) << 16) |
            (static_cast<io::u32>(p[3]) << 24));
    }

    static io::i32 LeI24(const io::u8* p) noexcept {
        io::i32 v = static_cast<io::i32>(p[0]) |
                    (static_cast<io::i32>(p[1]) << 8) |
                    (static_cast<io::i32>(p[2]) << 16);
        if ((v & 0x00800000) != 0)
            v |= static_cast<io::i32>(0xFF000000);
        return v;
    }

    static char AsciiLower(char c) noexcept {
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(c + ('a' - 'A'));
        return c;
    }

    static bool HasWavExtension(io::char_view path) noexcept {
        if (path.empty())
            return false;

        io::usize dot = io::npos;
        for (io::usize i=0; i<path.size(); ++i) {
            const char c = path[i];
            if (c == '/' || c == '\\') {
                dot = io::npos;
            } else if (c == '.') {
                dot = i;
            }
        }

        if (dot == io::npos)
            return false;
        if ((path.size() - dot) != 4)
            return false;

        return AsciiLower(path[dot + 1]) == 'w' &&
               AsciiLower(path[dot + 2]) == 'a' &&
               AsciiLower(path[dot + 3]) == 'v';
    }

    static bool BuildAssetIdFromPath(io::char_view path, io::string& out_id) noexcept {
        out_id.clear();
        if (path.empty())
            return false;

        io::usize start = 0;
        io::usize end = path.size();
        for (io::usize i=0; i<path.size(); ++i) {
            const char c = path[i];
            if (c == '/' || c == '\\') {
                start = i + 1;
                end = path.size();
            } else if (c == '.') {
                end = i;
            }
        }

        if (end <= start)
            return false;

        return out_id.append(io::char_view{ path.data() + start, end - start });
    }

    static bool MulUsize(io::usize a, io::usize b, io::usize& out) noexcept {
        if (a == 0 || b == 0) {
            out = 0;
            return true;
        }

        const io::usize max_value = static_cast<io::usize>(-1);
        if (a > (max_value / b))
            return false;

        out = a * b;
        return true;
    }

    static bool MulU32U32(io::u32 a, io::u32 b, io::u32& out) noexcept {
        if (a == 0u || b == 0u) {
            out = 0u;
            return true;
        }
        const io::u32 max_value = static_cast<io::u32>(-1);
        if (a > (max_value / b))
            return false;
        out = static_cast<io::u32>(a * b);
        return true;
    }

    static bool MulU64U32(io::u64 a, io::u32 b, io::u64& out) noexcept {
        io::u64 result = 0;
        io::u64 addend = a;
        io::u32 factor = b;
        const io::u64 max_value = static_cast<io::u64>(-1);

        while (factor != 0) {
            if ((factor & 1u) != 0u) {
                if (result > (max_value - addend))
                    return false;
                result += addend;
            }

            factor >>= 1u;
            if (factor != 0) {
                if (addend > (max_value >> 1u))
                    return false;
                addend <<= 1u;
            }
        }

        out = result;
        return true;
    }

    static float Clamp(float value, float min_value, float max_value) noexcept {
        if (value < min_value)
            return min_value;
        if (value > max_value)
            return max_value;
        return value;
    }

    static void ClampMood(Mood& mood) noexcept {
        mood.stillness = Clamp(mood.stillness, 0.0f, 1.0f);
        mood.wander = Clamp(mood.wander, 0.0f, 1.0f);
        mood.strain = Clamp(mood.strain, 0.0f, 1.0f);
        mood.threat = Clamp(mood.threat, 0.0f, 1.0f);
        mood.unreality = Clamp(mood.unreality, 0.0f, 1.0f);
        mood.ambient = Clamp(mood.ambient, 0.0f, 1.0f);
    }

    void ClampMoodLocked(Mood& mood) noexcept {
        ClampMood(mood);
    }

    static bool EventLess(const ScheduledEvent& lhs, const ScheduledEvent& rhs) noexcept {
        if (lhs.start_frame == rhs.start_frame)
            return lhs.sequence < rhs.sequence;
        return lhs.start_frame < rhs.start_frame;
    }

    bool InsertEventLocked(const ScheduledEvent& event) noexcept {
        io::usize insert_pos = 0;
        while (insert_pos < _events.size() && EventLess(_events[insert_pos], event))
            ++insert_pos;

        const io::usize old_size = _events.size();
        if (!_events.resize(old_size+1))
            return false;

        for (io::usize i = old_size; i > insert_pos; --i)
            _events[i] = _events[i-1];
        _events[insert_pos] = event;
        return true;
    }

    static void DeviceCallback(void* user, float* output, io::u32 frame_count) noexcept {
        AudioContext* self = reinterpret_cast<AudioContext*>(user);
        if (self != nullptr)
            self->Mix(output, frame_count);
    }

    static float WavSampleToFloat(const io::u8* p, io::u16 audio_format, io::u16 bits_per_sample) noexcept {
        if (audio_format == 1) {
            if (bits_per_sample == 8) {
                const io::i32 v = static_cast<io::i32>(p[0]) - 128;
                return static_cast<float>(v) / 128.0f;
            }
            if (bits_per_sample == 16) {
                const io::i16 s = static_cast<io::i16>(LeU16(p));
                return static_cast<float>(s) / 32768.0f;
            }
            if (bits_per_sample == 24) {
                const io::i32 s = LeI24(p);
                return static_cast<float>(s) / 8388608.0f;
            }
            if (bits_per_sample == 32) {
                const io::i32 s = static_cast<io::i32>(LeU32(p));
                return static_cast<float>(s) / 2147483648.0f;
            }
        }

        if (audio_format == 3 && bits_per_sample == 32) {
            union {
                io::u32 u;
                float f;
            } cvt{};
            cvt.u = LeU32(p);
            return cvt.f;
        }

        return 0.0f;
    }

    static void DecodeWavBlockToFloat(const io::u8* src, io::u32 frame_count, const WavFormat& fmt, float* dst) noexcept {
        const io::u16 bytes_per_sample = static_cast<io::u16>(fmt.bits_per_sample / 8);
        const io::u8* read_ptr = src;
        for (io::u32 frame = 0; frame < frame_count; ++frame) {
            for (io::u32 ch = 0; ch < fmt.channels; ++ch) {
                *dst++ = WavSampleToFloat(read_ptr, fmt.audio_format, fmt.bits_per_sample);
                read_ptr += bytes_per_sample;
            }
        }
    }

    static bool ValidateWavFormat(const WavFormat& fmt) noexcept {
        if (fmt.channels == 0 || fmt.sample_rate == 0 || fmt.block_align == 0 || fmt.bits_per_sample == 0)
            return false;
        const io::u16 bytes_per_sample = static_cast<io::u16>(fmt.bits_per_sample / 8);
        if (bytes_per_sample == 0)
            return false;
        if (!(fmt.audio_format == 1 || fmt.audio_format == 3))
            return false;
        return true;
    }

    bool ReadWavInfoFromFile(fs::File& file, WavInfo& info) noexcept {
        info = WavInfo{};

        const io::u64 file_size = file.size();
        if (file_size < 44)
            return false;

        io::u8 riff[12]{};
        if (!file.seek(0, io::SeekWhence::Begin))
            return false;
        if (!file.read_exact(riff, sizeof(riff)))
            return false;
        if (!HasTag(riff, 'R', 'I', 'F', 'F'))
            return false;
        if (!HasTag(riff + 8, 'W', 'A', 'V', 'E'))
            return false;

        io::u64 pos = 12;
        bool have_fmt = false;
        bool have_data = false;

        while (pos + 8 <= file_size) {
            io::u8 chunk_hdr[8]{};
            if (!file.seek(static_cast<io::i64>(pos), io::SeekWhence::Begin))
                return false;
            if (!file.read_exact(chunk_hdr, sizeof(chunk_hdr)))
                return false;

            const io::u32 chunk_size = LeU32(chunk_hdr + 4);
            const io::u64 chunk_data_pos = pos + 8;
            const io::u64 chunk_end_pos = chunk_data_pos + static_cast<io::u64>(chunk_size);
            if (chunk_end_pos > file_size)
                return false;

            if (HasTag(chunk_hdr, 'f', 'm', 't', ' ')) {
                if (chunk_size < 16)
                    return false;

                io::u8 fmt_buf[40]{};
                io::u32 read_count = chunk_size;
                if (read_count > 40u)
                    read_count = 40u;

                if (!file.seek(static_cast<io::i64>(chunk_data_pos), io::SeekWhence::Begin))
                    return false;
                if (!file.read_exact(fmt_buf, read_count))
                    return false;

                info.fmt.audio_format = LeU16(fmt_buf + 0);
                info.fmt.channels = LeU16(fmt_buf + 2);
                info.fmt.sample_rate = LeU32(fmt_buf + 4);
                info.fmt.block_align = LeU16(fmt_buf + 12);
                info.fmt.bits_per_sample = LeU16(fmt_buf + 14);

                if (info.fmt.audio_format == 0xFFFEu && chunk_size >= 40u) {
                    const io::u16 subtype = LeU16(fmt_buf + 24);
                    if (subtype == 1u || subtype == 3u)
                        info.fmt.audio_format = subtype;
                }
                have_fmt = true;
            } else if (HasTag(chunk_hdr, 'd', 'a', 't', 'a')) {
                info.data_offset = chunk_data_pos;
                info.data_size = chunk_size;
                have_data = true;
            }

            pos = chunk_end_pos + static_cast<io::u64>(chunk_size & 1u);
            if (have_fmt && have_data)
                break;
        }

        if (!have_fmt || !have_data)
            return false;
        if (info.data_size == 0)
            return false;
        return ValidateWavFormat(info.fmt);
    }

    bool LoadWavMetadataFromFile(io::char_view path, Asset& decoded_asset) noexcept {
        fs::File file{ path, io::OpenMode::Read };
        if (!file.is_open())
            return false;

        WavInfo info{};
        if (!ReadWavInfoFromFile(file, info))
            return false;

        const io::u64 frame_count64 = io::div_u64_u32(static_cast<io::u64>(info.data_size), info.fmt.block_align);
        if (frame_count64 == 0)
            return false;

        decoded_asset.audio_format = info.fmt.audio_format;
        decoded_asset.channels = info.fmt.channels;
        decoded_asset.sample_rate = info.fmt.sample_rate;
        decoded_asset.bits_per_sample = info.fmt.bits_per_sample;
        decoded_asset.block_align = info.fmt.block_align;
        decoded_asset.frame_count = frame_count64;
        decoded_asset.data_offset = info.data_offset;
        decoded_asset.pcm.clear();
        return true;
    }

    bool DecodeWavFromFile(io::char_view path, Asset& decoded_asset) noexcept {
        fs::File file{ path, io::OpenMode::Read };
        if (!file.is_open())
            return false;

        WavInfo info{};
        if (!ReadWavInfoFromFile(file, info))
            return false;

        const io::u64 frame_count64 = io::div_u64_u32(static_cast<io::u64>(info.data_size), info.fmt.block_align);
        if (frame_count64 == 0 || frame_count64 > static_cast<io::u64>(static_cast<io::u32>(-1)))
            return false;
        const io::u32 frame_count32 = static_cast<io::u32>(frame_count64);

        io::u32 sample_count_32 = 0;
        if (!MulU32U32(frame_count32, static_cast<io::u32>(info.fmt.channels), sample_count_32))
            return false;
        const io::usize sample_count = static_cast<io::usize>(sample_count_32);
        if (sample_count == 0)
            return false;
        if (!decoded_asset.pcm.resize(sample_count))
            return false;

        io::vector<io::u8> raw{};
        io::u32 frame_cursor = 0;

        while (frame_cursor < frame_count32) {
            const io::u32 remaining = static_cast<io::u32>(frame_count32 - frame_cursor);
            io::u32 frames_now = DECODE_CHUNK_FRAMES;
            if (remaining < frames_now)
                frames_now = remaining;

            io::u32 bytes_now32 = 0;
            if (!MulU32U32(frames_now, static_cast<io::u32>(info.fmt.block_align), bytes_now32))
                return false;
            const io::usize bytes_now = static_cast<io::usize>(bytes_now32);
            if (bytes_now == 0)
                return false;
            if (!raw.resize(bytes_now))
                return false;

            io::u32 frame_byte_offset32 = 0;
            if (!MulU32U32(frame_cursor, static_cast<io::u32>(info.fmt.block_align), frame_byte_offset32))
                return false;
            const io::u64 byte_offset = info.data_offset + static_cast<io::u64>(frame_byte_offset32);
            if (byte_offset > static_cast<io::u64>(static_cast<io::i64>(-1)))
                return false;
            if (!file.seek(static_cast<io::i64>(byte_offset), io::SeekWhence::Begin))
                return false;
            if (!file.read_exact(raw.data(), bytes_now))
                return false;

            io::u32 dst_sample_offset32 = 0;
            if (!MulU32U32(frame_cursor, static_cast<io::u32>(info.fmt.channels), dst_sample_offset32))
                return false;
            const io::usize dst_sample_offset = static_cast<io::usize>(dst_sample_offset32);
            if (dst_sample_offset >= decoded_asset.pcm.size())
                return false;

            DecodeWavBlockToFloat(raw.data(), frames_now, info.fmt, decoded_asset.pcm.data() + dst_sample_offset);
            frame_cursor = static_cast<io::u32>(frame_cursor + frames_now);
        }

        decoded_asset.audio_format = info.fmt.audio_format;
        decoded_asset.channels = info.fmt.channels;
        decoded_asset.sample_rate = info.fmt.sample_rate;
        decoded_asset.bits_per_sample = info.fmt.bits_per_sample;
        decoded_asset.block_align = info.fmt.block_align;
        decoded_asset.frame_count = static_cast<io::u64>(frame_count32);
        decoded_asset.data_offset = info.data_offset;
        return true;
    }

    bool DecodeWavFromMemory(const void* bytes, io::usize byte_count, Asset& decoded_asset) noexcept {
        const io::u8* src = reinterpret_cast<const io::u8*>(bytes);
        if (src == nullptr || byte_count < 44)
            return false;

        if (!HasTag(src, 'R', 'I', 'F', 'F'))
            return false;
        if (!HasTag(src + 8, 'W', 'A', 'V', 'E'))
            return false;

        WavFormat fmt{};
        const io::u8* data_ptr = nullptr;
        io::u32 data_size = 0;

        io::usize pos = 12;
        while (pos + 8 <= byte_count) {
            const io::u8* chunk = src + pos;
            const io::u32 chunk_size = LeU32(chunk + 4);
            const io::usize chunk_data_pos = pos + 8;

            if (chunk_data_pos > byte_count)
                return false;

            io::usize chunk_data_end = chunk_data_pos + static_cast<io::usize>(chunk_size);
            if (chunk_data_end > byte_count)
                return false;

            if (HasTag(chunk, 'f', 'm', 't', ' ')) {
                if (chunk_size < 16)
                    return false;
                fmt.audio_format = LeU16(src + chunk_data_pos + 0);
                fmt.channels = LeU16(src + chunk_data_pos + 2);
                fmt.sample_rate = LeU32(src + chunk_data_pos + 4);
                fmt.block_align = LeU16(src + chunk_data_pos + 12);
                fmt.bits_per_sample = LeU16(src + chunk_data_pos + 14);

                // WAVE_FORMAT_EXTENSIBLE: actual subtype is stored in GUID.
                if (fmt.audio_format == 0xFFFEu && chunk_size >= 40) {
                    const io::u16 subtype = LeU16(src + chunk_data_pos + 24);
                    if (subtype == 1u || subtype == 3u)
                        fmt.audio_format = subtype; // PCM or IEEE float
                }
            } else if (HasTag(chunk, 'd', 'a', 't', 'a')) {
                data_ptr = src + chunk_data_pos;
                data_size = chunk_size;
            }

            pos = chunk_data_end + (chunk_size & 1u);
        }

        if (data_ptr == nullptr || data_size == 0)
            return false;
        if (!ValidateWavFormat(fmt))
            return false;

        const io::u64 frame_count64 = io::div_u64_u32(static_cast<io::u64>(data_size), fmt.block_align);
        if (frame_count64 == 0)
            return false;
        if (frame_count64 > static_cast<io::u64>(static_cast<io::usize>(-1)))
            return false;

        const io::usize frame_count = static_cast<io::usize>(frame_count64);
        io::usize sample_count = 0;
        if (!MulUsize(frame_count, static_cast<io::usize>(fmt.channels), sample_count))
            return false;

        io::vector<float> pcm{};
        if (!pcm.resize(sample_count))
            return false;

        DecodeWavBlockToFloat(data_ptr, static_cast<io::u32>(frame_count), fmt, pcm.data());

        decoded_asset.audio_format = fmt.audio_format;
        decoded_asset.bits_per_sample = fmt.bits_per_sample;
        decoded_asset.block_align = fmt.block_align;
        decoded_asset.channels = fmt.channels;
        decoded_asset.sample_rate = fmt.sample_rate;
        decoded_asset.frame_count = frame_count64;
        decoded_asset.data_offset = 0;
        decoded_asset.pcm = io::move(pcm);
        return true;
    }

    bool UpsertAssetLocked(io::usize category, Asset& decoded) noexcept {
        if (category >= CATEGORY_COUNT)
            return false;

        Asset* existing = FindAssetLocked(category, decoded.id.as_view());
        if (existing != nullptr) {
            existing->path.clear();
            if (!decoded.path.empty() && !existing->path.append(decoded.path.as_view()))
                return false;
            existing->storage = decoded.storage;
            existing->has_last_start = false;
            existing->last_start_frame = 0;
            existing->audio_format = decoded.audio_format;
            existing->channels = decoded.channels;
            existing->sample_rate = decoded.sample_rate;
            existing->bits_per_sample = decoded.bits_per_sample;
            existing->block_align = decoded.block_align;
            existing->frame_count = decoded.frame_count;
            existing->data_offset = decoded.data_offset;
            existing->pcm = io::move(decoded.pcm);
            return true;
        }

        io::unique_ptr<Asset> stored = io::make_unique<Asset>();
        if (!stored)
            return false;

        if (!stored->id.append(decoded.id.as_view()))
            return false;
        if (!decoded.path.empty() && !stored->path.append(decoded.path.as_view()))
            return false;

        stored->storage = decoded.storage;
        stored->audio_format = decoded.audio_format;
        stored->channels = decoded.channels;
        stored->sample_rate = decoded.sample_rate;
        stored->bits_per_sample = decoded.bits_per_sample;
        stored->block_align = decoded.block_align;
        stored->frame_count = decoded.frame_count;
        stored->data_offset = decoded.data_offset;
        stored->pcm = io::move(decoded.pcm);

        return _assets[category].push_back(io::move(stored));
    }

    Asset* FindAssetLocked(io::usize category, io::char_view id) noexcept {
        if (category >= CATEGORY_COUNT)
            return nullptr;

        for (io::usize i = 0; i < _assets[category].size(); ++i) {
            if (IsSameId(_assets[category][i]->id.as_view(), id))
                return _assets[category][i].get();
        }

        return nullptr;
    }

    const Asset* FindAssetLocked(io::usize category, io::char_view id) const noexcept {
        if (category >= CATEGORY_COUNT)
            return nullptr;

        for (io::usize i = 0; i < _assets[category].size(); ++i) {
            if (IsSameId(_assets[category][i]->id.as_view(), id))
                return _assets[category][i].get();
        }

        return nullptr;
    }

    static void ClearVoiceStreamState(Voice& voice) noexcept {
        if (voice.stream_file.is_open())
            voice.stream_file.close();
        voice.stream_raw.clear();
        voice.stream_cache.clear();
        voice.stream_cache_start_frame = 0;
        voice.stream_cache_frames = 0;
    }

    static void DeactivateVoice(Voice& voice) noexcept {
        voice.active = false;
        voice.asset = nullptr;
        voice.spawn_sequence = 0;
        voice.cursor_fp = 0;
        voice.step_fp = CURSOR_FRAC_ONE;
        voice.left_gain = 1.0f;
        voice.right_gain = 1.0f;
        voice.startup_delay_frames = 0;
        voice.apply_music_fade = false;
        voice.fade_out_started = false;
        voice.fade_in_src_frames = 0;
        voice.fade_out_src_frames = 0;
        voice.fade_out_total_out_frames = 0;
        voice.fade_out_progress_out_frames = 0;
        voice.eq_low_state[0] = 0.0f; voice.eq_low_state[1] = 0.0f;
        voice.eq_high_state[0] = 0.0f; voice.eq_high_state[1] = 0.0f;
        voice.eq_high_prev_in[0] = 0.0f; voice.eq_high_prev_in[1] = 0.0f;
        ClearVoiceStreamState(voice);
    }

    void ReportVoiceEndedLocked(const Voice& voice, bool natural_end) noexcept {
        const io::usize index = CategoryIndex(voice.category);
        if (index >= CATEGORY_COUNT) return;
        ++_voice_end_counter[index];
        _voice_end_played_frames[index] = (voice.cursor_fp >> CURSOR_FRAC_BITS);
        _voice_end_natural[index] = natural_end ? 1u : 0u;
    }

    void EndVoiceLocked(Voice& voice, bool natural_end) noexcept {
        if (voice.active && voice.asset != nullptr)
            ReportVoiceEndedLocked(voice, natural_end);
        DeactivateVoice(voice);
    }

    bool RequestMusicFadeOut(Voice& voice) noexcept {
        if (!voice.active || voice.asset == nullptr) return false;
        if (!IsMusicCategory(voice.category)) return false;
        if (voice.fade_out_started) return true;
        if (_out_sample_rate == 0) return false;
        const io::u32 out_secs = MusicFadeOutSecs(voice.category);
        if (out_secs == 0) return false;
        const io::u64 out_frames64 = io::mul_u32(out_secs, _out_sample_rate);
        if (out_frames64 == 0) return false;
        io::u32 out_frames = static_cast<io::u32>(-1);
        if (out_frames64 < static_cast<io::u64>(out_frames))
            out_frames = static_cast<io::u32>(out_frames64);
        voice.fade_out_started = true;
        voice.fade_out_total_out_frames = out_frames;
        voice.fade_out_progress_out_frames = 0;
        return true;
    }

    static float ComputeMusicFadeGain(const Voice& voice, io::u64 frame_int) noexcept {
        if (!voice.apply_music_fade || voice.asset == nullptr) return 1.0f;
        float gain = 1.0f;
        if (voice.fade_in_src_frames > 0 && frame_int < static_cast<io::u64>(voice.fade_in_src_frames)) {
            const io::u32 frame32 = static_cast<io::u32>(frame_int);
            gain = static_cast<float>(frame32) / static_cast<float>(voice.fade_in_src_frames);
        }
        if (voice.fade_out_src_frames > 0 && frame_int < voice.asset->frame_count) {
            const io::u64 remain = voice.asset->frame_count - frame_int;
            if (remain < static_cast<io::u64>(voice.fade_out_src_frames)) {
                const io::u32 remain32 = static_cast<io::u32>(remain);
                const float out_gain = static_cast<float>(remain32) / static_cast<float>(voice.fade_out_src_frames);
                if (out_gain < gain) gain = out_gain;
            }
        }
        if (voice.fade_out_started && voice.fade_out_total_out_frames > 0) {
            if (voice.fade_out_progress_out_frames >= voice.fade_out_total_out_frames)
                return 0.0f;
            const io::u64 remain = voice.fade_out_total_out_frames - voice.fade_out_progress_out_frames;
            const float out_gain = static_cast<float>(remain) / static_cast<float>(voice.fade_out_total_out_frames);
            if (out_gain < gain) gain = out_gain;
        }
        return Clamp(gain, 0.0f, 1.0f);
    }

    bool EnsureVoiceStreamFrameCached(Voice& voice, io::u64 frame) noexcept {
        if (voice.asset == nullptr || voice.asset->storage != AssetStorage::Stream)
            return false;
        const Asset& asset = *voice.asset;
        if (frame >= asset.frame_count)
            return false;

        const io::u64 cache_end = voice.stream_cache_start_frame + static_cast<io::u64>(voice.stream_cache_frames);
        if (voice.stream_cache_frames > 0 && frame >= voice.stream_cache_start_frame && frame < cache_end)
            return true;

        if (asset.path.empty())
            return false;
        if (!voice.stream_file.is_open()) {
            if (!voice.stream_file.open(asset.path, io::OpenMode::Read))
                return false;
        }

        io::u64 remain = asset.frame_count - frame;
        io::u32 frames_to_read = STREAM_CHUNK_FRAMES;
        if (remain < static_cast<io::u64>(frames_to_read))
            frames_to_read = static_cast<io::u32>(remain);
        if (frames_to_read == 0)
            return false;

        io::u64 bytes_to_read64 = 0;
        if (!MulU64U32(static_cast<io::u64>(frames_to_read), static_cast<io::u32>(asset.block_align), bytes_to_read64))
            return false;
        if (bytes_to_read64 > static_cast<io::u64>(static_cast<io::usize>(-1)))
            return false;
        const io::usize bytes_to_read = static_cast<io::usize>(bytes_to_read64);

        io::usize sample_count = 0;
        if (!MulUsize(static_cast<io::usize>(frames_to_read), static_cast<io::usize>(asset.channels), sample_count))
            return false;

        if (!voice.stream_raw.resize(bytes_to_read))
            return false;
        if (!voice.stream_cache.resize(sample_count))
            return false;

        io::u64 frame_byte_offset = 0;
        if (!MulU64U32(frame, static_cast<io::u32>(asset.block_align), frame_byte_offset))
            return false;
        const io::u64 file_offset = asset.data_offset + frame_byte_offset;
        if (file_offset > static_cast<io::u64>(static_cast<io::i64>(-1)))
            return false;
        if (!voice.stream_file.seek(static_cast<io::i64>(file_offset), io::SeekWhence::Begin))
            return false;
        if (!voice.stream_file.read_exact(voice.stream_raw.data(), bytes_to_read))
            return false;

        WavFormat fmt{};
        fmt.audio_format = asset.audio_format;
        fmt.channels = asset.channels;
        fmt.sample_rate = asset.sample_rate;
        fmt.bits_per_sample = asset.bits_per_sample;
        fmt.block_align = asset.block_align;
        DecodeWavBlockToFloat(voice.stream_raw.data(), frames_to_read, fmt, voice.stream_cache.data());

        voice.stream_cache_start_frame = frame;
        voice.stream_cache_frames = frames_to_read;
        return true;
    }

    static float ReadStreamCacheSample(const Voice& voice, io::u64 frame, io::u32 output_channel) noexcept {
        if (voice.asset == nullptr || voice.stream_cache_frames == 0)
            return 0.0f;
        const Asset& asset = *voice.asset;

        const io::u64 cache_end = voice.stream_cache_start_frame + static_cast<io::u64>(voice.stream_cache_frames);
        if (frame < voice.stream_cache_start_frame || frame >= cache_end)
            return 0.0f;

        io::u32 channel = output_channel;
        if (asset.channels == 1) {
            channel = 0;
        } else if (channel >= asset.channels) {
            channel = static_cast<io::u32>(asset.channels - 1);
        }

        const io::u64 rel_frame = frame - voice.stream_cache_start_frame;
        io::u64 sample_index64 = 0;
        if (!MulU64U32(rel_frame, static_cast<io::u32>(asset.channels), sample_index64))
            return 0.0f;
        sample_index64 += channel;
        if (sample_index64 > static_cast<io::u64>(static_cast<io::usize>(-1)))
            return 0.0f;
        const io::usize sample_index = static_cast<io::usize>(sample_index64);
        if (sample_index >= voice.stream_cache.size())
            return 0.0f;
        return voice.stream_cache[sample_index];
    }

    void Mix(float* output, io::u32 frame_count) noexcept {
        const io::u32 channels = _out_channels;
        const io::usize sample_count = static_cast<io::usize>(frame_count) * channels;

        for (io::usize i=0; i<sample_count; ++i)
            output[i] = 0.0f;

        if (!_initialized || channels == 0 || frame_count == 0)
            return;

        const io::u64 block_start_frame = _frames_mixed.load(io::memory_order_relaxed);
        const io::u64 block_end_frame = block_start_frame + frame_count;

        MutexGuard lock(&_mutex);

        ActivateScheduledEventsLocked(block_start_frame, block_end_frame);
        UpdateSmoothedFiltersAndDuckingLocked(frame_count);

        for (io::usize voice_index = 0; voice_index < _voices.size(); ++voice_index) {
            Voice& voice = _voices[voice_index];
            if (!voice.active || voice.asset == nullptr || voice.asset->frame_count == 0)
                continue;

            io::u32 first_frame = 0;
            if (voice.startup_delay_frames >= frame_count) {
                voice.startup_delay_frames -= frame_count;
                continue;
            }
            if (voice.startup_delay_frames > 0) {
                first_frame = voice.startup_delay_frames;
                voice.startup_delay_frames = 0;
            }

            const io::usize category_idx = CategoryIndex(voice.category);
            const float CategoryGain = category_idx < CATEGORY_COUNT ? _bus_gains[category_idx] : 1.0f;
            const float filter_gain = category_idx < CATEGORY_COUNT ? _filter_gain_current[category_idx] : 1.0f;
            float duck_gain = 1.0f;
            if (IsMusicCategory(voice.category))
                duck_gain = _music_duck_current;
            else if (voice.category == Category::Ambient)
                duck_gain = _ambient_duck_current;
            const float base_gain = _master_gain * CategoryGain * filter_gain * duck_gain;
            const bool streamed = (voice.asset->storage == AssetStorage::Stream);

            for (io::u32 frame = first_frame; frame < frame_count; ++frame) {
                const io::u64 frame_int = (voice.cursor_fp >> CURSOR_FRAC_BITS);
                if (frame_int >= voice.asset->frame_count) {
                    EndVoiceLocked(voice, true);
                    break;
                }

                if (streamed) {
                    if (!EnsureVoiceStreamFrameCached(voice, frame_int)) {
                        EndVoiceLocked(voice, false);
                        break;
                    }
                }
                const float music_fade_gain = ComputeMusicFadeGain(voice, frame_int);

                for (io::u32 channel = 0; channel < channels; ++channel) {
                    float sample = 0.0f;
                    if (streamed) {
                        sample = ReadStreamCacheSample(voice, frame_int, channel);
                    } else {
                        sample = ReadAssetSample(*voice.asset, frame_int, channel);
                    }
                    sample = ApplyVoiceEq(voice, channel, sample, voice.category);

                    float channel_pan_gain = 0.5f * (voice.left_gain + voice.right_gain);
                    if (channels > 1) {
                        if (channel == 0) {
                            channel_pan_gain = voice.left_gain;
                        } else if (channel == 1) {
                            channel_pan_gain = voice.right_gain;
                        }
                    }

                    const io::usize out_index = static_cast<io::usize>(frame) * channels + channel;
                    output[out_index] += sample * base_gain * channel_pan_gain * music_fade_gain;
                }

                voice.cursor_fp += static_cast<io::u64>(voice.step_fp);
                if (voice.fade_out_started && voice.fade_out_total_out_frames > 0) {
                    ++voice.fade_out_progress_out_frames;
                    if (voice.fade_out_progress_out_frames >= voice.fade_out_total_out_frames) {
                        EndVoiceLocked(voice, false);
                        break;
                    }
                }
            }
        }

        _frames_mixed.store(block_end_frame, io::memory_order_relaxed);
    }

    void ActivateScheduledEventsLocked(io::u64 block_start_frame, io::u64 block_end_frame) noexcept {
        io::usize consumed = 0;

        while (consumed < _events.size()) {
            const ScheduledEvent& event = _events[consumed];
            if (event.start_frame >= block_end_frame)
                break;

            io::u32 startup_delay = 0;
            if (event.start_frame > block_start_frame)
                startup_delay = static_cast<io::u32>(event.start_frame - block_start_frame);

            StartVoiceLocked(event, startup_delay);
            ++consumed;
        }

        if (consumed == 0)
            return;

        const io::usize remaining = _events.size() - consumed;
        for (io::usize i=0; i<remaining; ++i)
            _events[i] = _events[i + consumed];
        (void)_events.resize(remaining);
    }

    void StartVoiceLocked(const ScheduledEvent& event, io::u32 startup_delay_frames) noexcept {
        if (event.asset == nullptr || event.asset->frame_count == 0)
            return;

        Voice* voice = nullptr;
        for (io::usize i=0; i<_voices.size(); ++i) {
            if (!_voices[i].active) {
                voice = &_voices[i];
                break;
            }
        }

        if (voice == nullptr) {
            if (_voices.size() < MAX_VOICES) {
                if (!_voices.push_back(Voice{}))
                    return;
                voice = &_voices.back();
            } else {
                voice = SelectVoiceForReplacementLocked(event.category);
                if (voice == nullptr)
                    return;
            }
        }

        if (voice->active)
            EndVoiceLocked(*voice, false);
        else
            DeactivateVoice(*voice);

        const float pan = Clamp(event.pan, -1.0f, 1.0f);
        const float gain = Clamp(event.gain, 0.0f, 4.0f);

        voice->active = true;
        voice->category = event.category;
        voice->asset = event.asset;
        voice->spawn_sequence = event.sequence;
        voice->cursor_fp = 0;
        voice->startup_delay_frames = startup_delay_frames;
        voice->apply_music_fade = IsMusicCategory(event.category);
        voice->fade_out_started = false;
        voice->fade_in_src_frames = 0;
        voice->fade_out_src_frames = 0;
        voice->fade_out_total_out_frames = 0;
        voice->fade_out_progress_out_frames = 0;
        if (voice->apply_music_fade) {
            const io::u32 in_secs = MusicFadeInSecs(event.category);
            const io::u32 out_secs = MusicFadeOutSecs(event.category);
            const io::u32 max_u32 = static_cast<io::u32>(-1);
            if (in_secs > 0) {
                const io::u64 in_frames64 = io::mul_u32(in_secs, event.asset->sample_rate);
                voice->fade_in_src_frames = in_frames64 < static_cast<io::u64>(max_u32) ? static_cast<io::u32>(in_frames64) : max_u32;
            }
            if (out_secs > 0) {
                const io::u64 out_frames64 = io::mul_u32(out_secs, event.asset->sample_rate);
                voice->fade_out_src_frames = out_frames64 < static_cast<io::u64>(max_u32) ? static_cast<io::u32>(out_frames64) : max_u32;
            }
        }

        io::u32 base_step_fp = CURSOR_FRAC_ONE;
        if (_out_sample_rate > 0) {
            const io::u64 numer = (static_cast<io::u64>(event.asset->sample_rate) << CURSOR_FRAC_BITS);
            base_step_fp = io::div_u64_u32(numer, _out_sample_rate);
            if (base_step_fp == 0)
                base_step_fp = 1;
        }

        // Keep freestanding-safe pitch scaling without float->int runtime helpers.
        float pitch = event.pitch;
        if (pitch < 0.375f) {
            voice->step_fp = (base_step_fp > 4) ? (base_step_fp >> 2) : 1;
        } else if (pitch < 0.75f) {
            voice->step_fp = (base_step_fp > 2) ? (base_step_fp >> 1) : 1;
        } else if (pitch < 1.5f) {
            voice->step_fp = base_step_fp;
        } else if (pitch < 3.0f) {
            voice->step_fp = base_step_fp << 1;
        } else {
            voice->step_fp = base_step_fp << 2;
        }
        if (voice->step_fp == 0)
            voice->step_fp = 1;

        // lightweight linear panning (freestanding-friendly, no libm dependency)
        voice->left_gain = gain * (0.5f * (1.0f - pan));
        voice->right_gain = gain * (0.5f * (1.0f + pan));
        voice->eq_low_state[0] = 0.0f; voice->eq_low_state[1] = 0.0f;
        voice->eq_high_state[0] = 0.0f; voice->eq_high_state[1] = 0.0f;
        voice->eq_high_prev_in[0] = 0.0f; voice->eq_high_prev_in[1] = 0.0f;

        if (voice->asset != nullptr) {
            voice->asset->has_last_start = true;
            voice->asset->last_start_frame = event.start_frame;
            const io::usize category_idx = CategoryIndex(event.category);
            if (category_idx < CATEGORY_COUNT)
                _last_started_asset[category_idx] = voice->asset;
        }

        if (event.asset->storage == AssetStorage::Stream) {
            if (event.asset->path.empty() || !voice->stream_file.open(event.asset->path, io::OpenMode::Read)) {
                EndVoiceLocked(*voice, false);
                return;
            }
            voice->stream_cache_start_frame = 0;
            voice->stream_cache_frames = 0;
        }
    }

    static float ReadAssetSample(const Asset& asset, io::u64 frame, io::u32 output_channel) noexcept {
        if (asset.channels == 0 || frame >= asset.frame_count)
            return 0.0f;

        io::u32 channel = output_channel;
        if (asset.channels == 1) {
            channel = 0;
        } else if (channel >= asset.channels) {
            channel = static_cast<io::u32>(asset.channels - 1);
        }

        io::u64 sample_index64 = 0;
        if (!MulU64U32(frame, static_cast<io::u32>(asset.channels), sample_index64))
            return 0.0f;
        sample_index64 += channel;
        if (sample_index64 > static_cast<io::u64>(static_cast<io::usize>(-1)))
            return 0.0f;
        const io::usize sample_index = static_cast<io::usize>(sample_index64);
        if (sample_index >= asset.pcm.size())
            return 0.0f;

        return asset.pcm[sample_index];
    }

private:
    detail::Device _device{};

    io::spin_mutex _mutex{};
    bool _initialized;

    io::u32 _out_sample_rate;
    io::u32 _out_channels;

    float _master_gain;
    float _bus_gains[CATEGORY_COUNT]{};
    float _filter_gains[CATEGORY_COUNT]{};
    float _filter_gain_current[CATEGORY_COUNT]{};
    float _filter_low_targets[CATEGORY_COUNT]{};
    float _filter_mid_targets[CATEGORY_COUNT]{};
    float _filter_high_targets[CATEGORY_COUNT]{};
    float _filter_low_current[CATEGORY_COUNT]{};
    float _filter_mid_current[CATEGORY_COUNT]{};
    float _filter_high_current[CATEGORY_COUNT]{};
    io::u32 _round_robin_index[CATEGORY_COUNT]{};
    Asset* _last_started_asset[CATEGORY_COUNT]{};

    io::vector<io::unique_ptr<Asset> > _assets[CATEGORY_COUNT]{};
    io::vector<Voice> _voices{};
    io::vector<ScheduledEvent> _events{};

    SchedulerPolicy _scheduler_policy{};
    SchedulerCallbacks _scheduler_callbacks{};
    GameCallbacks _game_callbacks{};
    Mood _mood{};
    MusicState _last_music_state = MusicState::Stillness;
    MusicState _scheduler_music_state = MusicState::Stillness;
    bool _scheduler_has_music_state = false;
    io::u64 _music_state_hold_until_frame = 0;
    io::u32 _rand_state = 0x9E3779B9u;
    float _music_duck_current = 1.0f;
    float _ambient_duck_current = 1.0f;
    io::atomic<io::u64> _frames_mixed;
    io::atomic<io::u64> _music_silence_until_frame{ 0 };
    io::atomic<io::u64> _ambient_silence_until_frame{ 0 };
    io::atomic<io::u64> _directive_hold_until_frame{ 0 };
    io::u64 _voice_end_counter[CATEGORY_COUNT]{};
    io::u64 _voice_end_seen_counter[CATEGORY_COUNT]{};
    io::u64 _voice_end_played_frames[CATEGORY_COUNT]{};
    io::u32 _voice_end_natural[CATEGORY_COUNT]{};
    io::u64 _schedule_seq;
    io::u32 _music_fade_in_secs[MUSIC_STATE_COUNT]{};
    io::u32 _music_fade_out_secs[MUSIC_STATE_COUNT]{};
};

} // namespace ac

#endif // AUDIOCONTEXT_HPP


