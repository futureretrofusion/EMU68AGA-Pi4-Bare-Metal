/* FRF Pi4 R118: Loaded resident vs per-stage OpenDevice diagnostics.
 * Read-only: never sends disk sector requests, no startup/mount changes.
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <exec/resident.h>
#include <stdio.h>
static void examine(const char *name)
{
    struct Resident *r=FindResident((char*)name);
    if(r)printf("RESIDENT %s found at %08lx flags=%02x version=%u type=%u\n",
                name,(unsigned long)r,r->rt_Flags,r->rt_Version,r->rt_Type);
    else printf("RESIDENT %s NOT_FOUND\n",name);
}
static void open_test(const char *name)
{
    struct MsgPort *port=CreateMsgPort();
    if(!port) {puts("CreateMsgPort FAILED");return;}
    struct IOStdReq *io=(struct IOStdReq*)CreateIORequest(port,sizeof(struct IOStdReq));
    if(!io) {DeleteMsgPort(port);puts("CreateIORequest FAILED");return;}
    LONG rc=OpenDevice((char*)name,0,(struct IORequest*)io,0);
    printf("OPEN %s -> rc=%ld io_Error=%d reqLen=%u\n",
           name,(long)rc,(int)io->io_Error,(unsigned)io->io_Message.mn_Length);
    if(!rc)CloseDevice((struct IORequest*)io);
    DeleteIORequest((struct IORequest*)io);
    DeleteMsgPort(port);
}
int main(void)
{
    puts("FRF Pi4 R118 Guest device driver loader / Open() stage test");
    puts("READ ONLY. Old pi4usbmass.device v0.2 remains unchanged.");
    BPTR seg=LoadSeg("DEVS:frfusbtest.device");
    if(seg) {
        puts("LOADSEG DEVS:frfusbtest.device PASS (can read/relocate Amiga HUNK)");
        UnLoadSeg(seg);
    } else {
        printf("LOADSEG DEVS:frfusbtest.device FAIL IoErr=%ld\n",
               (long)IoErr());
    }
    examine("frfusbtest.device");
    open_test("frfusbtest.device");
    examine("frfusbtest.device");
    /* A separate test of explicit DEVS: lookup excludes assign issues. */
    open_test("DEVS:frfusbtest.device");
    puts("Expected: rc=0 -> good. rc=-20..-24 -> our diagnostic Open() reached.");
    puts("rc=-1 with no resident -> OS LoadSeg/InitResident/DEV: search failure.");
    puts("rc=-1 with resident -> check load and device vector table.");
    puts("END R118: no sector or FAT95 I/O.");
    return 0;
}
