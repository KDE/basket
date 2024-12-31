/**
 * SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "weaver.h"

#include <config.h>

#include <KAboutData>
#include <KLocalizedString>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("basketweaver"), i18n("basketweaver"), QStringLiteral(VERSION));
    aboutData.setShortDescription(i18n("Encodes and decodes .baskets files"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption output(QStringList() << QStringLiteral("o") << QStringLiteral("output"),
                              i18n("Destination directory. Optional."),
                              QStringLiteral("directory"));
    parser.addOption(output);

    QCommandLineOption previewImg(QStringList() << QStringLiteral("p") << QStringLiteral("preview"),
                                  i18n("Use the <image> as preview image for a .baskets file. Must be a .png file."),
                                  QStringLiteral("image"));
    parser.addOption(previewImg);

    QCommandLineOption mode_unweave(QStringList() << QStringLiteral("u") << QStringLiteral("unweave") << QStringLiteral("x"),
                                    i18n("Decodes the <file>. Cannot be used together with --weave."),
                                    QStringLiteral("file"));
    parser.addOption(mode_unweave);

    QCommandLineOption mode_weave(QStringList() << QStringLiteral("w") << QStringLiteral("weave") << QStringLiteral("c"),
                                  i18n("Encodes the <directory>. Cannot be used together with --unweave."),
                                  QStringLiteral("directory"));
    parser.addOption(mode_weave);

    QCommandLineOption basename(QStringList() << QStringLiteral("n") << QStringLiteral("name"),
                                i18n("This optional value will be used to label the new content within the optionally given output directory"),
                                QStringLiteral("fileName"));
    parser.addOption(basename);

    QCommandLineOption forceOption(QStringList() << QStringLiteral("f") << QStringLiteral("force"), i18n("Overwrite existing files."));
    parser.addOption(forceOption);

    parser.process(app);

    Weaver weaver(&parser, mode_weave, mode_unweave, output, basename, previewImg, forceOption);

    return weaver.runMain();
}
