#ifndef KDEVJULIAHIGHLIGHTING_H
#define KDEVJULIAHIGHLIGHTING_H

#include <QObject>
#include <QHash>
#include <QModelIndex>
#include <QColor>

#include <language/highlighting/codehighlighting.h>
#include <language/duchain/topducontext.h>

namespace Julia {

class Highlighting;

class CodeHighlightingInstance : public KDevelop::CodeHighlightingInstance {
public:
    CodeHighlightingInstance(const Highlighting* highlighting);
    void highlightDeclaration(KDevelop::Declaration* declaration, const QColor& color) override;
    void highlightUse(KDevelop::DUContext* context, int index, const QColor& color) override;
    bool useRainbowColor(KDevelop::Declaration* dec) const override;
    KDevelop::CodeHighlightingType typeForDeclaration(KDevelop::Declaration* dec, KDevelop::DUContext* context) const override;
private:
    void checkHasBlocks(KDevelop::TopDUContext* top) const;
    mutable bool checked_blocks;
    mutable bool has_blocks;
};


class Highlighting : public KDevelop::CodeHighlighting
{
Q_OBJECT
public:
    Highlighting( QObject* parent );
    CodeHighlightingInstance* createInstance() const override;
    void highlightDUChain(KDevelop::ReferencedTopDUContext context) override;
};
}
#endif

