#include "parsesession.h"

#include <QUrl>

namespace Julia {

class ParseSessionData
{
public:
    QUrl url;
    QString sourceCode;
    Ast* ast = nullptr;
    KDevelop::TopDUContext* topContext = nullptr;
    bool success = false;
    QString errorMessage;
};

ParseSession::ParseSession()
    : d(new ParseSessionData())
{
}

ParseSession::~ParseSession() = default;

void ParseSession::setUrl(const QUrl& url)
{
    d->url = url;
}

QUrl ParseSession::url() const
{
    return d->url;
}

void ParseSession::setSourceCode(const QString& code)
{
    d->sourceCode = code;
}

QString ParseSession::sourceCode() const
{
    return d->sourceCode;
}

void ParseSession::setAst(Ast* ast)
{
    d->ast = ast;
}

Ast* ParseSession::ast() const
{
    return d->ast;
}

void ParseSession::setTopContext(KDevelop::TopDUContext* context)
{
    d->topContext = context;
}

KDevelop::TopDUContext* ParseSession::topContext() const
{
    return d->topContext;
}

bool ParseSession::success() const
{
    return d->success;
}

QString ParseSession::errorMessage() const
{
    return d->errorMessage;
}

void ParseSession::setSuccess(bool success, const QString& errorMessage)
{
    d->success = success;
    d->errorMessage = errorMessage;
}

void ParseSession::clear()
{
    d->url.clear();
    d->sourceCode.clear();
    delete d->ast;
    d->ast = nullptr;
    d->topContext = nullptr;
    d->success = false;
    d->errorMessage.clear();
}

} // namespace Julia
