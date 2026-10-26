#include "biblemanager.h"
#include "zip_extractor.h"
#include <QRegularExpression>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDate>
#include <QDirIterator>
#include <QNetworkRequest>
#include <QUrl>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegExp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QSqlDatabase>
#include <QImage>
#include <QSqlRecord>

BibleManager::BibleManager(QObject *parent)
    : QObject(parent), m_isFetchingCatalog(false)
{
    m_modulesPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/bible_modules";
    m_catalogPath = m_modulesPath + "/catalog";

    QDir().mkpath(m_modulesPath);
    QDir().mkpath(m_catalogPath);

    qDebug() << "[BibleManager] Initialized. Path:" << m_modulesPath;

    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &BibleManager::onDownloadFinished);
}

BibleManager::~BibleManager()
{
}

int BibleManager::getMyBibleBookNumber(const QString &bookId)
{
    static const QMap<QString, int> osisToMyBible = {
        {"Gen", 10}, {"Exo", 20}, {"Lev", 30}, {"Num", 40}, {"Deu", 50},
        {"Josh", 60}, {"Judg", 70}, {"Ruth", 80}, {"1Sam", 90}, {"2Sam", 100},
        {"1Kings", 110}, {"1Kin", 110}, {"2Kings", 120}, {"2Kin", 120},
        {"1Chr", 130}, {"2Chr", 140}, {"Ezra", 150}, {"Ezr", 150}, {"Neh", 160},
        {"Esth", 190}, {"Job", 220}, {"Ps", 230}, {"Prov", 240}, {"Eccl", 250},
        {"Song", 260}, {"Isa", 290}, {"Jer", 300}, {"Lam", 310}, {"Ezek", 330},
        {"Dan", 340}, {"Hos", 350}, {"Joel", 360}, {"Amos", 370}, {"Obad", 380},
        {"Jonah", 390}, {"Mic", 400}, {"Nah", 410}, {"Hab", 420}, {"Zeph", 430},
        {"Hag", 440}, {"Zech", 450}, {"Mal", 460},
        {"Matt", 470}, {"Mark", 480}, {"Luke", 490}, {"John", 500}, {"Acts", 510},
        {"Rom", 520}, {"1Cor", 530}, {"2Cor", 540}, {"Gal", 550}, {"Eph", 560},
        {"Phil", 570}, {"Col", 580}, {"1Thess", 590}, {"1Ths", 590}, {"2Thess", 600},
        {"2Ths", 600}, {"1Tim", 610}, {"2Tim", 620}, {"Titus", 630}, {"Tit", 630},
        {"Phlm", 640}, {"Phim", 640}, {"Heb", 650}, {"Jas", 660}, {"Jam", 660},
        {"1Pet", 670}, {"2Pet", 680}, {"1John", 690}, {"1Jn", 690}, {"2John", 700},
        {"2Jn", 700}, {"3John", 710}, {"3Jn", 710}, {"Jude", 720}, {"Jud", 720},
        {"Rev", 730}
    };

    return osisToMyBible.value(bookId, 0);
}

QString BibleManager::getOSISBookId(int myBibleBookNumber)
{
    static const QMap<int, QString> myBibleToOsis = {
        {10, "Gen"}, {20, "Exo"}, {30, "Lev"}, {40, "Num"}, {50, "Deu"},
        {60, "Josh"}, {70, "Judg"}, {80, "Ruth"}, {90, "1Sam"}, {100, "2Sam"},
        {110, "1Kings"}, {120, "2Kings"}, {130, "1Chr"}, {140, "2Chr"}, {150, "Ezra"},
        {160, "Neh"}, {190, "Esth"}, {220, "Job"}, {230, "Ps"}, {240, "Prov"},
        {250, "Eccl"}, {260, "Song"}, {290, "Isa"}, {300, "Jer"}, {310, "Lam"},
        {330, "Ezek"}, {340, "Dan"}, {350, "Hos"}, {360, "Joel"}, {370, "Amos"},
        {380, "Obad"}, {390, "Jonah"}, {400, "Mic"}, {410, "Nah"}, {420, "Hab"},
        {430, "Zeph"}, {440, "Hag"}, {450, "Zech"}, {460, "Mal"},
        {470, "Matt"}, {480, "Mark"}, {490, "Luke"}, {500, "John"}, {510, "Acts"},
        {520, "Rom"}, {530, "1Cor"}, {540, "2Cor"}, {550, "Gal"}, {560, "Eph"},
        {570, "Phil"}, {580, "Col"}, {590, "1Thess"}, {600, "2Thess"}, {610, "1Tim"},
        {620, "2Tim"}, {630, "Titus"}, {640, "Phlm"}, {650, "Heb"}, {660, "Jas"},
        {670, "1Pet"}, {680, "2Pet"}, {690, "1John"}, {700, "2John"}, {710, "3John"},
        {720, "Jude"}, {730, "Rev"}
    };

    return myBibleToOsis.value(myBibleBookNumber, QString("Book%1").arg(myBibleBookNumber));
}

QSqlDatabase BibleManager::getModuleDatabase(const QString &moduleName)
{
    QString connectionName = "MyBible_" + moduleName;
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) return db;
    }

    QString filePath = m_modulesPath + "/" + moduleName + ".SQLite3";
    if (!QFile::exists(filePath)) {
        filePath = m_modulesPath + "/" + moduleName + ".sqlite3";
    }

    if (!QFile::exists(filePath)) {
        QDirIterator it(m_modulesPath, QStringList() << moduleName + ".SQLite3" << moduleName + ".sqlite3",
                         QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext()) {
            filePath = it.next();
        }
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(filePath);

    if (!db.open()) {
        qDebug() << "[BibleManager] Failed to open SQLite database for:" << moduleName << "Path:" << filePath;
    }

    return db;
}

QSqlDatabase BibleManager::getCommentaryDatabase(const QString &moduleName)
{
    QString connectionName = "MyBible_Commentary_" + moduleName;
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) return db;
    }

    QString filePath = m_modulesPath + "/" + moduleName + ".SQLite3";
    if (!QFile::exists(filePath)) {
        filePath = m_modulesPath + "/" + moduleName + ".sqlite3";
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(filePath);

    if (!db.open()) {
        qDebug() << "[BibleManager] Failed to open standalone commentary database for:" << moduleName << "Path:" << filePath;
    }

    return db;
}

QString BibleManager::mapCategoryName(const QString &typeSuffix)
{
    QString suf = typeSuffix.toLower().trimmed();

    if (typeSuffix.contains("LDS", Qt::CaseInsensitive)) {
            return "Maps";
    }

    if (suf.contains("commentar") || suf.contains(".commentaries") || suf.endsWith("-c")) return "Commentaries";
    if (suf.contains("dict") || suf.contains(".dictionary")) return "Dictionaries";
    if (suf.contains("devotion") || suf.contains(".devotions")) return "Devotions";
    if (suf.contains("plan") || suf.contains(".plan")) return "Reading Plans";
    if (suf.contains("subheading") || suf.contains(".subheadings")) return "Subheadings";
    if (suf.contains("crossref") || suf.contains(".crossreferences")) return "Cross References";

    return "Bibles";
}

QString BibleManager::getLanguageDisplayName(const QString &languageCode)
{
    QString code = languageCode.trimmed();
    if (code.isEmpty()) return QString();
    if (code.compare("All", Qt::CaseInsensitive) == 0) return QString::fromUtf8("All languages");

    QLocale locale(code);
    if (locale.language() != QLocale::C) {
        QString nativeName = locale.nativeLanguageName();
        if (!nativeName.isEmpty()) {
            nativeName[0] = nativeName[0].toUpper();
            return nativeName;
        }
    }
    return code;
}

bool BibleManager::isModuleDownloading(const QString &moduleName) const
{
    return m_downloadingModules.contains(moduleName);
}

QVariantList BibleManager::getInstalledModules()
{
    QVariantList list;

    QDirIterator it(m_modulesPath, QStringList() << "*.SQLite3" << "*.sqlite3" << "*.SQLite" << "*.sqlite",
                     QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();

        if (fileInfo.fileName().contains(".commentaries.", Qt::CaseInsensitive)) {
            continue;
        }

        QString modName = fileInfo.completeBaseName();
        modName.remove(QRegExp("\\.(dictionary|devotions|plan|subheadings|crossreferences)$", Qt::CaseInsensitive));

        QSqlDatabase db = getModuleDatabase(modName);
        QString description = modName;
        QString language = "en";
        QString category = mapCategoryName(fileInfo.fileName());

        if (db.isOpen()) {
            QSqlQuery q(db);

            bool querySuccess = q.exec("SELECT name, value FROM details");
            if (!querySuccess) {
                querySuccess = q.exec("SELECT name, value FROM info");
            }

            if (querySuccess) {
                while (q.next()) {
                    QString key = q.value(0).toString().toLower();
                    QString val = q.value(1).toString();
                    if (key == "description") description = val;
                    else if (key == "language") language = val;
                }
            }
        }

        ModuleInfo m;
        m.id = modName;
        m.name = modName;
        m.description = description;
        m.category = category;
        m.language = language;
        m.groupTitle = "Installed Modules";
        m.isInstalled = true;

        // Custom override for LDS map modules
        if (m.name.startsWith("LDS", Qt::CaseInsensitive)) {
            m.category = "Maps";

            // Set clean, user-friendly display titles based on language
            if (m.language.toLower() == "sv" || m.name.compare("LDS-sv", Qt::CaseInsensitive) == 0) {
                m.description = "Bibliska kartor och fotografier";
            } else if (m.language.toLower() == "en" || m.name.compare("LDS-en", Qt::CaseInsensitive) == 0) {
                m.description = "Bible Maps and Photographs";
            } else {
                m.description = QString("Bible Maps (%1)").arg(m.language.toUpper());
            }
        }

        list.append(QVariant::fromValue(m));
    }

    return list;
}

