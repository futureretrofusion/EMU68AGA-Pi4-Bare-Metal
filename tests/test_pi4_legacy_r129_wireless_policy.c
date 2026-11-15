#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "frf_r129_wireless_policy.h"
#define ASSERT(ok) do{if(!(ok)){fprintf(stderr,"FAIL line %d\n",__LINE__);exit(2);}}while(0)
int main(void) {
 const uint64_t hz=54000000u, start=5u*54000000u;
 ASSERT(!frf_r129_gamepad_idle_pending(0u,0u,start,start+hz*60u,hz));
 ASSERT(!frf_r129_gamepad_idle_pending(0u,0x1000u,start,start+hz*9u,hz));
 ASSERT(frf_r129_gamepad_idle_pending(0u,0x1000u,start,start+hz*10u,hz));
 ASSERT(frf_r129_gamepad_idle_pending(0u,0x1000u,start,start+hz*120u,hz));
 ASSERT(!frf_r129_gamepad_idle_pending(1u,0x1000u,start,start+hz*15u,hz));
 ASSERT(!frf_r129_gamepad_idle_pending(2u,0x1000u,start,start+hz*15u,hz));
 ASSERT(!frf_r129_gamepad_idle_pending(0u,0x1000u,start,start,0u));
 ASSERT(!frf_r129_gamepad_idle_pending(0u,0x1000u,start,start-1u,hz));
 puts("PASS R129: outstanding gamepad interrupt-IN/NAKs are normal even beyond 120s; keyboard/mouse never affected");
 return 0;
}
