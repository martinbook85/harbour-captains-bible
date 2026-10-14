#ifndef ZIP_EXTRACTOR_H
#define ZIP_EXTRACTOR_H

#include <QString>

class ZipExtractor {
public:
    static bool extract(const QString &zipFilePath, const QString &destinationDir, const QString &moduleName = QString());
};

#endif // ZIP_EXTRACTOR_H
