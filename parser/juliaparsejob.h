#ifndef JULIA_PARSEJOB_H
#define JULIA_PARSEJOB_H

#include <memory>

#include <language/backgroundparser/parsejob.h>
#include <language/duchain/topducontext.h>

#include "ast.h"

namespace Julia {

class JuliaBridge;

class JuliaParseJob : public KDevelop::ParseJob
{
    Q_OBJECT

public:
    explicit JuliaParseJob(const KDevelop::IndexedString& url, KDevelop::ILanguageSupport* languageSupport);
    ~JuliaParseJob() override;

protected:
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread* thread) override;

private:
    bool parseWithJuliaBridge(const QString& content);
    Julia::AstNode* parseJsonResponse(const QByteArray& json);
    bool buildDUChain(Julia::AstNode* ast);

    std::unique_ptr<JuliaBridge> m_bridge;
    KDevelop::ReferencedTopDUContext m_topContext;
};

}

#endif
