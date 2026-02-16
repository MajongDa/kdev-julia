#include "juliaparsejob.h"

#include <climits>

#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/problem.h>
#include <language/duchain/parsingenvironment.h>
#include <language/backgroundparser/urlparselock.h>
#include <language/backgroundparser/backgroundparser.h>
#include <interfaces/icore.h>
#include <interfaces/ilanguagecontroller.h>

#include "juliabridge.h"
#include "../duchain/contextbuilder.h"
#include "../duchain/declarationbuilder.h"
#include "../duchain/usebuilder.h"
#include "juliadebug.h"

namespace Julia {

JuliaParseJob::JuliaParseJob(const KDevelop::IndexedString& url, KDevelop::ILanguageSupport* languageSupport)
    : KDevelop::ParseJob(url, languageSupport)
    , m_bridge(new JuliaBridge(this))
{
}

JuliaParseJob::~JuliaParseJob() = default;

void JuliaParseJob::run(ThreadWeaver::JobPointer /*self*/, ThreadWeaver::Thread* /*thread*/)
{
    if (abortRequested()) {
        return abortJob();
    }
    
    readContents();

    Contents contents = this->contents();
    if (contents.contents.isEmpty()) {
        return;
    }

    KDevelop::UrlParseLock urlLock(document());

    // Check if document is already up-to-date (similar to kdev-python)
    if (!(minimumFeatures() & KDevelop::TopDUContext::ForceUpdate)) {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        static const KDevelop::IndexedString langString("julia");
        const auto environmentFiles = KDevelop::DUChain::self()->allEnvironmentFiles(document());
        for (const KDevelop::ParsingEnvironmentFilePointer& file : environmentFiles) {
            if (file->language() != langString) {
                continue;
            }
            if (!file->needsUpdate() && file->featuresSatisfied(minimumFeatures()) && file->topContext()) {
                qCDebug(KDEV_JULIA) << "Document already up-to-date:" << document().str();
                setDuChain(file->topContext());
                auto* bgParser = KDevelop::ICore::self()->languageController()->backgroundParser();
                if (bgParser->trackerForUrl(document())) {
                    lock.unlock();
                    qCDebug(KDEV_JULIA) << "Calling highlightDUChain for cached document";
                    highlightDUChain();
                    qCDebug(KDEV_JULIA) << "highlightDUChain completed for cached document";
                }
                return;
            }
            break;
        }
    }
    
    QString code = QString::fromUtf8(contents.contents);
    parseWithJuliaBridge(code);
}

bool JuliaParseJob::parseWithJuliaBridge(const QString& content)
{
    if (!m_bridge->isJuliaAvailable()) {
        qCDebug(KDEV_JULIA) << "Julia is not available";
        return false;
    }

    QByteArray json = m_bridge->parseToJson(content);
    if (json.isEmpty()) {
        qCDebug(KDEV_JULIA) << "Failed to parse to JSON:" << m_bridge->lastError();
        return false;
    }

    AstNode* ast = parseJsonResponse(json);
    if (!ast) {
        qCDebug(KDEV_JULIA) << "Failed to parse JSON response";
        return false;
    }

    if (!buildDUChain(ast)) {
        delete ast;
        return false;
    }

    delete ast;
    return true;
}

AstNode* JuliaParseJob::parseJsonResponse(const QByteArray& json)
{
    return AstNode::parseJson(json);
}

bool JuliaParseJob::buildDUChain(AstNode* ast)
{
    if (!ast) {
        return false;
    }

    qCDebug(KDEV_JULIA) << "Building DUChain for" << document().str();
    
    // Check for existing context to update
    KDevelop::ReferencedTopDUContext toUpdate;
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        toUpdate = KDevelop::DUChain::self()->chainForDocument(document());
    }
    qCDebug(KDEV_JULIA) << "Existing context:" << (toUpdate ? "yes" : "no");
    
    if (toUpdate) {
        // Translate to current revision
        translateDUChainToRevision(toUpdate);
        toUpdate->setRange(KDevelop::RangeInRevision(0, 0, INT_MAX, INT_MAX));
    }

    // Build the DUChain using DeclarationBuilder
    qCDebug(KDEV_JULIA) << "Creating DeclarationBuilder...";
    DeclarationBuilder builder;
    qCDebug(KDEV_JULIA) << "Calling builder.build()...";
    KDevelop::ReferencedTopDUContext newContext = builder.build(document(), ast, toUpdate);
    qCDebug(KDEV_JULIA) << "Builder returned:" << newContext.data();

    if (!newContext) {
        qCDebug(KDEV_JULIA) << "Failed to build DUChain - builder returned null";
        return false;
    }
    
    qCDebug(KDEV_JULIA) << "Setting features...";

    // Set features and modification revision
    {
        KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
        newContext->setType(KDevelop::DUContext::Global);
        newContext->setFeatures(minimumFeatures());
        
        // Add to chain if it's a new context
        if (!toUpdate) {
            qCDebug(KDEV_JULIA) << "Adding to document chain...";
            KDevelop::DUChain::self()->addDocumentChain(newContext.data());
        }
        
        // Set modification revision on the environment file
        KDevelop::ParsingEnvironmentFilePointer envFile = newContext->parsingEnvironmentFile();
        if (envFile) {
            envFile->setModificationRevision(contents().modification);
        }
    }
    
    qCDebug(KDEV_JULIA) << "Calling setDuChain()...";

    // Set the DUChain
    setDuChain(newContext);
    
    // Build uses for highlighting (references to declarations)
    qCDebug(KDEV_JULIA) << "Building uses for highlighting...";
    UseBuilder usebuilder;
    usebuilder.buildUses(ast);
    qCDebug(KDEV_JULIA) << "Uses built successfully";

    // Start the code highlighter
    auto* bgParser = KDevelop::ICore::self()->languageController()->backgroundParser();
    auto tracker = bgParser->trackerForUrl(document());
    if (tracker) {
        qCDebug(KDEV_JULIA) << "Calling highlightDUChain";
        highlightDUChain();
        qCDebug(KDEV_JULIA) << "highlightDUChain completed";
    }

    return true;
}

}
