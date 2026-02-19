#ifndef JULIA_EDITORINTEGRATOR_H
#define JULIA_EDITORINTEGRATOR_H

#include <QString>

#include <language/editor/rangeinrevision.h>
#include <language/editor/cursorinrevision.h>

namespace Julia {

class AstNode;

class JuliaEditorIntegrator
{
public:
    JuliaEditorIntegrator();
    ~JuliaEditorIntegrator();

    enum Edge {
        FrontEdge,
        BackEdge
    };

    enum RangeEdge {
        InnerEdge,
        OuterEdge
    };

    KDevelop::CursorInRevision findPosition(const AstNode* node, Edge edge = BackEdge) const;
    KDevelop::RangeInRevision findRange(const AstNode* node, RangeEdge edge = OuterEdge) const;
    KDevelop::RangeInRevision findRange(const AstNode* from, const AstNode* to) const;
};

}

#endif
