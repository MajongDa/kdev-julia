/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include <QString>
#include <QUrl>
#include <QWidgetList>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QAction;
class QFont;
class QIcon;
class QVariant;
class QWidget;
typedef QMap<QString, QVariant> QVariantMap;
QT_END_NAMESPACE

namespace KTextEditor
{
class View;
class Document;
class MainWindow;
class Range;
class Cursor;
}
class DiagnosticsProvider;
struct DiffParams;

enum MessageType {
    Log = 0,
    Info,
    Warning,
    Error,
};

namespace Utils
{

// A helper class that maintains scroll position
struct KateScrollBarRestorer {
    explicit KateScrollBarRestorer(KTextEditor::View *view);
    ~KateScrollBarRestorer();

    void restore();

private:
    class KateScrollBarRestorerPrivate *const d = nullptr;
};

/**
 * returns the current active global font
 */
QFont editorFont();

/**
 * Returns the range that is currently visible in the @p view
 */
KTextEditor::Range getVisibleRange(KTextEditor::View *view);

/**
 * @brief Given path "/home/user/xyz.txt" returns "xyz.txt"
 */
inline QString fileNameFromPath(const QString &path)
{
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    return lastSlash == -1 ? path : path.mid(lastSlash + 1);
}

/**
 * Return a matching icon for the given document.
 * We use the mime type icon for unmodified stuff and the modified one for modified docs.
 * @param doc document to get icon for
 * @return icon, always valid
 */
QIcon iconForDocument(KTextEditor::Document *doc);

QAction *toolviewShowAction(KTextEditor::MainWindow *, const QString &toolviewName);

QWidget *toolviewForName(KTextEditor::MainWindow *, const QString &toolviewName);

QWidget *activeToolviewForSide(KTextEditor::MainWindow *mainWindow, int side);

/*** BEGIN KTextEditor::MainWindow extensions **/
void
showMessage(const QString &message, const QIcon &icon, const QString &category, MessageType type, KTextEditor::MainWindow *mainWindow = nullptr);
void showMessage(const QVariantMap &map, KTextEditor::MainWindow *mainWindow = nullptr);

void showDiff(const QByteArray &diff, const DiffParams &params, KTextEditor::MainWindow *mainWindow);

/// @returns list of document area widgets that are not KTextEditor::Views (added by addWidget()
/// TODO: Maybe it would be more versatile to return *all* widgets, including KTextEditor::Views, here, or perhaps
///       controlled by a flag to filter which widgets to return. Think e.g. about the document switching plugin.
///       activeWidget() (not yet "public") uses those semantics, already.
QWidgetList widgets(KTextEditor::MainWindow *mainWindow);

void insertWidgetInStatusbar(QWidget *widget, KTextEditor::MainWindow *mainWindow);

void addPositionToHistory(const QUrl &url, KTextEditor::Cursor c, KTextEditor::MainWindow *mainWindow);
/*** END KTextEditor::MainWindow extensions **/

/**
 * Returns project base dir for provided document
 */
QString projectBaseDirForDocument(KTextEditor::Document *doc);

/**
 * Returns project map for provided document
 */
QVariantMap projectMapForDocument(KTextEditor::Document *doc);

/**
 * Convert an url to a xxx.yyy @ dir representation, used e.g. in the window title
 */
QString niceFileNameWithPath(const QUrl &url);

/**
 * Convert an url to a normalize one, used by the document manager and Co.
 */
QUrl normalizeUrl(QUrl url);

/**
 * Convert an url to an absolute one, used by the document manager and Co.
 */
QUrl absoluteUrl(QUrl url);
}
