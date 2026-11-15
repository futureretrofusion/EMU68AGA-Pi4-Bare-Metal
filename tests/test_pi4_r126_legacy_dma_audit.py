#!/usr/bin/env python3
from pathlib import Path
p=Path(__file__).resolve().parents[1]
h=(p/"machine/Emu68/src/frf_xhci/frf_xhci_host.c").read_text()
d=(p/"machine/Emu68/src/raspi/start_rpi64.c").read_text()
assert "frf_r126_dma_bad_pa=pa;" in h
assert "frf_r126_dma_bad_reason=why;" in h
assert "return UINT64_MAX; /* FAIL CLOSED:" in h
assert "frf_r55_dma_bad |= 1U;" in h
for field in ("CPU BASE","BUS BASE","DMA SIZE","BAD PHYS","CMD PHYS",
              "DMA REASON","FW PH","DMA BAD"):
 assert field in d,field
assert "const int w=320,h=620" in d
cfg=(p/"build-pi4-r130-legacy/CMakeCache.txt").read_text()
for k in ("FRF_PI4_SAFE_VL805_NOTIFY:BOOL=ON",
          "FRF_PI4_XHCI_HID:BOOL=ON",
          "FRF_BOOT_POST:BOOL=ON"):
 assert k in cfg,k
print("PASS R126 legacy: fail-closed DMA guard preserved, first rejected PA/reason and FDT window displayed")
