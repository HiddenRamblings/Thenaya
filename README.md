# Thenaya - TagMo for 3ds

### Restoration of this project is currently WIP. Please be patient.

Follow the instructions at https://devkitpro.org/wiki/devkitPro_pacman to install pacman

For OSX users, also add `alias pacman="sudo /opt/devkitpro/pacman/bin/./pacman"` to ~/.profile

For OSX and debian distros, replace any instances of `dkp-pacman` with `pacman`
   - See https://devkitpro.org/wiki/devkitPro_pacman#Using_Pacman

Run `sudo dkp-pacman -S 3ds-dev` to install the necessary packages.

Run `git submodule update --init --recursive` to sync amiitool submodules

for OSX users, remove `#include "portable_endian.h"` from amiitool/amiibo.c

Replace amiitool.c in the amitool folder with the one in the replace folder
Move replace/include/nfc3d/amitool.h to amitool/include/nfc3d
