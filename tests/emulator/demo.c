/**
 * @file tests/emulator/demo.c
 * @brief Checks the four fullscreen demos, animation pacing, mouse launch and resource restoration.
 *
 * Unity integration fixture. Drives controller/mouse input and reads observations; no RAM
 * injection.
 */
#include "../../tools/native/runner.h"

static int id, index;
static int heap;

static void Open(void) {
    Root();
    SelectEntry(2);
    Press(1);
    SelectEntry(index);
    Press(1);
    Advance(Hz * 2);
    A(D("app_id") == id, "Demo launch failed");
    A(Ui().v[0] == 2 && Ui().v[6] != 0, "Exclusive mode/lease failed");
}
static void Restored(void) {
    Advance(Hz);
    A(D("app_id") == 65535 && D("audio_active") == 0 && D("heap_used") == heap &&
          D("stack_guard_ok") == 1,
      "Demo cleanup failed");
    A(Ui().v[6] == 0 && Shell().v[0] == 3, "Demo lease/folder restoration failed");
    A(core_peak(10) <= 2, "PSG remained after exit");
}
/** Sample actual sea/shore pixels so a changing timer alone cannot pass animation. */
static Hash OceanPixels(int top, int bottom) {
    uint32_t pixels[4096];
    size_t n = 0;
    for (int y = top; y <= bottom; y += 2)
        for (int x = 96; x < 320; x += 16)
            pixels[n++] = core_pixel(x, y);
    return mb_hash(pixels, n * sizeof(uint32_t));
}
void scenario_demo(int demo) {
    id = demo;
    index = demo == 7 ? 2 : demo == 8 ? 3 : demo == 9 ? 0 : 1;
    heap = D("heap_used");
    core_start_audio();
    Open();
    Hash first = Capture("first");
    uint32_t updates = U("mb_demo_timing"), ticks = U("mb_demo_timing", 4);
    Advance(Hz);
    Hash moving = Capture("moving");
    updates = U("mb_demo_timing") - updates;
    ticks = U("mb_demo_timing", 4) - ticks;
    A(updates >= Hz - 1 && ticks >= 294 && ticks <= 306, "Steady demo pacing failed");
    A(!hash_equal(first, moving), "Demo did not animate");
    Words stats = W("mb_graphics_observation", 6);
    A(stats.v[0] > 0 && stats.v[1] == 0 && stats.v[5] <= 20, "Graphics upload/band budget failed");
    if (id == 7) {
        IntSet positions = {0};
        IntSet heights = {0};
        IntSet frames = {0};
        int left = 224, right = 0, walls = 0;
        for (int n = 0; n < 200; n++) {
            Advance(Round(Hz / 10.0));
            Words m = W("mb_bounce_observation", 10);
            A(m.v[5] <= 224 && m.v[6] >= 4 && m.v[6] <= 104, "Sphere left bounds");
            int_add(&positions, m.v[5]);
            int_add(&heights, m.v[9]);
            int_add(&frames, m.v[2]);
            left = MIN(left, m.v[5]);
            right = MAX(right, m.v[5]);
            if (m.v[8] != walls) {
                Capture(mb_fmt("bounce-side-%d", m.v[8]));
                walls = m.v[8];
            }
        }
        A(right - left >= 180 && positions.count > 50 && heights.count >= 3 && walls >= 1,
          "Bounce paths/heights did not vary");
        A(frames.count == 9, "Rotation did not show eight frames and squash");
        Capture("bounce-varied-motion");
        int speed = W("mb_bounce_observation", 5).v[4];
        Press(1);
        A(W("mb_bounce_observation", 5).v[4] != speed, "Bounce speed failed");
        Press(3);
        Advance(2);
        Hash state = WordsHash(W("mb_bounce_observation", 3), 3);
        Advance(Hz);
        A(hash_equal(WordsHash(W("mb_bounce_observation", 3), 3), state), "Paused sphere moved");
        Press(3);
        Advance(Hz);
        A(core_peak(Hz) > 0, "Impact audio failed");
    }
    if (id == 8) {
        for (int look = 1; look <= 3; look++) {
            Press(7);
            Advance(Hz);
            A(W("mb_colour_observation", 1).v[0] == look, "Colour preset failed");
            Capture(mb_fmt("colour-look-%d", look));
        }
        Press(1);
        Press(1);
        Advance(Hz);
        int phase = W("mb_colour_observation", 1, 4).v[0];
        Advance(Hz);
        A(W("mb_colour_observation", 1, 4).v[0] == phase, "Reduced motion still cycled");
        Capture("colour-reduced");
    }
    if (id == 9) {
        Advance(Hz * 2);
        A(W("mb_ocean_observation", 5).v[2] == 8, "Ocean fade did not settle");
        HashSet sea = {0};
        HashSet shore = {0};
        for (int n = 0; n < 8; n++) {
            Advance((int)ceil(Hz * .2));
            hash_add(&sea, OceanPixels(92, 154));
            hash_add(&shore, OceanPixels(160, 174));
            if (n < 4)
                Capture(mb_fmt("ocean-surf-%d", n));
        }
        A(sea.count >= 3 && shore.count >= 3, "Sea/shore pixels stayed static");
        for (int scene = 1; scene <= 2; scene++) {
            Press(7);
            Advance(Hz * 4);
            Words o = W("mb_ocean_observation", 5);
            A(o.v[0] == scene && o.v[2] == 8 && o.v[3] == 0, "Scene fade did not complete");
            Capture(mb_fmt("ocean-scene-%d", scene));
        }
        Press(1);
        Advance(Hz);
        A(D("audio_active") == 0, "Ambience off failed");
        Press(1);
        Press(6);
        Press(6);
        Press(7);
        Advance(Hz * 5);
        Words queued = W("mb_ocean_observation", 5);
        A(queued.v[0] == queued.v[1] && queued.v[3] == 0, "Queued scene switches failed");
        Press(3);
        Capture("ocean-label-off");
        Press(3);
    }
    if (id == 10) {
        Press(1);
        Advance(Hz);
        A(W("mb_star_observation", 1, 2).v[0] == 1, "Warp failed");
        Capture("stardrive-warp");
        int speed = W("mb_star_observation", 1).v[0];
        Press(4);
        A(W("mb_star_observation", 1).v[0] > speed, "Star speed failed");
        Press(5);
        Press(3);
        Advance(4);
        Hash still = Capture("star-paused");
        Advance(Hz);
        A(hash_equal(Capture("star-paused-after"), still), "Paused stars moved");
        Press(3);
    }
    cJSON *metrics = cJSON_CreateObject();
    json_num(metrics, "app", id);
    json_num(metrics, "tiles", stats.v[0]);
    json_num(metrics, "sprites", stats.v[2]);
    json_num(metrics, "transfer_peak_bytes", stats.v[4]);
    json_num(metrics, "band_peak", stats.v[5]);
    json_num(metrics, "updates_per_second", updates);
    json_num(metrics, "ticks_per_second", ticks);
    json_num(metrics, "max_tick_delta", U("mb_demo_timing", 8));
    json_num(metrics, "heap", D("heap_used"));
    json_num(metrics, "static", D("static_bytes"));
    json_num(metrics, "work_max", D("work_max"));
    json_set(Report, "metrics", metrics);
    Advance(Hz * 2);
    core_save_audio(mb_fmt("%s/%s", Output, "demo.wav"));
    Chord(Round(Hz / 2.0), 3, 0);
    A(D("app_id") == id, "Short chord exited demo");
    Chord(Hz + 3, 3, 0);
    Restored();
    for (int n = 1; n < Cycles; n++) {
        Open();
        Press(8);
        Restored();
    }
    Capture("restored");
    core_enable_mouse();
    Advance(240);
    MouseTo(200, 42, 20);
    core_mouse(2, 0, 0, 1);
    Advance(18);
    A(Shell().v[0] == 3, "Mouse Demos folder failed");
    MouseTo(44 + index % 3 * 80, 60 + index / 3 * 48, 20);
    core_mouse(2, 0, 0, 1);
    Advance(Hz * 2);
    A(D("app_id") == id, "Mouse demo launch failed");
    Press(8);
    Restored();
    json_str(Report, "mouse", "PASS emulated Sega Mouse registry launch");
}
