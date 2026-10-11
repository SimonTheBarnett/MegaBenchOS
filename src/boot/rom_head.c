/**
 * @file src/boot/rom_head.c
 * @brief SGDK-compatible console ROM header: branding, region and read-only resource metadata.
 *
 * Console startup code linked by SGDK; owns boot and main-loop integration.
 * Linked by SGDK boot assembly; host publication fills additive checksum and ROM end.
 * No SRAM descriptor is advertised because persistence is not implemented.
 */
#include <genesis.h>
__attribute__((externally_visible))
const ROMHeader rom_header = {"SEGA MEGA DRIVE ",
                              "(C)MBOS 2026    ",
                              "MEGABENCHOS                                     ",
                              "MEGABENCHOS                                     ",
                              "GM 00000000-00",
                              0,
                              "J               ",
                              0,
                              0x0001FFFF,
                              0xE0FF0000,
                              0xE0FFFFFF,
                              "  ",
                              0,
                              0,
                              0,
                              "            ",
                              "MEGABENCHOS SDK BASELINE                ",
                              "JUE             "};
