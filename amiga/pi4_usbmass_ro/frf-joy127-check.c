/* FRF EMU68AGA R127 AmigaOS physical joystick test (READ ONLY).
 * CIA-A PRA7 is active-low primary fire (joystick port 2);
 * POTGOR bits14/12 are active-low secondary/third. */
#include <proto/dos.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    volatile const uint8_t *ciaa_pra=(volatile const uint8_t *)0xBFE001u;
    volatile const uint16_t *potgor=(volatile const uint16_t *)0xDFF016u;
    uint32_t last=0xffffffffu;
    puts("FRF R127 Pi4 USB JOY1 button check (15 seconds)");
    puts("Press face buttons 1,2,3,4 individually, then release.");
    puts("CIA fire should activate on ANY of the 4 face buttons.");
    puts("POTGOR14 extra with face2; POTGOR12 extra with face3.");
    puts("Read-only hardware sampling; no USB sector I/O or resets.");
    for(unsigned k=0;k<250u;k++) {
        uint8_t pra=*ciaa_pra;
        uint16_t pg=*potgor;
        uint32_t flags=((pra&0x80u)==0u?1u:0u) |
                       ((pg&0x4000u)==0u?2u:0u) |
                       ((pg&0x1000u)==0u?4u:0u);
        if(flags!=last) {
            printf("CIA=%02lx POTGOR=%04lx FIRE=%lu EXTRA2=%lu EXTRA3=%lu\n",
                (unsigned long)pra,(unsigned long)pg,
                (unsigned long)(flags&1u?1u:0u),
                (unsigned long)(flags&2u?1u:0u),
                (unsigned long)(flags&4u?1u:0u));
            last=flags;
        }
        Delay(3);
    }
    puts("End R127 joystick button test.");
    return 0;
}
