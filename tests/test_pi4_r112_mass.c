/* Host-run R111 BOT/FRUM protocol test. Mock only the xHCI transport:
 * the *actual* ARM worker source compiles and executes unchanged. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frf_xhci_host.h"
#include "frf_usb_mass.h"

volatile struct frf_um_shared *frf_um_host;
volatile uint32_t frf_r76_usb_mass_count;
static struct frf_um_shared shared;
static uint8_t cbw_capture[31];
static uint32_t fake_slot=5u;
static uint32_t last_tag=0u;
static uint32_t last_opcode=0u;
static uint32_t last_lba=0u;
static uint32_t last_count=0u;
static uint32_t cbw_calls=0u;
static uint32_t data_calls=0u;
static uint32_t write_commands=0u;
static int fake_bad_csw=0;
static uint32_t fake_busy_capacity_reads=0u;
static uint32_t capacity_requests=0u;
static int fake_bot_failure=0;

static uint32_t be32(const uint8_t *b)
{
    return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];
}
static uint32_t le32(const uint8_t *b)
{
    return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
}
static void le32_set(uint8_t *b,uint32_t x)
{
    b[0]=x;b[1]=x>>8;b[2]=x>>16;b[3]=x>>24;
}
static void require(int pass,const char *msg)
{
    if(!pass){fprintf(stderr,"FAIL %s\n",msg);exit(2);}
}
uint32_t frf_xhci_host_msc_slot(void){return fake_slot;}
int frf_xhci_host_bulk_out(const void *data,uint32_t length,uint32_t *got)
{
    require(length==31u,"CBW length must be 31");
    memcpy(cbw_capture,data,31u);
    require(memcmp(cbw_capture,"USBC",4u)==0,"CBW little endian signature");
    require(cbw_capture[12]==0x80u,"BOT data direction IN");
    require(cbw_capture[13]==0u && cbw_capture[14]==10u,"LUN0 SCSI10");
    last_opcode=cbw_capture[15];
    if(last_opcode==0x25u) ++capacity_requests;
    last_tag=le32(cbw_capture+4u);
    last_lba=be32(cbw_capture+17u);
    last_count=((uint32_t)cbw_capture[22]<<8)|cbw_capture[23];
    if(last_opcode==0x2au)++write_commands;
    ++cbw_calls;
    if(got)*got=31u;
    return !fake_bot_failure;
}
int frf_xhci_host_bulk_in(void *data,uint32_t length,uint32_t *got)
{
    uint8_t *b=data;
    if(length==13u) {
        memcpy(b,"USBS",4u);
        le32_set(b+4u,last_tag+(uint32_t)fake_bad_csw);
        le32_set(b+8u,0);
        b[12]=(last_opcode==0x25u &&
              capacity_requests<=fake_busy_capacity_reads)?1u:0u;
    } else if(length==8u && last_opcode==0x25u) {
        /* READ CAPACITY(10): 8192 sectors of exactly 512 bytes */
        b[0]=0;b[1]=0;b[2]=0x1f;b[3]=0xff;
        b[4]=0;b[5]=0;b[6]=2;b[7]=0;
    } else {
        require(last_opcode==0x28u && length==last_count*512u,
                "READ10 data size, not unexpected bulk transfer");
        for(uint32_t i=0u;i<length;i++)b[i]=(uint8_t)(i+last_lba);
        ++data_calls;
    }
    if(got)*got=length;
    return !fake_bot_failure;
}
extern void frf_pi4_um_init(void);
extern void frf_pi4_um_service(void);
extern uint32_t frf_pi4_um_capacity(void);
extern uint32_t frf_pi4_um_attempts(void);
extern uint32_t frf_pi4_um_last_bot_error(void);
extern void frf_pi4_um_retry(uint64_t now,uint64_t hz);

static void request(uint32_t cmd,uint32_t lba,uint32_t sectors,uint32_t seq)
{
    shared.command=cmd;shared.lba=lba;shared.sectors=sectors;shared.sequence=seq;
    shared.state=FRF_UM_REQ;
    frf_pi4_um_service();
    require(shared.completed==seq,"sequence echoed");
}

