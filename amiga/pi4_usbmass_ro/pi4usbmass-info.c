/* FRF EMU68AGA Pi4 R111 RO USB mass diagnostic, AmigaOS 68k.
 * Reads USB0 sector 0 without assuming MBR. Native Amiga RDB is "RDSK".
 * Never writes or formats media. */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <stdio.h>
#include <string.h>
static unsigned char sector[512] __attribute__((aligned(16)));
int main(void)
{
    struct MsgPort *port=CreateMsgPort();
    if(!port) {puts("USBMASS: cannot create message port");return 20;}
    struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOExtTD));
    if(!io){DeleteMsgPort(port);return 20;}
    LONG e=OpenDevice("pi4usbmass.device",0,(struct IORequest*)io,0);
    if(e){
        printf("USBMASS: OpenDevice pi4usbmass.device unit0 FAILED (%ld)\n",(long)e);
        puts("Check DEVS:pi4usbmass.device, R115 POST, and USB stick at power-on.");
        DeleteIORequest((struct IORequest*)io);DeleteMsgPort(port);
        return 10;
    }
    struct DriveGeometry g;
    memset(&g,0,sizeof g);
    io->io_Command=TD_GETGEOMETRY;
    io->io_Data=&g;
    io->io_Length=sizeof g;io->io_Offset=0;
    DoIO((struct IORequest*)io);
    printf("USB0 geometry status=%d sectors=%lu sectorBytes=%lu\n",
           (int)io->io_Error,(unsigned long)g.dg_TotalSectors,
           (unsigned long)g.dg_SectorSize);
    int failed=(io->io_Error || g.dg_SectorSize!=512 || !g.dg_TotalSectors);
    memset(sector,0,sizeof sector);
    io->io_Command=CMD_READ;io->io_Data=sector;
    io->io_Length=512;io->io_Offset=0;
    DoIO((struct IORequest*)io);
    printf("USB0 READ LBA0 status=%d actual=%lu (read-only)\n",
           (int)io->io_Error,(unsigned long)io->io_Actual);
    if(io->io_Error || io->io_Actual!=512) failed=1;
    else {
        printf("LBA0 first16:");
        for(unsigned i=0;i<16;i++) printf(" %02x",(unsigned)sector[i]);
        puts("");
        if(!memcmp(sector,"RDSK",4)) puts("Format: Amiga RDB (RDSK)");
        else if(sector[510]==0x55 && sector[511]==0xaa)
             puts("Format: MBR-compatible 55 AA boot signature");
        else puts("Format: not RDB or MBR, but raw USB sector read succeeded");
    }
    CloseDevice((struct IORequest*)io);
    DeleteIORequest((struct IORequest*)io);
    DeleteMsgPort(port);
    puts(failed?"USBMASS READ TEST FAIL":"USBMASS READ TEST PASS");
    return failed?5:0;
}
