/*
 * FRF Pi4 native USB mouse diagnostic for AmigaOS (read-only).
 * Tests physical CIA-A PRA (LMB) and Paula POTGOR (RMB/MMB).
 * No timing-sensitive writes and no SET_PROTOCOL, no device resets.
 */
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <stdio.h>
static volatile UBYTE * const ciaa_pra=(volatile UBYTE*)0xBFE001ul;
static volatile UWORD * const potgor=(volatile UWORD*)0xDFF016ul;
int main(void)
{
    UBYTE old_l=2u,old_r=2u,old_m=2u;
    UWORD old_p=0xffffu;
    unsigned n=0u,lc=0u,rc=0u,mc=0u;
    puts("FRF Pi4 mouse button test R1: CIA-A and POTGOR.");
    puts("Click LEFT and RIGHT separately while this runs (~20 seconds).");
    puts("Raw LMB low = CIAA PRA bit 6; RMB low = POTGOR bit 10.");
    puts("This program only reads hardware registers; Ctrl-C to exit.");
    for(n=0u;n<200u;n++) {
        UBYTE pra=*ciaa_pra;
        UWORD pg=*potgor;
        UBYTE l=(pra&0x40u)?0u:1u;
        UBYTE r=(pg&0x0400u)?0u:1u;
        UBYTE m=(pg&0x0100u)?0u:1u;
        if(l!=old_l || r!=old_r || m!=old_m || ((old_p^pg)&0x0500u)) {
            printf("sample=%03u CIA=%02lx POTGOR=%04lx L=%lu R=%lu M=%lu\n",
                   n,(unsigned long)pra,(unsigned long)pg,
                   (unsigned long)l,(unsigned long)r,(unsigned long)m);
            if(l && old_l==0u) ++lc;
            if(r && old_r==0u) ++rc;
            if(m && old_m==0u) ++mc;
            old_l=l;old_r=r;old_m=m;old_p=pg;
        }
        if(SetSignal(0,0)&SIGBREAKF_CTRL_C)break;
        Delay(5u); /* AmigaOS ticks: typically 50 Hz -> 100ms */
    }
    printf("DONE: Left=%u Right=%u Middle=%u press transitions\n",lc,rc,mc);
    puts("If left rises but right stays 0, investigate USB HID versus POTGOR.");
    return 0;
}
