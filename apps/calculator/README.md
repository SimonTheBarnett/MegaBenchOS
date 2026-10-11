# Calculator

[MegaBenchOS](../../README.md) / [Apps](../README.md) / Calculator

Quick integer maths from the desktop. Add, subtract, multiply and divide, chain calculations, or flip a value's sign.

## Controls

Use D-pad and A to choose keys, or C to switch to pointer controls. **Clr** starts fresh, **Del** removes a digit and **Neg** changes the sign. B closes the window; fullscreen also has Exit and the held Start+B shortcut.

Calculations use signed 32-bit integers. Division drops the fractional part, and overflow or division by zero shows an error.

## Development

The UI is in [calculator.c](calculator.c); arithmetic and entry rules live in [model.c](model.c). Host tests execute the same model used by the ROM. See [building and packaging](../../docs/BUILD.md), the [SDK](../../sdk/README.md) and [testing](../../docs/testing.md).