bool BibleManager::isBibleModule(const QString &moduleName)
{
    // 1. Filtrera bort filer som uttryckligen innehåller commentary eller dictionary i namnet
    QString lowerName = moduleName.toLower();
    if (lowerName.contains("commentary") || lowerName.contains("dictionary")) {
        return false;
    }

    // 2. Öppna databasen med din befintliga getModuleDatabase-metod
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return false;

    // 3. Kontrollera att tabellen 'verses' faktiskt finns
    QStringList tables = db.tables();
    if (!tables.contains("verses", Qt::CaseInsensitive)) {
        return false;
    }

    // 4. Se till att 'verses'-tabellen faktiskt innehåller bibelverser (tvillingfiler är ofta tomma)
    QSqlQuery q(db);
    if (q.exec("SELECT COUNT(*) FROM verses") && q.next()) {
        int verseCount = q.value(0).toInt();
        if (verseCount == 0) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

QVariantList BibleManager::getInstalledBibles()
{
    QVariantList list;
    QVariantList allMods = getInstalledModules();

    for (const QVariant &v : allMods) {
        ModuleInfo m = v.value<ModuleInfo>();

        // Vi kollar både att kategorin är Bibles OCH att modulen faktiskt innehåller bibeltext
        if (m.category == "Bibles" && isBibleModule(m.name)) {
            QVariantMap map;
            map["moduleName"] = m.name;
            map["title"] = m.description;
            // Lägger till det mänskligt läsbara språknamnet (t.ex. "Svenska", "English")
            map["language"] = getLanguageDisplayName(m.language);
            list.append(map);
        }
    }

    return list;
}

QVariantList BibleManager::getInstalledCommentaries()
{
    QVariantList list;
    QVariantList allMods = getInstalledModules();

    for (const QVariant &v : allMods) {
        ModuleInfo m = v.value<ModuleInfo>();
        if (m.category == "Commentaries" || m.name.endsWith("-c", Qt::CaseInsensitive)) {
            QVariantMap map;
            map["moduleName"] = m.name;
            map["title"] = m.description;
            // Lägger till det mänskligt läsbara språknamnet här också
            map["language"] = getLanguageDisplayName(m.language);
            list.append(map);
        }
    }

    return list;
}

QVariantList BibleManager::getBibleBooks(const QString &moduleName)
{
    QVariantList books;
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return books;

    QSqlQuery q(db);
    bool hasBooksAll = q.exec("SELECT book_number, short_name, long_name FROM books_all WHERE is_present = 1 ORDER BY book_number ASC");

    if (!hasBooksAll || !q.next()) {
        q.exec("SELECT book_number, short_name, long_name FROM books ORDER BY book_number ASC");
    } else {
        q.exec("SELECT book_number, short_name, long_name FROM books_all WHERE is_present = 1 ORDER BY book_number ASC");
    }

    while (q.next()) {
        int bookNum = q.value(0).toInt();
        QString shortName = q.value(1).toString().trimmed();
        QString longName = q.value(2).toString().trimmed();

        QVariantMap bookMap;
        bookMap["bookId"] = getOSISBookId(bookNum);
        bookMap["bookName"] = !longName.isEmpty() ? longName : shortName;
        // NEW: Return the localized module abbreviation (e.g., "1 Mos" for Swedish, "Gen" for English)
        bookMap["shortBookName"] = !shortName.isEmpty() ? shortName : bookMap["bookName"].toString();

        books.append(bookMap);
    }

    return books;
}

int BibleManager::getChapterCount(const QString &moduleName, const QString &bookId)
{
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return 0;

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);
    q.prepare("SELECT MAX(chapter) FROM verses WHERE book_number = :book");
    q.bindValue(":book", bookNum);

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }

    return 0;
}

int BibleManager::getVerseCount(const QString &moduleName, const QString &bookId, int chapter)
{
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return 0;

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);
    q.prepare("SELECT MAX(verse) FROM verses WHERE book_number = :book AND chapter = :chapter");
    q.bindValue(":book", bookNum);
    q.bindValue(":chapter", chapter);

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }

    return 0;
}

QVariantList BibleManager::getChapterVerses(const QString &moduleName, const QString &bookId, int chapter)
{
    QVariantList versesList;
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return versesList;

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);
    q.prepare("SELECT verse, text FROM verses WHERE book_number = :book AND chapter = :chapter ORDER BY verse ASC");
    q.bindValue(":book", bookNum);
    q.bindValue(":chapter", chapter);

    if (!q.exec()) return versesList;

    // Check for accompanying footnote database (e.g., RFB.commentaries.SQLite3)
    QString commPath = m_modulesPath + "/" + moduleName + ".commentaries.SQLite3";
    QSqlDatabase commDb;
    bool hasFootnoteDb = false;

    if (QFile::exists(commPath)) {
        QString connName = "Footnotes_" + moduleName;
        if (QSqlDatabase::contains(connName)) {
            commDb = QSqlDatabase::database(connName);
        } else {
            commDb = QSqlDatabase::addDatabase("QSQLITE", connName);
            commDb.setDatabaseName(commPath);
        }
        hasFootnoteDb = commDb.open();
    }

    // Regex to detect explicit markers at the start of a string:
    // Captures circled letters/numbers (\u24B6-\u24E9, \u2460-\u2473), [1], (a), a), or *
    QRegExp markerExtractor("^\\s*([\u24B6-\u24E9\u2460-\u2473]|\\[\\d+\\]|\\([a-zA-Z0-9]+\\)|[a-zA-Z0-9]+[\\)\\.]|\\*)\\s*(.*)$");

    while (q.next()) {
        int verse = q.value(0).toInt();
        QString rawText = q.value(1).toString();

        if (rawText.trimmed().isEmpty()) continue;

        QStringList formattedFootnotes;
        QString displayText = rawText.trimmed();

        // 1. Process <f>...</f> and <n>...</n> tags
        QRegExp fTagRegex("<(f|n)>(.*?)</\\1>");
        int pos = 0;
        int autoIndex = 1;

        while ((pos = fTagRegex.indexIn(rawText, pos)) != -1) {
            QString innerText = fTagRegex.cap(2).trimmed();

            // Strip internal HTML/XML formatting from the note body
            QString cleanInner = innerText;
            cleanInner.remove(QRegExp("<[^>]*>")).trimmed();

            if (!cleanInner.isEmpty()) {
                QString markerSymbol;
                QString noteBody;

                if (markerExtractor.indexIn(cleanInner) != -1) {
                    // Marker symbol explicitly captured in Group 1
                    markerSymbol = markerExtractor.cap(1).trimmed();
                    // Remaining text captured in Group 2
                    noteBody = markerExtractor.cap(2).trimmed();
                } else {
                    // Fallback marker if no symbol was prefixed inside the tag
                    markerSymbol = QString("[%1]").arg(autoIndex++);
                    noteBody = cleanInner;
                }

                if (!noteBody.isEmpty()) {
                    formattedFootnotes.append(QString("<b>%1</b> %2").arg(markerSymbol, noteBody));
                } else {
                    formattedFootnotes.append(QString("<b>%1</b>").arg(markerSymbol));
                }
            }

            pos += fTagRegex.matchedLength();
        }

        // 2. Process cross-reference tags <r>...</r> and <p>...</p>
        QRegExp rTagRegex("<(r|p)>(.*?)</\\1>");
        pos = 0;
        while ((pos = rTagRegex.indexIn(rawText, pos)) != -1) {
            QString pContent = rTagRegex.cap(2).trimmed();
            pContent.remove(QRegExp("<[^>]*>")).trimmed();

            if (!pContent.isEmpty()) {
                formattedFootnotes.append(QString("<b>Ref:</b> %1").arg(pContent));
            }
            pos += rTagRegex.matchedLength();
        }

        // 3. Process external commentaries DB notes if present
        if (hasFootnoteDb) {
            QSqlQuery fq(commDb);
            fq.prepare("SELECT text FROM commentaries WHERE book_number = :book AND chapter_number_from = :chapter AND verse_number_from = :verse");
            fq.bindValue(":book", bookNum);
            fq.bindValue(":chapter", chapter);
            fq.bindValue(":verse", verse);
            if (fq.exec()) {
                while (fq.next()) {
                    QString sqlNote = fq.value(0).toString().trimmed();
                    sqlNote.remove(QRegExp("<[^>]*>")).trimmed();
                    if (!sqlNote.isEmpty()) {
                        formattedFootnotes.append(QString("<b>*</b> %1").arg(sqlNote));
                    }
                }
            }
        }

        // 4. Format verse text for display

        // Remove leading <pb/> tags so verse text isn't pushed below the verse number
        while (displayText.startsWith("<pb/>")) {
            displayText.remove(0, 5);
            displayText = displayText.trimmed();
        }

        // Clean out note/cross-reference content tags from main text
        displayText.replace(QRegExp("<(f|n|r|p)>.*?</\\1>"), " ");
        displayText.replace(QRegExp("</?(f|n|r|p)>"), " ");

        // Clean out morph/grammar tags if present (<m>...</m>)
        displayText.remove(QRegExp("<m>[^<]*</m>"));

        // Preserve <S> tag contents! (REMOVED: displayText.remove(QRegExp("<S>[^<]*</S>"));)

        // Styling tags
        displayText.replace("<J>", "<font color='#d62828'>").replace("</J>", "</font>");
        displayText.replace("<pb/>", "<br/><br/>").replace("<br/>", "<br/>");
        displayText.replace("<t>", "<span style='margin-left: 15px;'>").replace("</t>", "</span>");
        displayText.replace("&nbsp;", " ");

        QString combinedFootnote = formattedFootnotes.join("<br/>");
        bool hasFootnote = !combinedFootnote.trimmed().isEmpty();

        QVariantMap verseMap;
        verseMap["verse"] = verse;
        verseMap["text"] = displayText.trimmed();
        verseMap["footnote"] = combinedFootnote.trimmed();
        verseMap["hasFootnote"] = hasFootnote;
        verseMap["hasParallel"] = (rawText.contains("<r>") || rawText.contains("<p>"));

        versesList.append(verseMap);
    }

    return versesList;
}

QString BibleManager::renderChapterText(const QString &moduleName, const QString &bookId, int chapter)
{
    QVariantList verses = getChapterVerses(moduleName, bookId, chapter);
    QString htmlResult;

    for (const QVariant &v : verses) {
        QVariantMap vm = v.toMap();
        int verse = vm["verse"].toInt();
        QString text = vm["text"].toString();
        bool hasFootnote = vm["hasFootnote"].toBool();

        htmlResult.append(QString("<div style='margin-bottom: 8px;'><b>%1</b> %2").arg(verse).arg(text));
        if (hasFootnote) {
            htmlResult.append(" <b>*</b>");
        }
        htmlResult.append("</div>");
    }

    return htmlResult;
}