int main(void)
{
    memset(&shared,0,sizeof(shared));
    shared.magic=FRF_UM_MAGIC;shared.version=FRF_UM_VERSION;
    shared.bytes=sizeof(shared);shared.sector_size=512;
    frf_um_host=&shared;

    frf_pi4_um_init();
    require(frf_pi4_um_capacity()==8192u,"capacity parsed from big-endian SCSI");
    require(cbw_calls==1u,"one capacity command");
    require(last_opcode==0x25u && le32(cbw_capture+8u)==8u,
            "read capacity protocol");
    frf_pi4_um_service();
    require(shared.present==1u && shared.capacity==8192u &&
            shared.media_change==1u,"Z2 mailbox ready");
    require(shared.reserved[0]==5u && shared.reserved[1]==0x50493452u,
            "Pi4 read-only diagnostic");
    request(FRF_UM_READ,4u,2u,101u);
    require(shared.state==FRF_UM_DONE && shared.result==1024u,
            "READ10 successful 1024 bytes");
    require(last_opcode==0x28u && last_lba==4u && last_count==2u,
            "READ10 LBA and count big endian");
    require(shared.data[0]==4u && shared.data[1]==5u &&
            shared.data[512]==4u,"sector read payload");
    shared.state=FRF_UM_IDLE;
    request(FRF_UM_READ,8191u,1u,102u);
    require(shared.state==FRF_UM_DONE,"read final sector");
    shared.state=FRF_UM_IDLE;
    uint32_t before=cbw_calls;
    request(FRF_UM_READ,8191u,2u,103u);
    require(shared.state==FRF_UM_ERROR && cbw_calls==before,
            "reject beyond last LBA without USB I/O");
    shared.state=FRF_UM_IDLE;
    request(FRF_UM_READ,0u,17u,104u);
    require(shared.state==FRF_UM_ERROR && cbw_calls==before,
            "reject more than 16 sectors");
    shared.state=FRF_UM_IDLE;
    request(FRF_UM_WRITE,1u,1u,105u);
    require(shared.state==FRF_UM_ERROR && cbw_calls==before &&
            write_commands==0u,"write not sent to USB drive");
    shared.state=FRF_UM_IDLE;
    fake_bad_csw=1;
    request(FRF_UM_READ,10u,1u,106u);
    require(shared.state==FRF_UM_ERROR,"reject mismatched CSW tag");
    fake_bad_csw=0;
    /* USB HDD spin-up: first two valid BOT transactions return CHECK
     * CONDITION; R112 must schedule retries, and publish a late media change
     * without USB controller reset or falsely advertising ready at boot. */
    memset(&shared,0,sizeof(shared));
    shared.magic=FRF_UM_MAGIC; shared.version=FRF_UM_VERSION;
    shared.bytes=sizeof(shared); shared.sector_size=512u;
    fake_busy_capacity_reads=2u;
    capacity_requests=0u;
    fake_slot=7u;
    frf_pi4_um_init();
    require(frf_pi4_um_capacity()==0u && frf_pi4_um_attempts()==1u,
            "initial HDD spinning up");
    require(frf_pi4_um_last_bot_error()==6u,"SCSI CHECK CONDITION logged");
    frf_pi4_um_service();
    require(shared.present==0u && shared.capacity==0u &&
            shared.media_change==1u,"not ready must not expose media");
    frf_pi4_um_retry(1000u,1000u);
    frf_pi4_um_retry(2999u,1000u);
    require(frf_pi4_um_attempts()==1u,"retry cadence first 2 seconds");
    frf_pi4_um_retry(3000u,1000u);
    require(frf_pi4_um_capacity()==0u && frf_pi4_um_attempts()==2u,
            "second failed spinup check");
    frf_pi4_um_retry(5000u,1000u);
    require(frf_pi4_um_capacity()==8192u && frf_pi4_um_attempts()==3u,
            "later HDD ready without any USB reset");
    frf_pi4_um_service();
    require(shared.present==1u && shared.capacity==8192u &&
            shared.media_change==2u,"late disk attached, notify AmigaOS");
    uint32_t probes=capacity_requests;
    frf_pi4_um_retry(9000u,1000u);
    require(capacity_requests==probes,"ready means no further CAPACITY");
    request(FRF_UM_READ,0u,1u,107u);
    require(shared.state==FRF_UM_DONE && shared.result==512u,
            "late-ready HDD READ10");
    fake_busy_capacity_reads=0u;
    fake_slot=0u;
    frf_pi4_um_init();
    require(frf_pi4_um_capacity()==0u,"no mass slot means absent media");
    require(data_calls>=2u,"USB data stage tested");
    puts("PASS R112: BOT/READ10, RO writes rejected, 2sec HDD spin-up retries, late FRUM media change, absent slot");
    return 0;
}
