/* FRF R121 safe read-only driver capability check (no disk writes). */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <devices/trackdisk.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    puts("FRF Pi4 R121 usbmass.device v0.5 — native write capability");
    struct MsgPort *port=CreateMsgPort();
    if(!port){puts("ERROR: CreateMsgPort");return 20;}
    struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOExtTD));
    if(!io){puts("ERROR: CreateIORequest");DeleteMsgPort(port);return 20;}
    LONG rc=OpenDevice("usbmass.device",0,(struct IORequest*)io,0);
    printf("OpenDevice usbmass.device unit0 rc=%ld\n",(long)rc);
    if(rc){
        puts("FAIL: name, loader or Zorro FRUM. No writes performed.");
        DeleteIORequest((struct IORequest*)io);
        DeleteMsgPort(port);
        return 10;
    }
    struct DriveGeometry geo;
    memset(&geo,0,sizeof(geo));
    io->io_Command=TD_GETGEOMETRY;
    io->io_Data=&geo;
    io->io_Length=sizeof(geo);
    io->io_Offset=0;
    DoIO((struct IORequest*)io);
    printf("Geometry rc=%d sectors=%lu sectorBytes=%lu\n",
           (int)io->io_Error,(unsigned long)geo.dg_TotalSectors,
           (unsigned long)geo.dg_SectorSize);
    int failed=(io->io_Error!=0 || geo.dg_SectorSize!=512u ||
                geo.dg_TotalSectors==0u);
    io->io_Command=TD_PROTSTATUS;
    io->io_Data=NULL;io->io_Length=0u;io->io_Actual=0xffffffffu;
    DoIO((struct IORequest*)io);
    printf("TD_PROTSTATUS rc=%d protected=%lu\n",
           (int)io->io_Error,(unsigned long)io->io_Actual);
    if(io->io_Error || io->io_Actual!=0u){
        puts("WRITE NOT ENABLED — check matching R121 kernel PI4W marker.");
        failed=1;
    }else puts("RW ENABLED: the R121 host advertises WRITE10 to AmigaOS.");
    CloseDevice((struct IORequest*)io);
    DeleteIORequest((struct IORequest*)io);
    DeleteMsgPort(port);
    puts(failed?"R121 RW CHECK FAILED (no sectors written)":
                "R121 RW CHECK PASSED (no sectors written)");
    return failed?10:0;
}