QString BibleManager::renderCommentaryText(const QString &moduleName,
                                            const QString &activeBibleModule,
                                            const QString &bookId,
                                            int chapter)
{
    qDebug() << "==========================================";
    qDebug() << "[DEBUG renderCommentaryText] Called with:";
    qDebug() << "  -> moduleName:" << moduleName;
    qDebug() << "  -> activeBibleModule:" << activeBibleModule;
    qDebug() << "  -> bookId:" << bookId << " (chapter:" << chapter << ")";

    if (moduleName.isEmpty() || bookId.isEmpty()) {
        qDebug() << "[DEBUG renderCommentaryText] ERROR: Empty moduleName or bookId!";
        return QString();
    }

    QSqlDatabase db = getCommentaryDatabase(moduleName);
    if (!db.isOpen()) {
        qDebug() << "[DEBUG renderCommentaryText] ERROR: Commentary DB not open for:" << moduleName;
        return QString();
    }

    int bookNum = getMyBibleBookNumber(bookId);
    qDebug() << "  -> Mapped bookNum:" << bookNum;
    if (bookNum <= 0) return QString();

    // Table detection
    QString tableName;
    if (db.tables().contains("commentaries", Qt::CaseInsensitive)) {
        tableName = "commentaries";
    } else if (db.tables().contains("commentary", Qt::CaseInsensitive)) {
        tableName = "commentary";
    } else {
        qDebug() << "[DEBUG renderCommentaryText] ERROR: No commentary table found!";
        return QString();
    }

    // Column mapping
    QSqlRecord rec = db.record(tableName);
    QStringList lowerCols;
    for (int i = 0; i < rec.count(); ++i) lowerCols << rec.fieldName(i).toLower();

    QString textCol = lowerCols.contains("text") ? "text" :
                      lowerCols.contains("commentary") ? "commentary" : "d";

    QString chapCol = lowerCols.contains("chapter_number_from") ? "chapter_number_from" :
                      lowerCols.contains("chapter_number") ? "chapter_number" : "chapter";

    QString vFromCol = lowerCols.contains("verse_number_from") ? "verse_number_from" :
                       lowerCols.contains("verse_from") ? "verse_from" :
                       lowerCols.contains("verse_number") ? "verse_number" : "verse";

    QString vToCol = lowerCols.contains("verse_number_to") ? "verse_number_to" :
                     lowerCols.contains("verse_to") ? "verse_to" : vFromCol;

    QSqlQuery q(db);
    QString sql = QString("SELECT %1, %2, %3 FROM %4 "
                          "WHERE book_number = :book AND %5 = :chap "
                          "ORDER BY %1 ASC")
                  .arg(vFromCol).arg(vToCol).arg(textCol).arg(tableName).arg(chapCol);

    q.prepare(sql);
    q.bindValue(":book", bookNum);
    q.bindValue(":chap", chapter);

    if (!q.exec()) {
        qDebug() << "[DEBUG renderCommentaryText] ERROR executing main SQL:" << q.lastError().text();
        return QString();
    }

    // Test Bible DB connection before entering loop
    if (!activeBibleModule.isEmpty()) {
        QSqlDatabase testBibleDb = getModuleDatabase(activeBibleModule);
        qDebug() << "  -> Testing Bible DB connection for" << activeBibleModule
                 << "isOpen:" << testBibleDb.isOpen();
    } else {
        qDebug() << "  -> WARNING: activeBibleModule is EMPTY!";
    }

    QString resultHtml;
    int commentCount = 0;
    int verseQuoteCount = 0;

    while (q.next()) {
        commentCount++;
        int vFrom = q.value(0).toInt();
        int rawVTo = q.value(1).toInt();
        int fetchTo = (rawVTo > 0) ? rawVTo : vFrom;

        QString commentText = q.value(2).toString();

        // Badge
        QString verseBadge;
        if (vFrom == 0) {
            verseBadge = "<b style='color: #008080;'>[Chapter Overview]</b><br/>";
        } else if (vFrom == fetchTo) {
            verseBadge = QString("<b style='color: #008080;'>[Verse %1]</b><br/>").arg(vFrom);
        } else {
            verseBadge = QString("<b style='color: #008080;'>[Verses %1–%2]</b><br/>").arg(vFrom).arg(fetchTo);
        }

        // Bible Verse Retrieval
        QString coveredVerseText;
        if (vFrom > 0 && !activeBibleModule.isEmpty()) {
            QSqlDatabase bibleDb = getModuleDatabase(activeBibleModule);
            if (bibleDb.isOpen()) {
                QSqlQuery bibleQuery(bibleDb);
                bibleQuery.prepare("SELECT verse, text FROM verses "
                                   "WHERE book_number = :book AND chapter = :chap "
                                   "AND verse >= :vFrom AND verse <= :vTo "
                                   "ORDER BY verse ASC");
                bibleQuery.bindValue(":book", bookNum);
                bibleQuery.bindValue(":chap", chapter);
                bibleQuery.bindValue(":vFrom", vFrom);
                bibleQuery.bindValue(":vTo", fetchTo);

                if (bibleQuery.exec()) {
                    while (bibleQuery.next()) {
                        verseQuoteCount++;
                        int vNum = bibleQuery.value("verse").toInt();
                        QString text = bibleQuery.value("text").toString();
                        coveredVerseText += QString("<p><b>%1:%2</b> %3</p>")
                                            .arg(chapter).arg(vNum).arg(text);
                    }
                } else {
                    qDebug() << "  -> ERROR in bibleQuery execution:" << bibleQuery.lastError().text();
                }
            }
        }

        resultHtml.append(verseBadge);
        if (!coveredVerseText.isEmpty()) {
            resultHtml.append(QString("<div style='color:#808080;'><i>%1</i></div><hr/>")
                              .arg(coveredVerseText));
        }
        resultHtml.append(commentText);
        resultHtml.append("<br/><hr/><br/>");
    }

    qDebug() << "[DEBUG renderCommentaryText] Processed" << commentCount
             << "comments and fetched" << verseQuoteCount << "verse quotes.";
    qDebug() << "==========================================";

    return resultHtml;
}

int BibleManager::getCommentaryCount(const QString &moduleName, const QString &bookId, int chapter)
{
    QSqlDatabase db = getCommentaryDatabase(moduleName);
    if (!db.isOpen()) return 0;

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM commentaries WHERE book_number = :book AND chapter_number_from = :chapter");
    q.bindValue(":book", bookNum);
    q.bindValue(":chapter", chapter);

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }

    return 0;
}

QVariantList BibleManager::parseCatalog()
{
    QVariantList list;

    QString catalogFilePath = m_catalogPath + "/registry.json";
    if (!QFile::exists(catalogFilePath)) {
        QDir dir(m_catalogPath);
        QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
        if (!jsonFiles.isEmpty()) {
            catalogFilePath = m_catalogPath + "/" + jsonFiles.first();
        } else {
            qDebug() << "[BibleManager] No JSON catalog found in:" << m_catalogPath;
            return list;
        }
    }

    QFile file(catalogFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[BibleManager] Failed to open catalog file:" << catalogFilePath;
        return list;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "[BibleManager] Invalid JSON in catalog.";
        return list;
    }

    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("downloads") || !rootObj["downloads"].isArray()) {
        qDebug() << "[BibleManager] 'downloads' array not found in registry.json.";
        return list;
    }

    QJsonArray downloadsArray = rootObj["downloads"].toArray();

    QVariantList installedMods = getInstalledModules();
    QSet<QString> installedNames;
    for (const QVariant &v : installedMods) {
        installedNames.insert(v.value<ModuleInfo>().name.toLower().trimmed());
    }

    for (const QJsonValue &val : downloadsArray) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        QString modName = obj["abr"].toString().trimmed();
        if (modName.isEmpty()) modName = obj["fil"].toString().trimmed();

        if (modName.endsWith(".zip", Qt::CaseInsensitive)) modName.chop(4);
        if (modName.endsWith(".SQLite3", Qt::CaseInsensitive)) modName.chop(8);

        if (modName.isEmpty()) continue;

        ModuleInfo m;
        m.id = modName;
        m.name = modName;

        QString rawLang = obj["lng"].toString().trimmed();
        if (rawLang.isEmpty()) rawLang = "en";
        m.language = rawLang;

        if (obj.contains("des") && !obj["des"].toString().trimmed().isEmpty()) {
            m.description = obj["des"].toString().trimmed();
        } else if (obj.contains("cmt") && !obj["cmt"].toString().trimmed().isEmpty()) {
            m.description = obj["cmt"].toString().trimmed();
        } else {
            m.description = modName;
        }

        QString rawFileName = obj["fil"].toString().trimmed();
        m.category = mapCategoryName(rawFileName);

        QString langDisplayName = getLanguageDisplayName(m.language);
        m.groupTitle = langDisplayName + " - " + m.category;

        if (obj.contains("upd")) m.date = obj["upd"].toString().trimmed();

        if (!rawFileName.endsWith(".zip", Qt::CaseInsensitive)) {
            rawFileName += ".zip";
        }
        m.downloadUrl = QString("http://mph4.ru/m/%1").arg(QString(QUrl::toPercentEncoding(rawFileName)));

        m.isInstalled = installedNames.contains(m.name.toLower().trimmed());

        list.append(QVariant::fromValue(m));
    }

    qDebug() << "[BibleManager] Successfully parsed" << list.size() << "modules from registry into language groups.";
    return list;
}

QVariantList BibleManager::getAvailableModules(const QString &languageFilter)
{
    QVariantList allModules = parseCatalog();
    if (languageFilter.isEmpty() || languageFilter.compare("All", Qt::CaseInsensitive) == 0) {
        return allModules;
    }

    QVariantList filtered;
    QString filter = languageFilter.toLower().trimmed();

    for (const QVariant &v : allModules) {
        ModuleInfo m = v.value<ModuleInfo>();
        if (m.language.toLower() == filter ||
            m.language.toLower().startsWith(filter) ||
            m.groupTitle.toLower().contains(filter)) {
            filtered.append(v);
        }
    }

    return filtered;
}

