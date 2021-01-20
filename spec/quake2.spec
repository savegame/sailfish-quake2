Name:       harbour-quake2
Summary:    Quake 2 
Version:    1.1
Release:    1
Group:      Amusements/Games
License:    GPLv2
BuildArch:  %{_arch}
URL:        https://github.com/savegame/lp-public
Source0:    %{name}.tar.gz
# Requires:   SDL2
Requires:   GLESv2
Requires:   dbus
# Requires:   openssl
# Requires:   zlib
# Requires:   glib2
Requires:   libaudioresource

BuildRequires: pulseaudio-devel,  wayland-devel
BuildRequires: libGLESv2-devel, wayland-egl-devel
BuildRequires: wayland-protocols-devel, libusb-devel
BuildRequires: libxkbcommon-devel, mce-headers, dbus-devel


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
    --disable-shared\
    --enable-hidapi\
    --enable-libudev


%build
cd %{_topdir}/BUILD/SDL2
make -j8
cd %{_topdir}/BUILD/Ports/Quake2/Premake/Build-SailfishOS/gmake
make -j8 \
    config=release\
    sailfish_x86=no\
    sailfish_fbo=yes\
    quake2-game\
    quake2-gles2\
    CFLAGS=-DRESC='\\\"%{_datadir}/%{name}/res\\\"'

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/{bin,share/%{name}}
cp %{_topdir}/BUILD/Ports/Quake2/Targets/SailfishOS-32/bin/quake2-gles2 %{buildroot}%{_bindir}/%{name}
cp %{_topdir}/BUILD/Engine/Sources/SDL/res %{buildroot}%{_datadir}/%{name}/res
cp %{_topdir}/BUILD/spec/harbour-quake2.desktop %{buildroot}%{_bindir}/%{name}
cp %{_topdir}/BUILD/spec/Quake2.png %{buildroot}%{_datadir}/%{name}/%{name}.png
cp -r %{_topdir}/BUILD/Ports/Quake2/Targets/SailfishOS-32/bin/baseq2 %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%defattr(644,root,root,-)
%attr(755,root,root) %{_bindir}/%{name}
%attr(644,root,root) %{_datadir}/%{name}/%{name}.png
%attr(644,root,root) %{_datadir}/%{name}/baseq2
%attr(644,root,root) %{_datadir}/applications/%{name}.desktop

%changelog 
* Thu Jan 21 2021 sashikknox <sashikknox@gmail.com>
- Thenesis Quake2 GLESv2 SailfishOS Port by sashikknox
- add touch screen controls (based on https://github.com/glKarin/glquake2wayland4nemo - GLESv1 port)
- add rendering to buffer, for landscape drawing 
- add disable display blanking while game run
- pause game when app is minimized
- patch SDL2 for Wayland + EGL context and build it static
