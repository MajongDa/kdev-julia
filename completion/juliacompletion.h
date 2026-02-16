#ifndef JULIA_COMPLETION_H
#define JULIA_COMPLETION_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QStringList>

#include <language/codecompletion/codecompletionworker.h>
#include <language/duchain/declaration.h>
#include <language/duchain/duchainpointer.h>

namespace Julia {

class JuliaCompletionWorker : public KDevelop::CodeCompletionWorker
{
    Q_OBJECT

public:
    explicit JuliaCompletionWorker(KDevelop::CodeCompletionModel* model);
    ~JuliaCompletionWorker() override;

protected:
    void computeCompletions(const KDevelop::DUContextPointer& context,
                          const KTextEditor::Cursor& position,
                          const QString& followingText,
                          const KTextEditor::Range& contextRange,
                          const QString& contextText) override;

private:
    void addKeywordCompletions(const QString& followingText);
    void addDeclarationCompletions(const KDevelop::DUContextPointer& context,
                                   const QString& followingText);
};

}

#endif
