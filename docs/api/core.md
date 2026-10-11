# MegaBenchOS public API v1 (generated)

Experimental source API v1 with additive 1.1 UI services; descriptor layout unchanged, no external binary ABI. NAVIGATE x/y are direction signs, MOVE/ACTIVATE are pixels. One trusted foreground app; 24 widgets; no heap API. B exits normal/maximized unless an app intercepts Back; every foreground app supports held Start+B or app Exit. Additive 1.2 session RAM files and shared text/canvas controls; guarded close for dirty maximized editors. Additive 1.3 VFS drive capabilities/version negotiation; original file names and whole-file write semantics retained. Backends private. Additive 1.4 trusted bundled XGM2 media service and fixed RAM Music directory; external playback unavailable. Additive 1.5 variable 16-colour tile canvas, dedicated PAL2, integer zoom/pan; legacy canvas remains supported. Additive 1.6 compact shared chrome and one-row menus; query client/content rectangles after resize, never assume fixed header offsets. Additive 1.7 copied custom cursor bitmaps/hotspots and centred brush markers; descriptor layout remains unchanged.

## app.launch

```c
MB_Result mb_app_launch(const MB_App *app);
```

**Parameters:**

- `app`: Borrowed ROM-resident descriptor; version 1, name and every lifecycle callback required.

**arguments:** ROM-resident descriptor; version 1; name and all callbacks required.  
**capability:** MB_CAP_ROM_LINKED; no external loading.  
**edges:** Repeated launch/close; shutdown follows successful init only.  
**errors:** Null fields invalid; second launch busy; unsupported version rejected.  
**example:** mb_app_launch(&clock_app);  
**family:** app  
**negative:** Null descriptor, missing callback, incompatible version, failed init, busy runtime.  
**ownership:** Descriptor borrowed until close; event pointers valid only during callback.  
**purpose:** Launch one statically linked foreground app; init failure leaves desktop active.  
**returns:** MB_OK, MB_INVALID, MB_BUSY, MB_VERSION, or init error.  
**timing:** Foreground only; synchronous; lifecycle callbacks must return promptly; nested lifecycle calls return MB_BUSY. Prefer launch_id for callbacks.  

## app.close

```c
void mb_app_close(void);
```

**arguments:** None.  
**capability:** MB_CAP_ROM_LINKED.  
**edges:** Repeated close is harmless.  
**errors:** No active app is a no-op.  
**example:** mb_app_close();  
**family:** app  
**negative:** Close when idle.  
**ownership:** Releases borrowed descriptor after shutdown.  
**purpose:** Close foreground app, release its widgets, silence owned PSG and restore desktop; safe to request from events.  
**returns:** None.  
**timing:** Foreground; deferred when called from event/tick callbacks; forbidden inside init/activate/deactivate/shutdown callbacks.  

## system.capabilities

```c
uint16_t mb_system_capabilities(void);
```

**arguments:** None.  
**capability:** Always available; no SRAM, RTC, mouse or network advertised.  
**edges:** Bitmask permits future additions.  
**errors:** None.  
**example:** uint16_t caps = mb_system_capabilities();  
**family:** system  
**negative:** Do not infer writable storage from flags.  
**ownership:** Value.  
**purpose:** Report proven baseline services.  
**returns:** Controller, ROM-linked app, read-only ROM-volume metadata and PSG flags; no SRAM/RTC/network.  
**timing:** Bounded synchronous query; safe in app callbacks.  

## time.uptime

```c
uint32_t mb_time_seconds(void);
```

**arguments:** None.  
**capability:** No wall-clock source.  
**edges:** PAL 50 and NTSC 60 ticks per second.  
**errors:** None.  
**example:** uint32_t seconds = mb_time_seconds();  
**family:** time  
**negative:** Must not interpret as calendar date.  
**ownership:** Value.  
**purpose:** Session uptime from PAL/NTSC VBlank count.  
**returns:** Elapsed whole seconds since boot; wraps after UINT32_MAX seconds.  
**timing:** Bounded synchronous; callback safe; reset restarts uptime.  

## window.open

```c
MB_Result mb_window_open(const char *title);
```

**Parameters:**

- `title`: Borrowed NUL-terminated ASCII window title; copied into display, clipped to 25 characters.

**arguments:** Non-null ASCII title, at most 25 characters displayed.  
**capability:** Single foreground window; registry supported modes and minimum geometry.  
**edges:** Long title explicitly clipped to title capacity.  
**errors:** Null title invalid.  
**example:** mb_window_open("Hello");  
**family:** window  
**negative:** Null title.  
**ownership:** Title copied to rendered tiles; not retained.  
**purpose:** Open foreground window with close and mode controls.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callback; bounded retained-tile invalidation; hardware updates occur during VBlank.  

## draw.text

```c
MB_Result mb_draw_text(uint16_t x, uint16_t y, const char *text);
```

**Parameters:**

- `x`: Client-relative tile coordinate; must fit the queried client rectangle.
- `y`: Client-relative tile coordinate; must fit the queried client rectangle.
- `text`: Borrowed NUL-terminated ASCII text; rendered/clipped within the client area.

**arguments:** x within queried client, y within queried client, non-null null-terminated ASCII string.  
**capability:** Shared window must be open.  
**edges:** Clips at right client edge without touching frame.  
**errors:** Null or out-of-bounds coordinates rejected.  
**example:** mb_draw_text(0, 0, "Hello MegaBenchOS");  
**family:** draw  
**negative:** Invalid coordinates, null string, no window.  
**ownership:** Text read synchronously; not retained.  
**purpose:** Draw clipped text relative to window client tile grid.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded client-row preparation; writes retained tile metadata, flushed by the OS at VBlank.  

## memory.stats

```c
MB_Result mb_memory_stats(MB_MemoryStats *stats);
```

**Parameters:**

- `stats`: Non-null caller-owned output structure; filled synchronously, no retained pointer.

**arguments:** Non-null output; version field returned as 1.  
**capability:** Scope SGDK heap includes allocator block headers, excludes static data and reserved stack.  
**edges:** total=used+free; static+heap+2-byte sentinel+2560 stack=65536; high_water sampled since boot.  
**errors:** Null rejected; guard failure or inconsistent counts unavailable.  
**example:** MB_MemoryStats stats; mb_memory_stats(&stats);  
**family:** memory  
**negative:** Null output and inconsistent scope.  
**ownership:** Output owned by caller; no allocation.  
**purpose:** Snapshot SGDK allocator bytes, sampled high-water and conservative stack watermark.  
**returns:** MB_OK or MB_INVALID/MB_UNAVAILABLE.  
**timing:** Bounded allocator scan; sampled on launch/close and at most once/sec in normal UI.  

## system.info

```c
MB_Result mb_system_info(MB_SystemInfo *info);
```

**Parameters:**

- `info`: Non-null caller-owned output structure; returned strings reference immutable ROM.

**arguments:** Non-null output.  
**capability:** PAL 50 Hz or NTSC 60 Hz; no RTC.  
**edges:** Nominal refresh differs slightly from physical oscillator rates; release version is 1.0, independent of the public API version.  
**errors:** Null rejected.  
**example:** MB_SystemInfo info; mb_system_info(&info);  
**family:** system  
**negative:** Null output.  
**ownership:** Output caller-owned; strings immutable ROM for entire session.  
**purpose:** Query build/source API version, detected region and nominal refresh.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded; callbacks safe.  

## system.diagnostics

```c
MB_Result mb_system_diagnostics(MB_Diagnostics *stats);
```

**Parameters:**

- `stats`: Non-null caller-owned output structure; filled synchronously, no retained pointer.

**arguments:** Non-null output.  
**capability:** work values are SGDK 1/76800-second subticks; scanline quantized, not cycle-accurate.  
**edges:** Maxima since boot include diagnostic overhead; dirty_cells pending bounded queue; transition max measures preparation work, not completion latency.  
**errors:** Null rejected.  
**example:** MB_Diagnostics stats; mb_system_diagnostics(&stats);  
**family:** system  
**negative:** Null output.  
**ownership:** Caller-owned copy.  
**purpose:** Snapshot heap, stack, retained tiles and coarse V-counter work/transition maxima.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded; callbacks safe; no fresh profiling or heap scan.  

## app.get

```c
MB_Result mb_app_get(uint16_t index, MB_AppInfo *info);
```

**Parameters:**

- `index`: Zero-based enumeration index; unsupported entries return MB_NOT_FOUND.
- `info`: Non-null caller-owned output structure; returned strings reference immutable ROM.

**arguments:** Index 0..MB_APP_COUNT-1; non-null output; retired slot zero returns MB_NOT_FOUND. Existing app IDs remain stable.  
**capability:** ROM relinking only.  
**edges:** Names and IDs are source API; descriptors stay private.  
**errors:** Null invalid; unknown index not found.  
**example:** MB_AppInfo app; mb_app_get(MB_APP_CALCULATOR, &app);  
**family:** app  
**negative:** Unknown ID and null.  
**ownership:** Names borrowed immutable ROM.  
**purpose:** Enumerate ROM-linked registered apps by stable ID.  
**returns:** MB_OK, MB_INVALID, MB_NOT_FOUND.  
**timing:** Bounded; callback safe.  

## app.launchid

```c
MB_Result mb_app_launch_id(uint16_t id);
```

**Parameters:**

- `id`: Stable app/widget identifier; must identify a registered app or enabled widget as applicable.

**arguments:** Stable ID from app_get.  
**capability:** ROM-linked apps, one foreground app.  
**edges:** Previous app closes and releases audio/widgets; fullscreen exits via chord or app-visible Exit.  
**errors:** Invalid ID does not close current app; lifecycle failures handled by desktop error status.  
**example:** mb_app_launch_id(MB_APP_CLOCK);  
**family:** app  
**negative:** Invalid ID, nested callback and failed init.  
**ownership:** No dynamic allocation; descriptor host-owned.  
**purpose:** Launch registered app or queue transition from an event/tick.  
**returns:** MB_OK accepted; MB_NOT_FOUND unknown; MB_BUSY nested lifecycle or already pending.  
**timing:** Event/tick requests deferred until callback returns; init/activate/deactivate/shutdown may not transition.  

## input.state

```c
MB_Result mb_input_state(MB_Input *state);
```

**Parameters:**

- `state`: Non-null caller-owned MB_Input output; observed semantic buttons and pixel pointer position.

**arguments:** Non-null output.  
**capability:** Three-button controller plus detected port-2 Sega Mouse; physical device verification pending.  
**edges:** NAVIGATE uses signed x/y direction; MOVE uses pixel coordinates. Held buttons remain available to game.  
**errors:** Null invalid.  
**example:** MB_Input state; mb_input_state(&state);  
**family:** input  
**negative:** Null.  
**ownership:** Caller-owned copy; no device pointers.  
**purpose:** Query cached semantic controller buttons, pointer and interaction mode.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded; event/tick safe.  

## input.mode

```c
MB_Result mb_input_mode(uint16_t mode);
```

**Parameters:**

- `mode`: MB_INPUT_FOCUS, MB_INPUT_POINTER or MB_INPUT_GAME; applies to the active foreground app.

**arguments:** MB_INPUT_FOCUS, MB_INPUT_POINTER, MB_INPUT_GAME.  
**capability:** C toggles focus/pointer for UI apps; game receives C as context.  
**edges:** D-pad pointer behavior preserved; focus navigation repeats at bounded intervals. Game MB_NAVIGATE x/y reflect newly pressed directions only, without held-direction repeats; MB_Input.buttons retains all held buttons. Simultaneous axes may both be nonzero; games choose their own tie-break and turn buffer.  
**errors:** Unknown mode invalid.  
**example:** mb_input_mode(MB_INPUT_GAME);  
**family:** input  
**negative:** Invalid mode.  
**ownership:** Session app state; reset to focus on launch and pointer on desktop.  
**purpose:** Choose focus, pointer or game direction routing.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Callback safe; no synchronous input delivery.  

## input.speed

```c
MB_Result mb_input_speed(uint16_t level);
```

**Parameters:**