QVariantMap BibleManager::getVerseByOffset(const QString &moduleName, int offset)
{
    QVariantMap result;

    if (offset > 0) offset = 0;

    QString jsonPath = QString("/usr/share/%1/data/votd.json").arg("harbour-captains-bible");
    QFile file(jsonPath);
    if (!file.exists()) {
        file.setFileName("data/votd.json");
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[BibleManager] Unable to open votd.json at:" << jsonPath;
        result["reference"] = "Captain's Bible";
        result["text"] = "Select a Bible module to enable Verse of the Day.";
        return result;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return result;

    QJsonArray votdArray = doc.array();
    if (votdArray.isEmpty()) return result;

    int dayOfYear = QDate::currentDate().dayOfYear();
    int targetIndex = (dayOfYear + offset - 1) % votdArray.size();
    if (targetIndex < 0) targetIndex += votdArray.size();

    QJsonObject entry = votdArray.at(targetIndex).toObject();
    QString rawBookId = entry["bookId"].toString().trimmed();
    int chapter = entry["chapter"].toInt();
    int verse = entry["verse"].toInt();

    // Exakt kartläggning från förkortningar (svenska/engelska/OSIS) till MyBible book_number
    static const QMap<QString, int> bookNumberMap = {
        // Gamla Testamentet
        {"1Mos", 10}, {"1 Mos", 10}, {"Gen", 10},
        {"2Mos", 20}, {"2 Mos", 20}, {"Exo", 20},
        {"3Mos", 30}, {"3 Mos", 30}, {"Lev", 30},
        {"4Mos", 40}, {"4 Mos", 40}, {"Num", 40},
        {"5Mos", 50}, {"5 Mos", 50}, {"Deu", 50},
        {"Jos", 60},  {"Josh", 60},
        {"Dom", 70},  {"Jdg", 70},  {"Judg", 70},
        {"Rut", 80},  {"Ruth", 80},
        {"1Sam", 90}, {"1 Sam", 90}, {"1Sa", 90},
        {"2Sam", 100}, {"2 Sam", 100}, {"2Sa", 100},
        {"1Kung", 110}, {"1 Kung", 110}, {"1Kin", 110}, {"1Ki", 110},
        {"2Kung", 120}, {"2 Kung", 120}, {"2Kin", 120}, {"2Ki", 120},
        {"1Krön", 130}, {"1 Krön", 130}, {"1Chr", 130}, {"1Ch", 130},
        {"2Krön", 140}, {"2 Krön", 140}, {"2Chr", 140}, {"2Ch", 140},
        {"Esr", 150}, {"Ezr", 150},
        {"Neh", 160},
        {"Est", 170}, {"Esth", 170},
        {"Job", 220},
        {"Ps", 230},  {"Psl", 230},  {"Psa", 230},
        {"Ords", 240}, {"Prov", 240}, {"Pro", 240},
        {"Pred", 250}, {"Eccl", 250}, {"Ecc", 250},
        {"Högs", 260}, {"Song", 260}, {"Sol", 260},
        {"Jes", 290}, {"Isa", 290},
        {"Jer", 300},
        {"Klag", 310}, {"Lam", 310},
        {"Hes", 330}, {"Ezek", 330}, {"Eze", 330},
        {"Dan", 340},
        {"Hos", 350},
        {"Joel", 360}, {"Joe", 360},
        {"Am", 370},  {"Amo", 370},
        {"Ob", 380},  {"Oba", 380},
        {"Jona", 390}, {"Jon", 390},
        {"Mik", 400}, {"Mic", 400},
        {"Nah", 410},
        {"Hab", 420},
        {"Sef", 430}, {"Zeph", 430}, {"Zep", 430},
        {"Hagg", 440}, {"Hag", 440},
        {"Sak", 450}, {"Zech", 450}, {"Zec", 450},
        {"Mal", 460},

        // Nya Testamentet
        {"Matt", 470}, {"Mat", 470},
        {"Mark", 480}, {"Mar", 480},
        {"Luk", 490},
        {"Joh", 500}, {"John", 500}, {"Jhn", 500},
        {"Apg", 510}, {"Acts", 510}, {"Act", 510}, // Apg / Acts -> 510
        {"Rom", 520}, {"Ro", 520}, {"Room", 520}, {"Romans", 520}, {"Roms", 520},
        {"1Kor", 530}, {"1 Kor", 530}, {"1Cor", 530}, {"1Co", 530},
        {"2Kor", 540}, {"2 Kor", 540}, {"2Cor", 540}, {"2Co", 540},
        {"Gal", 550},
        {"Eph", 560}, {"Ef", 560}, {"Efes", 560}, {"Ephes", 560}, {"Ephesians", 560},
        {"Fil", 570}, {"Phil", 570}, {"Php", 570},
        {"Kol", 580}, {"Col", 580},
        {"1Tess", 590}, {"1 Tess", 590}, {"1Ths", 590}, {"1Th", 590},
        {"2Tess", 600}, {"2 Tess", 600}, {"2Ths", 600}, {"2Th", 600},
        {"1Tim", 610}, {"1 Tim", 610}, {"1Tim", 610}, {"1Ti", 610},
        {"2Tim", 620}, {"2 Tim", 620}, {"2Tim", 620}, {"2Ti", 620},
        {"Tit", 630},
        {"Filem", 640}, {"Phlm", 640},
        {"Heb", 650},
        {"Jak", 660}, {"Jam", 660},  {"Jas", 660},
        {"1Pet", 670}, {"1 Pet", 670}, {"1Pet", 670}, {"1Pe", 670},
        {"2Pet", 680}, {"2 Pet", 680}, {"2Pet", 680}, {"2Pe", 680},
        {"1Joh", 690}, {"1 Joh", 690}, {"1Jn", 690},
        {"2Joh", 700}, {"2 Joh", 700}, {"2Jn", 700},
        {"3Joh", 710}, {"3 Joh", 710}, {"3Jn", 710},
        {"Jud", 720},
        {"Upp", 730}, {"Rev", 730}
    };

    int bookNum = bookNumberMap.value(rawBookId, -1);

    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) {
        result["reference"] = QString("%1 %2:%3").arg(rawBookId).arg(chapter).arg(verse);
        result["text"] = "Select a Bible translation to load verse text.";
        return result;
    }

    QString bookDisplay = rawBookId;

    // 1. Fetch the book display name directly from the module via book_number
    if (bookNum > 0) {
        QSqlQuery qBook(db);
        qBook.prepare("SELECT short_name, long_name FROM books WHERE book_number = :num LIMIT 1");
        qBook.bindValue(":num", bookNum);

        if (qBook.exec() && qBook.next()) {
            QString shortName = qBook.value(0).toString().trimmed();
            QString fullName = qBook.value(1).toString().trimmed();
            // Prefer short_name (e.g. "1 Kor") over long_name ("Första Korintierbrevet")
            bookDisplay = !shortName.isEmpty() ? shortName : (!fullName.isEmpty() ? fullName : rawBookId);
        }
    } else {
        // Fallback search via short_name
        QSqlQuery qBook(db);
        qBook.prepare("SELECT book_number, short_name, long_name FROM books WHERE short_name = :id LIMIT 1");
        qBook.bindValue(":id", rawBookId);
        if (qBook.exec() && qBook.next()) {
            bookNum = qBook.value(0).toInt();
            QString shortName = qBook.value(1).toString().trimmed();
            QString fullName = qBook.value(2).toString().trimmed();
            bookDisplay = !shortName.isEmpty() ? shortName : (!fullName.isEmpty() ? fullName : rawBookId);

        }
     }

    QString text = "";

    // 2. Hämta vertexten från verses-tabellen med det exakta book_number
    if (bookNum > 0) {
        QSqlQuery q(db);
        q.prepare("SELECT text FROM verses WHERE book_number = :book AND chapter = :chapter AND verse = :verse");
        q.bindValue(":book", bookNum);
        q.bindValue(":chapter", chapter);
        q.bindValue(":verse", verse);

        if (q.exec() && q.next()) {
            text = q.value(0).toString();

            qDebug() << "[BibleManager RAW VERSE]" << text;
        }
    }

    if (!text.isEmpty()) {
        // Rensa bort HTML/XML-taggar och fotnoter
        text.remove(QRegExp("<[^>]*>"));
        text.remove(QRegExp("[\u24B6-\u24E9\u2460-\u2473]"));
        text.remove(QRegExp("\\[\\d+\\]"));

        result["reference"] = QString("%1 %2:%3").arg(bookDisplay).arg(chapter).arg(verse);
        result["text"] = text.trimmed();
        result["bookId"] = rawBookId;
        result["chapter"] = chapter;
    } else {
        qWarning() << "[BibleManager] Failed VOTD for" << moduleName << "Book:" << rawBookId << "Num:" << bookNum;
        result["reference"] = QString("%1 %2:%3").arg(bookDisplay).arg(chapter).arg(verse);
        result["text"] = "Verse text not found in selected Bible.";
    }

    return result;
}

void BibleManager::fetchRemoteCatalog()
{
    if (m_isFetchingCatalog) return;

    m_isFetchingCatalog = true;
    emit isFetchingCatalogChanged();

    qDebug() << "[BibleManager] Fetching MyBible master registry archive...";

    QString catalogUrl = "http://mph4.ru/registry.zip";

    QUrl url(catalogUrl);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HarbourCaptainsBible/1.0 (Sailfish OS)");

    QNetworkReply *reply = m_networkManager.get(request);
    m_replyToModuleNameMap[reply] = "__CATALOG__";
}

void BibleManager::downloadModule(const QString &moduleName)
{
    if (m_downloadingModules.contains(moduleName)) return;

    m_downloadingModules.insert(moduleName);
    emit downloadingModulesChanged();
    emit downloadStarted(moduleName);

    QVariantList allMods = parseCatalog();
    QString downloadUrl;
    for (const QVariant &v : allMods) {
        ModuleInfo m = v.value<ModuleInfo>();
        if (m.name.compare(moduleName, Qt::CaseInsensitive) == 0) {
            downloadUrl = m.downloadUrl;
            break;
        }
    }

    if (downloadUrl.isEmpty()) {
        downloadUrl = QString("http://mph4.ru/m/%1.zip").arg(QString(QUrl::toPercentEncoding(moduleName)));
    }

    qDebug() << "[BibleManager] Downloading MyBible module:" << moduleName << "from:" << downloadUrl;

    QUrl url(downloadUrl);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HarbourCaptainsBible/1.0 (Sailfish OS)");

    QNetworkReply *reply = m_networkManager.get(request);
    m_replyToModuleNameMap[reply] = moduleName;
}

