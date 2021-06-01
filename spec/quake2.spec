Name:       harbour-quake2
Summary:    Quake 2 
Release:    27
Version:    1.1
Group:      Amusements/Games
License:    GPLv2
BuildArch:  %{_arch}
URL:        https://github.com/savegame/lp-public
Source0:    %{name}.tar.gz
# Requires:   SDL2
Requires:   libGLESv2
Requires:   dbus
Requires:   libogg libvorbis
# Requires:   zlib
# Requires:   glib2
Requires:   libaudioresource

BuildRequires: pulseaudio-devel,  wayland-devel, rsync
BuildRequires: libGLESv2-devel, wayland-egl-devel
BuildRequires: wayland-protocols-devel, libusb-devel
BuildRequires: libxkbcommon-devel, mce-headers, dbus-devel
BuildRequires: libogg-devel libvorbis-devel

%global build_subfolder Debug
%if %{_arch} == "armv7hl"
%global build_folder %{_topdir}/BUILD/Ports/Quake2/Output/Targets/SailfishOS-32
%else
%global build_folder %{_topdir}/BUILD/Ports/Quake2/Output/Targets/SailfishOS-32-x86
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
make -j8
cd %{_topdir}/BUILD/Ports/Quake2/Premake/Build-SailfishOS/gmake
make -j8 \
    config=release\
    sailfish_x86=$( [[ "%{_arch}" == "armv7hl" ]] && echo no || echo yes )\
    sailfish_fbo=no\
    quake2-game\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'
make -j8 \
    config=debug\
    sailfish_x86=$( [[ "%{_arch}" == "armv7hl" ]] && echo no || echo yes )\
    sailfish_fbo=no\
    quake2-gles2\
    CFLAGS=-DRESC='\"%{_datadir}/%{name}/res/\"'
strip %{build_folder}/%{build_subfolder}/bin/quake2-gles2
# exit 0

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/{bin,share/%{name}/baseq2,share/applications}
rsync -avP %{build_folder}/%{build_subfolder}/bin/quake2-gles2 %{buildroot}%{_bindir}/%{name}
rsync -avP %{_topdir}/BUILD/Engine/Sources/Compatibility/SDL/res %{buildroot}%{_datadir}/%{name}/
rsync -avP %{_topdir}/BUILD/spec/harbour-quake2.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop
rsync -avP %{_topdir}/BUILD/spec/Quake2.png %{buildroot}%{_datadir}/%{name}/%{name}.png
rsync -avP %{build_folder}/Release/bin/baseq2/game.so %{buildroot}%{_datadir}/%{name}/baseq2/

%files
%defattr(644,root,root,-)
%attr(755,root,root) %{_bindir}/%{name}
%attr(644,root,root) %{_datadir}/%{name}/%{name}.png
%dir %{_datadir}/%{name}/baseq2
%attr(755,root,root) %{_datadir}/%{name}/baseq2/*
%dir %{_datadir}/%{name}/res
%attr(644,root,root) %{_datadir}/%{name}/res/*
%attr(644,root,root) %{_datadir}/applications/%{name}.desktop

%changelog 
* Fri Feb 12 2021 sashikknox <sashikknox@gmail.com>
- rotate render dynamic when change settings
* Wed Feb 10 2021 sashikknox <sashikknox@gmail.com>
- Add advanced graphics option for rotating render to 180 degrees (for gemini PDA)
* Mon Feb 1  2021 sashikknox <sashikknos@gmail.com>
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