- `level`: Cursor speed level: 1 slow, 2 normal, 3 fast; applies for this session.

**arguments:** Level 1 slow, 2 normal, 3 fast.  
**capability:** No persistence.  
**edges:** Slow 40/120 px/s; normal 60/180; fast 80/240. Reset returns normal.  
**errors:** Outside 1..3 rejected.  
**example:** mb_input_speed(3);  
**family:** input  
**negative:** Zero and four.  
**ownership:** Stored statically for session only.  
**purpose:** Set working pointer speed preference for this session.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded; callback safe.  

## input.preference

```c
uint16_t mb_input_speed_level(void);
```

**arguments:** None.  
**capability:** Session only.  
**edges:** Defaults to 2 after reset.  
**errors:** None.  
**example:** uint16_t level = mb_input_speed_level();  
**family:** input  
**negative:** Do not infer saved settings.  
**ownership:** Value.  
**purpose:** Query current pointer speed preference.  
**returns:** Level 1..3.  
**timing:** Bounded callback-safe query.  

## time.ticks

```c
uint32_t mb_time_ticks(void);
```

**arguments:** None.  
**capability:** No wall-clock source.  
**edges:** Unsigned subtraction supports short wrap-safe intervals; game movement uses 30 ticks = 10 moves/sec.  
**errors:** None.  
**example:** uint32_t elapsed = mb_time_ticks() - start;  
**family:** time  
**negative:** Not a date or RTC.  
**ownership:** Value.  
**purpose:** Read monotonic nominal 300-Hz session ticks.  
**returns:** Ticks since boot, wraps modulo 2^32 after about 165 days.  
**timing:** Bounded; updated once per frame by +6 PAL/+5 NTSC.  

## time.milliseconds

```c
uint32_t mb_time_milliseconds(void);
```

**arguments:** None.  
**capability:** PAL/NTSC nominal session timing.  
**edges:** Runtime seconds continue while applications are active or closed; OS restart resets the timebase.  
**errors:** None.  
**example:** uint32_t elapsed = mb_time_milliseconds();  
**family:** time  
**negative:** Not wall time.  
**ownership:** Value.  
**purpose:** Convert session ticks to elapsed nominal milliseconds.  
**returns:** Milliseconds modulo 2^32; tick granularity 3.33ms.  
**timing:** Bounded integer arithmetic; callback safe.  

## time.format

```c
MB_Result mb_time_format(uint32_t seconds, char *out, uint16_t capacity);
```

**Parameters:**

- `seconds`: Unsigned elapsed session seconds, including values beyond 24 hours.
- `out`: Caller-owned output buffer; NUL-terminated on success, cleared on insufficient capacity.
- `capacity`: Output buffer size in bytes including NUL; required: time 14, signed decimal 12, unsigned decimal 11.

**arguments:** Output buffer; capacity >=14 handles every uint32 seconds value.  
**capability:** Host/target utility.  
**edges:** Hours at least two digits; no 24-hour rollover.  
**errors:** Null or insufficient capacity rejected.  
**example:** char value[16]; mb_time_format(mb_time_seconds(), value, sizeof(value));  
**family:** time  
**negative:** Null, short buffer; 59/60/3599/3600/MAX seconds.  
**ownership:** Caller-owned buffer, never retained.  
**purpose:** Format elapsed hours/minutes/seconds with unbounded uint32 hours.  
**returns:** MB_OK or MB_INVALID; writes empty string on short nonzero buffer.  
**timing:** Bounded; no libc, heap or hardware.  

## text.unsigned

```c
MB_Result mb_format_u32(uint32_t value, char *out, uint16_t capacity);
```

**Parameters:**

- `value`: Integer value to format; the full declared signed/unsigned 32-bit range is supported.
- `out`: Caller-owned output buffer; NUL-terminated on success, cleared on insufficient capacity.
- `capacity`: Output buffer size in bytes including NUL; required: time 14, signed decimal 12, unsigned decimal 11.

**arguments:** Non-null buffer; capacity>=11 handles uint32.  
**capability:** Host/target utility.  
**edges:** NUL terminator guaranteed on success.  
**errors:** Short/null invalid.  
**example:** char n[12]; mb_format_u32(42, n, sizeof(n));  
**family:** draw  
**negative:** Zero-capacity and MAX.  
**ownership:** Caller-owned; no allocation.  
**purpose:** Format unsigned decimal without libc.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded <=10 digits.  

## text.signed

```c
MB_Result mb_format_i32(int32_t value, char *out, uint16_t capacity);
```

**Parameters:**

- `value`: Integer value to format; the full declared signed/unsigned 32-bit range is supported.
- `out`: Caller-owned output buffer; NUL-terminated on success, cleared on insufficient capacity.
- `capacity`: Output buffer size in bytes including NUL; required: time 14, signed decimal 12, unsigned decimal 11.

**arguments:** Non-null buffer; capacity>=12 handles int32.  
**capability:** Host/target utility.  
**edges:** Avoids signed negation overflow.  
**errors:** Short/null invalid.  
**example:** char n[12]; mb_format_i32(-42, n, sizeof(n));  
**family:** draw  
**negative:** INT32_MIN, zero, short buffer.  
**ownership:** Caller-owned.  
**purpose:** Format signed integer including INT32_MIN.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded <=11 characters.  

## window.clear

```c
MB_Result mb_window_clear(void);
```

**arguments:** None.  
**capability:** Queried client tile geometry.  
**edges:** Clears only client; title and desktop remain.  
**errors:** No window unavailable.  
**example:** mb_window_clear();  
**family:** window  
**negative:** No window.  
**ownership:** No retained app buffers; fixed host widget pool.  
**purpose:** Clear client and reset shared widgets without redrawing chrome.  
**returns:** MB_OK or MB_UNAVAILABLE.  
**timing:** Bounded dirty-tile preparation.  

## draw.field

```c
MB_Result mb_draw_field(uint16_t x, uint16_t y, uint16_t width, const char *text);
```

**Parameters:**

- `x`: Client-relative tile coordinate; must fit the queried client rectangle.
- `y`: Client-relative tile coordinate; must fit the queried client rectangle.
- `width`: Width in client tiles; nonzero and within the remaining client area (buttons require at least 3).
- `text`: Borrowed NUL-terminated ASCII text; rendered/clipped within the client area.

**arguments:** x/y client tile coordinates; width 1..remaining row; text non-null ASCII.  
**capability:** Shared light document surface.  
**edges:** Long text clips; important messages must fit or be split into rows by caller.  
**errors:** Invalid/empty/out-of-bounds field rejected.  
**example:** mb_draw_field(1, 1, 25, "Ready");  
**family:** draw  
**negative:** Null, width zero, x+width>30.  
**ownership:** Text read synchronously; no retention.  
**purpose:** Replace a client text field, clearing stale trailing characters.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded to row; only changed tiles scheduled.  

## draw.rect

```c
MB_Result mb_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t style);
```

**Parameters:**

- `x`: Client-relative tile coordinate; must fit the queried client rectangle.
- `y`: Client-relative tile coordinate; must fit the queried client rectangle.
- `width`: Width in client tiles; nonzero and within the remaining client area (buttons require at least 3).
- `height`: Height in client tiles; nonzero and within the remaining client area.
- `style`: MB_STYLE_* semantic rectangle style; palettes and tile ownership remain private.

**arguments:** Client x/y/w/h withinqueried client; semantic MB_STYLE_* index.  
**capability:** Semantic shared fills in all window modes; exclusive art uses leases.  
**edges:** Bounds query governs clipping; desktop restores on exit.  
**errors:** Zero dimensions/unknown style/out-of-client rejected.  
**example:** mb_draw_rect(5, 2, 20, 10, MB_STYLE_SURFACE);  
**family:** draw  
**negative:** Illegal style and bounds.  
**ownership:** Retained host tile metadata; no per-app VRAM ownership.  
**purpose:** Prepare a bounded filled tile rectangle for simple game/UI graphics.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Bounded to queried client (at most 1120 cells), flush budget 192 cells/frame.  

## ui.reset

```c
void mb_ui_reset(void);
```

**arguments:** None.  
**capability:** Shared client; no heap.  
**edges:** Window open/clear and app exit reset widgets automatically.  
**errors:** None.  
**example:** mb_ui_reset();  
**family:** ui  
**negative:** Repeated reset harmless.  
**ownership:** Fixed pool of at most24 buttons; labels copied <=11 characters.  
**purpose:** Release all retained buttons for current client page.  
**returns:** None.  
**timing:** Callback safe; existing drawings are left until cleared.  

## ui.button

```c
MB_Result mb_ui_button(uint16_t id, uint16_t x, uint16_t y, uint16_t width, const char *label, uint16_t flags);
```

**Parameters:**

- `id`: New widget ID, unique within the current window; 0xffff is reserved and rejected. MB_UI_MENU_DISMISS is reserved and cannot be registered.
- `x`: Client-relative tile coordinate; must fit the queried client rectangle.
- `y`: Client-relative tile coordinate; must fit the queried client rectangle.
- `width`: Width 3..40 tiles, fitting the queried client.
- `label`: Borrowed NUL-terminated ASCII button label; copied, at most 11 visible characters.
- `flags`: MB_UI_ENABLED to enable focus/activation, or zero for a disabled button.

**arguments:** Unique ID; x/y/width within client; width>=3; label non-null; MB_UI_ENABLED or0.  
**capability:** Controller/pointer shared widget.  
**edges:** Visible bracket marker plus accent focus; labels clipped with > marker.  
**errors:** Duplicate ID, bounds, unsupported flags invalid; pool full busy.  
**example:** mb_ui_button(1,1,2,12,"Open",MB_UI_ENABLED);  
**family:** ui  
**negative:** Duplicate, null, disabled activation,24/25th button.  
**ownership:** Copies <=11 label chars to fixed pool; no borrowed pointer.  
**purpose:** Create one shared two-tile-high button with focus and disabled states.  
**returns:** MB_OK or MB_INVALID/MB_BUSY.  
**timing:** Bounded pool scan and dirty preparation; first enabled button gains focus.  

## ui.event

```c
MB_Result mb_ui_event(const MB_Event *event, uint16_t *activated);
```

**Parameters:**

- `event`: Borrowed semantic event, valid only during its callback; NAVIGATE uses direction signs.
- `activated`: Non-null caller-owned output widget ID, or 0xffff when no activation occurred.

**arguments:** Non-null event/output; button event IDs returned via output.  
**capability:** Focus mode D-pad nearest neighbor then wrap; pointer mode hovered hit-test; A primary.  
**edges:** Focus excludes disabled widgets; press feedback retained for two ticks unless page replaced. An open menu heading causes repeated activation or a pointer click outside registered widgets to return MB_OK with MB_UI_MENU_DISMISS. Outside-click dismissal is consumed; do not forward it to the underlying content.  
**errors:** Disabled/empty/missed buttons not found.  
**example:** uint16_t id; if (mb_ui_event(event,&id)==MB_OK) { }  
**family:** ui  
**negative:** Null output, disabled selection.  
**ownership:** No event retention; output caller-owned.  
**purpose:** Route semantic navigation/move/activation through shared buttons.  
**returns:** MB_OK when activated; MB_NOT_FOUND when no action; MB_INVALID null.  
**timing:** Callbacks safe; bounded24 widgets.  

## ui.focus

```c
MB_Result mb_ui_focus(uint16_t id);
```

**Parameters:**

- `id`: Stable app/widget identifier; must identify a registered app or enabled widget as applicable.

**arguments:** Known button ID.  
**capability:** Focus shows color and brackets.  
**edges:** Pointer selection and controller focus share state.  
**errors:** Unknown/disabled not found.  
**example:** mb_ui_focus(1);  
**family:** ui  
**negative:** Disabled/unknown button.  
**ownership:** Host-owned focus.  
**purpose:** Focus an existing enabled button.  
**returns:** MB_OK or MB_NOT_FOUND.  
**timing:** Bounded; callback safe.  

## audio.tone

```c
MB_Result mb_audio_tone(uint16_t frequency, uint16_t attenuation);
```

