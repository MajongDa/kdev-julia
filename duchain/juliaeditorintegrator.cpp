#include "juliaeditorintegrator.h"

#include "parser/ast.h"

namespace Julia {

JuliaEditorIntegrator::JuliaEditorIntegrator()
{
}

JuliaEditorIntegrator::~JuliaEditorIntegrator()
{
}

KDevelop::CursorInRevision JuliaEditorIntegrator::findPosition(const Ast* node, Edge edge) const
{
    if (!node) {
        return KDevelop::CursorInRevision::invalid();
    }

    KDevelop::RangeInRevision range = node->range();
    if (edge == BackEdge) {
        return KDevelop::CursorInRevision(range.end.line, range.end.column);
    } else {
        return KDevelop::CursorInRevision(range.start.line, range.start.column);
    }
}

KDevelop::RangeInRevision JuliaEditorIntegrator::findRange(const Ast* node, RangeEdge edge) const
{
    Q_UNUSED(edge);
    return KDevelop::RangeInRevision(findPosition(node, FrontEdge), findPosition(node, BackEdge));
}

KDevelop::RangeInRevision JuliaEditorIntegrator::findRange(const Ast* from, const Ast* to) const
{
    return KDevelop::RangeInRevision(findPosition(from, FrontEdge), findPosition(to, BackEdge));
}

}
