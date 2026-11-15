/* SPDX-License-Identifier: GPL-2.0-only
 * FRF legacy Pi4 R129 xHCI gamepad: Pi3-style idle interrupt behavior.
 * HID interrupt-IN pending / USB NAK is NOT evidence of a lost receiver.
 * No global or per-endpoint reset is justified by this observation alone.
 */
#ifndef FRF_R129_WIRELESS_POLICY_H
#define FRF_R129_WIRELESS_POLICY_H
#include <stdint.h>
static inline int frf_r129_gamepad_idle_pending(uint32_t protocol,
                                                uint64_t pending_td,
                                                uint64_t since,
                                                uint64_t now,
                                                uint64_t hz)
{
    return protocol==0u && pending_td!=0u && since!=0u &&
           hz!=0u && now>=since && (now-since)>=hz*10u;
}
#endif