**Parameters:**

- `frequency`: PSG tone frequency in Hz, 110..1760; requires active foreground ownership.
- `attenuation`: Requested attenuation 8..15; 8..14 use maximum hardware volume (attenuation 0); 15 is silent.

**arguments:** Frequency110..1760 Hz; attenuation8..15 (15 silent).  
**capability:** MB_CAP_PSG; no FM capability.  
**edges:** Defaults silent; excludes OS sound because none is enabled; exit forcibly stops.  
**errors:** No app unavailable; unsafe frequency/volume invalid.  
**example:** mb_audio_tone(440, 12);  
**family:** audio  
**negative:** Invalid volume/frequency and no app.  
**ownership:** App owns PSG until stop/exit; host also mutes all channels on cleanup.  
**purpose:** Play one conservative PSG channel0 tone owned by foreground app.  
**returns:** MB_OK, MB_INVALID, MB_UNAVAILABLE.  
**timing:** Bounded direct PSG writes; event callback safe; no blocking playback.  

## audio.noise

```c
MB_Result mb_audio_noise(uint16_t attenuation);
```

**Parameters:**

- `attenuation`: Requested attenuation 8..15; 8..14 use maximum hardware volume (attenuation 0); 15 is silent.

**arguments:** Attenuation8..15.  
**capability:** PSG white noise; FM remains optional and unimplemented.  
**edges:** Stop/exit silences all PSG channels.  
**errors:** Invalid level or no app.  
**example:** mb_audio_noise(12);  
**family:** audio  
**negative:** No foreground and excessive volume.  
**ownership:** Foreground owner only.  
**purpose:** Play conservative white PSG noise on channel3; mute tone first.  
**returns:** MB_OK, MB_INVALID, MB_UNAVAILABLE.  
**timing:** Bounded direct writes.  

## audio.stop

```c
void mb_audio_stop(void);
```

**arguments:** None.  
**capability:** PSG only.  
**edges:** Visible status must also update in app; no audio information-only feedback.  
**errors:** Idle stop harmless.  
**example:** mb_audio_stop();  
**family:** audio  
**negative:** Repeated stop.  
**ownership:** Host always forces stop on app transition/reset.  
**purpose:** Silence all PSG channels and clear ownership playback status.  
**returns:** None.  
**timing:** Bounded; callback safe.  

## storage.volume

```c
MB_Result mb_storage_volume(uint16_t index, MB_Volume *volume);
```

**Parameters:**

- `index`: Zero-based enumeration index; unsupported entries return MB_NOT_FOUND.
- `volume`: Non-null caller-owned output volume metadata; ROM labels are borrowed immutable strings.

**arguments:** Index0; non-null output.  
**capability:** ROM: read-only, capacity reflects actual compiled/padded ROM; no file operations.  
**edges:** SRAM/SD/RTC remain unavailable; this hook does not implement a filesystem.  
**errors:** Null invalid; unsupported index not found.  
**example:** MB_Volume v; mb_storage_volume(0,&v);  
**family:** storage  
**negative:** Index1 must not advertise SAVE.  
**ownership:** Name ROM-resident; output caller-owned.  
**purpose:** Compatibility cartridge ROM metadata query through VFS mount capacity.  
**returns:** MB_OK, MB_NOT_FOUND or MB_INVALID.  
**timing:** Bounded callback-safe metadata query.  

## window.mode

```c
MB_Result mb_window_set_mode(MB_WindowMode mode);
```

**Parameters:**

- `mode`: Normal, maximized, or fullscreen; current app must support it.

**arguments:** See typed parameters; no allocation.  
**capability:** H40 320x224 in PAL and NTSC; 30x14 normal client, 40x27 maximized, 40x28 fullscreen. Maximized combined header is one tile row; fullscreen has no chrome.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** No open window unavailable; invalid mode invalid; active lease busy.  
**example:** MB_Result mb_window_set_mode(MB_WindowMode mode);  
**family:** window  
**negative:** No open window unavailable; invalid mode invalid; active lease busy.  
**ownership:** OS owns bounds; normal bounds remembered. Apps redraw on MB_RESIZE.  
**purpose:** Change foreground window geometry; deliver mode and resize notifications after the callback returns.  
**returns:** MB_OK, MB_INVALID, MB_UNAVAILABLE or MB_BUSY.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## window.rect

```c
MB_Result mb_window_get_client_rect(MB_Rect *out);
```

**Parameters:**

- `out`: Caller-owned non-null output, in 8x8 tiles.

**arguments:** See typed parameters; no allocation.  
**capability:** Accurate current drawable client in all modes: normal 30x14, maximized 40x27 at (0,1), fullscreen 40x28 at (0,0). Includes an optional app menu row.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Null invalid; no window unavailable.  
**example:** MB_Result mb_window_get_client_rect(MB_Rect *out);  
**family:** window  
**negative:** Null invalid; no window unavailable.  
**ownership:** Snapshot only; no retained pointer.  
**purpose:** Query client rectangle in screen tile coordinates; app drawing coordinates are relative to this origin.  
**returns:** MB_OK, MB_INVALID or MB_UNAVAILABLE.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## window.restore

```c
MB_Result mb_window_restore(void);
```

**arguments:** See typed parameters; no allocation.  
**capability:** Normal mode.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** An outstanding resource lease must be released first.  
**example:** MB_Result mb_window_restore(void);  
**family:** window  
**negative:** An outstanding resource lease must be released first.  
**ownership:** OS owns bounds.  
**purpose:** Restore the remembered normal window bounds.  
**returns:** Same as window.set_mode.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## window.current

```c
MB_WindowMode mb_window_mode(void);
```

**arguments:** See typed parameters; no allocation.  
**capability:** All modes.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** None.  
**example:** MB_WindowMode mb_window_mode(void);  
**family:** window  
**negative:** None.  
**ownership:** Value only.  
**purpose:** Return current window mode; idle returns normal.  
**returns:** Current mode.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## app.context

```c
uint16_t mb_app_context(void);
```

**arguments:** See typed parameters; no allocation.  
**capability:** ROM registry.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** None.  
**example:** uint16_t mb_app_context(void);  
**family:** app  
**negative:** None.  
**ownership:** Session value; immutable during app lifetime.  
**purpose:** Query launch entry context: 0 default; System 2 info, 3 storage, 4 settings, 5 memory, 6 input.  
**returns:** Launch context.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## theme.preview

```c
MB_Result mb_theme_preview(MB_ThemeId id);
```

**Parameters:**

- `id`: One of five MB_THEME_* values.

**arguments:** See typed parameters; no allocation.  
**capability:** Shared UI; theme notification deferred.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Invalid ID invalid; exclusive lease busy.  
**example:** MB_Result mb_theme_preview(MB_ThemeId id);  
**family:** theme  
**negative:** Invalid ID invalid; exclusive lease busy.  
**ownership:** OS CRAM roles; session only, no SAVE persistence.  
**purpose:** Preview a role theme without changing the applied session preference.  
**returns:** MB_OK, MB_INVALID or MB_BUSY.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## theme.apply

```c
MB_Result mb_theme_apply(MB_ThemeId id);
```

**Parameters:**

- `id`: One of five MB_THEME_* values.

**arguments:** See typed parameters; no allocation.  
**capability:** RGB333 shared palettes.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Invalid ID invalid; exclusive lease busy.  
**example:** MB_Result mb_theme_apply(MB_ThemeId id);  
**family:** theme  
**negative:** Invalid ID invalid; exclusive lease busy.  
**ownership:** No persistent storage; all visible shared art uses semantic roles.  
**purpose:** Apply a shared UI theme for this session.  
**returns:** MB_OK, MB_INVALID or MB_BUSY.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## theme.cancel

```c
void mb_theme_cancel_preview(void);
```

**arguments:** See typed parameters; no allocation.  
**capability:** Shared UI.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Lease prevents writes; applied theme restores on exit.  
**example:** void mb_theme_cancel_preview(void);  
**family:** theme  
**negative:** Lease prevents writes; applied theme restores on exit.  
**ownership:** OS palette ownership.  
**purpose:** Restore applied theme after preview; no-op while exclusive resources are leased.  
**returns:** None.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## theme.current

```c
MB_ThemeId mb_theme_current(void);
```

**arguments:** See typed parameters; no allocation.  
**capability:** Five themes.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** None.  
**example:** MB_ThemeId mb_theme_current(void);  
**family:** theme  
**negative:** None.  
**ownership:** Value only.  
**purpose:** Return applied session theme, independent of preview.  
**returns:** Applied theme ID.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## palette.request

```c
MB_Result mb_palette_request(const MB_PaletteRequest *req, MB_PaletteLease *out);
```

**Parameters:**

- `req`: Non-null request; lines must equal 15 (all four lines).
- `out`: Caller-owned lease output; OS issues a fresh token.

**arguments:** See typed parameters; no allocation.  
**capability:** Fullscreen only; user pattern bank 64 through font start, 79 sprites excluding OS cursor.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Invalid pointers/mask invalid; existing lease busy; non-fullscreen or idle unavailable.  
**example:** MB_Result mb_palette_request(const MB_PaletteRequest *req, MB_PaletteLease *out);  
**family:** palette  
**negative:** Invalid pointers/mask invalid; existing lease busy; non-fullscreen or idle unavailable.  
**ownership:** Lease belongs to current app and is forcibly revoked on close; fixed recovery tiles/font remain reserved.  
**purpose:** Lease all four palette lines and free pattern/sprite capacity for an exclusive foreground app.  
**returns:** MB_OK, MB_INVALID, MB_BUSY or MB_UNAVAILABLE.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## palette.set

```c
MB_Result mb_palette_set(const MB_PaletteLease *lease, uint16_t line, const uint16_t *colors);
```

**Parameters:**

- `lease`: Current app-owned token; stale/forged leases rejected.
- `line`: Palette line 0..3.
- `colors`: Borrowed sixteen-word array, each value limited to mask 0x0eee; index 0 transparent.

**arguments:** See typed parameters; no allocation.  
**capability:** Exclusive lease only.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Stale token, invalid line/color or null invalid.  
**example:** MB_Result mb_palette_set(const MB_PaletteLease *lease, uint16_t line, const uint16_t *colors);  
**family:** palette  
**negative:** Stale token, invalid line/color or null invalid.  
**ownership:** Reads synchronously; CRAM owned until release.  
**purpose:** Set one leased palette line with sixteen legal Genesis CRAM words.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callbacks only; sixteen words queued for the next OS VBlank flush.  

## palette.release

```c
MB_Result mb_palette_release(MB_PaletteLease *lease);
```

**Parameters:**

- `lease`: Caller-owned active token; cleared on success.

**arguments:** See typed parameters; no allocation.  
**capability:** H40 baseline recovery; no raster or alternate resolution API.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Stale or null token invalid.  
**example:** MB_Result mb_palette_release(MB_PaletteLease *lease);  
**family:** palette  
**negative:** Stale or null token invalid.  
**ownership:** Resources revoked; display restored notification deferred.  
**purpose:** Release exclusive resources and restore OS VDP, interrupts, font, tiles, palettes and sprites.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## graphics.pattern

```c
MB_Result mb_graphics_pattern(const MB_PaletteLease *lease, uint16_t tile, const uint32_t *rows);
```

**Parameters:**

- `lease`: Current exclusive token.
- `tile`: Absolute tile index in returned leased range.
- `rows`: Borrowed eight packed 32-bit pattern rows.

**arguments:** See typed parameters; no allocation.  
**capability:** Bounded 32-byte CPU transfer.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Invalid token, bounds or null invalid.  
**example:** MB_Result mb_graphics_pattern(const MB_PaletteLease *lease, uint16_t tile, const uint32_t *rows);  
**family:** graphics  
**negative:** Invalid token, bounds or null invalid.  
**ownership:** Pattern remains app-owned until release; recovery resources protected.  
**purpose:** Upload one leased 8x8 four-bit pattern.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## graphics.tile

```c
MB_Result mb_graphics_tile(const MB_PaletteLease *lease, uint16_t x, uint16_t y, uint16_t tile, uint16_t line);
```

