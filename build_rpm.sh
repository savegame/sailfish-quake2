#!/usr/bin/bash
export PWD=`pwd`

export engine="sfdk engine exec"
dependencies="pulseaudio-devel wayland-devel libGLESv2-devel 
    wayland-egl-devel wayland-protocols-devel libusb-devel 
    libxkbcommon-devel mce-headers dbus-devel libvorbis-devel 
    libogg-devel rsync systemd-devel autoconf automake libtool"
# uncomment right engine for your 
if [ "$1" == "aurora" ]; then
    export engine="docker exec --user mersdk -w `pwd` aurora-os-build-engine"
else
    export PATH=$HOME/SailfishOS/bin:${PATH}
export engine="sfdk engine exec"
fi

build_dir="build_rpm"
if [[ "${engine}" == *"aurora"* ]]; then
    build_dir="build_aurora_rpm"
fi
echo "Pack latest git cmmit to an archive: ${build_dir}/SOURCES/harbour-quake2.tar.gz"
rm -fr `pwd`/${build_dir}/{BUILD,SRPMS}
mkdir -p `pwd`/${build_dir}/SOURCES
git archive --output `pwd`/${build_dir}/SOURCES/harbour-quake2.tar.gz HEAD
#tar czvf `pwd`/${build_dir}/SOURCES/harbour-quake2.tar.gz Engine Ports SDL2 spec

if [[ "${engine}" == *"aurora"* ]]; then
    for each in key cert; do
        if [ -f `pwd`/regular_${each}.pem ]; then 
            echo "Файл ключа regular_${each}.pem найден: OK"
            continue;
        fi
        echo -n "Скачиваем ключ regular_${each}.pem для подписи пактов под АврораОС: "
        curl https://community.omprussia.ru/documentation/files/doc/regular_${each}.pem -o regular_${each}.pem &> /dev/null
        if [ $? -eq 0 ]; then 
            echo "OK"
        else
            echo "FAIL"
            echo "Ошибка скачивания regular_${each}.pem: https://community.omprussia.ru/documentation/files/doc/regular_${each}.pem"
            exit 1
        fi
    done
fi

sfdk_targets=`${engine} sb2-config -l|grep default|grep aarch`

echo "WARNING: Build Quake 2 for ALL your targets in SailfishSDK"
for each in $sfdk_targets; do
    target_arch=${each##*-}
    target_arch=${target_arch/.default/}
    echo "Build for '$each' target with '$target_arch' architecture"
    # clean build folder
    if [ -d `pwd`/${build_dir}/BUILD ]; then
        rm -fr `pwd`/${build_dir}/BUILD
    fi
    target="${engine} sb2 -t $each"
    #install deps for current target
    ${target} -R -m sdk-install zypper in -y ${dependencies}
    
    # build RPM for current target
    ${target} rpmbuild --define "_topdir `pwd`/${build_dir}" --define "_arch $target_arch" -ba spec/quake2.spec
    if [ $? -ne 0 ] ; then 
        echo "Build RPM for ${each} : FAIL"
        continue; 
    fi

    # sign RPM packacge 
    if [[ "${engine}" == *"aurora"* ]]; then
        echo -n "Signing RPMs: "
        ${target} rpmsign-external sign --key `pwd`/regular_key.pem --cert `pwd`/regular_cert.pem `pwd`/${build_dir}/RPMS/${target_arch}/harbour-quake2-1.*
        if [ $? -ne 0 ] ; then 
            echo "FAIL"
            break; 
        fi
        echo "OK"
        echo -n "Validate RPMs: "
        validator_output=`${target} rpm-validator -p regular $(pwd)/${build_dir}/RPMS/${target_arch}/harbour-quake2-1.2* 2>&1`
        if [ $? -ne 0 ] ; then 
            echo "FAIL"
            echo "${validator_output}"
            break; 
        fi
        echo "OK"
    elif [[ "${engine}" == "sfdk "* ]] ;then
        echo -n "Validate RPM: "
        sfdk config target=${each/.default/}
        validator_output=`sfdk check $(pwd)/${build_dir}/RPMS/${target_arch}/harbour-quake2-1.2* 2>&1`
        if [ $? -ne 0 ] ; then 
            echo "FAIL"
            echo "${validator_output}"
            break;
        fi
        echo "OK"
    fi
done
echo "All build done! All yopur packages in `pwd`/build_rpm/RPMS"