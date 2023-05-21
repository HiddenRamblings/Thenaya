# Thenaya - TagMo for 3ds

<p align="center">
  <a href="https://discord.gg/uc2YheD4CK" target="_blank">
    <img src="https://img.shields.io/discord/1109571129242296320.svg?label=&logo=discord&logoColor=ffffff&color=7289DA&labelColor=6A5ACD">
  </a>
  <a href="https://github.com/HiddenRamblings/Thenaya/actions/workflows/pacman.yml" target="_blank">
    <img src="https://github.com/HiddenRamblings/Thenaya/actions/workflows/pacman.yml/badge.svg">
  </a>
</p>

## Thenaya Install

<p align="center">
<img alt='Thenaya QR Code' src='https://github.com/HiddenRamblings/Thenaya/blob/master/assets/Thenaya_CIAQR.png?raw=true'/></a>
</p>
Open FBI, select Remote Install, select Scan QR Code, scan QR above
<br>Download and unzip Thenaya_vX.X.zip, copy to matching sdmc folders

## Thenaya Setup

Place "key_retail.bin" in the root of sdmc:/ folder (sdcard).  
If using separate keys, they will need to be combined first.  
 - `cat unfixed-info.bin locked-secret.bin > key_retail.bin`

## Building Thenaya

Follow the instructions at https://devkitpro.org/wiki/devkitPro_pacman to install pacman

For OSX users, add `alias pacman="sudo /opt/devkitpro/pacman/bin/./pacman"` to ~/.profile

For OSX and debian distros, replace any instances of `dkp-pacman` with `pacman`
   - See https://devkitpro.org/wiki/devkitPro_pacman#Using_Pacman

Run `sudo dkp-pacman -S 3ds-dev` to install the necessary packages.

Run `git submodule update --init --recursive` to sync amiitool submodules

for OSX users, remove `#include "portable_endian.h"` from amiitool/amiibo.c

## [Contributors](https://github.com/HiddenRamblings/Thenaya/graphs/contributors)

Additional thanks go out to (alphabetically):

### Miscellaneous
* JaySea77 - [Maintainance](https://github.com/JaySea77/Thenaya)

### Source
* Falco20019 - [libamiibo](https://github.com/Falco20019/libamiibo)
* hax0karti - [Amiibo Generator For Wumiibo](https://github.com/hax0kartik/amiibo-generator)
* N3evin - [AmiiboAPI](https://github.com/N3evin/AmiiboAPI)
* socram8888 - [amiitool](https://github.com/socram8888/amiitool)

##
![Thenaya Logo](assets/thenaya_feature.png)
###
*Thenaya is not affiliated, authorized, sponsored, endorsed, or in any way connected with Nintendo Co., Ltd or its subsidiaries. amiibo is a registered trademark of Nintendo of America Inc. No licensed resources are owned. Files created with or resulting from Thenaya are not intended for sale or distribution. Thenaya is for educational and archival purposes only.*