**Parameters:**

- `lease`: Current exclusive token.
- `x`: Client column.
- `y`: Client row.
- `tile`: Absolute tile index within leased range.
- `line`: Palette line 0..3.

**arguments:** See typed parameters; no allocation.  
**capability:** Exclusive foreground only.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Invalid token, tile, line or client bounds invalid.  
**example:** MB_Result mb_graphics_tile(const MB_PaletteLease *lease, uint16_t x, uint16_t y, uint16_t tile, uint16_t line);  
**family:** graphics  
**negative:** Invalid token, tile, line or client bounds invalid.  
**ownership:** Retained OS tilemaps; ownership revoked on exit.  
**purpose:** Draw a leased tile clipped to the current client.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callbacks only; bounded, never call from interrupts; drawing is retained and flushed by the OS.  

## graphics.sprite

```c
MB_Result mb_graphics_sprite(const MB_PaletteLease *lease, uint16_t index, int16_t x, int16_t y, uint16_t tile, uint16_t line);
```

**Parameters:**

- `lease`: Current exclusive token.
- `index`: 0..78; create sequentially or update an existing index.
- `x`: Screen pixel X, 0..312.
- `y`: Screen pixel Y, 0..216.
- `tile`: Absolute leased tile index.
- `line`: Palette line 0..3.

**arguments:** See typed parameters; no allocation.  
**capability:** H40 79 app sprites plus hidden OS anchor; hardware limit 20 sprites/320 sprite pixels per scanline; app must stay within limits.  
**edges:** No heap allocation; malformed inputs leave ownership unchanged.  
**errors:** Invalid token, skipped index, pixel/tile bounds or line invalid.  
**example:** MB_Result mb_graphics_tile(const MB_PaletteLease *lease, uint16_t x, uint16_t y, uint16_t tile, uint16_t line);  
**family:** graphics  
**negative:** Invalid token, tile, line or client bounds invalid.  
**ownership:** Retained OS tilemaps; ownership revoked on exit.  
**purpose:** Create/update one leased 8x8 sprite; indices form a dense prefix, starting at zero.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callback; sprite metadata queued, committed once at next VBlank.  

## ui.exit

```c
MB_Result mb_ui_exit(void);
```

**arguments:** None.  
**capability:** Fullscreen app UI; no OS chrome.  
**edges:** Pointer selection and controller focus share state.  
**errors:** Widget pool full busy; duplicate MB_UI_EXIT invalid.  
**example:** mb_ui_focus(1);  
**family:** ui  
**negative:** Disabled/unknown button.  
**ownership:** App opts into this client widget; occupies one of 24 slots, two rows at bottom right.  
**purpose:** Add an app-visible Exit button in the fullscreen client; app handles MB_UI_EXIT by requesting close.  
**returns:** MB_OK, MB_UNAVAILABLE outside fullscreen, or button creation error.  
**timing:** Bounded; callback safe.  

## graphics.load

```c
MB_Result mb_graphics_load(const MB_PaletteLease *lease, uint16_t tile, const uint32_t *rows, uint16_t count);
```

**Parameters:**

- `lease`: Active exclusive token.
- `tile`: Absolute first leased tile.
- `rows`: Immutable aligned ROM rows, eight uint32 words per tile; borrowed until ready or lease revocation.
- `count`: Nonzero tile count wholly within the leased bank.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_load(const MB_PaletteLease *lease, uint16_t tile, const uint32_t *rows, uint16_t count);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Queue a ROM pattern block; at most 24 tiles upload per VBlank.  
**returns:** MB_OK, MB_BUSY while another block is pending, or MB_INVALID for stale/null/out-of-bank input.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## graphics.ready

```c
MB_Result mb_graphics_ready(const MB_PaletteLease *lease);
```

**Parameters:**

- `lease`: Active exclusive token.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_ready(const MB_PaletteLease *lease);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Query whether the queued ROM upload is complete.  
**returns:** MB_OK when complete, MB_BUSY while pending, MB_INVALID for stale/null lease.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## graphics.fill

```c
MB_Result mb_graphics_fill(const MB_PaletteLease *lease, uint16_t plane, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t tile, uint16_t line);
```

**Parameters:**

- `lease`: Active token.
- `plane`: 0 for A, 1 for B.
- `x`: Client tile column.
- `y`: Client tile row.
- `width`: Nonzero tile columns within client.
- `height`: Nonzero tile rows within client.
- `tile`: Absolute leased pattern.
- `line`: Palette bank 0..3.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_fill(const MB_PaletteLease *lease, uint16_t plane, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t tile, uint16_t line);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Fill a retained leased plane rectangle.  
**returns:** MB_OK, MB_INVALID for malformed/stale inputs; MB_BUSY for queued upload or conservative sprite-band limit; MB_UNAVAILABLE without active ownership.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## graphics.map

```c
MB_Result mb_graphics_map(const MB_PaletteLease *lease, uint16_t plane, const uint16_t *cells);
```

**Parameters:**

- `lease`: Active token.
- `plane`: 0 for A, 1 for B.
- `cells`: Borrowed 1120 uint16 cells; low 11 bits relative leased tile, upper bits VDP flips, palette and priority. Copied during call.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_map(const MB_PaletteLease *lease, uint16_t plane, const uint16_t *cells);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Replace a full 40x28 fullscreen plane from a ROM map.  
**returns:** MB_OK, MB_INVALID for malformed/stale inputs; MB_BUSY for queued upload or conservative sprite-band limit; MB_UNAVAILABLE without active ownership.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## graphics.spritebox

```c
MB_Result mb_graphics_sprite_box(const MB_PaletteLease *lease, uint16_t index, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t tile, uint16_t line);
```

**Parameters:**

- `lease`: Active token.
- `index`: Dense prefix index 0..78; update existing or append next.
- `x`: Screen pixels -32..319.
- `y`: Screen pixels -32..223.
- `width`: 1..4 tiles.
- `height`: 1..4 tiles.
- `tile`: Absolute first tile; width times height consecutive patterns must fit.
- `line`: Palette bank 0..3.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_sprite_box(const MB_PaletteLease *lease, uint16_t index, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t tile, uint16_t line);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Queue a composed hardware sprite using column-major pattern order.  
**returns:** MB_OK, MB_INVALID for malformed/stale inputs; MB_BUSY for queued upload or conservative sprite-band limit; MB_UNAVAILABLE without active ownership.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## graphics.hide

```c
MB_Result mb_graphics_hide(const MB_PaletteLease *lease, uint16_t index);
```

**Parameters:**

- `lease`: Active token.
- `index`: Existing dense prefix index.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_hide(const MB_PaletteLease *lease, uint16_t index);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Hide an existing leased sprite without changing its index.  
**returns:** MB_OK, MB_INVALID for malformed/stale inputs; MB_BUSY for queued upload or conservative sprite-band limit; MB_UNAVAILABLE without active ownership.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## graphics.stats

```c
MB_Result mb_graphics_stats(MB_GraphicsStats *out);
```

**Parameters:**

- `out`: Caller-owned six uint16 fields; transfer values are CPU bytes per flush, not DMA or cycle measurements.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Ready returns MB_BUSY while uploading. Sprite budget conservatively checks 20 sprites / 320 pixels per eight-line band.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_graphics_stats(MB_GraphicsStats *out);  
**family:** graphics  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Read cumulative uploaded tiles, current queue/sprite prefix, CPU bytes per flush, and conservative eight-line band peak.  
**returns:** MB_OK, MB_INVALID for null output, MB_UNAVAILABLE without a lease.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## audio.volume

```c
MB_Result mb_audio_volume(uint16_t attenuation);
```

**Parameters:**

- `attenuation`: Requested attenuation 8..15; 8..14 use maximum hardware volume (attenuation 0); 15 mutes. Voice remains owned until audio_stop.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Attenuation 15 mutes without releasing the voice; audio_stop releases ownership.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_audio_volume(uint16_t attenuation);  
**family:** audio  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Change foreground PSG envelope without restarting its tone or noise.  
**returns:** MB_OK, MB_INVALID outside 8..15, MB_UNAVAILABLE without active foreground voice.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## app.fail

```c
MB_Result mb_app_fail(MB_Result error);
```

**Parameters:**

- `error`: Negative MB_Result failure code; valid during activate/event/tick, never interrupts.

**arguments:** Typed parameters specify pixels, tiles, words and attenuation.  
**capability:** H40 320x224 on PAL and NTSC. No HBlank writes, raster ownership or preemption.  
**edges:** Accepted during activation; teardown/modal happens after callback return. Not watchdog preemption.  
**errors:** Validation precedes mutation; failed requests preserve current resources.  
**example:** MB_Result mb_app_fail(MB_Result error);  
**family:** app  
**negative:** Null pointers, stale token, range overflow and unavailable resources fail safely.  
**ownership:** One foreground owner; borrowed queued ROM pointer canceled before shutdown/desktop reconstruction. No allocation.  
**purpose:** Defer foreground teardown after an activation or update error, then show a shell error.  
**returns:** MB_OK once deferred, MB_INVALID for nonnegative error, MB_UNAVAILABLE without active app.  
**timing:** Foreground activate/event/tick only. Never interrupts. Queued transfers commit after OS VBlank; retained maps are bounded to 192 cells per plane per flush.  

## fs.info

```c
MB_Result mb_fs_info(MB_FSInfo *info);
```

**Parameters:**

- `info`: Caller-owned output; version set to 1.

**arguments:** Caller-owned output; version set to 1.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** 4 KiB including sixteen metadata slots and six handle slots.  
**errors:** Null invalid; corrupt metadata MB_CORRUPT.  
**example:** mb_fs_info(MB_FSInfo *info);  
**family:** fs  
**negative:** Null invalid; corrupt metadata MB_CORRUPT.  
**ownership:** Output copied, not retained; reservation already excluded from heap.  
**purpose:** Compatibility query for RAM: capacity through the unified VFS.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## fs.list

```c
MB_Result mb_dir_list(uint16_t directory, uint16_t index, MB_FileInfo *info);
```

**Parameters:**

- `directory`: MB_DIR_DOCUMENTS or MB_DIR_PICTURES.
- `index`: Dense zero-based index; reorder after deletion.
- `info`: Caller-owned output; version 1, basename, directory and byte length.

**arguments:** MB_DIR_DOCUMENTS or MB_DIR_PICTURES.; Caller-owned output; version 1, basename, directory and byte length.; Dense zero-based index; reorder after deletion.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Sixteen total files shared between directories.  
**errors:** Invalid directory/null MB_INVALID; past end MB_NOT_FOUND; corruption MB_CORRUPT.  
**example:** mb_dir_list(uint16_t directory, uint16_t index, MB_FileInfo *info);  
**family:** fs  
**negative:** Invalid directory/null MB_INVALID; past end MB_NOT_FOUND; corruption MB_CORRUPT.  
**ownership:** Output copy; names are inline ASCII, not borrowed pointers.  
**purpose:** Compatibility directory-number adapter to unified RAM path enumeration.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.stat

```c
MB_Result mb_file_stat(const char *path, MB_FileInfo *info);
```

**Parameters:**

- `path`: NUL-terminated absolute forward-slash path, at most 63 ASCII bytes, no dot/traversal/duplicate separators; drive and fixed paths case insensitive. RAM names retain 1..15 basename letters/digits/_/- plus .txt/.mbp; ROM paths are fixed bundled resources.
- `info`: Caller-owned output version 1.

**arguments:** NUL terminated RAM:/Documents/name.txt or RAM:/Pictures/name.mbp; basename 1..15 letters, digits, underscore or hyphen.; Caller-owned output version 1.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** No traversal, recursive folders, spaces or other extensions.  
**errors:** Bad path MB_INVALID; absent MB_NOT_FOUND; corruption MB_CORRUPT. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_stat(const char *path, MB_FileInfo *info);  
**family:** file  
**negative:** Bad path MB_INVALID; absent MB_NOT_FOUND; corruption MB_CORRUPT.  
**ownership:** Path read synchronously, output copied. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Query a RAM file by case-insensitive full path.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.open

