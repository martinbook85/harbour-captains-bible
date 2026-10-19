Name:       harbour-captains-bible
Summary:    Captain's Bible
Version:    0.2
Release:    7
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
* Mon Oct 19 2026 Martin Book <martin@fambook.nu> 0.2-7
- Made a filter to make sure only Bibles are in the list of "Select Bible translation"

* Sun Oct 18 2026 Martin Book <martin@fambook.nu> 0.2-6
- Commentary view updated with verse numbers
- Module selection in commentary view shows if the commentary has comments on selected chapter
- Search function added with ability to search current or other Bible modules and different parts of Holy Scripture

* Sat Oct 17 2026 Martin Book <martin@fambook.nu> 0.2-5
- Fixes to Bible Maps, still only Swedish and English are stable
- Map zoom works in the Swedish module, best way for larger image in other modules is landscape mode

* Sat Oct 17 2026 Martin Book <martin@fambook.nu> 0.2-4
- Fixed image scaling issue in MapPage.qml by injecting dynamic width attributes

* Sat Oct 17 2026 Martin Book <martin@fambook.nu> 0.2-3
- Fixed VOTD book mapping for Ephesians (Efes) and Romans on Cover page
- Started work on Bible Maps but it is not yet fully functional
- Fixed verse text rendering line breaks for MyBible modules
- Updated SearchField to filter on EnterKey in ModuleDownloaderPage

* Fri Oct 16 2026 Martin Book <martin@fambook.nu> 0.2-1
- Added language dividers with custom boxes in ModuleSelectPage
- Shortened book names in Verse of the Day to short format (e.g., "1 Kor")
- Expanded verse text budget on CoverPage to fit up to 9 lines
- Optimized circular chapter buffer for smooth 60fps swipes
- Updated RPM versioning for OpenRepos release

* Thu Oct 15 2026 Martin Book <martin@fambook.nu> 0.1-4
- Preserved original inline note markers in verse text
- Initial cover action and circular buffer foundation
