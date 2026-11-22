/* R139 CDB decoder: no AmigaOS dependencies, tested on host.
 * Return 1 = valid READ/WRITE 6/10/12; 0 = other SCSI opcode;
 * -1 = malformed CDB, impossible (>32-bit byte) transfer length. */
#ifndef FRF_R139_SCSI_RW_H
#define FRF_R139_SCSI_RW_H
#include <stdint.h>
struct FrfScsiRW { uint64_t offset; uint32_t bytes; unsigned writing; };
static int frf_r139_scsi_rw(const uint8_t *cdb,unsigned cmdlen,struct FrfScsiRW *out) {
    if(!cdb || !out || cmdlen<1u)return -1;
    uint8_t op=cdb[0];
    uint32_t lba=0u,blocks=0u;
    unsigned is_write=0u;
    if(op==0x08u || op==0x0au){
        if(cmdlen<6u)return -1;
        lba=((uint32_t)(cdb[1]&0x1fu)<<16)|((uint32_t)cdb[2]<<8)|cdb[3];
        blocks=cdb[4]?cdb[4]:256u;
        is_write=op==0x0au;
    }else if(op==0x28u || op==0x2au){
        if(cmdlen<10u)return -1;
        lba=((uint32_t)cdb[2]<<24)|((uint32_t)cdb[3]<<16)|
            ((uint32_t)cdb[4]<<8)|cdb[5];
        blocks=((uint32_t)cdb[7]<<8)|cdb[8];
        is_write=op==0x2au;
    }else if(op==0xa8u || op==0xaau){
        if(cmdlen<12u)return -1;
        lba=((uint32_t)cdb[2]<<24)|((uint32_t)cdb[3]<<16)|
            ((uint32_t)cdb[4]<<8)|cdb[5];
        blocks=((uint32_t)cdb[6]<<24)|((uint32_t)cdb[7]<<16)|
            ((uint32_t)cdb[8]<<8)|cdb[9];
        is_write=op==0xaau;
    }else return 0;
    if(blocks>0x007fffffu)return -1;  /* bytes must fit ULONG */
    out->offset=(uint64_t)lba*512u;
    out->bytes=blocks*512u;
    out->writing=is_write;
    return 1;
}
#endif