```c
MB_Result mb_file_open(const char *path, uint16_t flags, MB_FileHandle *handle);
```

**Parameters:**

- `path`: NUL-terminated absolute forward-slash path, at most 63 ASCII bytes, no dot/traversal/duplicate separators; drive and fixed paths case insensitive. RAM names retain 1..15 basename letters/digits/_/- plus .txt/.mbp; ROM paths are fixed bundled resources.
- `flags`: READ=1, WRITE=2, CREATE=4; CREATE requires WRITE; one writer excludes all other handles.
- `handle`: Caller-owned opaque nonzero token output.

**arguments:** Validated full RAM path as for stat.; Caller-owned opaque nonzero token output.; READ=1, WRITE=2, CREATE=4; CREATE requires WRITE; one writer excludes all other handles.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** Creation is zero bytes; failed write may leave new empty file; never truncates existing data. Global six-handle exhaustion is checked before backend conflicts. Tokens never wrap; 65535 lifetime opens require reboot.  
**errors:** Invalid path/flags/output MB_INVALID; absent MB_NOT_FOUND; conflicts MB_BUSY; six handles or sixteen files MB_LIMIT. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_open(const char *path, uint16_t flags, MB_FileHandle *handle);  
**family:** file  
**negative:** Invalid path/flags/output MB_INVALID; absent MB_NOT_FOUND; conflicts MB_BUSY; six handles or sixteen files MB_LIMIT.  
**ownership:** Handle owned by foreground, valid until close, app exit or reset; never a RAM pointer. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Open or create an empty file without truncating an existing file.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.read

```c
MB_Result mb_file_read(MB_FileHandle handle, uint16_t offset, uint8_t *data, uint16_t capacity, uint16_t *actual);
```

**Parameters:**

- `handle`: Live READ handle.
- `offset`: Byte offset no greater than file size.
- `data`: Caller-owned capacity bytes; null allowed only when capacity zero.
- `capacity`: Maximum copied bytes.
- `actual`: Caller-owned byte count.

**arguments:** Byte offset no greater than file size.; Live READ handle.; Caller-owned capacity bytes; null allowed only when capacity zero.; Maximum copied bytes.; Caller-owned byte count.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** EOF returns zero; short reads allowed.  
**errors:** Bad token/offset/buffer MB_INVALID; corruption MB_CORRUPT. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_read(MB_FileHandle handle, uint16_t offset, uint8_t *data, uint16_t capacity, uint16_t *actual);  
**family:** file  
**negative:** Bad token/offset/buffer MB_INVALID; corruption MB_CORRUPT.  
**ownership:** Bytes copied synchronously; no retained pointers. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Read bounded bytes from a file, reporting actual length.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.write

```c
MB_Result mb_file_write(MB_FileHandle handle, const uint8_t *data, uint16_t length);
```

**Parameters:**

- `handle`: Live WRITE handle.
- `data`: Borrowed caller-owned bytes; null allowed for empty file.
- `length`: Replacement byte count, at most usable disk capacity.

**arguments:** Live WRITE handle.; Borrowed caller-owned bytes; null allowed for empty file.; Replacement byte count, at most usable disk capacity.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** Validate full capacity before compaction; no failure after commit starts.  
**errors:** Bad token/buffer MB_INVALID; insufficient capacity MB_NO_SPACE; corruption MB_CORRUPT; failures preserve existing bytes. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_write(MB_FileHandle handle, const uint8_t *data, uint16_t length);  
**family:** file  
**negative:** Bad token/buffer MB_INVALID; insufficient capacity MB_NO_SPACE; corruption MB_CORRUPT; failures preserve existing bytes.  
**ownership:** Input read synchronously; no pointer retained or raw arena exposed. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Atomically replace the entire contents of an open file.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.close

```c
MB_Result mb_file_close(MB_FileHandle handle);
```

**Parameters:**

- `handle`: Live token; stale tokens rejected.

**arguments:** Live token; stale tokens rejected.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** Six handles, monotonic nonwrapping tokens per boot; exhaustion MB_LIMIT.  
**errors:** Invalid/stale token MB_INVALID. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_close(MB_FileHandle handle);  
**family:** file  
**negative:** Invalid/stale token MB_INVALID.  
**ownership:** All outstanding handles close automatically on app exit; files persist. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Release an opaque file handle.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.rename

```c
MB_Result mb_file_rename(const char *path, const char *new_path);
```

**Parameters:**

- `path`: NUL-terminated absolute forward-slash path, at most 63 ASCII bytes, no dot/traversal/duplicate separators; drive and fixed paths case insensitive. RAM names retain 1..15 basename letters/digits/_/- plus .txt/.mbp; ROM paths are fixed bundled resources.
- `new_path`: New validated path in the same directory.

**arguments:** Existing full RAM path.; New validated path in the same directory.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** Case-only rename allowed; filename limits enforced.  
**errors:** Invalid path/cross-directory MB_INVALID; duplicate MB_EXISTS; open file MB_BUSY; absent MB_NOT_FOUND. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_rename(const char *path, const char *new_path);  
**family:** file  
**negative:** Invalid path/cross-directory MB_INVALID; duplicate MB_EXISTS; open file MB_BUSY; absent MB_NOT_FOUND.  
**ownership:** Both paths borrowed only for call. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Rename within the same fixed directory without altering content.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## file.delete

```c
MB_Result mb_file_delete(const char *path);
```

**Parameters:**

- `path`: NUL-terminated absolute forward-slash path, at most 63 ASCII bytes, no dot/traversal/duplicate separators; drive and fixed paths case insensitive. RAM names retain 1..15 basename letters/digits/_/- plus .txt/.mbp; ROM paths are fixed bundled resources.

**arguments:** Validated full RAM path.  
**capability:** Unified drive-dispatched VFS; RAM: volatile/writable and ROM: read-only; SAVE:/SD: MB_NOT_SUPPORTED.  
**edges:** Zero-size and final file deletion safe.  
**errors:** Absent MB_NOT_FOUND; open file MB_BUSY; invalid MB_INVALID; corruption MB_CORRUPT. ROM mutation MB_READ_ONLY; unavailable future drives MB_NOT_SUPPORTED; backend I/O failure MB_IO.  
**example:** mb_file_delete(const char *path);  
**family:** file  
**negative:** Absent MB_NOT_FOUND; open file MB_BUSY; invalid MB_INVALID; corruption MB_CORRUPT.  
**ownership:** Path borrowed only for call; contents permanently removed this session. VFS tokens are global across mounts, nonwrapping, invalidated at close/app exit/reset.  
**purpose:** Delete a closed RAM file and reclaim its payload bytes.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## app.document

```c
MB_Result mb_app_document(char *path, uint16_t capacity);
```

**Parameters:**

- `path`: Caller-owned ASCII output.
- `capacity`: At least 44 bytes for any accepted path.

**arguments:** At least 44 bytes for any accepted path.; Caller-owned ASCII output.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Direct Apps launches have no association path.  
**errors:** Null/too small MB_INVALID/MB_OVERFLOW; no document MB_NOT_FOUND.  
**example:** mb_app_document(char *path, uint16_t capacity);  
**family:** app  
**negative:** Null/too small MB_INVALID/MB_OVERFLOW; no document MB_NOT_FOUND.  
**ownership:** Path copy valid independently of launch; no file handle implied.  
**purpose:** Copy the file association launch path, or report a new document launch.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## app.guard

```c
void mb_app_close_guard(uint16_t required);
```

**Parameters:**

- `required`: Zero clears guard; nonzero asks active app to confirm.

**arguments:** Zero clears guard; nonzero asks active app to confirm.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Editor apps maximize only; fullscreen universal escape remains OS-owned.  
**errors:** Idle is harmless; explicit mb_app_close still closes.  
**example:** mb_app_close_guard(uint16_t required);  
**family:** app  
**negative:** Idle is harmless; explicit mb_app_close still closes.  
**ownership:** Guard automatically cleared between apps.  
**purpose:** Request MB_CLOSE_REQUEST instead of normal B/chrome closing, for dirty document confirmation.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## text.keyboard

```c
MB_Result mb_text_keyboard(uint16_t x, uint16_t y);
```

**Parameters:**

- `x`: Client tile origin, requires 40 columns.
- `y`: Client tile origin, requires six rows.

**arguments:** Client tile origin, requires six rows.; Client tile origin, requires 40 columns.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Letters, digits, punctuation, space, Enter, Backspace, Delete, cursor left/right, Done and Cancel.  
**errors:** Outside client MB_INVALID.  
**example:** mb_text_keyboard(uint16_t x, uint16_t y);  
**family:** text  
**negative:** Outside client MB_INVALID.  
**ownership:** No document buffer retained; OS owns only key focus/case.  
**purpose:** Draw shared 10 by 6 on-screen ASCII keyboard using existing theme roles.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## text.event

```c
MB_Result mb_text_event(const MB_Event *event, MB_TextInput *input);
```

**Parameters:**

- `event`: Borrowed semantic event; pointer coordinates in screen pixels.
- `input`: Caller-owned output; NONE when event is not a key.

**arguments:** Caller-owned output; NONE when event is not a key.; Borrowed semantic event; pointer coordinates in screen pixels.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Navigation wraps; Shift toggles case; future physical keyboard can emit the same commands.  
**errors:** Null MB_INVALID; keyboard not drawn MB_UNAVAILABLE.  
**example:** mb_text_event(const MB_Event *event, MB_TextInput *input);  
**family:** text  
**negative:** Null MB_INVALID; keyboard not drawn MB_UNAVAILABLE.  
**ownership:** No text buffer mutation; application owns edits and limits.  
**purpose:** Translate keyboard controller or pointer actions into shared text-edit commands.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## text.reset

```c
void mb_text_reset(void);
```

**arguments:** None.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** OS also resets keyboard on app close.  
**errors:** None.  
**example:** mb_text_reset(void);  
**family:** text  
**negative:** None.  
**ownership:** No borrowed resources; call after replacing a view.  
**purpose:** Reset transient on-screen keyboard focus and visibility.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## canvas.open

```c
MB_Result mb_canvas_open(uint16_t x, uint16_t y);
```

**Parameters:**

- `x`: Client tile origin with sixteen tiles available horizontally.
- `y`: Client tile origin with sixteen tiles available vertically.

**arguments:** Client tile origin with sixteen tiles available vertically.; Client tile origin with sixteen tiles available horizontally.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Eight changed patterns uploaded per VBlank; no full-screen framebuffer.  
**errors:** Idle/lease MB_UNAVAILABLE; outside client MB_INVALID.  
**example:** mb_canvas_open(uint16_t x, uint16_t y);  
**family:** canvas  
**negative:** Idle/lease MB_UNAVAILABLE; outside client MB_INVALID.  
**ownership:** One foreground canvas; 256 patterns after the 64 reserved OS patterns, no palette lease.  
**purpose:** Open a shared-palette 64 by 64 canvas at 2x integer scale.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## canvas.update

```c
MB_Result mb_canvas_update(const uint8_t *pixels, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t cursor_x, uint16_t cursor_y);
```

**Parameters:**

- `pixels`: Borrowed 1024-byte 2bpp row-major image; four pixels per byte, leftmost in bits 7..6.
- `x`: Logical dirty rectangle origin 0..63.
- `y`: Logical dirty rectangle origin 0..63.
- `width`: Nonzero extent fully inside 64 pixels.
- `height`: Nonzero extent fully inside 64 pixels.
- `cursor_x`: 0..63 cursor coordinate, or 65535 to hide.
- `cursor_y`: 0..63 cursor coordinate, or 65535 to hide.