void BibleManager::deleteModule(const QString &moduleName)
{
    qDebug() << "[BibleManager] Removing module:" << moduleName;

    QDirIterator it(m_modulesPath, QStringList() << moduleName + "*.SQLite3" << moduleName + "*.sqlite3" << moduleName + "*.SQLite" << moduleName + "*.sqlite",
                     QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QFile::remove(it.next());
    }

    emit catalogUpdated();
}

void BibleManager::onDownloadFinished(QNetworkReply *reply)
{
    if (!m_replyToModuleNameMap.contains(reply)) {
        reply->deleteLater();
        return;
    }

    QString target = m_replyToModuleNameMap.take(reply);

    if (target == "__CATALOG__") {
        m_isFetchingCatalog = false;
        emit isFetchingCatalogChanged();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QString tempZipPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/registry.zip";
            QFile tempFile(tempZipPath);

            if (tempFile.open(QIODevice::WriteOnly)) {
                tempFile.write(data);
                tempFile.close();

                bool success = ZipExtractor::extract(tempZipPath, m_catalogPath);
                QFile::remove(tempZipPath);

                if (success) {
                    qDebug() << "[BibleManager] Master catalog registry extracted successfully.";
                } else {
                    qDebug() << "[BibleManager] Failed to extract registry.zip archive.";
                }
            }
        } else {
            qDebug() << "[BibleManager] Failed to fetch catalog zip:" << reply->errorString();
        }

        emit catalogUpdated();
        reply->deleteLater();
        return;
    }

    QString moduleName = target;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[BibleManager] Download error for module:" << moduleName << reply->errorString();
        m_downloadingModules.remove(moduleName);
        emit downloadingModulesChanged();
        emit downloadFinished(moduleName, false);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QString tempZipPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + moduleName + ".zip";
    QFile tempFile(tempZipPath);

    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(data);
        tempFile.close();

        bool success = ZipExtractor::extract(tempZipPath, m_modulesPath, moduleName);
        qDebug() << "[BibleManager] Module extraction status for" << moduleName << ":" << success;

        QFile::remove(tempZipPath);
        m_downloadingModules.remove(moduleName);
        emit downloadingModulesChanged();
        emit downloadFinished(moduleName, success);
        emit catalogUpdated();
    } else {
        m_downloadingModules.remove(moduleName);
        emit downloadingModulesChanged();
        emit downloadFinished(moduleName, false);
    }

    reply->deleteLater();
}

QVariantList BibleManager::getInstalledMaps()
{
    QVariantList list;

    QDirIterator it(m_modulesPath, QStringList() << "*LDS*.SQLite3" << "*LDS*.sqlite3",
                     QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();

        QString modName = fileInfo.completeBaseName();

        QSqlDatabase db = getModuleDatabase(modName);
        QString description = "Bibliska kartor och fotografier";

        if (db.isOpen()) {
            QSqlQuery q(db);
            if (q.exec("SELECT value FROM details WHERE name = 'description' LIMIT 1") && q.next()) {
                QString val = q.value(0).toString();
                if (!val.isEmpty()) {
                    description = val;
                }
            }
        }

        // Sätt en tydlig beskrivning om det är standardnamn
        if (modName.compare("LDS-sv", Qt::CaseInsensitive) == 0) {
            description = "Bibliska kartor och fotografier";
        } else if (modName.compare("LDS-en", Qt::CaseInsensitive) == 0) {
            description = "Bible Maps and Photographs";
        }

        QVariantMap map;
        map["moduleName"] = modName;
        map["title"] = description;
        map["language"] = modName.mid(4, 2).toLower(); // T.ex. "sv", "en", "fi"
        list.append(map);
    }

    return list;
}

QVariantList BibleManager::getMapTopics(const QString &moduleName)
{
    QVariantList topics;
    if (moduleName.isEmpty()) return topics;

    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return topics;

    QSqlQuery q(db);
    // Standard MyBible dictionary table query
    if (q.exec("SELECT topic FROM dictionary ORDER BY topic ASC")) {
        while (q.next()) {
            topics.append(q.value(0).toString());
        }
    }
    return topics;
}

QString BibleManager::getMapDefinition(const QString &moduleName, const QString &topic)
{
    if (moduleName.isEmpty() || topic.isEmpty()) return QString();

    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return QString();

    QSqlQuery q(db);
    q.prepare("SELECT definition FROM dictionary WHERE topic = :topic LIMIT 1");
    q.bindValue(":topic", topic);

    if (q.exec() && q.next()) {
        QString html = q.value(0).toString();

        // 1. Skapa cache-mapp för bilder
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/map_images";
        QDir().mkpath(cacheDir);

        // 2. Matcha <img ... src="data:image/..." ... >
        static QRegularExpression regExp("src=\\\"data:image\\/(png|jpg|jpeg);base64,([^\"]+)\\\"");
        QRegularExpressionMatchIterator it = regExp.globalMatch(html);

        int imgIndex = 0;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString format = match.captured(1);
            QString base64Data = match.captured(2);

            // Generera ett unikt och säkert filnamn
            QString fileName = QString("%1_%2_%3.%4")
                               .arg(moduleName)
                               .arg(topic)
                               .arg(imgIndex++)
                               .arg(format);

            fileName.replace(QRegularExpression("[^a-zA-Z0-9_\\.]"), "_");
            QString filePath = cacheDir + "/" + fileName;

            // --- OPTIMERING: CACHE CHECK ---
            // Om bilden redan finns på disken slipper vi dekoda Base64 och skriva till disk!
            if (!QFile::exists(filePath)) {
                QByteArray imgData = QByteArray::fromBase64(base64Data.toUtf8());
                QFile file(filePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(imgData);
                    file.close();
                }
            }

            QString fileUrl = "file://" + filePath;

            // Tvinga bildens bredd i HTML-taggen till 100% av Label-komponentens bredd
            html.replace(match.captured(0), QString("src=\"%1\" width=\"100%\"").arg(fileUrl));
        }

        return html;
    }

    return QString();
}

QString BibleManager::getCoverVerseText(const QString &moduleName, int bookNumber, int chapter, int verse)
{
    if (moduleName.isEmpty()) return QString();

    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return QString();

    QSqlQuery q(db);

    // 1. First attempt: Query using exact book_number
    q.prepare("SELECT text FROM verses WHERE book_number = :book AND chapter = :chap AND verse = :v LIMIT 1");
    q.bindValue(":book", bookNumber);
    q.bindValue(":chap", chapter);
    q.bindValue(":v", verse);

    QString resultText = "";

    if (q.exec() && q.next()) {
        resultText = q.value(0).toString();
    } else {
        // 2. Fallback: If exact book_number missed due to non-standard indexing, log debug info
        qDebug() << "[Cover Debug] Failed lookup for book:" << bookNumber << "chap:" << chapter << "verse:" << verse;
    }

    if (resultText.isEmpty()) {
        return QString();
    }

    // 3. Clean up raw HTML tags / XML formatting so QML Label doesn't break
    resultText.remove(QRegularExpression("<[^>]*>")); // Strip HTML tags
    resultText = resultText.trimmed();

    return resultText;
}

QString BibleManager::getTranslatedBookName(const QString &moduleName, const QString &bookId)
{
    if (moduleName.isEmpty() || bookId.isEmpty()) return bookId;

    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return bookId;

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);

    // Try standard MyBible books table schema
    q.prepare("SELECT long_name, short_name FROM books WHERE book_number = :book");
    q.bindValue(":book", bookNum);

    if (q.exec() && q.next()) {
        QString longName = q.value(0).toString().trimmed();
        QString shortName = q.value(1).toString().trimmed();
        if (!longName.isEmpty()) return longName;
        if (!shortName.isEmpty()) return shortName;
    }

    return bookId;
}

QVariantList BibleManager::searchBible(const QString &moduleName,
                                       const QString &searchTerm,
                                       int scopeIndex,
                                       const QString &currentBookId,
                                       bool exactMatch,
                                       int limit,
                                       int offset)
{
    QVariantList results;
    QString cleanTerm = searchTerm.trimmed();
    if (moduleName.isEmpty() || cleanTerm.isEmpty()) return results;

    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return results;

    // 1. Determine Scope SQL Condition using MyBible 10-based numbering
    QString scopeCondition;
    int currentBookNum = getMyBibleBookNumber(currentBookId);

    switch (scopeIndex) {
        case 0:  scopeCondition = QString("book_number = %1").arg(currentBookNum); break; // Current Book
        case 1:  scopeCondition = "book_number BETWEEN 10 AND 460"; break;                // Old Testament
        case 2:  scopeCondition = "book_number BETWEEN 10 AND 50"; break;                 // Pentateuch
        case 3:  scopeCondition = "book_number BETWEEN 60 AND 190"; break;                // Historic
        case 4:  scopeCondition = "book_number BETWEEN 220 AND 260"; break;               // Poetic
        case 5:  scopeCondition = "book_number BETWEEN 290 AND 460"; break;               // Prophets
        case 6:  scopeCondition = "book_number BETWEEN 470 AND 730"; break;               // New Testament
        case 7:  scopeCondition = "book_number BETWEEN 470 AND 500"; break;               // Gospels
        case 8:  scopeCondition = "book_number BETWEEN 520 AND 720"; break;               // Epistles
        default: scopeCondition = "book_number >= 10"; break;                             // Entire Bible
    }

    // 2. Determine Match Type SQL Condition
    QString matchCondition;
    if (exactMatch) {
        matchCondition = "(text LIKE :pattern1 OR text LIKE :pattern2 OR text LIKE :pattern3 OR text = :exact)";
    } else {
        matchCondition = "text LIKE :pattern";
    }

    QString sql = QString("SELECT book_number, chapter, verse, text FROM verses "
                          "WHERE %1 AND %2 "
                          "ORDER BY book_number ASC, chapter ASC, verse ASC "
                          "LIMIT %3 OFFSET %4")
                  .arg(scopeCondition)
                  .arg(matchCondition)
                  .arg(limit)
                  .arg(offset);

    QSqlQuery q(db);
    q.prepare(sql);

    if (exactMatch) {
        q.bindValue(":pattern1", cleanTerm + " %");
        q.bindValue(":pattern2", "% " + cleanTerm + " %");
        q.bindValue(":pattern3", "% " + cleanTerm);
        q.bindValue(":exact", cleanTerm);
    } else {
        q.bindValue(":pattern", "%" + cleanTerm + "%");
    }

    if (q.exec()) {
        while (q.next()) {
            int bNum = q.value(0).toInt();
            int chap = q.value(1).toInt();
            int vNum = q.value(2).toInt();
            QString rawText = q.value(3).toString();

            // Highlight Search Term
            QString highlightedText = rawText;
            QRegularExpression re(QString("(%1)").arg(QRegularExpression::escape(cleanTerm)),
                                 QRegularExpression::CaseInsensitiveOption);

             highlightedText.replace(re, "<font color='#008080'><b>\\1</b></font>");

            // Use your existing getOSISBookId method here:
            QString bId = getOSISBookId(bNum);
            QString localizedBookName = getTranslatedBookName(moduleName, bId);

            QVariantMap item;
            item["bookNumber"] = bNum;
            item["bookId"] = bId;
            item["bookName"] = localizedBookName;
            item["chapter"] = chap;
            item["verse"] = vNum;
            item["snippet"] = highlightedText;

            results.append(item);
        }
    }

    return results;
}

