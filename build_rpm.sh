#!/bin/bash
rm -fr `pwd`/build_rpm
mkdir -p `pwd`/build_rpm/SOURCES
git archive --output `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz HEAD
#tar czvf `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz Engine Ports SDL2 spec

# rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch armv7hl" -ba spec/quake2.spec
sfdk_targets=`sfdk engine exec sb2-config -l`

echo "WARNING: Build Quake 2 for ALL ypur targets in SailfishSDK"
for each in $sfdk_targets; do
    target_arch=${each##*-}
    echo "Build for '$each' target with '$target_arch' architecture"
    # clean build folder
    if [ -d `pwd`/build_rpm/BUILD ]; then
        rm -fr `pwd`/build_rpm/BUILD
    fi
    #install deps for current target
    sfdk engine exec sb2 -t $each -R -m sdk-install zypper in -y pulseaudio-devel\
        wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
        mce-headers dbus-devel libvorbis-devel libogg-devel rsync
    # build RPM for current target
    sfdk engine exec sb2 -t $each rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch $target_arch" -ba spec/quake2.spec
    if [ $? -ne 0 ] ; then break; fi
done
echo "All build done! All yopur packages in "`pwd`/build_rpm/RPMS
# #install deps for arm target
# sfdk engine exec sb2 -t SailfishOS-3.4.0.24-armv7hl -R -m sdk-install zypper in -y pulseaudio-devel\
#     wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
#     mce-headers dbus-devel libvorbis-devel libogg-devel rsync
# #build arm 
# sfdk engine exec sb2 -t SailfishOS-3.4.0.24-armv7hl rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch armv7hl" -ba spec/quake2.spec
# exit 0
# rm -fr `pwd`/build_rpm/BUILD
# #install deps in x86
# sfdk engine exec sb2 -t SailfishOS-3.4.0.24-i486 -R -m sdk-install zypper in -y pulseaudio-devel\
#     wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
#     mce-headers dbus-devel libvorbis-devel libogg-devel rsync
# #build x86
# sfdk engine exec sb2 -t SailfishOS-3.4.0.24-i486 rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch i486" -ba spec/quake2.spec