#define IO_IMPLEMENTATION

#include "audio_test_utils.hpp"

struct Mood {
    float strain = 0.0f;
};

struct FadeDriver {
    ac::AudioContext* audio = nullptr;
    bool started = false;
    bool stop_requested = false;
    bool play_failed = false;
};

using MoodCallback = void(*)(void* user, const Mood& mood, io::u32 elapsed_ms);

static void ApplyMood(void* user, const Mood& mood, io::u32 elapsed_ms) noexcept {
    (void)elapsed_ms;
    FadeDriver* driver = reinterpret_cast<FadeDriver*>(user);
    if (driver == nullptr || driver->audio == nullptr) return;
    if (mood.strain > 0.0f) {
        if (!driver->started) {
            if (!driver->audio->PlayMusic(ac::MusicState::Strain, 1.0f, 0.0f, 1.0f))
                driver->play_failed = true;
            else
                driver->started = true;
        }
        return;
    }
    if (driver->started && !driver->stop_requested) {
        driver->audio->StopCategory(ac::Category::Strain);
        driver->stop_requested = true;
    }
}

int main() {
    const ac::Fade fades[] = {
        { ac::MusicState::Strain, 5, 5 },
        { ac::MusicState::Threat, 4, 4 },
        { ac::MusicState::Unreality, 4, 4 }
    };
    audio_test::AudioFixture fx{};
    if (!fx.Init(44100, 1024, 4, fades, audio_test::CountOf(fades))) {
        io::out << "test_fade_in_out: failed to init audio/resources\n";
        return 1;
    }
    fx.audio.SetCategoryGain(ac::Category::Strain, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Threat, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Unreality, 1.0f);

    Mood mood{};
    FadeDriver driver{};
    driver.audio = &fx.audio;
    MoodCallback callback = &ApplyMood;

    static const io::u32 INTRO_MS = 3000;
    static const io::u32 FADE_MS = 5000;
    static const io::u32 HOLD_MS = 5000;
    static const io::u32 TOTAL_MS = INTRO_MS + FADE_MS + HOLD_MS + FADE_MS;
    static const io::u32 TICK_MS = 20;

    for (io::u32 elapsed_ms = 0; elapsed_ms <= TOTAL_MS; elapsed_ms += TICK_MS) {
        mood.strain = (elapsed_ms >= INTRO_MS && elapsed_ms < (INTRO_MS + FADE_MS + HOLD_MS)) ? 1.0f : 0.0f;
        callback(&driver, mood, elapsed_ms);
        io::sleep_ms(TICK_MS);
    }

    if (driver.play_failed) {
        io::out << "test_fade_in_out: failed to start strain track\n";
        return 2;
    }
    if (!driver.started) {
        io::out << "test_fade_in_out: callback never started playback\n";
        return 3;
    }
    if (!driver.stop_requested) {
        io::out << "test_fade_in_out: soundtrack stop was never requested\n";
        return 4;
    }

    io::out << "test_fade_in_out: phase2 explicit crossfade start\n";
    if (!fx.audio.PlayMusic(ac::MusicState::Strain, 1.0f, 0.0f, 1.0f)) {
        io::out << "test_fade_in_out: phase2 failed to start source track\n";
        return 5;
    }
    io::sleep_ms(3500);

    fx.audio.StopCategory(ac::Category::Strain);
    bool cross_target_started = fx.audio.PlayMusic(ac::MusicState::Threat, 1.0f, 0.0f, 1.0f);
    if (!cross_target_started)
        cross_target_started = fx.audio.PlayMusic(ac::MusicState::Unreality, 1.0f, 0.0f, 1.0f);
    if (!cross_target_started) {
        io::out << "test_fade_in_out: phase2 failed to start target track\n";
        return 6;
    }

    io::out << "test_fade_in_out: phase2 crossfade running (source fade-out + target fade-in)\n";
    io::sleep_ms(6500);
    fx.audio.StopMusic();

    io::out << "test_fade_in_out: ok\n";
    return 0;
}