**arguments:** Borrowed 1024-byte 2bpp row-major image; four pixels per byte, leftmost in bits 7..6.; Nonzero extent fully inside 64 pixels.; 0..63 cursor coordinate, or 65535 to hide.; Logical dirty rectangle origin 0..63.; 0..63 cursor coordinate, or 65535 to hide.; Logical dirty rectangle origin 0..63.; Nonzero extent fully inside 64 pixels.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** Four logical colors use palette-zero roles 2,3,5,6; changes never overwrite CRAM.  
**errors:** Closed/null/out-of-range MB_INVALID.  
**example:** mb_canvas_update(const uint8_t *pixels, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t cursor_x, uint16_t cursor_y);  
**family:** canvas  
**negative:** Closed/null/out-of-range MB_INVALID.  
**ownership:** Pixels borrowed until canvas close/app close; app must keep static workspace alive.  
**purpose:** Queue changed packed pixels and old/new cursor cells for bounded VBlank updates.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## canvas.close

```c
void mb_canvas_close(void);
```

**arguments:** None.  
**capability:** Native trusted foreground SDK; bounded static storage, no heap or persistent media.  
**edges:** No allocation. Legacy shared canvas keeps palette ownership; variable 16-colour canvas restores PAL2 and OS background on close.  
**errors:** None.  
**example:** mb_canvas_close(void);  
**family:** canvas  
**negative:** None.  
**ownership:** Must precede reuse of image buffer for another view; OS revokes before app shutdown.  
**purpose:** Cancel canvas uploads and release borrowed image/context, restoring dedicated palette ownership if used.  
**returns:** MB_OK on success; negative MB_Result on failure unless returning void.  
**timing:** Synchronous foreground callbacks only; no interrupt or concurrent access.  

## fs.capacity

```c
MB_Result mb_fs_capacity(const char *drive, uint16_t version, MB_DriveInfo *info);
```

**Parameters:**

- `drive`: Case-insensitive drive prefix RAM: or ROM:; no path suffix.
- `version`: Required output contract version, currently 1; other versions rejected.
- `info`: Caller-owned output; capability bits describe mounted/writable/volatile/directories/offset reads/rename/exact space/atomic replacement.

**arguments:** Case-insensitive drive prefix RAM: or ROM:; no path suffix.; Caller-owned output; capability bits describe mounted/writable/volatile/directories/offset reads/rename/exact space/atomic replacement.; Required output contract version, currently 1; other versions rejected.  
**capability:** One unified VFS with internal RAM and read-only ROM adapters; SAVE/SD unavailable.  
**edges:** RAM reservation already excludes heap; RAM capacity is payload; ROM capacity/used is exposed immutable resource bytes, reserved is actual cartridge image size. No writable free-byte claim for ROM.  
**errors:** Bad output/prefix MB_INVALID; unsupported version MB_VERSION; unknown MB_NOT_FOUND; SAVE:/SD: MB_NOT_SUPPORTED.  
**example:** mb_fs_capacity(const char *drive, uint16_t version, MB_DriveInfo *info);  
**family:** fs  
**negative:** Bad output/prefix MB_INVALID; unsupported version MB_VERSION; unknown MB_NOT_FOUND; SAVE:/SD: MB_NOT_SUPPORTED.  
**ownership:** Inputs borrowed synchronously; output copied, name/backend strings immutable ROM for the boot.  
**purpose:** Query a drive and exact-capacity capabilities through the unified VFS.  
**returns:** MB_OK, MB_INVALID, MB_VERSION, MB_NOT_FOUND, MB_NOT_SUPPORTED, or backend error.  
**timing:** Bounded synchronous foreground callbacks; never interrupts; no allocation.  

## fs.drive

```c
MB_Result mb_fs_drive(uint16_t index, uint16_t version, MB_DriveInfo *info);
```

**Parameters:**

- `index`: 0 RAM:, 1 ROM:, 2 SAVE:, 3 SD:; later indices MB_NOT_FOUND.
- `version`: Required descriptor version 1.
- `info`: Caller-owned descriptor; future devices have zero capability/space and backend Unavailable.

**arguments:** Caller-owned descriptor; future devices have zero capability/space and backend Unavailable.; Required descriptor version 1.; 0 RAM:, 1 ROM:, 2 SAVE:, 3 SD:; later indices MB_NOT_FOUND.  
**capability:** One unified VFS with internal RAM and read-only ROM adapters; SAVE/SD unavailable.  
**edges:** Indices 0 RAM, 1 ROM, 2 SAVE, 3 SD. Unavailable descriptors return MB_OK with zero capabilities; operations return MB_NOT_SUPPORTED.  
**errors:** Null MB_INVALID; bad version MB_VERSION; index beyond table MB_NOT_FOUND.  
**example:** mb_fs_drive(uint16_t index, uint16_t version, MB_DriveInfo *info);  
**family:** fs  
**negative:** Null MB_INVALID; bad version MB_VERSION; index beyond table MB_NOT_FOUND.  
**ownership:** Inputs borrowed synchronously; output copied, name/backend strings immutable ROM for the boot.  
**purpose:** Enumerate bounded mount descriptors and unavailable future devices.  
**returns:** MB_OK, MB_INVALID, MB_VERSION, MB_NOT_FOUND, MB_NOT_SUPPORTED, or backend error.  
**timing:** Bounded synchronous foreground callbacks; never interrupts; no allocation.  

## fs.entries

```c
MB_Result mb_dir_list_path(const char *directory, uint16_t index, MB_FileInfo *info);
```

**Parameters:**

- `directory`: Forward-slash absolute path RAM:/, RAM:/Documents/, RAM:/Pictures/, ROM:/, ROM:/Help/ or ROM:/Resources/; trailing slash optional.
- `index`: Dense zero-based index; may change on deletion.
- `info`: Caller-owned version-1 output; flags identify DIRECTORY or REGULAR plus READ_ONLY.

**arguments:** Forward-slash absolute path RAM:/, RAM:/Documents/, RAM:/Pictures/, ROM:/, ROM:/Help/ or ROM:/Resources/; trailing slash optional.; Caller-owned version-1 output; flags identify DIRECTORY or REGULAR plus READ_ONLY.; Dense zero-based index; may change on deletion.  
**capability:** One unified VFS with internal RAM and read-only ROM adapters; SAVE/SD unavailable.  
**edges:** No dot components, duplicate separators, backslashes or recursive creation; ASCII paths at most 63 bytes.  
**errors:** Invalid/null MB_INVALID; absent directory/end MB_NOT_FOUND; unsupported SAVE/SD MB_NOT_SUPPORTED.  
**example:** mb_dir_list_path(const char *directory, uint16_t index, MB_FileInfo *info);  
**family:** fs  
**negative:** Invalid/null MB_INVALID; absent directory/end MB_NOT_FOUND; unsupported SAVE/SD MB_NOT_SUPPORTED.  
**ownership:** Inputs borrowed synchronously; output copied, name/backend strings immutable ROM for the boot.  
**purpose:** Enumerate files or fixed directory entries by drive-qualified path.  
**returns:** MB_OK, MB_INVALID, MB_VERSION, MB_NOT_FOUND, MB_NOT_SUPPORTED, or backend error.  
**timing:** Bounded synchronous foreground callbacks; never interrupts; no allocation.  

## music.track

```c
MB_Result mb_music_track(uint16_t index, uint16_t version, MB_MusicTrack *track);
```

**Parameters:**

- `index`: Zero-based bundled track index.
- `version`: Required metadata version 1.
- `track`: Caller-owned output; title and credit are borrowed immutable ROM strings.

**arguments:** See parameters; bounded verified ROM catalog and VFS file inspection.  
**capability:** SGDK 2.11 XGM2 YM2612/PSG, PAL/NTSC automatic timing. Play once; no seek, pause, streaming or persistence.  
**edges:** Changing track stops old audio. Stop/error/source switch/close silence music; completion returns stopped. External storage must pass a future command-stream/addressability validation before enabling playback.  
**errors:** No output MB_INVALID; version other than 1 MB_VERSION; index beyond catalog MB_NOT_FOUND.  
**example:** MB_Result mb_music_track(uint16_t index, uint16_t version, MB_MusicTrack *track);  
**family:** music  
**negative:** No output MB_INVALID; version other than 1 MB_VERSION; index beyond catalog MB_NOT_FOUND.  
**ownership:** One trusted foreground app owns audio; bundled source remains in ROM. No shared concurrent PSG/music playback, external pointers or heap/song-buffer allocation. Stops on app teardown.  
**purpose:** Enumerate verified bundled XGM2 music metadata.  
**returns:** MB_OK, MB_INVALID, MB_VERSION, MB_NOT_FOUND.  
**timing:** Foreground only after activation; synchronous driver commands, never interrupt or init context. Stop also safe at OS reset/teardown.  

## music.play

```c
MB_Result mb_music_play(uint16_t index);
```

**Parameters:**

- `index`: Zero-based verified ROM catalog index; no raw pointers accepted.

**arguments:** See parameters; bounded verified ROM catalog and VFS file inspection.  
**capability:** SGDK 2.11 XGM2 YM2612/PSG, PAL/NTSC automatic timing. Play once; no seek, pause, streaming or persistence.  
**edges:** Changing track stops old audio. Stop/error/source switch/close silence music; completion returns stopped. External storage must pass a future command-stream/addressability validation before enabling playback.  
**errors:** No foreground app MB_UNAVAILABLE; bad index MB_NOT_FOUND; unaligned compiled ROM resource MB_CORRUPT.  
**example:** MB_Result mb_music_play(uint16_t index);  
**family:** music  
**negative:** No foreground app MB_UNAVAILABLE; bad index MB_NOT_FOUND; unaligned compiled ROM resource MB_CORRUPT.  
**ownership:** One trusted foreground app owns audio; bundled source remains in ROM. No shared concurrent PSG/music playback, external pointers or heap/song-buffer allocation. Stops on app teardown.  
**purpose:** Stop previous audio and start one trusted bundled XGM2 track.  
**returns:** MB_OK, MB_UNAVAILABLE, MB_NOT_FOUND, MB_CORRUPT.  
**timing:** Foreground only after activation; synchronous driver commands, never interrupt or init context. Stop also safe at OS reset/teardown.  

## music.stop

```c
void mb_music_stop(void);
```

**arguments:** See parameters; bounded verified ROM catalog and VFS file inspection.  
**capability:** SGDK 2.11 XGM2 YM2612/PSG, PAL/NTSC automatic timing. Play once; no seek, pause, streaming or persistence.  
**edges:** Changing track stops old audio. Stop/error/source switch/close silence music; completion returns stopped. External storage must pass a future command-stream/addressability validation before enabling playback.  
**errors:** Idempotent; safe with no music or foreground app.  
**example:** void mb_music_stop(void);  
**family:** music  
**negative:** Idempotent; safe with no music or foreground app.  
**ownership:** One trusted foreground app owns audio; bundled source remains in ROM. No shared concurrent PSG/music playback, external pointers or heap/song-buffer allocation. Stops on app teardown.  
**purpose:** Stop music, unload its Z80 driver and silence FM/PSG.  
**returns:** None.  
**timing:** Foreground only after activation; synchronous driver commands, never interrupt or init context. Stop also safe at OS reset/teardown.  

## music.state

```c
uint16_t mb_music_state(void);
```

**arguments:** See parameters; bounded verified ROM catalog and VFS file inspection.  
**capability:** SGDK 2.11 XGM2 YM2612/PSG, PAL/NTSC automatic timing. Play once; no seek, pause, streaming or persistence.  
**edges:** Changing track stops old audio. Stop/error/source switch/close silence music; completion returns stopped. External storage must pass a future command-stream/addressability validation before enabling playback.  
**errors:** Driver queried only while owned; startup waits two VBlanks before completion polling.  
**example:** uint16_t mb_music_state(void);  
**family:** music  
**negative:** Driver queried only while owned; startup waits two VBlanks before completion polling.  
**ownership:** One trusted foreground app owns audio; bundled source remains in ROM. No shared concurrent PSG/music playback, external pointers or heap/song-buffer allocation. Stops on app teardown.  
**purpose:** Query stopped, playing or error state, polling natural completion.  
**returns:** MB_MUSIC_STOPPED, MB_MUSIC_PLAYING or MB_MUSIC_ERROR.  
**timing:** Foreground only after activation; synchronous driver commands, never interrupt or init context. Stop also safe at OS reset/teardown.  

## music.capabilities

```c
uint16_t mb_music_capabilities(void);
```

