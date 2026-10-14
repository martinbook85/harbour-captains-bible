#ifndef MODULEINFO_H
#define MODULEINFO_H

#include <QString>
#include <QObject>

struct ModuleInfo {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString category MEMBER category)
    Q_PROPERTY(QString language MEMBER language)
    Q_PROPERTY(bool isInstalled MEMBER isInstalled)

public:
    QString name;        // ex "Swe1998"
    QString description; // ex "Svenska Folkbibeln 1998"
    QString category;    // ex "Biblia", "Commentaries", "Lexicons"
    QString language;    // ex "sv"
    bool isInstalled = false;
};

Q_DECLARE_METATYPE(ModuleInfo)

#endif // MODULEINFO_H
