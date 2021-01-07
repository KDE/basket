/**
 * SPDX-FileCopyrightText: (C) 2021 Sebastian Engel <dev@sebastianengel.eu>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KTar>

#include <QObject>
#include <QtTest/QtTest>

#include <algorithm>
#include <archive.h>
#include <memory>

class ArchiveTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void testExtractArchive();
    void testCreateArchive();

    void cleanupTestCase();

private:
    bool compareDirTree(const QString &toTestPath, const QString &referencePath);
    bool compareDirHashes(const QString &toTestPath, const QString &referencePath);
    QStringList createDirTree(const QString &path, bool returnRelativePaths);
    //    QByteArray fileHash(const QString &path, QCryptographicHash::Algorithm hashAlgorithm);
};

QTEST_MAIN(ArchiveTest)

void ArchiveTest::testExtractArchive()
{
    const QString referencePath = QFINDTESTDATA("archive/");

    // Test the Archive::extractArchive function
    QString referenceSource = QDir::currentPath() + QDir::separator() + "sample_source";
    QString testArchive = referencePath + "sample.baskets";
    QString testPath = QDir::currentPath() + QDir::separator() + "sample/";
    Archive::IOErrorCode ioCode = Archive::extractArchive(testArchive, testPath, false);

    QVERIFY2(ioCode == Archive::IOErrorCode::NoError, "An issue occured while extracting .baskets archive");
    QVERIFY2(compareDirTree(testPath, referenceSource), "Extracted .baskets archive is not identical with the reference source");
    QVERIFY2(compareDirHashes(testPath, referenceSource), "Extracted .baskets archive content is not identical with the reference source (hashes different)");

    QDir dir(testPath);
    dir.removeRecursively();

    // Test the Archive::extractArchive function with incompatible file
    testArchive = referencePath + "notABasket.baskets";
    testPath = QStringLiteral("notABasket/");
    ioCode = Archive::extractArchive(testArchive, testPath, false);

    QVERIFY2(ioCode == Archive::IOErrorCode::NotABasketArchive, ".baskets was not recoqnized as incompatible baskets file");

    // Test the Archive::extractArchive function with incompatible version
    testArchive = referencePath + "incompatible.baskets";
    testPath = QStringLiteral("incompatible/");
    ioCode = Archive::extractArchive(testArchive, testPath, false);

    QVERIFY2(ioCode == Archive::IOErrorCode::IncompatibleBasketVersion, ".baskets was not recoqnized as incompatible baskets file");

    /// \todo prepare more tests
    // Test the Archive::extractArchive function with corrupt file
    // Test the Archive::extractArchive function with 0-length preview
    // Test the Archive::extractArchive function with 0-length tar archive
    // Test the Archive::extractArchive function with non integer values as size
}

void ArchiveTest::testCreateArchive()
{
    const QString referencePath = QFINDTESTDATA("archive/");

    // Test the Archive::createArchiveFromSource function

    QString referenceSource = referencePath + "sample.baskets";
    QString testArchive = "test.baskets";

    QFile(testArchive).remove();

    QString testSourcePath = QDir::currentPath() + QDir::separator() + "sample_source/";
    Archive::IOErrorCode ioCode = Archive::createArchiveFromSource(testSourcePath, testSourcePath + "preview.png", testArchive);

    QVERIFY2(ioCode == Archive::IOErrorCode::NoError, "An issue occured while creating a .baskets archive");

    /// \todo find a simple way to compare created archive. KTar could write specific meta data...

    //    auto hashRef = fileHash(referenceSource, QCryptographicHash::Sha256);
    //    auto hashTest = fileHash(testArchive, QCryptographicHash::Sha256);

    QFile testArchiveFile(testArchive);
    testArchiveFile.remove();

    //    QVERIFY2(hashRef == hashTest, "Created .baskets archive is not identical with the reference (hashes different)");
}

void ArchiveTest::initTestCase()
{
    const QString referenceData = QFINDTESTDATA("archive/sample_source.tar.gz");

    KTar archive(referenceData, "application/x-gzip");

    // Open the archive
    archive.open(QIODevice::ReadOnly);
    QString destination = QDir::currentPath();
    archive.directory()->copyTo(destination, true);
    archive.close();
}

void ArchiveTest::cleanupTestCase()
{
    QStringList toDeleteDirectories{"sample_source", "sample", "notABasket", "incompatible"};

    std::for_each(toDeleteDirectories.begin(), toDeleteDirectories.end(), [](const QString &path) {
        QDir dir(path);
        dir.removeRecursively();
    });

    QStringList toDeleteFiles{"test.baskets"};

    std::for_each(toDeleteDirectories.begin(), toDeleteDirectories.end(), [](const QString &path) {
        QFile file(path);
        file.remove();
    });
}

bool ArchiveTest::compareDirTree(const QString &toTestPath, const QString &referencePath)
{
    const auto testPathDirTree = createDirTree(toTestPath, true);
    const auto referencePathDirTree = createDirTree(referencePath, true);

    if (testPathDirTree.size() != referencePathDirTree.size()) {
        return false;
    }

    // until C++14, this test is required for using std::mismatch
    if (testPathDirTree.size() > referencePathDirTree.size()) {
        return false;
    }

    auto mismatched = std::mismatch(testPathDirTree.begin(), testPathDirTree.end(), referencePathDirTree.begin());

    return !(mismatched.first != testPathDirTree.end() || mismatched.second != referencePathDirTree.end());
}

QStringList ArchiveTest::createDirTree(const QString &path, const bool returnRelativePaths)
{
    QStringList dirTree;
    QDir dir(path);

    QDirIterator it(dir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);

    if (returnRelativePaths) {
        while (it.hasNext()) {
            dirTree.append(dir.relativeFilePath(it.next()));
        }
    } else {
        while (it.hasNext()) {
            dirTree.append(it.next());
        }
    }

    dirTree.sort();

    return dirTree;
}

bool ArchiveTest::compareDirHashes(const QString &toTestPath, const QString &referencePath)
{
    const auto testPathDirTree = createDirTree(toTestPath, false);
    const auto referencePathDirTree = createDirTree(referencePath, false);

    if (testPathDirTree.size() != referencePathDirTree.size()) {
        return false;
    }

    // until C++14, this test is required for using std::mismatch
    if (testPathDirTree.size() > referencePathDirTree.size()) {
        return false;
    }

    QCryptographicHash hashTest(QCryptographicHash::Sha256);
    QCryptographicHash hashReference(QCryptographicHash::Sha256);

    std::for_each(testPathDirTree.begin(), testPathDirTree.end(), [&](const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            hashTest.addData(&file);
            file.close();
        }
    });

    std::for_each(referencePathDirTree.begin(), referencePathDirTree.end(), [&](const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            hashTest.addData(&file);
            file.close();
        }
    });

    return hashTest.result() == hashReference.result();
}

// QByteArray ArchiveTest::fileHash(const QString &path, QCryptographicHash::Algorithm hashAlgorithm)
//{
//    QFile file(path);
//    if (file.open(QFile::ReadOnly)) {
//        QCryptographicHash hash(hashAlgorithm);
//        if (hash.addData(&file)) {
//            return hash.result();
//        }
//    }
//    return QByteArray();
//}

#include "archivetest.moc"
/* vim: set et sts=4 sw=4 ts=8 tw=0 : */