**arguments:** See parameters; bounded verified ROM catalog and VFS file inspection.  
**capability:** SGDK 2.11 XGM2 YM2612/PSG, PAL/NTSC automatic timing. Play once; no seek, pause, streaming or persistence.  
**edges:** Changing track stops old audio. Stop/error/source switch/close silence music; completion returns stopped. External storage must pass a future command-stream/addressability validation before enabling playback.  
**errors:** Raw VGM, arbitrary XGC and RAM playback are unsupported.  
**example:** uint16_t mb_music_capabilities(void);  
**family:** music  
**negative:** Raw VGM, arbitrary XGC and RAM playback are unsupported.  
**ownership:** One trusted foreground app owns audio; bundled source remains in ROM. No shared concurrent PSG/music playback, external pointers or heap/song-buffer allocation. Stops on app teardown.  
**purpose:** Query supported music sources.  
**returns:** MB_MUSIC_BUNDLED; MB_MUSIC_EXTERNAL is not advertised.  
**timing:** Foreground only after activation; synchronous driver commands, never interrupt or init context. Stop also safe at OS reset/teardown.  

## music.file

```c
MB_Result mb_music_play_file(const char *path);
```

**Parameters:**

- `path`: Borrowed full VFS file path; drive-independent. Read-only header inspection and checked close; no retained buffer.

**arguments:** See parameters; bounded verified ROM catalog and VFS file inspection.  
**capability:** SGDK 2.11 XGM2 YM2612/PSG, PAL/NTSC automatic timing. Play once; no seek, pause, streaming or persistence.  
**edges:** Changing track stops old audio. Stop/error/source switch/close silence music; completion returns stopped. External storage must pass a future command-stream/addressability validation before enabling playback.  
**errors:** Malformed XGC header/length MB_CORRUPT; version other than 0x10 MB_VERSION. Raw .vgm and structurally plausible .xgc return MB_NOT_SUPPORTED; no file is sent to the driver.  
**example:** MB_Result mb_music_play_file(const char *path);  
**family:** music  
**negative:** Malformed XGC header/length MB_CORRUPT; version other than 0x10 MB_VERSION. Raw .vgm and structurally plausible .xgc return MB_NOT_SUPPORTED; no file is sent to the driver.  
**ownership:** One trusted foreground app owns audio; bundled source remains in ROM. No shared concurrent PSG/music playback, external pointers or heap/song-buffer allocation. Stops on app teardown.  
**purpose:** Inspect a VFS music file safely and report experimental playback as unsupported.  
**returns:** MB_NOT_SUPPORTED, MB_CORRUPT, MB_VERSION, MB_INVALID, MB_NOT_FOUND, MB_UNAVAILABLE, MB_IO or VFS error.  
**timing:** Foreground only after activation; synchronous driver commands, never interrupt or init context. Stop also safe at OS reset/teardown.  

## canvas.attach

```c
MB_Result mb_canvas_attach(const MB_CanvasSource *source, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *colors);
```

**Parameters:**

- `source`: Version 1, 8-aligned nonzero dimensions at most 512x256; context remains borrowed. Provide pixel callback or preferred tile callback: tile receives valid 8x8 tile coordinates and writes eight packed 4bpp uint32 rows synchronously. Never mutate source or call services from callbacks.
- `x`: Client tile viewport origin.
- `y`: Client tile viewport origin.
- `width`: Nonzero viewport width in tiles, wholly within client, at most 40.
- `height`: Nonzero viewport height in tiles, wholly within client, at most 28.
- `colors`: Sixteen copied legal CRAM RGB333 words (mask 0xeee); index zero is the canvas background colour.

**arguments:** See parameter bounds; all extents are logical pixels or client tiles as specified.  
**capability:** 16 colours, up to 512x256 logical pixels, tile-aligned viewport within client. Maximum 16 dirty patterns per refresh; integer scale 1/2/4/8 or half-size overview.  
**edges:** Copies description/palette, not pixels; callback indices are checked before calls. Legacy open replaces this canvas; attach replaces legacy canvas.  
**errors:** Null/invalid bounds or palette/cursor MB_INVALID; source version not 1 MB_VERSION; idle/exclusive lease MB_UNAVAILABLE.  
**example:** MB_Result mb_canvas_attach(const MB_CanvasSource *source, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *colors);  
**family:** canvas  
**negative:** Invalid source, palette words, zoom and bounds rejected before ownership changes.  
**ownership:** One foreground canvas borrows source context until close. PAL2 and viewport patterns are privately owned by the OS; desktop PAL0/1 and pointer PAL3 are preserved. Close restores palette/background. No framebuffer or heap allocation.  
**purpose:** Attach a native 4bpp variable-resolution canvas to a maximized or normal client viewport.  
**returns:** MB_OK on success; MB_INVALID, MB_VERSION or MB_UNAVAILABLE on failure.  
**timing:** Foreground only. Pixel callback is bounded and read-only; called during video flush after VBlank, never interrupt or reentrant service context.  

## canvas.view

```c
MB_Result mb_canvas_view(uint16_t x, uint16_t y, uint16_t scale, uint16_t cursor_x, uint16_t cursor_y);
```

**Parameters:**

- `x`: Logical origin within source width.
- `y`: Logical origin within source height.
- `scale`: 1,2,4,8 integer magnification; 0 is a sampled half-size overview. No aspect distortion.
- `cursor_x`: Logical cursor within source, or 65535 paired with cursor_y to hide.
- `cursor_y`: Logical cursor within source, or 65535 paired with cursor_x to hide.

**arguments:** See parameter bounds; all extents are logical pixels or client tiles as specified.  
**capability:** 16 colours, up to 512x256 logical pixels, tile-aligned viewport within client. Maximum 16 dirty patterns per refresh; integer scale 1/2/4/8 or half-size overview.  
**edges:** View changes mark all viewport tiles; cursor-only changes prioritize pending drawing updates and move the square brush outline without recolouring image pixels. Outside the image uses canvas colour zero; cursor never changes document bytes.  
**errors:** Null/invalid bounds or palette/cursor MB_INVALID; source version not 1 MB_VERSION; idle/exclusive lease MB_UNAVAILABLE.  
**example:** MB_Result mb_canvas_view(uint16_t x, uint16_t y, uint16_t scale, uint16_t cursor_x, uint16_t cursor_y);  
**family:** canvas  
**negative:** Invalid source, palette words, zoom and bounds rejected before ownership changes.  
**ownership:** One foreground canvas borrows source context until close. PAL2 and viewport patterns are privately owned by the OS; desktop PAL0/1 and pointer PAL3 are preserved. Close restores palette/background. No framebuffer or heap allocation.  
**purpose:** Set logical viewport origin, integer zoom and document cursor without changing pixels.  
**returns:** MB_OK on success; MB_INVALID, MB_VERSION or MB_UNAVAILABLE on failure.  
**timing:** Foreground only. Pixel callback is bounded and read-only; called during video flush after VBlank, never interrupt or reentrant service context.  

## canvas.damage

```c
MB_Result mb_canvas_damage(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
```

**Parameters:**

- `x`: Logical dirty origin within source.
- `y`: Logical dirty origin within source.
- `width`: Nonzero extent within source.
- `height`: Nonzero extent within source.

**arguments:** See parameter bounds; all extents are logical pixels or client tiles as specified.  
**capability:** 16 colours, up to 512x256 logical pixels, tile-aligned viewport within client. Maximum 16 dirty patterns per refresh; integer scale 1/2/4/8 or half-size overview.  
**edges:** Damage outside viewport costs no pattern upload. Repeated updates coalesce in a fixed bitmap.  
**errors:** Null/invalid bounds or palette/cursor MB_INVALID; source version not 1 MB_VERSION; idle/exclusive lease MB_UNAVAILABLE.  
**example:** MB_Result mb_canvas_damage(uint16_t x, uint16_t y, uint16_t width, uint16_t height);  
**family:** canvas  
**negative:** Invalid source, palette words, zoom and bounds rejected before ownership changes.  
**ownership:** One foreground canvas borrows source context until close. PAL2 and viewport patterns are privately owned by the OS; desktop PAL0/1 and pointer PAL3 are preserved. Close restores palette/background. No framebuffer or heap allocation.  
**purpose:** Queue only viewport tiles intersecting a changed logical image rectangle.  
**returns:** MB_OK on success; MB_INVALID, MB_VERSION or MB_UNAVAILABLE on failure.  
**timing:** Foreground only. Pixel callback is bounded and read-only; called during video flush after VBlank, never interrupt or reentrant service context.  

## canvas.palette

```c
MB_Result mb_canvas_palette(const uint16_t *colors);
```

**Parameters:**

- `colors`: Sixteen copied legal CRAM words, each masked by 0xeee; no partial update on error.

**arguments:** See parameter bounds; all extents are logical pixels or client tiles as specified.  
**capability:** 16 colours, up to 512x256 logical pixels, tile-aligned viewport within client. Maximum 16 dirty patterns per refresh; integer scale 1/2/4/8 or half-size overview.  
**edges:** Palette writes queue for VBlank; pixels are unchanged. Desktop and pointer colours stay intact.  
**errors:** Null/invalid bounds or palette/cursor MB_INVALID; source version not 1 MB_VERSION; idle/exclusive lease MB_UNAVAILABLE.  
**example:** MB_Result mb_canvas_palette(const uint16_t *colors);  
**family:** canvas  
**negative:** Invalid source, palette words, zoom and bounds rejected before ownership changes.  
**ownership:** One foreground canvas borrows source context until close. PAL2 and viewport patterns are privately owned by the OS; desktop PAL0/1 and pointer PAL3 are preserved. Close restores palette/background. No framebuffer or heap allocation.  
**purpose:** Replace the canvas dedicated 16-colour RGB333 palette.  
**returns:** MB_OK on success; MB_INVALID, MB_VERSION or MB_UNAVAILABLE on failure.  
**timing:** Foreground only. Pixel callback is bounded and read-only; called during video flush after VBlank, never interrupt or reentrant service context.  

## canvas.swatch

```c
MB_Result mb_canvas_swatch(uint16_t x, uint16_t y, uint16_t color);
```

**Parameters:**

- `x`: Client tile x within bounds.
- `y`: Client tile y within bounds.
- `color`: Palette index 0..15.

**arguments:** See parameter bounds; all extents are logical pixels or client tiles as specified.  
**capability:** 16 colours, up to 512x256 logical pixels, tile-aligned viewport within client. Maximum 16 dirty patterns per refresh; integer scale 1/2/4/8 or half-size overview.  
**edges:** Sixteen solid patterns share the canvas pattern reservation; uploaded during foreground VBlank flush. Swatches do not change document pixels. A swatch on the final cell of a UI button is retained by that widget through hover/focus/pressed redraws. An empty-label button renders a 2x2-tile colour square across its final two columns; other cells are one-shot drawing.  
**errors:** Null/invalid bounds or palette/cursor MB_INVALID; source version not 1 MB_VERSION; idle/exclusive lease MB_UNAVAILABLE.  
**example:** MB_Result mb_canvas_swatch(uint16_t x, uint16_t y, uint16_t color);  
**family:** canvas  
**negative:** Invalid source, palette words, zoom and bounds rejected before ownership changes.  
**ownership:** One foreground canvas borrows source context until close. PAL2 and viewport patterns are privately owned by the OS; desktop PAL0/1 and pointer PAL3 are preserved. Close restores palette/background. No framebuffer or heap allocation.  
**purpose:** Draw one canvas-palette colour swatch through the existing tile renderer.  
**returns:** MB_OK on success; MB_INVALID, MB_VERSION or MB_UNAVAILABLE on failure.  
**timing:** Foreground only. Pixel callback is bounded and read-only; called during video flush after VBlank, never interrupt or reentrant service context.  

## app.back

```c
void mb_app_back_intercept(uint16_t enabled);
```

**Parameters:**

- `enabled`: Nonzero intercepts B for this foreground app; zero restores default close behavior.

