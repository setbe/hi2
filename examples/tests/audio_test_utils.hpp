#ifndef AUDIO_TEST_UTILS_HPP
#define AUDIO_TEST_UTILS_HPP

#include "../../hi/audiocontext.hpp"

namespace audio_test {

static io::char_view FindAudioRoot() noexcept {
    const io::char_view roots[] = {
        io::char_view{ "resources/audio" },
        io::char_view{ "../resources/audio" },
        io::char_view{ "../../resources/audio" },
        io::char_view{ "../../../resources/audio" },
        io::char_view{ "../../../../resources/audio" }
    };
    for (io::usize i=0; i<(sizeof(roots) / sizeof(roots[0])); ++i)
        if (fs::is_directory(roots[i]))
            return roots[i];
    return io::char_view{};
}

template <io::usize N>
static IO_CONSTEXPR io::usize CountOf(const ac::Fade (&)[N]) noexcept { return N; }

struct AudioFixture {
    ac::AudioContext audio{};
    io::char_view root{};
    bool initialized = false;

    bool Init(io::u32 sample_rate = 44100, io::u32 period_size_in_frames = 1024, io::u32 periods = 4, const ac::Fade* fades = nullptr, io::usize fade_count = 0) noexcept {
        root = FindAudioRoot();
        if (root.empty()) return false;
        ac::DeviceOptions opts{};
        opts.sample_rate = sample_rate;
        opts.period_size_in_frames = period_size_in_frames;
        opts.periods = periods;
        initialized = audio.Init(opts, root, fades, fade_count);
        return initialized;
    }

    void Shutdown() noexcept {
        if (!initialized) return;
        audio.Shutdown();
        initialized = false;
    }

    ~AudioFixture() noexcept {
        Shutdown();
    }
};

} // namespace audio_test

#endif // AUDIO_TEST_UTILS_HPP
