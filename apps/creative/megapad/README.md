# MegaPad

[MegaBenchOS](../../../README.md) / [Apps](../../README.md) / [Creative apps](../README.md) / MegaPad

A little text editor for notes on the Mega Drive. Type with the on-screen keyboard, move around your document and save it to RAM Disk.

## Controls

D-pad and A choose keys. Start cycles between keyboard, toolbar and cursor modes; D-pad moves through text in cursor mode. Pointer clicks work on the keyboard and toolbar too.

The keyboard includes Backspace, Delete, Enter, arrows and a case toggle. **Done** moves to the toolbar; **Esc** requests close. Use the toolbar's **Mode** control to return to typing.

## Files

New, Open, Save and Save As use `RAM:/Documents/`. The filename shows a `*` while edits are unsaved. Save updates the current file; Save As chooses a new name. Failed saves keep your edits, and closing or replacing an unsaved document asks before discarding it.

Names accept 1-15 letters, digits, underscores or hyphens; the editor adds `.txt`. Done accepts a name, while Esc or B cancels the filename dialog. Files disappear when MegaBenchOS restarts.

MegaPad holds up to 1,024 bytes of ASCII text, with LF newlines and 38-column wrapping. It opens maximised. Physical keyboard input, selection and undo are future work.

## Development

[editors.c](../editors.c) contains the descriptor and bounded file dialogs; [model.c](../model.c) contains the text model used by both the app and host tests. Saves use checked whole-file VFS replacement. See [file APIs and limits](../../../docs/VFS.md), the [SDK](../../../sdk/README.md) and [building and packaging](../../../docs/BUILD.md).

## Licensing

See [project licensing](../../../README.md#licensing).
