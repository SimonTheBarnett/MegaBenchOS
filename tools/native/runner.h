/**
 * @file tools/native/runner.h
 * @brief Shared input, assertions and read-only observations for emulator fixtures.
 *
 * Word snapshots and hashes are returned by value, so later frame advances do
 * not alter saved expectations. Only the runner owns the session and report.
 */
#ifndef MB_NATIVE_RUNNER_H
#define MB_NATIVE_RUNNER_H
#include "core.h"
typedef struct {
    int v[512];
} Words;
typedef struct {
    int values[2048];
    size_t count;
} IntSet;
typedef struct {
    Hash values[2048];
    size_t count;
} HashSet;
extern int Hz, Cycles, Assertions, PressFrames, SettleFrames, PointerTolerance, PointerSteps,
    PointerSettle, RootAttempts;
extern char Output[4096];
extern const char *Scenario;
extern cJSON *Report;
/** Snapshot at most 512 words from a named ROM symbol plus a byte offset.
 * Larger reads use core_words with caller-provided storage and bounds checks.
 */
Words W(const char *name, int count, int offset);
uint32_t U(const char *name, int offset);
int D(const char *name);
int Symbol(const char *name);
void A(bool value, const char *message);
void Advance(int frames, int button);
void Press(int button);
void mb_chord(int frames, int one, int two);
Hash Capture(const char *name);
Words Shell(void), Ui(void), Position(void);
Hash WordsHash(Words words, int count);
void Focus(void), SelectEntry(int index), Root(void), MovePointer(int x, int y),
    Click(int x, int y);
/** Return caller-owned big-endian RAM bytes; free after use. Never mutates RAM. */
unsigned char *ReadBytes(int address, int count);
/** Borrow a NUL-terminated symbol snapshot from an eight-slot temporary ring. */
const char *Text(const char *symbol, int count);
void MouseTo(int x, int y, int steps);
void int_add(IntSet *set, int value);
void hash_add(HashSet *set, Hash value);
void scenario_ui(void), scenario_creative(void), scenario_paint(void), scenario_legacy_paint(void),
    scenario_music(void), scenario_demo(int id);
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MB_SELECT_W(_1, _2, _3, NAME, ...) NAME
#define MB_W2(name, count) (W)(name, count, 0)
#define MB_W3(name, count, offset) (W)(name, count, offset)
#define W(...) MB_SELECT_W(__VA_ARGS__, MB_W3, MB_W2)(__VA_ARGS__)
#define MB_SELECT_ADV(_1, _2, NAME, ...) NAME
#define MB_ADV1(frames) (Advance)(frames, -1)
#define MB_ADV2(frames, button) (Advance)(frames, button)
#define Advance(...) MB_SELECT_ADV(__VA_ARGS__, MB_ADV2, MB_ADV1)(__VA_ARGS__)
#define Round mb_round
#define Chord mb_chord
#define MB_U1(name) (U)(name, 0)
#define MB_U2(name, offset) (U)(name, offset)
#define U(...) MB_SELECT_ADV(__VA_ARGS__, MB_U2, MB_U1)(__VA_ARGS__)
#endif
