#ifndef KDEVJULIALANGUAGESUPPORT_H
#define KDEVJULIALANGUAGESUPPORT_H

#include <interfaces/iplugin.h>
#include <interfaces/ilanguagecheckprovider.h>
#include <outputview/ioutputview.h>
#include <language/interfaces/ilanguagesupport.h>
#include <language/duchain/topducontext.h>

#include <QVariant>
#include <QProcess>
#include <memory>

#include "graphicstoolviewfactory.h"

namespace KDevelop
{
class ParseJob;
class IDocument;
class ICodeHighlighting;
}

namespace Julia
{

class Highlighting;

class LanguageSupport
: public KDevelop::IPlugin
, public KDevelop::ILanguageSupport
//, public KDevelop::ILanguageCheckProvider
{
    Q_OBJECT
    Q_INTERFACES(KDevelop::ILanguageSupport)

public:
    LanguageSupport(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args = QVariantList());
    ~LanguageSupport() override;
    
    /*Name Of the Language*/
    QString name() const override;
    
    /*The Language Support ID*/
    KDevelop::ParseJob* createParseJob(const KDevelop::IndexedString& url) override;
    
    /*The code highlighter*/
    // KDevelop::ICodeHighlighting* codeHighlighting() const override;

    KDevelop::ContextMenuExtension contextMenuExtension(KDevelop::Context* context, QWidget* parent) override;

    static LanguageSupport* self();

    /// Tells whether this plugin is enabled for the given file.
    static bool enabledForFile(const QUrl& url);

    // int configPages() const override;
    // KDevelop::ConfigPage* configPage(int number, QWidget* parent) override;

    int perProjectConfigPages() const override;
    KDevelop::ConfigPage* perProjectConfigPage(int number, const KDevelop::ProjectConfigOptions& options, QWidget* parent) override;

public Q_SLOTS:
    void documentOpened(KDevelop::IDocument*);
    void executeCurrentJuliaFile();

private:
    static LanguageSupport* m_self;
    Highlighting* m_highlighting = nullptr;
    GraphicsToolViewFactory* m_view = nullptr;

    void checkJuliaLanguageServer();
};

}
#endif
