#define IO_IMPLEMENTATION

#include "audio_test_utils.hpp"

using Mood = ac::Mood;

struct TimelineEvent {
    io::u32 at_ms = 0;
    Mood mood{};
    io::char_view label{};
};

struct Driver {
    Mood mood{};
    io::u32 next_timeline_event = 0;
    bool pending_stop_music = false;
    bool pending_stop_ambient = false;
    io::u32 music_block_until_ms = 0;
    io::u32 next_log_ms = 0;

    io::u32 music_started = 0;
    io::u32 ambient_started = 0;
    io::u32 transitions_checked = 0;
    io::u32 illegal_transitions = 0;
    io::u32 seen_music_mask = 0;
    bool has_observed_music = false;
    ac::MusicState observed_music_state = ac::MusicState::Stillness;

    bool scenario_finished = false;
    bool shutdown_requested = false;
};

static io::char_view CategoryName(ac::Category category) noexcept {
    switch (category) {
    case ac::Category::Stillness: return io::char_view{ "stillness" };
    case ac::Category::Wander: return io::char_view{ "wander" };
    case ac::Category::Strain: return io::char_view{ "strain" };
    case ac::Category::Threat: return io::char_view{ "threat" };
    case ac::Category::Unreality: return io::char_view{ "unreality" };
    case ac::Category::Ambient: return io::char_view{ "ambient" };
    case ac::Category::SFX: return io::char_view{ "sfx" };
    case ac::Category::UI: return io::char_view{ "ui" };
    default: return io::char_view{ "unknown" };
    }
}

static io::char_view MusicStateName(ac::MusicState state) noexcept {
    switch (state) {
    case ac::MusicState::Stillness: return io::char_view{ "stillness" };
    case ac::MusicState::Wander: return io::char_view{ "wander" };
    case ac::MusicState::Strain: return io::char_view{ "strain" };
    case ac::MusicState::Threat: return io::char_view{ "threat" };
    case ac::MusicState::Unreality: return io::char_view{ "unreality" };
    default: return io::char_view{ "unknown" };
    }
}