QVariantList BibleManager::getCommentariesForVerse(const QString &bookId, int chapter, int verse)
{
    QVariantList list;
    if (bookId.isEmpty()) return list;

    int bookNum = getMyBibleBookNumber(bookId);
    QDir dir(m_modulesPath);

    // Target commentary files with the '-c' naming convention
    QStringList filters;
    filters << "*-c.SQLite3" << "*-c.sqlite3"
            << "*-c.SQLite"  << "*-c.sqlite";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fileInfo : files) {
        QString moduleName = fileInfo.completeBaseName();

        QSqlDatabase db = getCommentaryDatabase(moduleName);
        if (!db.isOpen()) continue;

        QStringList tables = db.tables();

        // Ensure the database contains the 'commentaries' table
        if (!tables.contains("commentaries", Qt::CaseInsensitive)) {
            continue;
        }

        // Query if a commentary note exists for this specific verse or verse range
        QSqlQuery q(db);
        q.prepare("SELECT COUNT(*) FROM commentaries "
                  "WHERE book_number = :book "
                  "AND ((chapter_number_from < :chap OR (chapter_number_from = :chap AND verse_number_from <= :verse)) "
                  "AND (chapter_number_to > :chap OR (chapter_number_to = :chap AND verse_number_to >= :verse)))");
        q.bindValue(":book", bookNum);
        q.bindValue(":chap", chapter);
        q.bindValue(":verse", verse);

        int count = 0;
        if (q.exec() && q.next()) {
            count = q.value(0).toInt();
        }

        // Alternate schema check (verse_from / verse_to)
        if (count == 0) {
            q.prepare("SELECT COUNT(*) FROM commentaries "
                      "WHERE book_number = :book "
                      "AND ((chapter_number_from < :chap OR (chapter_number_from = :chap AND verse_from <= :verse)) "
                      "AND (chapter_number_to > :chap OR (chapter_number_to = :chap AND verse_to >= :verse)))");
            q.bindValue(":book", bookNum);
            q.bindValue(":chap", chapter);
            q.bindValue(":verse", verse);

            if (q.exec() && q.next()) {
                count = q.value(0).toInt();
            }
        }

        // Retrieve human-readable title and language from details/info table
        QString title = moduleName;
        QString language = "en";

        QSqlQuery metaQuery(db);
        if (metaQuery.exec("SELECT name, value FROM details")) {
            while (metaQuery.next()) {
                QString key = metaQuery.value(0).toString().toLower();
                if (key == "description") title = metaQuery.value(1).toString();
                else if (key == "language") language = metaQuery.value(1).toString();
            }
        } else if (metaQuery.exec("SELECT name, value FROM info")) {
            while (metaQuery.next()) {
                QString key = metaQuery.value(0).toString().toLower();
                if (key == "description") title = metaQuery.value(1).toString();
                else if (key == "language") language = metaQuery.value(1).toString();
            }
        }

        QVariantMap item;
        item["moduleName"] = moduleName;
        item["title"] = title;
        item["language"] = getLanguageDisplayName(language);
        item["commentCount"] = count; // Shows count in ModuleSelectPage

        list.append(item);
    }

    return list;
}

QString BibleManager::getVerseCommentaryText(const QString &commentaryModule,
                                             const QString &activeBibleModule,
                                             const QString &bookId,
                                             int chapter,
                                             int verse)
{
    if (commentaryModule.isEmpty() || bookId.isEmpty()) {
        return QString();
    }

    int bookNum = getMyBibleBookNumber(bookId);
    if (bookNum <= 0) {
        qDebug() << "[BibleManager] Invalid book mapping for bookId:" << bookId;
        return QString();
    }

    QSqlDatabase db = getCommentaryDatabase(commentaryModule);
    if (!db.isOpen()) {
        qDebug() << "[BibleManager] Database not open for commentary module:" << commentaryModule;
        return QString();
    }

    // 1. Resolve table name ("commentaries" or "commentary")
    QString tableName;
    if (db.tables().contains("commentaries", Qt::CaseInsensitive)) {
        tableName = "commentaries";
    } else if (db.tables().contains("commentary", Qt::CaseInsensitive)) {
        tableName = "commentary";
    } else {
        qDebug() << "[BibleManager] ERROR: No commentary table found in module:" << commentaryModule;
        return QString();
    }

    // 2. Resolve columns in lowercase for case-insensitive matching
    QSqlRecord rec = db.record(tableName);
    QStringList lowerCols;
    for (int i = 0; i < rec.count(); ++i) {
        lowerCols << rec.fieldName(i).toLower();
    }

    // Text column
    QString textCol = "text";
    if (lowerCols.contains("text")) textCol = "text";
    else if (lowerCols.contains("commentary")) textCol = "commentary";
    else if (lowerCols.contains("d")) textCol = "d";

    // Chapter column (Geneva uses chapter_number_from)
    QString chapCol = lowerCols.contains("chapter_number_from") ? "chapter_number_from" :
                      lowerCols.contains("chapter_number") ? "chapter_number" : "chapter";

    // Verse range columns
    QString vFromCol = lowerCols.contains("verse_number_from") ? "verse_number_from" :
                       lowerCols.contains("verse_from") ? "verse_from" :
                       lowerCols.contains("verse_number") ? "verse_number" : "verse";

    QString vToCol = lowerCols.contains("verse_number_to") ? "verse_number_to" :
                     lowerCols.contains("verse_to") ? "verse_to" : vFromCol;

    QSqlQuery q(db);
    int vFrom = 0;
    int vTo = 0;
    QString rawCommentary;
    bool foundMatch = false;

    // --- QUERY: Fetch all commentary entries for this Book and Chapter ---
    // Mirroring renderCommentaryText's proven approach
    QString sql = QString("SELECT %1, %2, %3 FROM %4 "
                          "WHERE book_number = :book AND %5 = :chap "
                          "ORDER BY %1 ASC")
                  .arg(vFromCol)
                  .arg(vToCol)
                  .arg(textCol)
                  .arg(tableName)
                  .arg(chapCol);

    q.prepare(sql);
    q.bindValue(":book", bookNum);
    q.bindValue(":chap", chapter);

    if (q.exec()) {
        while (q.next()) {
            int entryFrom = q.value(0).toInt();
            int entryTo = q.value(1).toInt();
            if (entryTo == 0) entryTo = entryFrom;

            // Check if requested verse falls within this commentary entry's range
            if ((entryFrom == 0 && verse == 0) ||
                (verse >= entryFrom && verse <= entryTo) ||
                (entryFrom == 0 && !foundMatch)) { // Fallback to chapter overview if verse specific isn't found
                vFrom = entryFrom;
                vTo = entryTo;
                rawCommentary = q.value(2).toString();
                foundMatch = true;

                // Exact verse or range match takes precedence
                if (verse >= entryFrom && verse <= entryTo && entryFrom > 0) {
                    break;
                }
            }
        }
    }

    if (!foundMatch || rawCommentary.trimmed().isEmpty()) {
        qDebug() << "[BibleManager] No commentary entry found for B:" << bookNum << "C:" << chapter << "V:" << verse << "in module:" << commentaryModule;
        return QString();
    }

    // --- HTML RENDER & BIBLE VERSE EMBEDDING ---
    QString verseBadge;
    if (vFrom == 0) {
        verseBadge = "<b style='color: #008080;'>[Chapter Overview]</b><br/>";
    } else if (vFrom == vTo) {
        verseBadge = QString("<b style='color: #008080;'>[Verse %1]</b><br/>").arg(vFrom);
    } else {
        verseBadge = QString("<b style='color: #008080;'>[Verses %1–%2]</b><br/>").arg(vFrom).arg(vTo);
    }

    QString coveredVerseText;
    if (!activeBibleModule.isEmpty()) {
        QSqlDatabase bibleDb = getModuleDatabase(activeBibleModule);
        if (bibleDb.isOpen()) {
            QSqlQuery bibleQuery(bibleDb);
            int fetchTo = (vTo > 0) ? vTo : vFrom;
            bibleQuery.prepare("SELECT verse, text FROM verses "
                               "WHERE book_number = :book AND chapter = :chap "
                               "AND verse >= :vFrom AND verse <= :vTo "
                               "ORDER BY verse ASC");
            bibleQuery.bindValue(":book", bookNum);
            bibleQuery.bindValue(":chap", chapter);
            bibleQuery.bindValue(":vFrom", vFrom);
            bibleQuery.bindValue(":vTo", fetchTo);

            if (bibleQuery.exec()) {
                while (bibleQuery.next()) {
                    int vNum = bibleQuery.value("verse").toInt();
                    QString text = bibleQuery.value("text").toString();
                    coveredVerseText += QString("<p><b>%1:%2</b> %3</p>")
                                        .arg(chapter)
                                        .arg(vNum)
                                        .arg(text);
                }
            }
        }
    }

    QString formattedOutput;
    formattedOutput += verseBadge;
    if (!coveredVerseText.isEmpty()) {
        formattedOutput += QString("<div style='color:#808080;'><i>%1</i></div><hr/>")
                           .arg(coveredVerseText);
    }
    formattedOutput += rawCommentary;

    return formattedOutput;
}

