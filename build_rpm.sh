#!/bin/bash
rm -fr `pwd`/build_rpm
mkdir -p `pwd`/build_rpm/SOURCES
git archive --output `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz HEAD

# rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch armv7hl" -ba spec/quake2.spec

#install deps in arm
sfdk engine exec sb2 -t SailfishOS-3.4.0.24-armv7hl -R -m sdk-install zypper in -y pulseaudio-devel\
    wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
    mce-headers dbus-devel libvorbis-devel libogg-devel rsync
#build arm 
sfdk engine exec sb2 -t SailfishOS-3.4.0.24-armv7hl rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch armv7hl" -ba spec/quake2.spec

rm -fr `pwd`/build_rpm/BUILD
#install deps in x86
sfdk engine exec sb2 -t SailfishOS-3.4.0.24-i486 -R -m sdk-install zypper in -y pulseaudio-devel\
    wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
    mce-headers dbus-devel libvorbis-devel libogg-devel rsync
sfdk engine exec sb2 -t SailfishOS-3.4.0.24-i486 rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch i486" -ba spec/quake2.spec