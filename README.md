# SailfishOS port of Quake2 
###### (by [sashikknox](https://github.com/savegame/sailfish-quake2))

## First, some screenshots:
|![Снимок_Экрана_20210122_007](https://user-images.githubusercontent.com/16311332/119626682-2556c900-be14-11eb-999d-467e94417d00.png)|![Снимок_Экрана_20210122_010](https://user-images.githubusercontent.com/16311332/119626706-28ea5000-be14-11eb-9df4-44aeb2dcf2f6.png)|
|:-:|:-:|
|![Снимок_Экрана_20210122_008](https://user-images.githubusercontent.com/16311332/119626720-2c7dd700-be14-11eb-922f-c177e6425945.png)|![Снимок_Экрана_20210320_001](https://user-images.githubusercontent.com/16311332/119626861-4d462c80-be14-11eb-8266-e9ba48dfc874.png)|

## How to build
1. first, install [SailfishSDK](https://sailfishos.org/wiki/Application_SDK)
2. prepare for build
  - on Linux: you should add `SailfishSDK/bin` folder to your **PATH**
    ```sh
    # by default, SailfishSDK installs to ~/SailfishOS
    export PATH=~/SailfishOS/bin/:${PATH}
    ```
    or just run **sfdk** tool, use absolute path, like
    ```sh
    ~/SailfishOS/bin/sfdk ...
    ```
  - on Windows, SailfishOS SDK bin folder should be too in PATH
3. list SailfishOS targets names
```sh
sfdk engien exec sb2-config -l
```
  it should show you something like 
```
SailfishOS-4.0.1.48-armv7hl
SailfishOS-4.0.1.48-i486
``` 
  this targets you should use in next step for build rpm files for SailfishOS   
4. then, you can build game (on Windows and Linux, gae build in same way)
```sh
# first create buildroot 
mkdir -p `pwd`/build_rpm/SOURCES
# and create sources archive from current commit
git archive --output `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz HEAD
# if ypu change comething, you should first commit it, then build RPM, 
# or pack you sources by yourself, something like
# tar -czvf `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz Engine Ports SDL2 spec

# then install dependencies in arm target
sfdk engine exec sb2 -t SailfishOS-4.0.1.48-armv7hl -R -m sdk-install zypper in -y pulseaudio-devel\
  wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
  mce-headers dbus-devel libvorbis-devel libogg-devel rsync
# and build ARM version of Quake2 
sfdk engine exec sb2 -t SailfishOS-4.0.1.48-armv7hl rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch armv7hl" -ba spec/quake2.spec

# then install dependencies in i486 target
sfdk engine exec sb2 -t SailfishOS-4.0.1.48-i486 -R -m sdk-install zypper in -y pulseaudio-devel\
  wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
  mce-headers dbus-devel libvorbis-devel libogg-devel rsync
# and build ч86 version of Quake2 
sfdk engine exec sb2 -t SailfishOS-4.0.1.48-i486 rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch i486" -ba spec/quake2.spec
```