QVariantList BibleManager::getDictionaryModules()
{
    QVariantList list;
    QDir dir(m_modulesPath);

    // Scan all SQLite files in the modules directory
    QStringList filters;
    filters << "*.SQLite3" << "*.sqlite3" << "*.SQLite" << "*.sqlite";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fileInfo : files) {
        QString fileName = fileInfo.fileName();
        QString moduleName = fileInfo.completeBaseName();

        // Skip files explicitly marked as commentaries
        if (fileName.contains(".commentaries.", Qt::CaseInsensitive) ||
            fileName.endsWith("-c.SQLite3", Qt::CaseInsensitive) ||
            fileName.endsWith("-c.sqlite3", Qt::CaseInsensitive)) {
            continue;
        }

        QString connectionName = "MyBible_Dict_" + moduleName;
        QSqlDatabase db;
        if (QSqlDatabase::contains(connectionName)) {
            db = QSqlDatabase::database(connectionName);
        } else {
            db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(fileInfo.absoluteFilePath());
            if (!db.open()) continue;
        }

        QStringList tables = db.tables();

        // Must contain a 'dictionary' table and MUST NOT contain a 'verses' table
        bool hasDictTable = tables.contains("dictionary", Qt::CaseInsensitive);
        bool hasVersesTable = tables.contains("verses", Qt::CaseInsensitive);

        if (!hasDictTable || hasVersesTable) {
            continue; // Skip Bibles, Commentaries, and other non-dictionary databases
        }

        QString title = moduleName;
        QString language = "en";

        QSqlQuery metaQuery(db);
        if (metaQuery.exec("SELECT name, value FROM info")) {
            while (metaQuery.next()) {
                QString key = metaQuery.value(0).toString().toLower();
                if (key == "description") title = metaQuery.value(1).toString();
                else if (key == "language") language = metaQuery.value(1).toString();
            }
        } else if (metaQuery.exec("SELECT name, value FROM details")) {
            while (metaQuery.next()) {
                QString key = metaQuery.value(0).toString().toLower();
                if (key == "description") title = metaQuery.value(1).toString();
                else if (key == "language") language = metaQuery.value(1).toString();
            }
        }

        QVariantMap item;
        item["moduleName"] = moduleName;
        item["title"] = title;
        item["language"] = getLanguageDisplayName(language);
        list.append(item);
    }

    return list;
}

QString BibleManager::getDictionaryDefinition(const QString &moduleName, const QString &topic)
{
    qDebug() << "[BibleManager] getDictionaryDefinition requested -> Module:" << moduleName << "| Topic:" << topic;

    if (moduleName.trimmed().isEmpty()) {
        qDebug() << "[BibleManager] Error: moduleName is empty!";
        return QString();
    }

    // Locate module file dynamically
    QDir dir(m_modulesPath);
    QStringList filters;
    filters << moduleName + ".SQLite3" << moduleName + ".sqlite3"
            << moduleName + ".dictionary.SQLite3" << moduleName + ".dictionary.sqlite3"
            << moduleName + ".SQLite" << moduleName + ".sqlite";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    if (files.isEmpty()) {
        qDebug() << "[BibleManager] Error: Could not find dictionary file for module:" << moduleName;
        return QString();
    }

    QString connectionName = "MyBible_Dict_" + moduleName;
    QSqlDatabase db;

    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(files.first().absoluteFilePath());
    }

    if (!db.isOpen() && !db.open()) {
        qDebug() << "[BibleManager] Error: Failed to open SQLite database for connection:" << connectionName;
        return QString();
    }

    QSqlQuery q(db);

    // Identify actual column names in the 'dictionary' table (schema tolerance)
    QString topicCol = "topic";
    QString defCol = "definition";

    QSqlRecord rec = db.record("dictionary");
    if (!rec.isEmpty()) {
        if (rec.contains("word")) topicCol = "word";
        else if (rec.contains("topic")) topicCol = "topic";

        if (rec.contains("definition")) defCol = "definition";
        else if (rec.contains("text")) defCol = "text";
        else if (rec.contains("d")) defCol = "d";
    }

    // Case 1: If topic is empty, fetch the first available record in the table
    if (topic.trimmed().isEmpty()) {
        QString defaultQueryStr = QString("SELECT %1 FROM dictionary LIMIT 1").arg(defCol);
        if (q.exec(defaultQueryStr) && q.next()) {
            qDebug() << "[BibleManager] Loaded fallback (first entry) from dictionary.";
            return q.value(0).toString();
        }
        return QString();
    }

    QString searchTerm = topic.trimmed();

    // Case 2: Direct case-insensitive search
    QString queryStr = QString("SELECT %1 FROM dictionary WHERE lower(%2) = lower(:topic) LIMIT 1")
                             .arg(defCol, topicCol);
    q.prepare(queryStr);
    q.bindValue(":topic", searchTerm);

    if (q.exec() && q.next()) {
        qDebug() << "[BibleManager] Found exact match for topic:" << searchTerm;
        return q.value(0).toString();
    }

    // Case 3: Strong's number variant match (stripping H/G prefixes and zero-padding)
    QString cleanNum = searchTerm;
    cleanNum.remove(QRegExp("^[HGhg]"));

    if (!cleanNum.isEmpty() && cleanNum.at(0).isDigit()) {
        int numVal = cleanNum.toInt();
        QString paddedNum4 = QString("%1").arg(numVal, 4, 10, QChar('0'));
        QString paddedNum5 = QString("%1").arg(numVal, 5, 10, QChar('0'));
        QString strongH = "H" + QString::number(numVal);
        QString strongG = "G" + QString::number(numVal);

        QString strongQueryStr = QString("SELECT %1 FROM dictionary WHERE %2 = :num "
                                         "OR %2 = :padded4 OR %2 = :padded5 "
                                         "OR %2 = :strongH OR %2 = :strongG LIMIT 1")
                                         .arg(defCol, topicCol);
        q.prepare(strongQueryStr);
        q.bindValue(":num", cleanNum);
        q.bindValue(":padded4", paddedNum4);
        q.bindValue(":padded5", paddedNum5);
        q.bindValue(":strongH", strongH);
        q.bindValue(":strongG", strongG);

        if (q.exec() && q.next()) {
            qDebug() << "[BibleManager] Found Strong's match for code:" << searchTerm;
            return q.value(0).toString();
        }
    }

    // Case 4: Prefix/LIKE search fallback
    QString likeQueryStr = QString("SELECT %1 FROM dictionary WHERE %2 LIKE :likeTerm LIMIT 1")
                               .arg(defCol, topicCol);
    q.prepare(likeQueryStr);
    q.bindValue(":likeTerm", searchTerm + "%");

    if (q.exec() && q.next()) {
        qDebug() << "[BibleManager] Found partial LIKE match for:" << searchTerm;
        return q.value(0).toString();
    }

    q.clear();
    qDebug() << "[BibleManager] No definition found for topic:" << searchTerm << "in module:" << moduleName;
    return QString();
}

QVariantList BibleManager::searchDictionary(const QString &moduleName, const QString &query)
{
    QVariantList list;
    if (moduleName.isEmpty() || query.trimmed().isEmpty()) return list;

    QDir dir(m_modulesPath);
    QStringList filters;
    filters << moduleName + ".SQLite3" << moduleName + ".sqlite3"
            << moduleName + ".dictionary.SQLite3" << moduleName + ".dictionary.sqlite3"
            << moduleName + ".SQLite" << moduleName + ".sqlite";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    if (files.isEmpty()) return list;

    QString connectionName = "MyBible_Dict_" + moduleName;
    QSqlDatabase db;
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(files.first().absoluteFilePath());
        if (!db.open()) return list;
    }

    QSqlQuery q(db);
    q.prepare("SELECT topic FROM dictionary WHERE topic LIKE :q ORDER BY topic ASC LIMIT 100");
    q.bindValue(":q", query.trimmed() + "%");

    if (q.exec()) {
        while (q.next()) {
            list.append(q.value(0).toString());
        }
    }

    return list;
}

QVariantList BibleManager::searchAllDictionaries(const QString &query)
{
    QVariantList results;
    QString searchTerm = query.trimmed();
    if (searchTerm.length() < 2) return results;

    // Hämta alla tillgängliga ordböcker
    QVariantList dictModules = getDictionaryModules();

    for (const QVariant &dictVar : dictModules) {
        QVariantMap dictMap = dictVar.toMap();
        QString moduleName = dictMap["moduleName"].toString();
        QString dictTitle = dictMap["title"].toString();

        QDir dir(m_modulesPath);
        QStringList filters;
        filters << moduleName + ".SQLite3" << moduleName + ".sqlite3"
                << moduleName + ".dictionary.SQLite3" << moduleName + ".dictionary.sqlite3"
                << moduleName + ".SQLite" << moduleName + ".sqlite";

        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        if (files.isEmpty()) continue;

        QString connectionName = "MyBible_Dict_" + moduleName;
        QSqlDatabase db;
        if (QSqlDatabase::contains(connectionName)) {
            db = QSqlDatabase::database(connectionName);
        } else {
            db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(files.first().absoluteFilePath());
            if (!db.open()) continue;
        }

        QSqlQuery q(db);
        // Sök strikt i topic-kolumnen (termer/uppslagsord)
        q.prepare("SELECT topic FROM dictionary WHERE topic LIKE :q ORDER BY topic ASC LIMIT 20");
        q.bindValue(":q", searchTerm + "%");

        if (q.exec()) {
            while (q.next()) {
                QVariantMap item;
                item["moduleName"] = moduleName;
                item["dictTitle"] = dictTitle;
                item["topic"] = q.value(0).toString();
                results.append(item);
            }
        }
    }

    return results;
}

QString BibleManager::findStrongDictionaryModule(const QString &code)
{
    QVariantList dicts = getDictionaryModules();
    if (dicts.isEmpty()) return QString();

    // 1. Klicka först i den senast använda ordboken om den har koden
    for (const QVariant &v : dicts) {
        QString modName = v.toMap()["moduleName"].toString();
        QString def = getDictionaryDefinition(modName, code);
        if (!def.isEmpty()) {
            return modName; // Hittade modulen som faktiskt svarar på koden!
        }
    }

    return QString(); // Ingen installerad ordbok innehöll koden
}

