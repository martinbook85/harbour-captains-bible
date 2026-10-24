QT += core gui quick concurrent network sql
LIBS += -lz

# Applikationsnamn (stämmer med QML-huvudfilen)
TARGET = harbour-captains-bible

# Aktivera SailfishApp-ramverket och C++17
CONFIG += sailfishapp c++17

# C++ Källkod
SOURCES += \
    src/harbour-captains-bible.cpp \
    src/biblemanager.cpp \
    src/zip_extractor.cpp

HEADERS += \
    src/biblemanager.h \
    src/zip_extractor.h

# QML-sidor och resurser som ska med i RPM-paketeringen
OTHER_FILES += \
    qml/harbour-captains-bible.qml \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    qml/pages/SelectPage.qml \
    qml/pages/ModuleSelectPage.qml \
    qml/pages/ModuleDownloaderPage.qml \
    qml/pages/CommentaryPage.qml \
    rpm/harbour-captains-bible.changes.in \
    rpm/harbour-captains-bible.changes.run.in \
    rpm/harbour-captains-bible.spec \
    harbour-captains-bible.desktop

# Applikationsikoner för alla skärmupplösningar
SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Språk och översättningar
CONFIG += sailfishapp_i18n
TRANSLATIONS += translations/harbour-captains-bible-de.ts

DISTFILES += \
    qml/pages/AboutPage.qml \
    qml/pages/BookmarksPage.qml \
    qml/pages/DictionaryPage.qml \
    qml/pages/ImagePage.qml \
    qml/pages/MapPage.qml \
    qml/pages/NoteEditorPage.qml \
    qml/pages/SearchPage.qml \
    qml/pages/SearchResultChapterPage.qml \
    qml/pages/SelectCommentaryDialog.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/StudyNotesPage.qml \
    qml/pages/VerseCommentaryPage.qml

# -----------------------------------------------------------------------------
# Installation av extra filer (votd.json)
# -----------------------------------------------------------------------------
votd_data.files = data/votd.json
votd_data.path = /usr/share/$${TARGET}/data
INSTALLS += votd_data
