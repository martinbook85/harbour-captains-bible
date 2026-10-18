#ifndef BIBLEMANAGER_H
#define BIBLEMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>

struct ModuleInfo {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString category MEMBER category)
    Q_PROPERTY(QString language MEMBER language)
    Q_PROPERTY(QString groupTitle MEMBER groupTitle)
    Q_PROPERTY(QString date MEMBER date)
    Q_PROPERTY(bool isInstalled MEMBER isInstalled)
    Q_PROPERTY(QString downloadUrl MEMBER downloadUrl)

public:
    QString id;
    QString name;
    QString description;
    QString category;
    QString language;
    QString groupTitle;
    QString date;
    bool isInstalled;
    QString downloadUrl;
};
Q_DECLARE_METATYPE(ModuleInfo)

class BibleManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isFetchingCatalog READ isFetchingCatalog NOTIFY isFetchingCatalogChanged)

public:
    explicit BibleManager(QObject *parent = nullptr);
    ~BibleManager();

    bool isFetchingCatalog() const { return m_isFetchingCatalog; }

    Q_INVOKABLE QVariantList getInstalledModules();
    Q_INVOKABLE QVariantList getInstalledBibles();
    Q_INVOKABLE QVariantList getInstalledCommentaries();
    Q_INVOKABLE QVariantList getAvailableModules(const QString &languageFilter = QString());
    Q_INVOKABLE QVariantList getInstalledMaps();
    Q_INVOKABLE QVariantList getBibleBooks(const QString &moduleName);
    Q_INVOKABLE int getChapterCount(const QString &moduleName, const QString &bookId);
    Q_INVOKABLE int getVerseCount(const QString &moduleName, const QString &bookId, int chapter);
    Q_INVOKABLE QVariantList getChapterVerses(const QString &moduleName, const QString &bookId, int chapter);
    Q_INVOKABLE QString renderChapterText(const QString &moduleName, const QString &bookId, int chapter);
    Q_INVOKABLE QString renderCommentaryText(const QString &moduleName, const QString &bookId, int chapter);
    Q_INVOKABLE QVariantList getMapTopics(const QString &moduleName);
    Q_INVOKABLE QString getMapDefinition(const QString &moduleName, const QString &topic);
    Q_INVOKABLE void fetchRemoteCatalog();
    Q_INVOKABLE void downloadModule(const QString &moduleName);
    Q_INVOKABLE void deleteModule(const QString &moduleName);
    Q_INVOKABLE void refreshModules();
    Q_INVOKABLE bool isModuleDownloading(const QString &moduleName) const;
    Q_INVOKABLE QVariantMap getVerseByOffset(const QString &moduleName, int offset);
    Q_INVOKABLE QString getCoverVerseText(const QString &moduleName, int bookNumber, int chapter, int verse);
    Q_INVOKABLE int getCommentaryCount(const QString &moduleName, const QString &bookId, int chapter);
    Q_INVOKABLE QVariantList searchBible(const QString &moduleName,
                                         const QString &searchTerm,
                                         int scopeIndex,
                                         const QString &currentBookId,
                                         bool exactMatch,
                                         int limit = 100,
                                         int offset = 0);
    Q_INVOKABLE QString getTranslatedBookName(const QString &moduleName, const QString &bookId);

signals:
    void catalogUpdated();
    void isFetchingCatalogChanged();
    void downloadingModulesChanged();
    void downloadStarted(const QString &moduleName);
    void downloadFinished(const QString &moduleName, bool success);

private slots:
    void onDownloadFinished(QNetworkReply *reply);

private:
    QSqlDatabase getModuleDatabase(const QString &moduleName);
    QSqlDatabase getCommentaryDatabase(const QString &moduleName);
    QString mapCategoryName(const QString &typeSuffix);
    QString getLanguageDisplayName(const QString &languageCode);
    int getMyBibleBookNumber(const QString &bookId);
    QString getOSISBookId(int myBibleBookNumber);
    QVariantList parseCatalog();

    QString m_modulesPath;
    QString m_catalogPath;
    bool m_isFetchingCatalog;

    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, QString> m_replyToModuleNameMap;
    QSet<QString> m_downloadingModules;
};

#endif // BIBLEMANAGER_H
