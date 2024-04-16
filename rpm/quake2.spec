%global build_subfolder Debug
%global dbus_include /usr/lib64/dbus-1.0/include
%ifarch armv7hl 
    %global build_folder Ports/Quake2/Output/Targets/SailfishOS-32
    %global dbus_include /usr/lib/dbus-1.0/include
%else 
    %ifarch aarch64
        %global build_folder Ports/Quake2/Output/Targets/SailfishOS-64
        %global dbus_include /usr/lib64/dbus-1.0/include
    %else
        %global build_folder Ports/Quake2/Output/Targets/SailfishOS-32-x86
        %global dbus_include /usr/lib/dbus-1.0/include
    %endif
%endif

Name:       ru.sashikknox.quake2
Summary:    Quake 2 
Release:    3
Version:    1.3
Group:      Amusements/Games
License:    GPLv2
# BuildArch:  %{_arch}
URL:        https://github.com/savegame/sailfish-quake2
Source0:    %{name}.tar.gz
BuildRequires: pkgconfig(openal)
BuildRequires: cmake
BuildRequires: dbus-devel
BuildRequires: pkgconfig(mce)
BuildRequires: pkgconfig(wayland-egl)
BuildRequires: pkgconfig(wayland-client)
BuildRequires: pkgconfig(wayland-cursor)
BuildRequires: pkgconfig(wayland-protocols)
BuildRequires: pkgconfig(wayland-scanner)
BuildRequires: pkgconfig(egl)
BuildRequires: pkgconfig(glesv2)
BuildRequires: pkgconfig(xkbcommon)
BuildRequires: pkgconfig(gbm)
BuildRequires: pkgconfig(libpulse)
BuildRequires: rsync
#BuildRequires: libusb-devel
BuildRequires: libogg-devel 
BuildRequires: libvorbis-devel


%description
Quake II is a first-person shooter, in which the player shoots 
enemies from the perspective of the main character. 

%prep
echo "arch is %{_arch}"
%setup -q -n %{name}-%{version}
echo "Configure SDL2"
cd SDL2
cmake -Bbuild \
    -DLIB_SUFFIX="" \
    -DPULSEAUDIO=ON \
    -DSDL_STATIC=ON \
    -DVIDEO_WAYLAND=ON \
    -DVIDEO_X11=OFF \
    .

cd ../libogg
./configure \
    --disable-shared

%build
pushd SDL2/build
make -j`nproc` \
    CFLAGS=-I%{dbus_include} \
    CXXFLAGS=-I%{dbus_include} \
    LDFLAGS="-lwayland-client"
popd 

pushd libogg
make -j`nproc`
popd

pushd Ports/Quake2/Premake/Build-SailfishOS/gmake
make -j`nproc` \
    config=release\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    vorbis\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I%{dbus_include}\
    CXXFLAGS=-I%{dbus_include} 
popd

pushd Ports/Quake2/Premake/Build-SailfishOS/gmake
make -j`nproc` \
    config=release\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-game\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I%{dbus_include}\
    CXXFLAGS=-I%{dbus_include}

make -j`nproc` \
    config=release\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-ctf\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I%{dbus_include}\
    CXXFLAGS=-I%{dbus_include}

make -j`nproc` \
    config=release\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-rogue\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I%{dbus_include}\
    CXXFLAGS=-I%{dbus_include}

make -j`nproc` \
    config=release\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-xatrix\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I%{dbus_include}\
    CXXFLAGS=-I%{dbus_include}
popd

mkdir -p %{build_folder}/%{build_subfolder}/lib/
rsync -avP libogg/src/.libs/libogg.a %{build_folder}/%{build_subfolder}/lib/

pushd Ports/Quake2/Premake/Build-SailfishOS/gmake
make -j`nproc` \
    config=debug\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-gles2\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I%{dbus_include}\
    CXXFLAGS=-I%{dbus_include}
popd