**arguments:** See parameter bounds; all extents are logical pixels or client tiles as specified.  
**capability:** Foreground app input only; Start+B emergency exit remains available.  
**edges:** Allows B to cancel menus and return to drawing. Does not intercept chrome close or the held universal exit chord.  
**errors:** Idle call is ignored.  
**example:** void mb_app_back_intercept(uint16_t enabled);  
**family:** app  
**negative:** Invalid source, palette words, zoom and bounds rejected before ownership changes.  
**ownership:** Opt-in flag resets at every app launch/close. Chrome close still sends guarded close requests.  
**purpose:** Opt into MB_BACK events for B instead of automatic close requests.  
**returns:** None.  
**timing:** Foreground only. Pixel callback is bounded and read-only; called during video flush after VBlank, never interrupt or reentrant service context.  

## canvas.brush

```c
MB_Result mb_canvas_brush(uint16_t size);
```

**Parameters:**

- `size`: Logical square brush width and height, from one through eight pixels.

**arguments:** Logical brush size 1..8 pixels; source canvas must be attached.  
**capability:** Source canvas API 1.5. PAL3 outline leaves the brush interior transparent; integer zoom follows logical size.  
**edges:** One-pixel brush is the default. Larger zoomed footprints use square corner outlines; menus restore the standard pointer. This changes no picture data or palette.  
**errors:** Size zero or above eight MB_INVALID; no attached source MB_UNAVAILABLE.  
**example:** mb_canvas_brush(4);  
**family:** canvas  
**negative:** Invalid sizes and absent canvas are rejected before changes.  
**ownership:** OS owns at most four pointer sprites and four cursor patterns; no application hardware ownership.  
**purpose:** Set the logical brush footprint displayed by the OS square canvas cursor.  
**returns:** MB_OK, MB_INVALID for unsupported size, or MB_UNAVAILABLE without a source canvas.  
**timing:** Foreground callbacks only. Shape uploads are queued to video flush; ordinary movement changes sprite positions only.  

## canvas.cursor

```c
MB_Result mb_canvas_cursor(uint16_t dot);
```

**Parameters:**

- `dot`: 0 for the legacy outline, 1 for a fixed three-screen-pixel dot, 2 for the centred brush footprint.

**arguments:** Zero selects the legacy outline; one selects a fixed dot; two selects a centred marker matching the brush footprint.  
**capability:** Source canvas API 1.6; additive 1.7 centred brush marker. PAL3 cursor independent of picture colours.  
**edges:** Style 2 follows brush size and integer zoom, centred like MegaPaint stamps; half-size overview rounds the footprint up to a screen pixel. A single-colour perimeter has no white centre; footprints above 16 screen pixels show corner markers within the same four-pattern budget. Style 1 stays fixed. Menus use the standard pointer unless a client custom cursor is installed. Closing resets style. Move the cursor aside to inspect covered pixels.  
**errors:** Values above two MB_INVALID; absent source canvas MB_UNAVAILABLE.  
**example:** mb_canvas_cursor(1);  
**family:** canvas  
**negative:** Unsupported style and absent canvas are rejected before changes.  
**ownership:** OS owns pointer sprites and cursor patterns; app picture memory remains untouched.  
**purpose:** Choose a brush-sized marker, compact dot or legacy outline for a source canvas.  
**returns:** MB_OK, MB_INVALID for an unsupported style, or MB_UNAVAILABLE without a source canvas.  
**timing:** Foreground callbacks only; shape updates are queued to video flush.  

## graphics.region

```c
MB_Result mb_graphics_map_region(const MB_PaletteLease *lease, uint16_t plane, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *cells);
```

**Parameters:**

- `lease`: Borrowed current exclusive graphics token.
- `plane`: Zero for foreground A, one for background B.
- `x`: First screen tile column, 0..39.
- `y`: First screen tile row, 0..27.
- `width`: Nonzero region width, at most 40 minus x.
- `height`: Nonzero region height, at most 28 minus y.
- `cells`: Borrowed width times height uint16 attributes, contiguous row-major; low 11 bits are relative to lease first_tile, upper bits retain priority/palette/flip attributes.

**arguments:** Valid exclusive lease; region within the 40x28 screen; contiguous row-major relative pattern attributes.  
**capability:** API 1.7 exclusive graphics lease; efficient incremental animation on either plane.  
**edges:** Uses the same relative tile attributes as graphics.map. Damage is region-local; large regions may take several flushes. No source stride; pass contiguous rows or separate regions.  
**errors:** Stale lease, null cells, invalid plane/bounds or any out-of-range pattern MB_INVALID; whole region validated before changes.  
**example:** mb_graphics_map_region(&lease, 1, 0, 11, 40, 2, &map[11 * 40]);  
**family:** graphics  
**negative:** Invalid final cell must leave every earlier cell unchanged; zero/overflow bounds and stale leases rejected.  
**ownership:** Cells copied synchronously into OS retained maps; source may be reused immediately. No heap or new retained workspace.  
**purpose:** Copy a bounded rectangular leased tile-map region without replacing the whole plane.  
**returns:** MB_OK or MB_INVALID.  
**timing:** Foreground callbacks only; normal bounded plane flush commits dirty cells after VBlank. Unchanged cells remain clean.  

## cursor.set

```c
MB_Result mb_cursor_set(const MB_Cursor *cursor, uint16_t scope);
```

**Parameters:**

- `cursor`: Borrowed synchronously; non-null descriptor with dimensions 1..16 screen pixels and hotspot within those dimensions. Each uint32 row stores sixteen left-aligned 2-bit pixels: 0 transparent, 1 dark, 2 light, 3 accent. Outside declared dimensions is ignored.
- `scope`: MB_CURSOR_CLIENT applies inside the client rectangle in pointer mode; MB_CURSOR_CANVAS follows the logical drawing point on an attached source canvas.

**arguments:** Active app with a window; bounded 2bpp bitmap and an in-bounds hotspot; client or source-canvas scope.  
**capability:** API 1.7. Four OS pointer sprites, reserved patterns and semantic PAL3 colours. No SGDK calls required from apps.  
**edges:** Hotspot stays at the input/drawing coordinate; bitmap remains screen-sized at every zoom. Canvas scope supersedes built-in canvas styles and disappears over menus. Client scope includes menus but excludes title-bar chrome. Leased fullscreen graphics retain their sprite/palette ownership and hide OS cursors. Reset cancels pending uploads.  
**errors:** Invalid pointer/dimensions/hotspot/scope MB_INVALID; no active app/window or required source canvas MB_UNAVAILABLE; exclusive graphics lease MB_BUSY. Failures preserve the current cursor.  
**example:** MB_Cursor c = {3, 3, 1, 1, {0x10000000, 0x54000000, 0x10000000}}; mb_cursor_set(&c, MB_CURSOR_CLIENT);  
**family:** input  
**negative:** Zero/oversize dimensions, hotspot outside bitmap, invalid scope, absent canvas and lease conflicts do not replace an existing cursor.  
**ownership:** Descriptor and all sixteen rows copied into one fixed OS workspace. Caller may immediately change or release its bitmap; no heap or hardware ownership. Reset automatically on app close/window reset/OS restart.  
**purpose:** Copy an app-defined cursor bitmap and hotspot for its client area or canvas.  
**returns:** MB_OK, MB_INVALID, MB_UNAVAILABLE or MB_BUSY.  
**timing:** Foreground callbacks only. Four patterns (128 VRAM bytes) upload during the next video flush; motion only changes sprite positions. Last set before a flush wins.  

## cursor.reset

```c
void mb_cursor_reset(void);
```

**arguments:** None.  
**capability:** API 1.7; safe without an active app and while graphics are leased.  
**edges:** Does not alter a picture, input mode, canvas brush/style, palette or an exclusive graphics lease. App exit and window reset call this automatically.  
**errors:** None; repeated calls are harmless.  
**example:** mb_cursor_reset();  
**family:** input  
**negative:** Reset before upload must not leave stale custom sprites on the desktop.  
**ownership:** Releases the app's cursor selection; OS retains its fixed workspace and patterns.  
**purpose:** Restore the normal OS or canvas cursor and cancel a pending custom upload.  
**returns:** None.  
**timing:** Foreground callbacks; visibility returns to normal on the next pointer update.  

## ui.menu

```c
MB_Result mb_ui_menu_button(uint16_t id, uint16_t x, uint16_t width, const char *label, uint16_t flags);
```

**Parameters:**

- `id`: Unique widget ID other than 65535. MB_UI_MENU_DISMISS is reserved and cannot be registered.
- `x`: Client-relative starting tile column.
- `width`: At least three tiles, wholly within the queried client.
- `label`: Borrowed ASCII label, copied and clipped to width minus two, at most eleven characters.
- `flags`: Zero for disabled, MB_UI_ENABLED, or MB_UI_ENABLED | MB_UI_MENU_OPEN for the currently open heading.

**arguments:** See parameters; menu row zero is inside the queried client.  
**capability:** Shared one-row app menus; no allocation or separate UI framework.  
**edges:** One-row hit area. Mark the active heading MB_UI_MENU_OPEN on each redraw. mb_ui_event returns MB_UI_MENU_DISMISS on selecting that heading again or clicking outside all registered widgets; another heading returns its ID normally. Handle dismissal by removing the overlay and consuming the click until A releases.  
**errors:** Invalid bounds, null label, duplicate ID or flags MB_INVALID; full 24-widget pool MB_BUSY.  
**example:** mb_ui_menu_button(1,0,10,"File",MB_UI_ENABLED);  
**family:** ui  
**negative:** Null label, duplicate ID, invalid flags, width or x. Open flag without enabled is invalid.  
**ownership:** OS copies label; occupies one existing widget slot, reset by mb_ui_reset/window clear.  
**purpose:** Add a one-tile-high menu item in client row zero using shared focus, hover and activation.  
**returns:** MB_OK, MB_INVALID or MB_BUSY.  
**timing:** Foreground callbacks only; retained tiles flush at VBlank.  

## window.content

```c
MB_Result mb_window_get_content_rect(uint16_t menu_rows, MB_Rect *out);
```

**Parameters:**

- `menu_rows`: Zero for no menu or one for a compact menu directly below the header.
- `out`: Caller-owned non-null rectangle; screen coordinates in 8x8 tiles.

**arguments:** See parameters; this query reserves no widgets and changes no window state.  
**capability:** Normal, maximized and fullscreen geometry. Maximized with a menu returns (0,2,40,26), or 320x208 pixels.  
**edges:** Subtract queried client origin when passing content coordinates to drawing/canvas APIs. Zero rows returns the entire client.  
**errors:** Null output or menu_rows greater than one MB_INVALID; no open window MB_UNAVAILABLE.  
**example:** mb_window_get_content_rect(1,&content);  
**family:** window  
**negative:** Null output, menu_rows two, no open window.  
**ownership:** Snapshot only; no allocation or retained pointer.  
**purpose:** Query drawable content in absolute screen tiles after reserving an optional single app menu row.  
**returns:** MB_OK, MB_INVALID or MB_UNAVAILABLE.  
**timing:** Foreground callbacks; query again on MB_RESIZE or MB_MODE_CHANGED.  

## canvas.ready

```c
MB_Result mb_canvas_ready(void);
```

**arguments:** No arguments; an attached canvas is required.  
**capability:** Native H40 attached canvas on PAL and NTSC.  
**edges:** A later edit or redraw can make a previously ready canvas busy again. Unused trailing dirty bits are ignored.  
**errors:** Missing attached canvas returns MB_UNAVAILABLE.  
**example:** if (mb_canvas_ready() == MB_OK) { /* start countdown */ }  
**family:** graphics  
**negative:** Query before attach or after close returns MB_UNAVAILABLE.  
**ownership:** Reads OS-owned bounded upload metadata; no allocation or mutation.  
**purpose:** Query whether attached tile canvas and retained window uploads are complete.  
**returns:** MB_OK when the current canvas, palette and retained maps are flushed; MB_BUSY while pending; MB_UNAVAILABLE without an attached canvas.  
**timing:** Foreground app callbacks only; observes the previous VBlank flush. Bounded scan of at most 140 dirty bytes.  
