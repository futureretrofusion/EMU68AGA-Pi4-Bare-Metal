/* FRF EMU68AGA Pi4 build identification.
 * Keep our independent Pi4 kernel revision separate from the upstream
 * AGA-Storm feature/API level: they are NOT identical releases. */
#ifndef FRF_PI4_POST_VERSIONS_H
#define FRF_PI4_POST_VERSIONS_H
#define FRF_POST_PI4_VERSION "R112"
#define FRF_POST_AGA_VERSION "v1.121"
#define FRF_POST_FRF_LABEL "FRF PI4 BARE METAL: " FRF_POST_PI4_VERSION
#define FRF_POST_AGA_LABEL "AGA-STORM PORT: " FRF_POST_AGA_VERSION
#endif
