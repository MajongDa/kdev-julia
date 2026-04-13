#ifndef JULIA_EDITORINTEGRATOR_H
#define JULIA_EDITORINTEGRATOR_H

#include <QString>

#include <language/editor/rangeinrevision.h>
#include <language/editor/cursorinrevision.h>

namespace Julia {

class Ast;

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

    KDevelop::CursorInRevision findPosition(const Ast* node, Edge edge = BackEdge) const;
    KDevelop::RangeInRevision findRange(const Ast* node, RangeEdge edge = OuterEdge) const;
    KDevelop::RangeInRevision findRange(const Ast* from, const Ast* to) const;
};

}

#endif
