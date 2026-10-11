# MegaBenchOS music credits

Five original Mega Drive compositions from [Safety Stoat Studios](https://safetystoatstudios.itch.io/free-vgms), released under CC0-1.0. The creator page identifies the pack as CC0 and links the primary [free_vgms repository](https://github.com/jerellsworth/free_vgms). Sources are pinned to commit `2e376d0abfa5709f6c0e648095672cd6c1654f93`, not a mutable branch.

| Title | Creator | Duration | Source filename | Converted filename | XGC bytes |
|---|---|---:|---|---|---:|
| Battle 7 | Safety Stoat Studios | 89.6 s | [battle_7.vgm](https://github.com/jerellsworth/free_vgms/blob/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/battle_7.vgm) | battle_7.xgc | 2364 |
| Mad Bossa | Spring; VGM transcription by Zachary / Safety Stoat Studios | 115.2 s | [mad_bossa.vgm](https://github.com/jerellsworth/free_vgms/blob/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/mad_bossa.vgm) | mad_bossa.xgc | 3916 |
| I Wondered What I Could Do With It | Safety Stoat Studios | 105.6 s | [i_wondered_what_i_could_do_with_it.vgm](https://github.com/jerellsworth/free_vgms/blob/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/i_wondered_what_i_could_do_with_it.vgm) | i_wondered_what_i_could_do_with_it.xgc | 40508 |
| Questions | Safety Stoat Studios | 96 s | [questions.vgm](https://github.com/jerellsworth/free_vgms/blob/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/questions.vgm) | questions.xgc | 12348 |
| Sad Polka | Safety Stoat Studios | 76.8 s | [sad_polka.vgm](https://github.com/jerellsworth/free_vgms/blob/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/sad_polka.vgm) | sad_polka.xgc | 109884 |

Mad Bossa's GD3 tag names **Spring**. The preserved [mad_bossa.txt](../third_party/music/safetystoat/mad_bossa.txt) explicitly credits Spring's CC0 composition and VGM transcription by **Zachary at Safety Stoat Studios**. The other four GD3 author fields are empty: credit the published collection author, Safety Stoat Studios, without inventing individual composer names. Display titles follow the published filenames; no WAV or DMF files are needed in the ROM.

Original [CC0 legal text](../third_party/music/safetystoat/LICENSE), [upstream README](../third_party/music/safetystoat/README.md), attribution note and VGMs remain separate from [generated XGC assets](../assets/music/generated/manifest.json). [Source provenance](../third_party/music/safetystoat/provenance.json) records original URLs, exact filenames, duration/loop fields, revision and hashes. The conversion manifest preserves authorship, CC0 evidence, source/output hashes and converter identity. No commercial Sega soundtrack rips are included.

All five sources use the Mega Drive YM2612/SN76489 command subset supported by SGDK. I Wondered What I Could Do With It, Questions and Sad Polka include YM2612 DAC streams and PCM blocks. The wrapper validates those existing SGDK inputs; SGDK performs all conversion.

Although the collection is described as loop-oriented, **these five pinned VGMs have zero loop offsets and zero loop samples**. They therefore play once, stop at their real endings and restart with Play. No loop is fabricated and no source bytes are modified. This preserves every valid loop present in the selected files (none). Older collection files can have missing programmed loops, as the creator explains on the download page.

See the [conversion workflow](../docs/MUSIC_CONVERSION.md) for asset generation and bundling.

## Listening comparisons

Use the creator's [listening playlist](https://www.youtube.com/playlist?list=PLQWBbpRkgtR2vixfM4J_pDUie9k-dbxVz) or these original WAV renders. The VGM source links in the table above point to the exact files converted for MegaPlayer. The WAVs are reference renders, not ROM assets; renderer/filter differences may affect timbre. Download a VGM and load it in your chosen web VGM player for a direct source comparison.

- [battle_7.wav](https://raw.githubusercontent.com/jerellsworth/free_vgms/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/battle_7.wav)
- [mad_bossa.wav](https://raw.githubusercontent.com/jerellsworth/free_vgms/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/mad_bossa.wav)
- [i_wondered_what_i_could_do_with_it.wav](https://raw.githubusercontent.com/jerellsworth/free_vgms/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/i_wondered_what_i_could_do_with_it.wav)
- [questions.wav](https://raw.githubusercontent.com/jerellsworth/free_vgms/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/questions.wav)
- [sad_polka.wav](https://raw.githubusercontent.com/jerellsworth/free_vgms/2e376d0abfa5709f6c0e648095672cd6c1654f93/MD_tracks/sad_polka.wav)
