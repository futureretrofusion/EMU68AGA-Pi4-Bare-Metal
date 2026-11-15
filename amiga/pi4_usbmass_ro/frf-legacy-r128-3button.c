/* FRF Pi4 legacy R128: read-only AmigaOS joystick-port-2 diagnostic.
 * B1= CIA-A PRA7, B2=POTGOR14, B3=POTGOR12, active low.
 * Does NOT write POTGO, USB controllers, filesystem or EEPROM.
 * If the game or OS configured POTGO output mode, some POTGOR button
 * signals can be suppressed; compare with native diagnostic HUD. */
#include <stdint.h>
#include <proto/dos.h>
#include <stdio.h>
int main(void)
{
 volatile const uint8_t *cia=(volatile const uint8_t *)(uintptr_t)0xBFE001u;
 volatile const uint16_t *pot=(volatile const uint16_t *)(uintptr_t)0xDFF016u;
 uint32_t last=0xffffffffu;
 puts("FRF LEGACY PI4 R128 - THREE NATIVE AMIGA BUTTONS");
 puts("Press face B1, B2, B3 individually; B4 must have no native effect.");
 puts("B1=CIA-A PRA7; B2=POTGOR14; B3=POTGOR12 (active-low).");
 puts("Read-only 15-second test; POTGO output configuration is not changed.");
 for(unsigned t=0;t<250;t++) {
    uint8_t ca=*cia;
    uint16_t pg=*pot;
    uint32_t flags=((ca&0x80u)?0u:1u) |
                   ((pg&0x4000u)?0u:2u) |
                   ((pg&0x1000u)?0u:4u);
    if(flags!=last) {
      printf("CIA=%02lx POTGOR=%04lx  B1=%lu  B2=%lu  B3=%lu\n",
       (unsigned long)ca,(unsigned long)pg,
       (unsigned long)(!!(flags&1u)),
       (unsigned long)(!!(flags&2u)),
       (unsigned long)(!!(flags&4u)));
      last=flags;
    }
    Delay(3);
 }
 puts("END: if raw HID changes but B2/B3 do not, photograph R128 native HUD.");
 return 0;
}