strip %{build_folder}/%{build_subfolder}/bin/quake2-gles2
# exit 0

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/{bin,share/%{name}/lib/baseq2,share/applications}
# mkdir -p %{buildroot}%{_datadir}/%{name}/lib
mkdir -p %{buildroot}/usr/share/icons/hicolor/{86x86,108x108,128x128,172x172}/apps/
rsync -avP %{build_folder}/%{build_subfolder}/bin/quake2-gles2 %{buildroot}%{_bindir}/%{name}
rsync -avP Engine/Sources/Compatibility/SDL/res %{buildroot}%{_datadir}/%{name}/
chmod -R go-rw %{buildroot}%{_datadir}/%{name}/res
rsync -avP spec/gamecontrollerdb.txt %{buildroot}%{_datadir}/%{name}/gamecontrollerdb.txt
rsync -avP spec/harbour-quake2.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop
rsync -avP spec/Quake2_86.png %{buildroot}/usr/share/icons/hicolor/86x86/apps/%{name}.png
rsync -avP spec/Quake2_108.png %{buildroot}/usr/share/icons/hicolor/108x108/apps/%{name}.png
rsync -avP spec/Quake2_128.png %{buildroot}/usr/share/icons/hicolor/128x128/apps/%{name}.png
rsync -avP spec/Quake2_172.png %{buildroot}/usr/share/icons/hicolor/172x172/apps/%{name}.png
rsync -avP %{build_folder}/Release/bin/baseq2/game.so %{buildroot}%{_datadir}/%{name}/lib/baseq2/
rsync -avP %{build_folder}/Release/bin/ctf/game.so %{buildroot}%{_datadir}/%{name}/lib/ctf/
rsync -avP %{build_folder}/Release/bin/rogue/game.so %{buildroot}%{_datadir}/%{name}/lib/rogue/
rsync -avP %{build_folder}/Release/bin/xatrix/game.so %{buildroot}%{_datadir}/%{name}/lib/xatrix/
# rsync -avP /usr/lib/libvorbis.so* %{buildroot}%{_datadir}/%{name}/lib/
# rsync -avP /usr/lib/libogg.so* %{buildroot}%{_datadir}/%{name}/lib/

%files
%defattr(-,root,root,-)
%attr(755,root,root) %{_bindir}/%{name}
%attr(655,root,root) %{_datadir}/icons/hicolor/86x86/apps/%{name}.png
%attr(655,root,root) %{_datadir}/icons/hicolor/108x108/apps/%{name}.png
%attr(655,root,root) %{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%attr(655,root,root) %{_datadir}/icons/hicolor/172x172/apps/%{name}.png
%dir %{_datadir}/%{name}/lib/baseq2
%attr(755,root,root) %{_datadir}/%{name}/lib/baseq2/*
%attr(755,root,root) %{_datadir}/%{name}/lib/rogue/*
%attr(755,root,root) %{_datadir}/%{name}/lib/ctf/*
%attr(755,root,root) %{_datadir}/%{name}/lib/xatrix/*
%dir %{_datadir}/%{name}/res
%attr(655,root,root) %{_datadir}/%{name}/res/*
%attr(655,root,root) %{_datadir}/%{name}/gamecontrollerdb.txt
%attr(655,root,root) %{_datadir}/applications/%{name}.desktop

%changelog 
* Wed Mar 30 2022 sashikknox <sashikknox@gmail.com>
- fixes for SailfishOS >= 4.3
* Fri Feb 12 2021 sashikknox <sashikknox@gmail.com>
- rotate render dynamic when change settings
* Wed Feb 10 2021 sashikknox <sashikknox@gmail.com>
- Add advanced graphics option for rotating render to 180 degrees (for gemini PDA)
* Mon Feb 1  2021 sashikknox <sashikknox@gmail.com>
- use OES_packed_depth_stencil GL_EXTENSION for support stencil shadows in FBO
* Sat Jan 23 2021 sashikknox <sashikknox@gmail.com>
- add gamma correction to shader (now brightness setup has effect)
* Fri Jan 22 2021 sashikknox <sashikknox@gmail.com>
- build and strip debug version, because relese not work on XA2+ (and some oter devices)
* Thu Jan 21 2021 sashikknox <sashikknox@gmail.com>
- Thenesis Quake2 GLESv2 SailfishOS Port by sashikknox
- move config to harbour-quake2 dir
- add touch screen controls (based on https://github.com/glKarin/glquake2wayland4nemo - GLESv1 port)
- add rendering to buffer, for landscape drawing 
- add disable display blanking while game run
- pause game when app is minimized
- patch SDL2 for Wayland + EGL context and build it static