QSqlDatabase BibleManager::getUserNotesDatabase()
{
    QString connName = "UserNotesDatabase";
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) return db;
    }

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/user_notes.db";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "[BibleManager] Failed to open user notes database at:" << dbPath;
    } else {
        initUserNotesDatabase();
    }
    return db;
}

void BibleManager::initUserNotesDatabase()
{
    QSqlDatabase db = QSqlDatabase::database("UserNotesDatabase");
    if (!db.isOpen()) return;

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS notes ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
           "book_id TEXT NOT NULL, "
           "chapter INTEGER NOT NULL, "
           "verse_start INTEGER NOT NULL, "
           "verse_end INTEGER NOT NULL, "
           "topic TEXT NOT NULL DEFAULT 'Verse Notes', "
           "title TEXT NOT NULL, "
           "body TEXT NOT NULL, "
           "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
           ");");
}

bool BibleManager::saveNote(const QString &bookId, int chapter, int verseStart, int verseEnd,
                            const QString &topic, const QString &title, const QString &body)
{
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("SELECT id FROM notes WHERE book_id = :book AND chapter = :chap AND verse_start = :vStart AND verse_end = :vEnd");
    q.bindValue(":book", bookId);
    q.bindValue(":chap", chapter);
    q.bindValue(":vStart", verseStart);
    q.bindValue(":vEnd", verseEnd);

    if (q.exec() && q.next()) {
        int noteId = q.value(0).toInt();
        QSqlQuery uq(db);
        uq.prepare("UPDATE notes SET topic = :topic, title = :title, body = :body WHERE id = :id");
        uq.bindValue(":topic", topic.isEmpty() ? "Verse Notes" : topic);
        uq.bindValue(":title", title);
        uq.bindValue(":body", body);
        uq.bindValue(":id", noteId);
        return uq.exec();
    } else {
        QSqlQuery iq(db);
        iq.prepare("INSERT INTO notes (book_id, chapter, verse_start, verse_end, topic, title, body) "
                   "VALUES (:book, :chap, :vStart, :vEnd, :topic, :title, :body)");
        iq.bindValue(":book", bookId);
        iq.bindValue(":chap", chapter);
        iq.bindValue(":vStart", verseStart);
        iq.bindValue(":vEnd", verseEnd);
        iq.bindValue(":topic", topic.isEmpty() ? "Verse Notes" : topic);
        iq.bindValue(":title", title);
        iq.bindValue(":body", body);
        return iq.exec();
    }
}

QVariantMap BibleManager::getNoteForVerse(const QString &bookId, int chapter, int verse)
{
    QVariantMap res;
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return res;

    QSqlQuery q(db);
    q.prepare("SELECT id, topic, title, body, created_at FROM notes "
              "WHERE book_id = :book AND chapter = :chap AND :v >= verse_start AND :v <= verse_end "
              "ORDER BY id DESC LIMIT 1");
    q.bindValue(":book", bookId);
    q.bindValue(":chap", chapter);
    q.bindValue(":v", verse);

    if (q.exec() && q.next()) {
        res["id"] = q.value(0).toInt();
        res["topic"] = q.value(1).toString();
        res["title"] = q.value(2).toString();
        res["body"] = q.value(3).toString();
        res["createdAt"] = q.value(4).toString();
    }
    return res;
}

QVariantList BibleManager::getAllNotes(const QString &topicFilter)
{
    QVariantList list;
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return list;

    QSqlQuery q(db);
    if (!topicFilter.isEmpty() && topicFilter.compare("All", Qt::CaseInsensitive) != 0) {
        q.prepare("SELECT id, book_id, chapter, verse_start, verse_end, topic, title, body, created_at FROM notes WHERE topic = :topic ORDER BY id DESC");
        q.bindValue(":topic", topicFilter);
    } else {
        q.prepare("SELECT id, book_id, chapter, verse_start, verse_end, topic, title, body, created_at FROM notes ORDER BY id DESC");
    }

    if (q.exec()) {
        while (q.next()) {
            QVariantMap item;
            item["id"] = q.value(0).toInt();
            item["bookId"] = q.value(1).toString();
            item["chapter"] = q.value(2).toInt();
            item["verseStart"] = q.value(3).toInt();
            item["verseEnd"] = q.value(4).toInt();
            item["topic"] = q.value(5).toString();
            item["title"] = q.value(6).toString();
            item["body"] = q.value(7).toString();
            item["createdAt"] = q.value(8).toString();
            list.append(item);
        }
    }
    return list;
}

bool BibleManager::deleteNote(int noteId)
{
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM notes WHERE id = :id");
    q.bindValue(":id", noteId);
    return q.exec();
}

QString BibleManager::formatVersesForClipboard(const QString &moduleName, const QString &bookId,
                                              const QString &bookName, int chapter,
                                              int verseStart, int verseEnd,
                                              int placementMode, bool showVerseNumbers,
                                              const QString &delimiter, const QString &rangeDash)
{
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return QString();

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);
    q.prepare("SELECT verse, text FROM verses WHERE book_number = :book AND chapter = :chap AND verse >= :vFrom AND verse <= :vTo ORDER BY verse ASC");
    q.bindValue(":book", bookNum);
    q.bindValue(":chap", chapter);
    q.bindValue(":vFrom", verseStart);
    q.bindValue(":vTo", verseEnd);

    if (!q.exec()) return QString();

    QString passageText;
    while (q.next()) {
        int vNum = q.value(0).toInt();
        QString raw = q.value(1).toString();

        // 1. Remove whole footnote, cross-reference, and Strong's number tags along with their contents
        raw.remove(QRegExp("<f>.*?</f>", Qt::CaseInsensitive));
        raw.remove(QRegExp("<r>.*?</r>", Qt::CaseInsensitive));
        raw.remove(QRegExp("<S>.*?</S>", Qt::CaseInsensitive));

        // 2. Remove any remaining HTML/XML tags
        raw.remove(QRegExp("<[^>]*>"));

        // 3. Clean up extra whitespaces and internal linebreaks
        raw = raw.trimmed().simplified();

        if (showVerseNumbers) {
            passageText += QString("%1 %2\n").arg(vNum).arg(raw);
        } else {
            passageText += QString("%1\n").arg(raw);
        }
    }

    passageText = passageText.trimmed();

    // Use dynamic book name from module metadata + configured delimiters
    QString sep = delimiter.isEmpty() ? ":" : delimiter;
    QString dash = rangeDash.isEmpty() ? "–" : rangeDash;

    QString refStr;
    if (verseStart == verseEnd) {
        refStr = QString("%1 %2%3%4").arg(bookName).arg(chapter).arg(sep).arg(verseStart);
    } else {
        refStr = QString("%1 %2%3%4%5%6").arg(bookName).arg(chapter).arg(sep).arg(verseStart).arg(dash).arg(verseEnd);
    }

    // placementMode: 0 = Header, 1 = Footer
    if (placementMode == 0) {
        return QString("%1\n%2").arg(refStr, passageText);
    } else {
        return QString("%1\n— %2").arg(passageText, refStr);
    }
}

QStringList BibleManager::getNoteTopics()
{
    QStringList topics;
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return topics;

    QSqlQuery q(db);
    if (q.exec("SELECT DISTINCT topic FROM notes ORDER BY topic ASC")) {
        while (q.next()) {
            QString t = q.value(0).toString().trimmed();
            if (!t.isEmpty()) {
                topics.append(t);
            }
        }
    }
    return topics;
}

QVariantList BibleManager::getTopicFolders()
{
    QVariantList folders;
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return folders;

    QSqlQuery q(db);
    // Hämtar unika ämnen (topics) och räknar antalet anteckningar per ämne
    if (q.exec("SELECT topic, COUNT(*) as note_count FROM notes GROUP BY topic ORDER BY topic ASC")) {
        while (q.next()) {
            QVariantMap map;
            map["topicName"] = q.value("topic").toString();
            map["noteCount"] = q.value("note_count").toInt();
            folders.append(map);
        }
    }
    return folders;
}

QVariantList BibleManager::getNotesForTopic(const QString &topic)
{
    QVariantList notes;
    QSqlDatabase db = getUserNotesDatabase();
    if (!db.isOpen()) return notes;

    QSqlQuery q(db);
    q.prepare("SELECT id, book_id, chapter, verse_start, verse_end, title, body, topic "
              "FROM notes WHERE topic = :topic ORDER BY id DESC");
    q.bindValue(":topic", topic);

    if (q.exec()) {
        while (q.next()) {
            QVariantMap map;
            map["id"] = q.value("id").toInt();
            map["bookId"] = q.value("book_id").toString();
            map["chapter"] = q.value("chapter").toInt();
            map["verseStart"] = q.value("verse_start").toInt();
            map["verseEnd"] = q.value("verse_end").toInt();
            map["title"] = q.value("title").toString();
            map["body"] = q.value("body").toString();
            map["topic"] = q.value("topic").toString();
            notes.append(map);
        }
    }
    return notes;
}

QString BibleManager::getShortBookName(const QString &moduleName, const QString &bookId)
{
    QSqlDatabase db = getModuleDatabase(moduleName);
    if (!db.isOpen()) return bookId;

    int bookNum = getMyBibleBookNumber(bookId); // Reverse lookup OSIS -> book_number
    if (bookNum <= 0) return bookId;

    QSqlQuery q(db);
    q.prepare("SELECT short_name, long_name FROM books_all WHERE book_number = :num AND is_present = 1");
    q.bindValue(":num", bookNum);

    if (!q.exec() || !q.next()) {
        q.prepare("SELECT short_name, long_name FROM books WHERE book_number = :num");
        q.bindValue(":num", bookNum);
        if (!q.exec() || !q.next()) return bookId;
    }

    QString shortName = q.value(0).toString().trimmed();
    QString longName = q.value(1).toString().trimmed();

    if (!shortName.isEmpty()) return shortName;
    if (!longName.isEmpty()) return longName;
    return bookId;
}

void BibleManager::refreshModules()
{
    emit catalogUpdated();
}
