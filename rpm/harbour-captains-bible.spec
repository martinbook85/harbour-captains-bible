Name:       harbour-captains-bible
Summary:    Captain's Bible
Version:    0.1
Release:    4
License:    LICENSE
URL:        http://example.org/
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  desktop-file-utils
BuildRequires:  pkgconfig(Qt5Concurrent)

%description
Bible study app that allows download and use of modules for different versions of the Bible, commentaries, dictionaries, graphics and books from the Sword project (http://crosswire.org/sword/index.jsp). The app or its developer are not affiliated with the Sword project.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install

desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
  %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog
* Fri Oct 16 2026 Martin Book <martin@fambook.nu> 0.1-4
- Only corrected a dependency
