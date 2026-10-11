/**
 * @file tests/emulator/creative.c
 * @brief Exercises MegaPad file operations, desktop associations and Restart through emulator
 * input.
 *
 * Unity integration fixture. Drives controller/mouse input and reads observations; no RAM
 * injection.
 */
#include "../../tools/native/runner.h"

static int DiskUsed(void) {
    return W("disk", 1, 2).v[0];
}
static void App(int id) {
    Root();
    SelectEntry(0);
    Press(1);
    SelectEntry(id == 11 ? 2 : 0);
    Press(1);
    A(D("app_id") == id, mb_fmt("Editor failed to open %d", id));
    A(Ui().v[0] == 1, "Editor did not maximise");
}
static void EditorClose(void) {
    Press(0);
    A(D("app_id") == 65535, "Clean editor did not close");
}
static void Name(char letter, int keyboardY) {
    int k = (int)(strchr("abcdefghijklmnopqrstuvwxyz0123456789.,?!-_:/+*=() ", letter) -
                  "abcdefghijklmnopqrstuvwxyz0123456789.,?!-_:/+*=() ");
    Click(k % 10 * 4, keyboardY + k / 10);
    Click(24, keyboardY + 5);
}
static void Browse(int directory) {
    Root();
    SelectEntry(5);
    Press(1);
    A(Shell().v[0] == 7, "RAM Disk root missing");
    SelectEntry(directory);
    Press(1);
    A(Shell().v[0] == 8 + directory, "RAM directory did not open");
}
/** Exercise file dialogs and Restart as a user would, including cancellation. */
void scenario_creative(void) {
    int heap = D("heap_used");
    SettleFrames = 20;
    RootAttempts = 4;
    cJSON *scenarios = cJSON_CreateArray();
    A(Shell().v[0] == 0, "Boot desktop unavailable");
    Capture("desktop-six-folders");
    Browse(0);
    A(DiskUsed() == 0, "Fresh disk not empty");
    Capture("empty-documents");
    App(11);
    Focus();
    Press(1);
    Press(7);
    Press(1);
    A(!strcmp(Text("pad", 4), "ab"), "Controller keyboard did not insert ab");
    Click(17, 0);
    Name('a', 16);
    A(DiskUsed() == 2, "MegaPad save failed");
    Capture("megapad-saved");
    EditorClose();
    Browse(0);
    SelectEntry(0);
    Press(1);
    A(D("app_id") == 11 && !strcmp(Text("pad", 4), "ab"), "Text association/reopen failed");
    Focus();
    Press(1);
    A(!strcmp(Text("pad", 4), "aab"), "Reopened insertion failed");
    Press(0);
    A(D("app_id") == 11 && W("view", 1).v[0] == 3, "Dirty close did not confirm");
    Press(1);
    A(D("app_id") == 11 && !strcmp(Text("pad", 4), "aab"), "Discard cancel lost edits");
    Click(17, 0);
    A(DiskUsed() == 3, "Existing text overwrite failed");
    EditorClose();
    Browse(0);
    SelectEntry(0);
    Press(8);
    A(Shell().v[3] == 2, "File actions did not open");
    Click(17, 3);
    Name('b', 10);
    A(Shell().v[3] == 0, "Rename did not finish");
    Focus();
    if (Shell().v[3] != 0)
        Press(0);
    Press(1);
    A(!strcmp(Text("document", 44), "RAM:/Documents/b.txt"), "Rename broke association");
    EditorClose();
    Browse(0);
    Press(8);
    Click(17, 6);
    Capture("file-properties");
    Press(0);
    cJSON_AddItemToArray(
        scenarios,
        cJSON_CreateString("Controller keyboard create/save/reopen/overwrite, dirty-close cancel, "
                           "rename/properties and association"));
    for (int cycle = 0; cycle < Cycles; cycle++) {
        App(11);
        EditorClose();
    }
    A(D("heap_used") == heap && D("stack_guard_ok") == 1, "Editor heap/stack cleanup failed");
    Capture("desktop-restored");
    Root();
    Click(35, 25);
    A(Shell().v[3] == 6, "Restart mouse button did not confirm");
    Capture("restart-confirm-no");
    Press(3);
    A(D("app_id") == 65535 && Shell().v[3] == 6, "Start escaped restart confirmation");
    Focus();
    Press(1);
    A(Shell().v[3] == 0 && DiskUsed() == 3, "Default No reset or lost files");
    Click(35, 25);
    Click(1, -1);
    A(Shell().v[3] == 0 && DiskUsed() == 3, "Restart close control did not cancel");
    SelectEntry(5);
    Press(5);
    A(Shell().v[1] == 6, "Controller could not focus Restart");
    Press(1);
    Press(0);
    A(Shell().v[3] == 0 && DiskUsed() == 3, "B did not cancel Restart");
    Press(1);
    Focus();
    Press(7);
    Press(1);
    Advance(Hz);
    A(W("mb_boot_observation", 1).v[0] == 2, "Confirmed restart did not replay logo");
    Capture("restart-logo");
    Advance(3 * Hz);
    A(DiskUsed() == 0 && Shell().v[0] == 0 && Shell().v[3] == 0, "Restart retained files or modal");
    A(D("heap_used") == heap && D("audio_active") == 0 && D("stack_guard_ok") == 1,
      "Restart memory/audio/stack restoration failed");
    Capture("reset-empty");
    cJSON_AddItemToArray(
        scenarios,
        cJSON_CreateString("Restart mouse/controller, No default, B/X cancellation, modal Start "
                           "isolation, reset and logo"));
    json_set(Report, "scenarios", scenarios);
    json_set(Report, "reset_empty", cJSON_CreateBool(true));
    json_num(Report, "ramdisk_reserved", 4096);
    json_num(Report, "payload_capacity", 3592);
}
