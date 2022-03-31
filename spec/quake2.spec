Name:       harbour-quake2
Summary:    Quake 2 
Release:    5
Version:    1.2
Group:      Amusements/Games
License:    GPLv2
BuildArch:  %{_arch}
URL:        https://github.com/savegame/lp-public
Source0:    %{name}.tar.gz
# Requires:   SDL2
# Requires:   libGLESv2
# Requires:   dbus
# Requires:   libogg libvorbis
# Requires:   zlib
# Requires:   glib2
# Requires:   libaudioresource

BuildRequires: pulseaudio-devel,  wayland-devel, rsync
BuildRequires: libGLESv2-devel, wayland-egl-devel
BuildRequires: wayland-protocols-devel, libusb-devel
BuildRequires: libxkbcommon-devel, mce-headers, dbus-devel
BuildRequires: libogg-devel libvorbis-devel

%global build_subfolder Debug
%ifarch armv7hl
%global build_folder %{_topdir}/BUILD/Ports/Quake2/Output/Targets/SailfishOS-32
%else 
    %ifarch aarch64
        %global build_folder %{_topdir}/BUILD/Ports/Quake2/Output/Targets/SailfishOS-64
    %else
        %global build_folder %{_topdir}/BUILD/Ports/Quake2/Output/Targets/SailfishOS-32-x86
    %endif
%endif
%{echo:set build bolder to %{build_folder}}


%description
Quake II is a first-person shooter, in which the player shoots enemies from the perspective of the main character. 

%prep
echo "Unpack sources"
cd %{_topdir}/BUILD
tar -xzf %{_topdir}/SOURCES/%{name}.tar.gz
echo "Configure SDL2"
cd %{_topdir}/BUILD/SDL2
#make clean 
./configure \
    --disable-video-x11\
    --enable-video-wayland\
    --enable-pulseaudio\
    --enable-hidapi\
    --enable-libudev \
    # --disable-shared


%build
cd %{_topdir}/BUILD/SDL2
make -j8 \
    CFLAGS=-I/usr/lib64/dbus-1.0/include \
    CXXFLAGS=-I/usr/lib64/dbus-1.0/include
cd %{_topdir}/BUILD/Ports/Quake2/Premake/Build-SailfishOS/gmake
make -j8 \
    config=release\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-game\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I/usr/lib64/dbus-1.0/include\
    CXXFLAGS=-I/usr/lib64/dbus-1.0/include 
make -j8 \
    config=debug\
    sailfish_arch=%{_arch}\
    sailfish_fbo=yes\
    quake2-gles2\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'\ -I/usr/lib64/dbus-1.0/include\
    LDFLAGS=-Wl,-rpath,%{_datadir}/%{name}/lib\
    CXXFLAGS=-I/usr/lib64/dbus-1.0/include 
strip %{build_folder}/%{build_subfolder}/bin/quake2-gles2
# exit 0

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/{bin,share/%{name}/baseq2,share/applications}
mkdir -p %{buildroot}%{_datadir}/%{name}/lib
mkdir -p %{buildroot}/usr/share/icons/hicolor/{86x86,108x108,128x128,172x172}/apps/
rsync -avP %{build_folder}/%{build_subfolder}/bin/quake2-gles2 %{buildroot}%{_bindir}/%{name}
rsync -avP %{_topdir}/BUILD/Engine/Sources/Compatibility/SDL/res %{buildroot}%{_datadir}/%{name}/
rsync -avP %{_topdir}/BUILD/spec/harbour-quake2.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop
rsync -avP %{_topdir}/BUILD/spec/Quake2.png %{buildroot}/usr/share/icons/hicolor/86x86/apps/%{name}.png
rsync -avP %{_topdir}/BUILD/spec/Quake2.png %{buildroot}/usr/share/icons/hicolor/108x108/apps/%{name}.png
rsync -avP %{_topdir}/BUILD/spec/Quake2.png %{buildroot}/usr/share/icons/hicolor/128x128/apps/%{name}.png
rsync -avP %{_topdir}/BUILD/spec/Quake2.png %{buildroot}/usr/share/icons/hicolor/172x172/apps/%{name}.png
rsync -avP %{build_folder}/Release/bin/baseq2/game.so %{buildroot}%{_datadir}/%{name}/baseq2/
rsync -avP /usr/lib/libvorbis.so* %{buildroot}%{_datadir}/%{name}/lib/
rsync -avP /usr/lib/libogg.so* %{buildroot}%{_datadir}/%{name}/lib/

%files
%defattr(644,root,root,-)
%attr(755,root,root) %{_bindir}/%{name}
%dir %{_datadir}/icons/hicolor
%attr(644,root,root) %{_datadir}/icons/hicolor/*
%dir %{_datadir}/%{name}/baseq2
%attr(755,root,root) %{_datadir}/%{name}/baseq2/*
%dir %{_datadir}/%{name}/res
%dir %{_datadir}/%{name}/lib
%attr(644,root,root) %{_datadir}/%{name}/res/*
%attr(644,root,root) %{_datadir}/%{name}/lib/*
%attr(644,root,root) %{_datadir}/applications/%{name}.desktop

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
