/* R138: unit0 RDB priority and identity preservation, host-mock only.
 * No media access and no card/device modifications. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "frf_r138_unit_map.h"
static void set(struct frf_um_extension *e,unsigned i,unsigned present,unsigned cap,unsigned flags) {
 e->units[i].present=present;
 e->units[i].capacity=cap;
 e->units[i].flags=flags;
}
int main(void) {
 struct frf_um_extension e;
 memset(&e,0,sizeof(e));
 e.magic=FRF_UM_EXT_MAGIC;e.version=FRF_UM_EXT_VERSION;e.count=3u;
 set(&e,0,1u,8192u,FRF_UM_UNIT_WRITE); /* FAT device discovered first */
 set(&e,1,1u,16384u,FRF_UM_UNIT_RDB);  /* Amiga RDB second */
 set(&e,2,1u,4096u,0u);              /* another storage device */
 assert(frf_r138_physical_unit(&e,0u)==1u);
 assert(frf_r138_physical_unit(&e,1u)==0u);
 assert(frf_r138_physical_unit(&e,2u)==2u);
 /* RDB already enumerated first: identity mapping preserved. */
 set(&e,0,1u,8192u,FRF_UM_UNIT_RDB);
 set(&e,1,1u,16384u,0u);
 assert(frf_r138_physical_unit(&e,0u)==0u);
 assert(frf_r138_physical_unit(&e,1u)==1u);
 assert(frf_r138_physical_unit(&e,2u)==2u);
 /* Last physical device is RDB, including a card reader empty LUN. */
 set(&e,0,0u,0u,0u);
 set(&e,1,1u,16384u,0u);
 set(&e,2,1u,4096u,FRF_UM_UNIT_RDB);
 assert(frf_r138_physical_unit(&e,0u)==2u);
 assert(frf_r138_physical_unit(&e,1u)==0u);
 assert(frf_r138_physical_unit(&e,2u)==1u);
 /* Absent or unprobed RDB must not be selected. */
 set(&e,2,0u,0u,FRF_UM_UNIT_RDB);
 assert(frf_r138_physical_unit(&e,0u)==0u);
 assert(frf_r138_physical_unit(&e,1u)==1u);
 /* Legacy Pi3 and R132 support: keep unit number unchanged. */
 e.magic=0u;
 assert(frf_r138_physical_unit(&e,0u)==0u);
 assert(frf_r138_physical_unit(&e,2u)==2u);
 e.magic=FRF_UM_EXT_MAGIC;
 e.version=999u;
 assert(frf_r138_physical_unit(&e,1u)==1u);
 /* Unit limit and direct-attached one-disk case. */
 e.version=FRF_UM_EXT_VERSION;e.count=1u;
 set(&e,0,1u,60000u,FRF_UM_UNIT_RDB);
 assert(frf_r138_physical_unit(&e,0u)==0u);
 assert(frf_r138_physical_unit(&e,1u)==1u);
 puts("PASS R138: RDB-first guest unit0 map, FAT remap, unready LUN safety, legacy fallback");
 return 0;
}
