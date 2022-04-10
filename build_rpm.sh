#!/usr/bin/bash
rm -fr `pwd`/build_rpm
mkdir -p `pwd`/build_rpm/SOURCES
git archive --output `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz HEAD
#tar czvf `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz Engine Ports SDL2 spec
export PATH=$HOME/SailfishOS/bin:${PATH}
export PWD=`pwd`

export engine="sfdk engine exec"
# export engine="docker exec --user mersdk -w `pwd` aurora-os-build-engine"

sfdk_targets=`${engine} sb2-config -l|grep -v default`

echo "WARNING: Build Quake 2 for ALL ypur targets in SailfishSDK"
for each in $sfdk_targets; do
    target_arch=${each##*-}
    echo "Build for '$each' target with '$target_arch' architecture"
    # clean build folder
    if [ -d `pwd`/build_rpm/BUILD ]; then
        rm -fr `pwd`/build_rpm/BUILD
    fi
    target="${engine} sb2 -t $each"
    #install deps for current target
    ${target} -R -m sdk-install zypper in -y pulseaudio-devel\
        wayland-devel libGLESv2-devel wayland-egl-devel wayland-protocols-devel libusb-devel  libxkbcommon-devel\
        mce-headers dbus-devel libvorbis-devel libogg-devel rsync systemd-devel autoconf automake libtool
    
    # build RPM for current target
    ${target} rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch $target_arch" -ba spec/quake2.spec

    # sign RPM packacge 
    if [[ "${engine}" == "*aurora*" ]]; then
        echo "Signing RPMs: "
        ${target} rpmsign-external sign --key /home/sashikknox/Downloads/regular_key.pem --cert /home/sashikknox/Downloads/regular_cert.pem `pwd`/build_rpm/RPMS/${target_arch}/*
        if [ $? -ne 0 ] ; then break; fi
        echo "Validate RPMs:"
        ${target} rpm-validator -p regular `pwd`/build_rpm/RPMS/${target_arch}/harbour-quake2-1.2*
        if [ $? -ne 0 ] ; then break; fi
    elif [[ "${engine}" == "sfdk *" ]] ;then
        sfdk config target=${target}
        sfdk check `pwd`/build_rpm/RPMS/${target_arch}/harbour-quake2-1.2*
        if [ $? -ne 0 ] ; then break; fi
    fi
done
echo "All build done! All yopur packages in "`pwd`/build_rpm/RPMS