/* Future Retro Fusion R121 USB0 FAT95 file-write smoke test.
 * Creates only USB0:FRF121.TST; refuses to overwrite existing file.
 * Writes 4096 bytes, closes, reopens, compares, and deletes on SUCCESS.
 */
#include <proto/dos.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>

#define TEST_PATH "USB0:FRF121.TST"
#define SIZE 4096
static unsigned char pattern[SIZE];
static unsigned char verify[SIZE];

int main(void)
{
    BPTR f;
    puts("FRF R121 FAT32 USB0 safe file write/read verification");
    puts("This test creates only " TEST_PATH);
    f=Open(TEST_PATH,MODE_OLDFILE);
    if(f) {
        Close(f);
        puts("REFUSED: test filename already exists. Existing data unchanged.");
        return 10;
    }
    /* A failed Open could mean access denied, not absence. Never call
     * MODE_NEWFILE (which truncates an existing file) unless AmigaDOS
     * explicitly reports that the path does not exist. */
    LONG probe_error=IoErr();
    if(probe_error!=ERROR_OBJECT_NOT_FOUND) {
        printf("REFUSED: cannot prove test path absent, IoErr=%ld\n",
               (long)probe_error);
        return 10;
    }
    for(unsigned i=0;i<SIZE;i++)
        pattern[i]=(unsigned char)((i*17u+(i>>4)+0xA5u)&255u);
    f=Open(TEST_PATH,MODE_NEWFILE);
    if(!f) {
        printf("FAIL: create file, AmigaDOS IoErr=%ld\n",(long)IoErr());
        return 10;
    }
    LONG wrote=Write(f,pattern,SIZE);
    if(wrote!=SIZE) {
        printf("FAIL: wrote %ld/%u bytes; file preserved for inspection\n",
               (long)wrote,(unsigned)SIZE);
        Close(f);
        return 10;
    }
    if(!Flush(f)) {
        printf("FAIL: Flush, IoErr=%ld; file preserved\n",(long)IoErr());
        Close(f);
        return 10;
    }
    Close(f);
    f=Open(TEST_PATH,MODE_OLDFILE);
    if(!f) {
        printf("FAIL: readback Open, IoErr=%ld; file preserved\n",(long)IoErr());
        return 10;
    }
    LONG got=Read(f,verify,SIZE);
    Close(f);
    if(got!=SIZE || memcmp(pattern,verify,SIZE)) {
        printf("FAIL: readback size=%ld/4096 or content mismatch; file preserved\n",
               (long)got);
        return 10;
    }
    puts("PASS: 4096-byte FAT95 file write, flush, close and readback matched.");
    if(!DeleteFile(TEST_PATH)) {
        printf("PASS DATA but clean-up DeleteFile failed IoErr=%ld\n",(long)IoErr());
        return 5;
    }
    puts("PASS: test file deleted, no existing file overwritten.");
    return 0;
}
