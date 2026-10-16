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

QVariantList BibleManager::getInstalledBibles()
{
    QVariantList list;
    QVariantList allMods = getInstalledModules();

    for (const QVariant &v : allMods) {
        ModuleInfo m = v.value<ModuleInfo>();
        if (m.category == "Bibles") {
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
        QString shortName = q.value(1).toString();
        QString longName = q.value(2).toString();

        QVariantMap bookMap;
        bookMap["bookId"] = getOSISBookId(bookNum);
        bookMap["bookName"] = !longName.trimmed().isEmpty() ? longName : shortName;

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
        QString displayText = rawText;

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

        // 4. Format verse text for display (keep inline markers intact in main text)
        displayText.replace(QRegExp("</?(f|n|r|p)>"), " ");
        displayText.remove(QRegExp("<S>[^<]*</S>"));
        displayText.remove(QRegExp("<m>[^<]*</m>"));
        displayText.replace("<J>", "<font color='#d62828'>").replace("</J>", "</font>");
        displayText.replace("<pb/>", "<br/><br/>").replace("<br/>", "<br/>");
        displayText.replace("<t>", "<div style='margin-left: 15px;'>").replace("</t>", "</div>");
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

        htmlResult.append(QString("<p style='margin-bottom: 8px;'><b>%1</b> %2").arg(verse).arg(text));
        if (hasFootnote) {
            htmlResult.append(" <b>*</b>");
        }
        htmlResult.append("</p>");
    }

    return htmlResult;
}

QString BibleManager::renderCommentaryText(const QString &moduleName, const QString &bookId, int chapter)
{
    QSqlDatabase db = getCommentaryDatabase(moduleName);
    if (!db.isOpen()) return QString();

    int bookNum = getMyBibleBookNumber(bookId);
    QSqlQuery q(db);
    q.prepare("SELECT text FROM commentaries WHERE book_number = :book AND chapter_number_from = :chapter");
    q.bindValue(":book", bookNum);
    q.bindValue(":chapter", chapter);

    if (!q.exec()) return QString();

    QString resultHtml;
    while (q.next()) {
        resultHtml.append(q.value(0).toString());
        resultHtml.append("<br/><hr/><br/>");
    }

    return resultHtml;
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
        {"Rom", 520}, {"Ro", 520}, {"Room", 520}, {"Romans", 520},
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

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QRegularExpression>

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

            QByteArray imgData = QByteArray::fromBase64(base64Data.toUtf8());

            QString fileName = QString("%1_%2_%3.%4")
                               .arg(moduleName)
                               .arg(topic)
                               .arg(imgIndex++)
                               .arg(format);

            fileName.replace(QRegularExpression("[^a-zA-Z0-9_\\.]"), "_");
            QString filePath = cacheDir + "/" + fileName;

            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(imgData);
                file.close();
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

void BibleManager::refreshModules()
{
    emit catalogUpdated();
}
