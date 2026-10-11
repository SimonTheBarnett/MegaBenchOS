# MegaPaint picture formats

All multibyte fields are unsigned big endian; structs are never written directly. Current saving emits MBP2. All palette words use Mega Drive RGB333: red bits 1..3, green 5..7, blue 9..11; other bits must be zero.

| MBP2 bytes | Meaning |
|---|---|
| 0..3 | ASCII MBP2 |
| 4..5 / 6..7 | Width / height; nonzero multiples of eight, up to 512 / 256 |
| 8 / 9 | Depth 4 / compression codec 1 |
| 10..11 | Exact compressed payload byte length |
| 12..13 | CRC-16/CCITT-FALSE (polynomial 0x1021, initial 0xffff) |
| 14 / 15 | Flags / reserved; both zero |
| 16..47 | Sixteen RGB333 palette words, indices 0..15 |
| 48..end | Byte RLE pairs: nonzero count 1..255, followed by repeated byte value |

Expanded pixels are row-major packed 4bpp, high nibble first (left pixel), exactly width*height/2 bytes. Runs may cross rows. CRC visits **expanded pixel bytes first**, then header bytes **4..11**, then **14..47**, in ascending order within each range. Magic and CRC bytes are excluded. Thus dimensions, codec, payload length, flags and palette are protected as well as pixels.

The file must have exact length 48+payload length; payload length must be even, runs must be nonzero and expansion must match exactly. Illegal palette words, unknown versions/codecs, unsupported dimensions, extra/truncated data and bad CRC are rejected. Current VFS scratch limit is 3592 bytes. The decoder checks sparse tile demand before replacing the current image; exceeding 384 mixed tiles reports a detail-memory limit. Save preflights serialized size and free capacity (crediting the previous file for atomic replacement). Failure leaves old files and current edits intact.

MBP1 remains read-compatible at exactly 1040 bytes: magic MBP1; width/height 64; depth 2 and palette preset 0; payload length 1024; flags/reserved zero. Bytes 16..1039 contain row-major 2bpp pixels (16 bytes per row, leftmost pixel bits 7..6). Its CRC covers only the packed payload, using the same polynomial/initial value. Indices 0..3 import unchanged, with Blue preset palette 0xccc, 0x222, 0xaa6, 0xeee. Saving an imported picture emits MBP2. Legacy shared-theme colours were not recorded in MBP1, so the original theme of an arbitrary file cannot be recovered.
