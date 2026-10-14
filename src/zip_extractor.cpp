#include "zip_extractor.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <zlib.h>

bool ZipExtractor::extract(const QString &zipFilePath, const QString &destinationDir, const QString &moduleName)
{
    QFile zipFile(zipFilePath);
    if (!zipFile.open(QIODevice::ReadOnly)) {
        qDebug() << "[ZipExtractor] Kan inte öppna zip-fil:" << zipFilePath;
        return false;
    }

    QDir destDir(destinationDir);
    if (!destDir.exists()) {
        destDir.mkpath(".");
    }

    QByteArray zipData = zipFile.readAll();
    zipFile.close();

    if (zipData.isEmpty()) {
        qDebug() << "[ZipExtractor] Zip-filen är tom!";
        return false;
    }

    int fileCount = 0;
    int pos = 0;

    while (pos < zipData.size() - 30) {
        const char *data = zipData.constData() + pos;
        quint32 sig = *reinterpret_cast<const quint32*>(data);

        if (sig != 0x04034b50) {
            pos++;
            continue;
        }

        quint16 flags = *reinterpret_cast<const quint16*>(data + 6);
        quint16 compression = *reinterpret_cast<const quint16*>(data + 8);
        quint32 compressedSize = *reinterpret_cast<const quint32*>(data + 18);
        quint32 uncompressedSize = *reinterpret_cast<const quint32*>(data + 22);
        quint16 fileNameLen = *reinterpret_cast<const quint16*>(data + 26);
        quint16 extraLen = *reinterpret_cast<const quint16*>(data + 28);

        bool hasDataDescriptor = (flags & 0x08) != 0;
        int headerSize = 30 + fileNameLen + extraLen;

        if (pos + headerSize > zipData.size()) break;

        QString fileName = QString::fromUtf8(zipData.mid(pos + 30, fileNameLen)).trimmed();
        int dataOffset = pos + headerSize;

        // Om filen är en katalog
        if (fileName.endsWith('/')) {
            destDir.mkpath(fileName);
            pos += headerSize;
            continue;
        }

        // --- NORMERING AV FILNAMN ---
        // Om filnamnet startar med punkt (t.ex. ".SQLite3") och vi har ett modulnamn (t.ex. "RFB")
        if (!moduleName.isEmpty()) {
            if (fileName.startsWith('.')) {
                fileName = moduleName + fileName;
            } else if (!fileName.contains(moduleName, Qt::CaseInsensitive) &&
                       (fileName.endsWith(".SQLite3", Qt::CaseInsensitive) || fileName.endsWith(".sqlite3", Qt::CaseInsensitive))) {
                // Om filen heter typ "commentaries.SQLite3", gör om till "RFB.commentaries.SQLite3"
                fileName = moduleName + "." + fileName;
            }
        }

        QFileInfo fileInfo(destDir.filePath(fileName));
        destDir.mkpath(fileInfo.absolutePath());

        if (hasDataDescriptor && compressedSize == 0) {
            int nextSigPos = zipData.indexOf("\x50\x4b\x03\x04", dataOffset);
            if (nextSigPos == -1) {
                nextSigPos = zipData.indexOf("\x50\x4b\x01\x02", dataOffset);
            }
            if (nextSigPos != -1) {
                compressedSize = nextSigPos - dataOffset;
            }
        }

        if (dataOffset + compressedSize > static_cast<quint32>(zipData.size())) {
            qDebug() << "[ZipExtractor] Ogiltig datastorlek för fil:" << fileName;
            pos += headerSize;
            continue;
        }

        QByteArray compressedData = zipData.mid(dataOffset, compressedSize);
        QFile outFile(fileInfo.absoluteFilePath());

        if (!outFile.open(QIODevice::WriteOnly)) {
            qDebug() << "[ZipExtractor] Kan inte skapa målfil:" << fileInfo.absoluteFilePath();
            pos = dataOffset + compressedSize;
            continue;
        }

        if (compression == 0) { // Okomprimerad
            outFile.write(compressedData);
            outFile.close();
            fileCount++;
            qDebug() << "[ZipExtractor] Extraherade (uncompressed):" << fileInfo.absoluteFilePath();
        } else if (compression == 8) { // Deflate
            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = compressedData.size();
            strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressedData.constData()));

            if (inflateInit2(&strm, -15) != Z_OK) {
                qDebug() << "[ZipExtractor] inflateInit2 misslyckades för:" << fileName;
                outFile.close();
                pos = dataOffset + compressedSize;
                continue;
            }

            QByteArray uncompressedData;
            int bufferSize = (uncompressedSize > 0) ? uncompressedSize : compressedData.size() * 6;
            uncompressedData.resize(bufferSize);

            strm.avail_out = uncompressedData.size();
            strm.next_out = reinterpret_cast<Bytef*>(uncompressedData.data());

            int res = inflate(&strm, Z_FINISH);
            inflateEnd(&strm);

            if (res == Z_STREAM_END || res == Z_OK) {
                if (strm.total_out > 0) {
                    uncompressedData.resize(strm.total_out);
                }
                outFile.write(uncompressedData);
                outFile.close();
                fileCount++;
                qDebug() << "[ZipExtractor] Extraherade (deflated):" << fileInfo.absoluteFilePath();
            } else {
                qDebug() << "[ZipExtractor] Inflate felkod:" << res << "för fil:" << fileName;
                outFile.close();
            }
        }

        pos = dataOffset + compressedSize;
    }

    qDebug() << "[ZipExtractor] Extrahering klar! Antal filer extraherade:" << fileCount;
    return (fileCount > 0);
}
