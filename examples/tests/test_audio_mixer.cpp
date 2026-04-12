#define IO_IMPLEMENTATION

#include "audio_test_utils.hpp"

struct Mood {
    float strain_level = 0.0f;
    float ambient_level = 0.0f;
    bool allow_ambient_overlap = false;
};

struct MixerDriver {
    ac::AudioContext* audio = nullptr;
    bool strain_started = false;
    io::u32 strain_starts = 0;
    io::u32 ambient_starts = 0;
    io::u32 failures = 0;
    io::u32 next_ambient_ms = 2000;
    io::u32 sfx_calls = 0;
};

using MusicMoodCallback = void(*)(void* user, const Mood& mood, io::u32 elapsed_ms);
using AmbientMoodCallback = void(*)(void* user, const Mood& mood, io::u32 elapsed_ms);

static void MusicCallback(void* user, const Mood& mood, io::u32 elapsed_ms) noexcept {
    (void)elapsed_ms;
    MixerDriver* d = reinterpret_cast<MixerDriver*>(user);
    if (d == nullptr || d->audio == nullptr) return;
    d->audio->SetCategoryGain(ac::Category::Strain, mood.strain_level);
    if (mood.strain_level <= 0.10f || d->strain_started) return;
    if (d->audio->PlayMusic(ac::MusicState::Strain, 1.0f, 0.0f, 1.0f)) {
        d->strain_started = true;
        ++d->strain_starts;
    } else {
        ++d->failures;
    }
}

static void AmbientCallback(void* user, const Mood& mood, io::u32 elapsed_ms) noexcept {
    MixerDriver* d = reinterpret_cast<MixerDriver*>(user);
    if (d == nullptr || d->audio == nullptr) return;
    d->audio->SetCategoryGain(ac::Category::Ambient, 0.20f + (mood.ambient_level * 0.45f));
    if (mood.ambient_level <= 0.05f || elapsed_ms < d->next_ambient_ms) return;
    if (d->audio->PlayAmbient(0.55f, 0.0f, 1.0f, mood.allow_ambient_overlap))
        ++d->ambient_starts;
    else
        ++d->failures;
    d->next_ambient_ms = elapsed_ms + (mood.allow_ambient_overlap ? 3500u : 8000u);
}

static bool ScheduleSfx(ac::AudioContext& audio, io::char_view id, double delay_seconds, float gain, float pan) noexcept {
    ac::PlayRequest req{};
    req.category = ac::Category::SFX;
    req.asset_id = id;
    req.gain = gain;
    req.pan = pan;
    req.pitch = 1.0f;
    req.delay_seconds = delay_seconds;
    return audio.SchedulePlay(req);
}

int main() {
    const ac::Fade fades[] = {
        { ac::MusicState::Strain, 2, 2 }
    };
    audio_test::AudioFixture fx{};
    if (!fx.Init(44100, 1024, 4, fades, audio_test::CountOf(fades))) {
        io::out << "test_audio_mixer: failed to init audio/resources\n";
        return 1;
    }
    fx.audio.SetMasterGain(0.85f);
    fx.audio.SetCategoryGain(ac::Category::Strain, 0.5f);
    fx.audio.SetCategoryGain(ac::Category::Ambient, 0.25f);
    fx.audio.SetCategoryGain(ac::Category::SFX, 0.20f);

    MixerDriver driver{};
    driver.audio = &fx.audio;
    MusicMoodCallback music_callback = &MusicCallback;
    AmbientMoodCallback ambient_callback = &AmbientCallback;
    Mood mood{};

    static const io::u32 TICK_MS = 20;
    static const io::u32 TOTAL_MS = 22000;

    for (io::u32 elapsed_ms = 0; elapsed_ms <= TOTAL_MS; elapsed_ms += TICK_MS) {
        if (elapsed_ms < 2000) {
            mood.strain_level = 0.0f;
            mood.ambient_level = 0.0f;
            mood.allow_ambient_overlap = false;
        } else if (elapsed_ms < 7000) {
            mood.strain_level = static_cast<float>(elapsed_ms - 2000) / 5000.0f;
            mood.ambient_level = 0.55f;
            mood.allow_ambient_overlap = false;
        } else if (elapsed_ms < 14000) {
            mood.strain_level = 1.0f;
            mood.ambient_level = 0.70f;
            mood.allow_ambient_overlap = true;
        } else {
            mood.strain_level = 0.60f;
            mood.ambient_level = 0.45f;
            mood.allow_ambient_overlap = true;
        }

        music_callback(&driver, mood, elapsed_ms);
        ambient_callback(&driver, mood, elapsed_ms);
        io::sleep_ms(TICK_MS);
    }

    if (ScheduleSfx(fx.audio, io::char_view{ "sfx1" }, 0.00, 0.30f, -0.25f)) ++driver.sfx_calls; else ++driver.failures;
    if (ScheduleSfx(fx.audio, io::char_view{ "sfx2" }, 0.10, 0.20f, 0.00f)) ++driver.sfx_calls; else ++driver.failures;
    if (ScheduleSfx(fx.audio, io::char_view{ "sfx3" }, 0.22, 0.10f, 0.25f)) ++driver.sfx_calls; else ++driver.failures;
    io::sleep_ms(1200);

    fx.audio.StopCategory(ac::Category::SFX);
    fx.audio.StopCategory(ac::Category::Ambient);
    fx.audio.StopCategory(ac::Category::Strain);

    if (driver.strain_starts == 0) return 2;
    if (driver.ambient_starts == 0) return 3;
    if (driver.sfx_calls != 3) return 4;
    if (driver.failures != 0) return 5;

    io::out << "test_audio_mixer: ok\n";
    return 0;
}
