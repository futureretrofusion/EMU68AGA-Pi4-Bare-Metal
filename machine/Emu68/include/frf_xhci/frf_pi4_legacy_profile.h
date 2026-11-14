/* SPDX-License-Identifier: MIT
 * Pi4B EEPROM/VL805 legacy compatibility classification by decoded board
 * type and revision, not a brittle list of complete manufacturing codes.
 * No physical hardware, boot EEPROM, or USB controller state is changed.
 */
#ifndef FRF_PI4_LEGACY_PROFILE_H
#define FRF_PI4_LEGACY_PROFILE_H
#include <stdint.h>
static inline uint32_t frf_pi4_legacy_vl805_eeprom(uint32_t revision)
{
    const uint32_t pi4b_type=(revision>>4u)&0xffu;
    const uint32_t pcb_revision=revision&15u;
    return ((revision&0x800000u)!=0u && pi4b_type==0x11u &&
            pcb_revision>=1u && pcb_revision<=2u)?1u:0u;
}
#endif
