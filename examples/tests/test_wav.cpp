#define IO_IMPLEMENTATION

#include "audio_test_utils.hpp"

int main() {
    audio_test::AudioFixture fx{};
    if (!fx.Init()) {
        io::out << "test_wav: failed to init audio/resources\n";
        return 1;
    }
    if (!fx.audio.PlaySfx(io::char_view{ "sfx1" }, 1.0f, 0.0f, 1.0f, 0.0)) {
        io::out << "test_wav: failed to play sfx1 (check resources/audio/sfx/sfx1.wav)\n";
        return 2;
    }
    if (!fx.audio.PlayMusic(ac::MusicState::Wander, 0.20f, 0.0f, 1.0f))
        io::out << "test_wav: wander stream was not started (no files?)\n";

    io::out << "test_wav: playing sfx + wander...\n";
    io::sleep_ms(2500);
    io::out << "test_wav: done\n";
    return 0;
}
