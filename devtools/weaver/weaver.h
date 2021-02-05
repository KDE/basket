/**
 * SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef WEAVER_H
#define WEAVER_H

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <QString>

/**
 * @brief The Weaver class encapsulates the flow control to encode and decode @c .baskets archives
 *
 * @author Sebastian Engel <kde@sebastianengel.eu>
 */
class Weaver
{
public:
    Weaver(QCommandLineParser *parser,
           const QCommandLineOption &mode_weave,
           const QCommandLineOption &mode_unweave,
           const QCommandLineOption &output,
           const QCommandLineOption &basename,
           const QCommandLineOption &previewImg,
           const QCommandLineOption &force);

    /**
     * This method processes the given command line options. If the options describe a valid operation it will encode,
     * or decode the input respectively.
     */
    int runMain();

private:
    /**
     * This method decodes the given @c .baskets file.
     */
    bool unweave();

    /**
     * This method encodes the directory given to a @c .baskets file.
     */
    bool weave();

    /**
     * Superficially tests whether @p basketsDirectory has a valid source structure.
     * @param basketsDirectory
     * @return @c true if the directory structure seems valid
     * @return @c false if the directory does not seem to be a baskets source, or an IO error occured.
     */
    static bool isBasketSourceValid(const QString &basketsDirectory);

    /**
     * Superficially tests whether @p basketsFile is a valid @c .baskets format.
     *
     * It tests for a correctly formated file header, whether the preview image and archive body have the specified
     * length, respectively. The calling method should make sure that @p basketsFile exists.
     *
     * modeled after Archive::extractArchive
     * @param basketsFile is the path to the .baskets file to be tested.
     * @return @c true if a valid file structure was found
     * @return @c false if the file structure was not valid, or any other IO error occured.
     */
    static bool isBasketFile(const QString &basketsFile);

    /**
     * This method tests whether the given path is an existing @c .png file.
     *
     * @param previewFile is the path to the file to be tested
     * @return true if @p previewFile is an existing @c .png file
     */
    static bool isPreviewValid(const QString &previewFile);

    QCommandLineParser *const m_parser;

    QCommandLineOption m_weave;
    QCommandLineOption m_unweave;
    QCommandLineOption m_output;
    QCommandLineOption m_basename;
    QCommandLineOption m_previewImg;
    QCommandLineOption m_force;

    QString m_in;
    QString m_out;
    QString m_preview;
};

#endif // WEAVER_H
