/* R138 - AmigaOS legacy RDB-first guest unit mapping for multi-BOT bridge.
 * Pure function: maps a requested AmigaOS logical unit to R133 physical
 * USB disk index without changing the ARM CPU1 BOT transport order.
 * Only a PRESENT and scanned RDB disk qualifies. No writes are enabled. */
#ifndef FRF_R138_UNIT_MAP_H
#define FRF_R138_UNIT_MAP_H
#include "frf_usb_mass.h"
static unsigned frf_r138_physical_unit(const volatile struct frf_um_extension *e,
                                      unsigned guest_unit)
{
    if (!e || e->magic!=FRF_UM_EXT_MAGIC ||
        e->version!=FRF_UM_EXT_VERSION || e->count>FRF_UM_MAX_UNITS ||
        guest_unit>=e->count)
        return guest_unit; /* Legacy single-unit transport or no extension */
    unsigned rdb=FRF_UM_MAX_UNITS;
    for(unsigned i=0u;i<e->count;i++)
        if(e->units[i].present && e->units[i].capacity &&
           (e->units[i].flags&FRF_UM_UNIT_RDB)){
            rdb=i; break;
        }
    if(rdb>=FRF_UM_MAX_UNITS)
        return guest_unit; /* No RDB; preserve FAT unit0 backwards compat */
    if(guest_unit==0u)return rdb;
    unsigned mapped=1u;
    for(unsigned i=0u;i<e->count;i++){
        if(i==rdb)continue;
        if(mapped++==guest_unit)return i;
    }
    return FRF_UM_MAX_UNITS; /* Never route to an arbitrary disk */
}
#endif
