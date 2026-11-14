/* Regression for Raspberry Pi new-style revision decode.
 * Never use RAM size alone to classify VL805 EEPROM revisions.
 */
#include <assert.h>
#include <stdio.h>
#include "frf_pi4_legacy_profile.h"
int main(void)
{
    assert(frf_pi4_legacy_vl805_eeprom(0xc03111u)==1u); /* 4GB 4B 1.1 */
    assert(frf_pi4_legacy_vl805_eeprom(0xa03111u)==1u); /* 1GB 4B 1.1 */
    assert(frf_pi4_legacy_vl805_eeprom(0xc03112u)==1u); /* 4GB 4B 1.2 */
    assert(frf_pi4_legacy_vl805_eeprom(0xa03115u)==0u); /* 1GB 4B 1.5 */
    assert(frf_pi4_legacy_vl805_eeprom(0xc03115u)==0u); /* 4GB 4B 1.5 */
    assert(frf_pi4_legacy_vl805_eeprom(0u)==0u); /* unknown */
    assert(frf_pi4_legacy_vl805_eeprom(0x003111u)==0u); /* old encoding */
    assert(frf_pi4_legacy_vl805_eeprom(0xc03125u)==0u); /* other model */
    assert(frf_pi4_legacy_vl805_eeprom(0xc03114u)==0u); /* 4B 1.4 */
    puts("PASS R117: 4GB rev1.1/1.2 legacy EEPROM; 1GB rev1.5 unaffected; RAM alone not discriminator");
    return 0;
}