static IO_CONSTEXPR io::u32 Seconds(io::u32 secs) noexcept { return secs * 1000u; }
static IO_CONSTEXPR io::u32 Minutes(io::u32 mins) noexcept { return mins * 60000u; }
static float Clamp01(float v) noexcept {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static bool IsMusicEdgeAllowed(ac::MusicState from, ac::MusicState to) noexcept {
    if (from == to) return true;
    if (from == ac::MusicState::Stillness && to == ac::MusicState::Wander) return true;
    if (from == ac::MusicState::Wander && (to == ac::MusicState::Stillness || to == ac::MusicState::Strain || to == ac::MusicState::Unreality)) return true;
    if (from == ac::MusicState::Strain && (to == ac::MusicState::Wander || to == ac::MusicState::Threat || to == ac::MusicState::Unreality)) return true;
    if (from == ac::MusicState::Threat && to == ac::MusicState::Strain) return true;
    if (from == ac::MusicState::Unreality && (to == ac::MusicState::Strain || to == ac::MusicState::Wander)) return true;
    return false;
}

static bool SelectMusicFromMood(const Mood& mood, ac::MusicState& out_state, float& out_activation) noexcept {
    out_state = ac::MusicState::Stillness;
    out_activation = mood.stillness;

    if (mood.wander > out_activation) {
        out_state = ac::MusicState::Wander;
        out_activation = mood.wander;
    }
    if (mood.strain > out_activation) {
        out_state = ac::MusicState::Strain;
        out_activation = mood.strain;
    }
    if (mood.threat > out_activation) {
        out_state = ac::MusicState::Threat;
        out_activation = mood.threat;
    }
    if (mood.unreality > out_activation) {
        out_state = ac::MusicState::Unreality;
        out_activation = mood.unreality;
    }

    if (out_activation <= 0.01f) return false;
    out_activation = Clamp01(out_activation);
    return true;
}

static void OnMoodChanged(Driver& driver, io::u32 elapsed_ms, const Mood& prev, const Mood& next) noexcept {
    static const io::u32 MUSIC_CROSSFADE_OVERLAP_MS = Seconds(5);

    ac::MusicState prev_state = ac::MusicState::Stillness;
    ac::MusicState next_state = ac::MusicState::Stillness;
    float prev_activation = 0.0f;
    float next_activation = 0.0f;
    const bool prev_has_music = SelectMusicFromMood(prev, prev_state, prev_activation);
    const bool next_has_music = SelectMusicFromMood(next, next_state, next_activation);

    if (prev_has_music && (!next_has_music || prev_state != next_state))
        driver.pending_stop_music = true;

    if (prev_has_music && next_has_music && prev_state != next_state)
        driver.music_block_until_ms = elapsed_ms + MUSIC_CROSSFADE_OVERLAP_MS;

    const bool prev_has_ambient = prev.ambient > 0.01f;
    const bool next_has_ambient = next.ambient > 0.01f;
    if (prev_has_ambient && !next_has_ambient)
        driver.pending_stop_ambient = true;
}

struct VoiceLine {
    ac::Category category = ac::Category::SFX;
    io::char_view asset_id{};
    io::u64 cursor_frames = 0;
    io::u64 total_frames = 0;
    bool streamed = false;
};

struct VoiceDumpState {
    VoiceLine lines[64]{};
    io::u32 count = 0;
};

static void CaptureVoice(void* user, ac::Category category, io::char_view asset_id, io::u64 cursor_frames, io::u64 total_frames, bool streamed) noexcept {
    VoiceDumpState* state = reinterpret_cast<VoiceDumpState*>(user);
    if (state == nullptr || state->count >= 64) return;

    VoiceLine& line = state->lines[state->count];
    line.category = category;
    line.asset_id = asset_id;
    line.cursor_frames = cursor_frames;
    line.total_frames = total_frames;
    line.streamed = streamed;
    ++state->count;
}

static void PrintRuntimeState(ac::AudioContext& audio, const Driver& driver, io::u32 elapsed_ms, const ac::SchedulerTickResult& step) noexcept {
    ac::RuntimeSnapshot snapshot{};
    VoiceDumpState dump{};
    audio.CollectRuntimeSnapshot(snapshot, &CaptureVoice, &dump);

    const io::u32 stillness = snapshot.by_category[static_cast<io::usize>(ac::Category::Stillness)];
    const io::u32 wander = snapshot.by_category[static_cast<io::usize>(ac::Category::Wander)];
    const io::u32 strain = snapshot.by_category[static_cast<io::usize>(ac::Category::Strain)];
    const io::u32 threat = snapshot.by_category[static_cast<io::usize>(ac::Category::Threat)];
    const io::u32 unreality = snapshot.by_category[static_cast<io::usize>(ac::Category::Unreality)];
    const io::u32 ambient = snapshot.by_category[static_cast<io::usize>(ac::Category::Ambient)];
    const io::u32 sfx = snapshot.by_category[static_cast<io::usize>(ac::Category::SFX)];
    const io::u32 ui = snapshot.by_category[static_cast<io::usize>(ac::Category::UI)];

    io::out << "\n[scheduler] t=" << elapsed_ms << "ms"
            << " mood(stillness=" << driver.mood.stillness
            << ", wander=" << driver.mood.wander
            << ", strain=" << driver.mood.strain
            << ", threat=" << driver.mood.threat
            << ", unreality=" << driver.mood.unreality
            << ", ambient=" << driver.mood.ambient << ")"
            << " tick(music_started=" << (step.music_started ? 1 : 0)
            << ", ambient_started=" << (step.ambient_started ? 1 : 0)
            << ", directive_consumed=" << (step.directive_consumed ? 1 : 0)
            << ", directive_applied=" << (step.directive_applied ? 1 : 0) << ")"
            << " voices=" << snapshot.active_voices << "/" << snapshot.voice_capacity
            << " queued=" << snapshot.scheduled_events
            << " silence(music=" << (snapshot.music_silence_active ? 1 : 0)
            << ", ambient=" << (snapshot.ambient_silence_active ? 1 : 0) << ")"
            << " active_music=" << (snapshot.has_active_music_state ? MusicStateName(snapshot.active_music_state) : io::char_view{ "none" })
            << '\n';

    io::out << "  by_category: stillness=" << stillness
            << " wander=" << wander
            << " strain=" << strain
            << " threat=" << threat
            << " unreality=" << unreality
            << " ambient=" << ambient
            << " sfx=" << sfx
            << " ui=" << ui
            << '\n';

    if (dump.count == 0) {
        io::out << "  voice list is empty\n";
        return;
    }

    for (io::u32 i=0; i<dump.count; ++i) {
        const VoiceLine& line = dump.lines[i];
        io::out << "  voice[" << i << "]"
                << " category=" << CategoryName(line.category)
                << " id=" << line.asset_id
                << " storage=" << (line.streamed ? io::char_view{ "stream" } : io::char_view{ "memory" })
                << " frame=" << line.cursor_frames << "/" << line.total_frames
                << '\n';
    }
}

static bool ApplyTimeline(ac::AudioContext& audio, Driver& driver, io::u32 elapsed_ms, const TimelineEvent* events, io::u32 event_count) noexcept {
    bool changed = false;
    while (driver.next_timeline_event < event_count && elapsed_ms >= events[driver.next_timeline_event].at_ms) {
        const Mood prev = driver.mood;
        const TimelineEvent& ev = events[driver.next_timeline_event];
        driver.mood = ev.mood;
        audio.SetMood(driver.mood);
        OnMoodChanged(driver, elapsed_ms, prev, ev.mood);
        io::out << "[timeline] t=" << elapsed_ms << "ms -> " << ev.label << '\n';
        ++driver.next_timeline_event;
        changed = true;
    }
    if (driver.next_timeline_event >= event_count)
        driver.scenario_finished = true;
    return changed;
}

static void UpdateTransitionStats(Driver& driver, const ac::RuntimeSnapshot& snapshot) noexcept {
    if (!snapshot.has_active_music_state) {
        driver.has_observed_music = false;
        return;
    }

    const io::u32 state_bit = 1u << static_cast<io::u32>(snapshot.active_music_state);
    driver.seen_music_mask |= state_bit;

    if (driver.has_observed_music && snapshot.active_music_state != driver.observed_music_state) {
        ++driver.transitions_checked;
        const bool allowed = IsMusicEdgeAllowed(driver.observed_music_state, snapshot.active_music_state);
        if (!allowed) ++driver.illegal_transitions;
        io::out << "[graph] " << MusicStateName(driver.observed_music_state)
                << " -> " << MusicStateName(snapshot.active_music_state)
                << (allowed ? io::char_view{ " (ok)" } : io::char_view{ " (ILLEGAL)" })
                << '\n';
    }

    driver.observed_music_state = snapshot.active_music_state;
    driver.has_observed_music = true;
}

static void MusicSelectionCallback(void* user, io::u32 elapsed_ms, const Mood& mood, ac::MusicSelection& out) noexcept {
    Driver* d = reinterpret_cast<Driver*>(user);
    if (d == nullptr) return;

    ac::MusicState state = ac::MusicState::Stillness;
    float activation = 0.0f;
    if (!SelectMusicFromMood(mood, state, activation))
        return;

    if (elapsed_ms < d->music_block_until_ms)
        return;

    out.request_play = true;
    out.force = false;
    out.bypass_silence = false;
    out.state = state;
    out.activation = activation;
    out.gain = 1.0f;
    out.pan = 0.0f;
    out.pitch = 1.0f;
}

static void AmbientSelectionCallback(void* user, io::u32 elapsed_ms, const Mood& mood, ac::AmbientSelection& out) noexcept {
    (void)elapsed_ms;
    Driver* d = reinterpret_cast<Driver*>(user);
    if (d == nullptr) return;
    if (mood.ambient <= 0.10f) return;

    out.request_play = true;
    out.allow_overlap = false;
    out.bypass_silence = false;
    out.activation = Clamp01(mood.ambient);
    out.gain = 0.30f + (0.50f * Clamp01(mood.ambient));
    out.pan = 0.0f;
    out.pitch = 1.0f;
}

static void MusicFilterCallback(void* user, io::u32 elapsed_ms, const Mood& mood, ac::MusicState state, ac::FilterParams& out) noexcept {
    (void)elapsed_ms;
    (void)state;
    Driver* d = reinterpret_cast<Driver*>(user);
    if (d == nullptr) return;

    const float pressure = Clamp01(mood.strain + mood.threat);
    const float surreal = Clamp01(mood.unreality);
    out.gain = 1.0f;
    out.low_pass = 1.0f - (0.20f * pressure);
    out.mid_pass = 1.0f - (0.15f * surreal);
    out.high_pass = 1.0f - (0.35f * pressure);
}

static void AmbientFilterCallback(void* user, io::u32 elapsed_ms, const Mood& mood, ac::FilterParams& out) noexcept {
    (void)elapsed_ms;
    Driver* d = reinterpret_cast<Driver*>(user);
    if (d == nullptr) return;

    const float calm = Clamp01(mood.stillness + mood.wander);
    const float surreal = Clamp01(mood.unreality);
    out.gain = 0.90f + (0.20f * Clamp01(mood.ambient));
    out.low_pass = 0.85f + (0.15f * calm);
    out.mid_pass = 1.0f;
    out.high_pass = 0.80f + (0.20f * surreal);
}

static void DirectiveCallback(void* user, io::u32 elapsed_ms, const Mood& mood, ac::Directive& out) noexcept {
    (void)elapsed_ms;
    (void)mood;
    Driver* d = reinterpret_cast<Driver*>(user);
    if (d == nullptr) return;

    if (d->pending_stop_music) {
        out.type = ac::DirectiveType::StopMusic;
        d->pending_stop_music = false;
        return;
    }

    if (d->pending_stop_ambient) {
        out.type = ac::DirectiveType::StopAmbient;
        d->pending_stop_ambient = false;
    }
}

int main() {
    static const io::u32 MUSIC_FADE_SECS = 10;
    static const io::u32 LOG_EVERY_MS = 2500;
    static const io::u32 TICK_MS = 20;

    static const TimelineEvent timeline[] = {
        { 0,                         Mood{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.45f }, io::char_view{ "spawn: day/surface -> stillness" } },
        { Minutes(5),                Mood{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.15f }, io::char_view{ "night start: stop music" } },
        { Minutes(8),                Mood{ 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.45f }, io::char_view{ "day again: wander" } },
        { Minutes(10),               Mood{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.30f }, io::char_view{ "after 2 min: unreality" } },
        { Minutes(13),               Mood{ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.25f }, io::char_view{ "after 3 min: strain" } },
        { Minutes(17),               Mood{ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.20f }, io::char_view{ "after 4 min: threat" } },
        { Minutes(17) + Seconds(30), Mood{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.40f }, io::char_view{ "after 30 sec: back to stillness" } }
    };
    static const io::u32 TIMELINE_COUNT = static_cast<io::u32>(sizeof(timeline) / sizeof(timeline[0]));

    const ac::Fade fades[] = {
        { ac::MusicState::Stillness, MUSIC_FADE_SECS, MUSIC_FADE_SECS },
        { ac::MusicState::Wander, MUSIC_FADE_SECS, MUSIC_FADE_SECS },
        { ac::MusicState::Strain, MUSIC_FADE_SECS, MUSIC_FADE_SECS },
        { ac::MusicState::Threat, MUSIC_FADE_SECS, MUSIC_FADE_SECS },
        { ac::MusicState::Unreality, MUSIC_FADE_SECS, MUSIC_FADE_SECS }
    };

    audio_test::AudioFixture fx{};
    if (!fx.Init(44100, 1024, 4, fades, audio_test::CountOf(fades))) {
        io::out << "test_audio_scheduler: failed to init audio/resources\n";
        return 1;
    }

    fx.audio.SetMasterGain(0.95f);
    fx.audio.SetCategoryGain(ac::Category::Stillness, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Wander, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Strain, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Threat, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Unreality, 1.0f);
    fx.audio.SetCategoryGain(ac::Category::Ambient, 0.70f);
    fx.audio.SetCategoryGain(ac::Category::SFX, 0.90f);
    fx.audio.SetCategoryGain(ac::Category::UI, 1.00f);

    ac::SchedulerPolicy policy{};
    policy.use_music_transition_graph = true;
    policy.music_min_hold_ms = 1400;
    policy.music_asset_cooldown_ms = 3000;
    policy.ambient_asset_cooldown_ms = 4000;
    policy.music_silence_secs = 8;
    policy.music_rand_silence_secs = 4;
    policy.ambient_silence_secs = 8;
    policy.ambient_rand_silence_secs = 4;
    policy.filter_smoothing = 0.40f;
    policy.duck_smoothing = 0.40f;
    policy.music_duck_by_sfx = 0.80f;
    policy.music_duck_by_ui = 0.70f;
    policy.ambient_duck_by_sfx = 0.74f;
    policy.ambient_duck_by_ui = 0.62f;
    fx.audio.SetSchedulerPolicy(policy);
    io::out << "[policy] music_silence=" << policy.music_silence_secs << ".." << (policy.music_silence_secs + policy.music_rand_silence_secs)
            << " sec ambient_silence=" << policy.ambient_silence_secs << ".." << (policy.ambient_silence_secs + policy.ambient_rand_silence_secs)
            << " sec fade=10 sec overlap=5 sec\n";

    Driver driver{};
    driver.next_log_ms = 0;
    driver.mood = Mood{};

    fx.audio.SetMood(driver.mood);

    ac::GameCallbacks callbacks{};
    callbacks.user = &driver;
    callbacks.music_selection = &MusicSelectionCallback;
    callbacks.ambient_selection = &AmbientSelectionCallback;
    callbacks.music_filter = &MusicFilterCallback;
    callbacks.ambient_filter = &AmbientFilterCallback;
    callbacks.directive = &DirectiveCallback;
    fx.audio.SetGameCallbacks(callbacks);

    const io::u32 timeline_end_ms = timeline[TIMELINE_COUNT - 1].at_ms + Minutes(1);
    const io::u32 hard_timeout_ms = timeline_end_ms + Minutes(4);

    for (io::u32 elapsed_ms = 0; elapsed_ms <= hard_timeout_ms; elapsed_ms += TICK_MS) {
        (void)ApplyTimeline(fx.audio, driver, elapsed_ms, timeline, TIMELINE_COUNT);
        const ac::SchedulerTickResult step = fx.audio.Update(elapsed_ms);
        if (step.music_started) ++driver.music_started;
        if (step.ambient_started) ++driver.ambient_started;

        ac::RuntimeSnapshot snapshot{};
        fx.audio.CollectRuntimeSnapshot(snapshot, nullptr, nullptr);
        UpdateTransitionStats(driver, snapshot);

        if (elapsed_ms >= driver.next_log_ms || step.music_started || step.ambient_started || step.directive_consumed) {
            PrintRuntimeState(fx.audio, driver, elapsed_ms, step);
            driver.next_log_ms = elapsed_ms + LOG_EVERY_MS;
        }

        if (!driver.shutdown_requested && elapsed_ms >= timeline_end_ms && driver.scenario_finished) {
            driver.shutdown_requested = true;
            fx.audio.ClearGameCallbacks();
            fx.audio.StopMusic();
            fx.audio.StopCategory(ac::Category::Ambient);
            io::out << "[scheduler] transitions completed, waiting for active voices to end...\n";
        }

        if (driver.shutdown_requested)
            if (fx.audio.ActiveVoiceCount() == 0 && fx.audio.ScheduledEventCount() == 0)
                break;

        io::sleep_ms(TICK_MS);
    }

    const io::u32 active_after = fx.audio.ActiveVoiceCount();
    const io::u32 queued_after = fx.audio.ScheduledEventCount();
    const io::u32 expected_mask =
        (1u << static_cast<io::u32>(ac::MusicState::Stillness)) |
        (1u << static_cast<io::u32>(ac::MusicState::Wander)) |
        (1u << static_cast<io::u32>(ac::MusicState::Strain)) |
        (1u << static_cast<io::u32>(ac::MusicState::Threat)) |
        (1u << static_cast<io::u32>(ac::MusicState::Unreality));

    io::out << "[summary] music_started=" << driver.music_started
            << " ambient_started=" << driver.ambient_started
            << " transitions_checked=" << driver.transitions_checked
            << " illegal_transitions=" << driver.illegal_transitions
            << " seen_mask=" << driver.seen_music_mask
            << " active_after=" << active_after
            << " queued_after=" << queued_after
            << '\n';

    if (!driver.shutdown_requested) return 2;
    if (driver.music_started == 0) return 3;
    if (driver.ambient_started == 0) return 4;
    if (driver.illegal_transitions != 0) return 5;
    if ((driver.seen_music_mask & expected_mask) != expected_mask) return 6;
    if (active_after != 0 || queued_after != 0) return 7;

    io::out << "Tests ended Continued to play until last sound end.\n";
    return 0;
}
