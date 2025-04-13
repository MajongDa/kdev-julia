
#ifndef KDEVPYTHONLANGUAGESUPPORT_H
#define KDEVPYTHONLANGUAGESUPPORT_H

#include <interfaces/iplugin.h>


#include <interfaces/iplugin.h>
#include <language/interfaces/ilanguagesupport.h>
#include <interfaces/ilanguagecheckprovider.h>
#include <language/duchain/topducontext.h>

#include <QVariant>
#include <QProcess>

// namespace KDevelop
// {
// class ParseJob;
// class IDocument;
// class ICodeHighlighting;
// }

namespace Julia
{

class Highlighting;

class LanguageSupport
: public KDevelop::IPlugin
//TODO fix include? its messing inheritanse
//, public KDevelop::ILanguageSupport
//, public KDevelop::ILanguageCheckProvider
{
    Q_OBJECT
    //Q_INTERFACES( KDevelop::ILanguageSupport )

public:
    LanguageSupport(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args = QVariantList());
    ~LanguageSupport() override;
    /*Name Of the Language*/
    QString name() const;
    /*the code highlighter*/
    //KDevelop::ICodeHighlighting* codeHighlighting() const override;

    // KDevelop::ContextMenuExtension contextMenuExtension(KDevelop::Context* context, QWidget* parent) override;

    static LanguageSupport* self();


    /// Tells whether this plugin is enabled for the given file.
    // static bool enabledForFile(const QUrl& url);

    // int configPages() const override;
    // KDevelop::ConfigPage* configPage(int number, QWidget* parent) override;

    // int perProjectConfigPages() const override;
    // KDevelop::ConfigPage* perProjectConfigPage(int number, const KDevelop::ProjectConfigOptions& options, QWidget* parent) override;

public Q_SLOTS:
    // void documentOpened(KDevelop::IDocument*);
    //FIXME No style checking for now
    //void updateStyleChecking(KDevelop::ReferencedTopDUContext top);

private:
    static LanguageSupport* m_self;
};


}
#endif
