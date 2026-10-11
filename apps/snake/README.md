# Snake

[MegaBenchOS](../../README.md) / [Apps](../README.md) / Snake

Chase food, grow your snake and beat your session best. The game fills the window with a green board, red food and amber obstacles.

A centred three-second countdown gives you time to get ready, with an ascending beep, beep, beeeep. Every four foods raises the level: the snake gets faster and another rock joins the board, up to four rocks. Walls, your own body and rocks end the round.

## Controls

- **D-pad:** steer. Two quick turns are remembered and applied on successive moves.
- **Start or C:** pause or resume.
- **A:** retry after game over, or exit while paused.
- **B:** exit a normal or maximised window; hold Start+B to exit fullscreen.
- **C+Start:** cycle supported window modes.

The snake starts at five moves per second and reaches about 10.7 at level five. Immediate reversals are ignored. Score and best score last for the current session.

## Development

[snake.c](snake.c) owns drawing, countdown and sound cues; [model.c](model.c) owns movement, growth, turn buffering and obstacles. The default board is 20x13 cells, each 16 pixels square. Other layouts use 8- or 16-pixel cells within a 400-cell bound; changing board dimensions starts a new round.

The two-turn buffer keeps accepted inputs in order, ignores duplicates and rejects reversals relative to the last accepted turn. A full buffer preserves its existing turns. Pause freezes it; retry clears it. Shared game input reports newly pressed directions even when another direction is held.

Rendering uses dirty-cell updates. Game over clears the body; retry waits for initial board uploads before starting its countdown.

See the [SDK](../../docs/API.md) and [testing](../../docs/testing.md) for shared input rules and regression checks.
