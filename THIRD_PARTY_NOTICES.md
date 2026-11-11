# Third-party sources and licences

The source headers, full licence files and provenance remain authoritative.

* AGA-PiStorm chipset: GPL-2.0-only (machine/LICENSE, machine/COPYING).
* UAE, E-UAE and WinUAE portions: source-level original notices and
  machine/package/UAE-WinUAE-notice.txt.
* Emu68, Michal Schulz and contributors: MPL-2.0
  (machine/Emu68/LICENSE, machine/MPL-2.0.txt).
* Capstone: BSD-style licence
  (machine/Emu68/external/capstone/LICENSE.TXT).
* libdeflate: MIT-style licence
  (machine/Emu68/external/libdeflate/COPYING).
* tiny-stl: MPL-2.0
  (machine/Emu68/external/tiny-stl/LICENSE).
* USPi: GPL-3.0-or-later; OPTIONAL source ONLY
  (machine/Emu68/third_party/uspi/COPYING and file headers).

USPi is not linked in the Pi4 build (FRF_USB_HOST=OFF). Do NOT
link GPLv3-or-later USPi together with GPL-2.0-only AGA core without
separately resolving the licence conflict. A Pi3 release requires an
independent licence audit.

Do not publish Kickstart ROMs, Workbench images, games or vendor
firmware downloaded to a local CMake build directory.
