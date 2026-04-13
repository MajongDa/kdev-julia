#ifndef JULIA_PARSESESSION_H
#define JULIA_PARSESESSION_H

#include <QUrl>
#include <QList>
#include <QSharedPointer>

#include <language/duchain/topducontext.h>

#include "ast.h"

namespace Julia {

class ParseSessionData;

/**
 * Manages the state of a single parse operation
 */
class ParseSession
{
public:
    ParseSession();
    ~ParseSession();
    
    /**
     * Set the URL being parsed
     */
    void setUrl(const QUrl& url);
    
    /**
     * Get the URL being parsed
     */
    QUrl url() const;
    
    /**
     * Set the source code content
     */
    void setSourceCode(const QString& code);
    
    /**
     * Get the source code content
     */
    QString sourceCode() const;
    
    /**
     * Set the parsed AST
     */
    void setAst(Ast* ast);
    
    /**
     * Get the parsed AST
     */
    Ast* ast() const;
    
    /**
     * Set the TopDUContext
     */
    void setTopContext(KDevelop::TopDUContext* context);
    
    /**
     * Get the TopDUContext
     */
    KDevelop::TopDUContext* topContext() const;
    
    /**
     * Check if parsing was successful
     */
    bool success() const;
    
    /**
     * Get error message if parsing failed
     */
    QString errorMessage() const;
    
    /**
     * Set parsing success status
     */
    void setSuccess(bool success, const QString& errorMessage = QString());
    
    /**
     * Clear all session data
     */
    void clear();

private:
    QSharedPointer<ParseSessionData> d;
};

} // namespace Julia

#endif // JULIA_PARSESESSION_H
