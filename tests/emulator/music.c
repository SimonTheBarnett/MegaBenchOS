/**
 * @file tests/emulator/music.c
 * @brief Plays all five ROM songs and checks duration, Stop, credits and malformed file handling.
 *
 * Unity integration fixture. Drives controller/mouse input and reads observations; no RAM
 * injection.
 */
#include "../../tools/native/runner.h"
static Words Player(void) {
    return W("mb_player_observation", 5);
}

static void OpenPlayer(void) {
    Root();
    SelectEntry(0);
    Press(1);
    SelectEntry(1);
    Press(1);
    A(D("app_id") == 13 && Ui().v[0] == 1, "MegaPlayer did not maximise");
}
static void Silent(void) {
    Advance(Hz);
    A(core_peak(15) < 8 && D("audio_active") == 0, "Music/PSG remained audible after stop");
}
/** Let real ROM tracks finish; compare frame duration with licensed source timing. */
void scenario_music(void) {
    int heap = D("heap_used");
    if (!strcmp(Scenario, "contracts-music")) {
        OpenPlayer();
        Click(22, 0);
        Capture("ram-files");
        Click(2, 2);
        Click(4, 18);
        A(Player().v[3] == 2 && Player().v[4] == 65525, "Malformed XGC did not report CORRUPT");
        Silent();
        Click(2, 4);
        Click(4, 18);
        A(Player().v[4] == 65524, "Raw VGM did not report NOT_SUPPORTED");
        Silent();
        Capture("vgm-unsupported");
        Press(0);
        Root();
        SelectEntry(5);
        Press(1);
        SelectEntry(2);
        Press(1);
        A(Shell().v[0] == 10, "RAM Music folder missing");
        SelectEntry(1);
        Press(1);
        A(D("app_id") == 13 && Player().v[0] == 1 && Player().v[4] == 65524,
          "Music association did not open safely");
        Press(0);
        SelectEntry(1);
        Press(8);
        Click(17, 3);
        Click(8, 11);
        Click(24, 15);
        A(Shell().v[3] == 0, "Music rename failed");
        Focus();
        Press(1);
        A(Player().v[4] == 65524, "Rename lost .vgm association");
        Press(0);
        Focus();
        Press(8);
        Click(2, 6);
        Click(2, 6);
        A(Shell().v[3] == 0, "Confirmed delete failed");
        Capture("music-after-delete");
        A(D("heap_used") == heap && D("stack_guard_ok") == 1, "Music file flow memory regression");
        json_str(Report, "checks",
                 "Malformed XGC/raw VGM errors; music association, rename and delete");
        return;
    }
    OpenPlayer();
    Capture("catalog");
    for (int i = 0; i < 5; i++)
        Press(5);
    Press(1);
    A(Player().v[1] == 0, "Controller changed track unexpectedly");
    Advance(Round(Hz / 3.0));
    A(Player().v[3] == 1 && D("audio_active") == 3 && core_peak(8) > 80,
      "First XGM2 track did not play");
    Capture("playing");
    Click(18, 18);
    Silent();
    cJSON *provenance = json_read(mb_path("third_party/music/safetystoat/provenance.json")),
          *source = json_get(provenance, "tracks");
    cJSON *tracks = cJSON_CreateArray();
    for (int index = 0; index < 5; index++) {
        Click(2, 2 + index * 2);
        A(Player().v[1] == index, mb_fmt("Track selection failed %d", index));
        Click(4, 18);
        uint32_t started = U("started");
        core_start_audio();
        int frames = 0, peak = 0;
        do {
            Advance(1);
            frames++;
            peak = MAX(peak, core_peak(1));
            if (frames == Hz * 3)
                core_save_audio(mb_fmt("%s/%s", Output, mb_fmt("track-%d.wav", index)));
        } while (Player().v[3] == 1 && frames < Hz * 245);
        A(peak > 80 && frames < Hz * 245 && Player().v[3] == 0,
          mb_fmt("Track playback/completion failed %d", index));
        uint32_t elapsed = U("vtimer") - started;
        Silent();
        double expected =
                   json_number(json_get(cJSON_GetArrayItem(source, index), "samples")) / 44100,
               seconds = elapsed / (double)Hz;
        A(fabs(seconds - expected) < 0.1 + expected * 0.005,
          mb_fmt("Track duration differs from source %d: %.3f vs %.3f", index, seconds, expected));
        Click(4, 18);
        Advance(Round(Hz / 4.0));
        A(Player().v[3] == 1, mb_fmt("Track did not restart %d", index));
        Click(18, 18);
        Silent();
        cJSON *track = cJSON_CreateObject();
        json_num(track, "index", index);
        json_num(track, "duration_frames", elapsed);
        json_num(track, "duration_seconds", nearbyint(seconds * 1000) / 1000);
        json_num(track, "source_seconds", expected);
        json_num(track, "peak", peak);
        json_str(track, "loop_behavior", "no source loop; ends once");
        json_str(track, "restart_and_stop", "PASS");
        json_str(track, "status", "PASS");
        cJSON_AddItemToArray(tracks, track);
    }
    Click(2, 4);
    Click(4, 18);
    Click(32, 18);
    Capture("credits-spring");
    A(Player().v[3] == 1, "Credits stopped playback");
    Click(3, 18);
    Capture("catalog-long-titles");
    Click(18, 18);
    Silent();
    Click(2, 2);
    Click(4, 18);
    Click(2, 4);
    Click(4, 18);
    Advance(5);
    A(Player().v[3] == 1, "Track switch failed");
    Click(22, 0);
    A(Player().v[0] == 1, "RAM source failed");
    Silent();
    Capture("ram-empty");
    Click(2, 0);
    Click(4, 18);
    Press(0);
    A(D("app_id") == 65535, "Close failed");
    Silent();
    for (int cycle = 1; cycle < Cycles; cycle++) {
        OpenPlayer();
        Click(4, 18);
        Advance(10);
        Press(0);
        Silent();
    }
    Root();
    SelectEntry(0);
    Press(1);
    SelectEntry(4);
    Press(1);
    A(D("app_id") == 5, "Sound Test missing");
    Click(2, 3);
    Advance(Hz);
    A(D("audio_active") == 1 && core_peak(10) > 80, "PSG failed after music");
    Press(0);
    Silent();
    A(D("heap_used") == heap && D("stack_guard_ok") == 1, "Music heap/stack regression");
    json_set(Report, "tracks", tracks);
    cJSON_Delete(provenance);
    json_str(Report, "external_playback", "NOT_SUPPORTED by design");
}
