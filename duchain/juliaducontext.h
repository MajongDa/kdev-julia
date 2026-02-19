#ifndef JULIA_DUCONTEXT_H
#define JULIA_DUCONTEXT_H

#include <QString>
#include <language/duchain/ducontext.h>
#include <language/editor/modificationrevision.h>

namespace KDevelop
{
    class Declaration;
    class TopDUContext;
}

namespace Julia {

template<class BaseContext, int IdentityT>
class JuliaDUContext : public BaseContext
{
public:
    template<class Data>
    JuliaDUContext(Data& data) : BaseContext(data) {
    }

    template<class Param1, class Param2>
    JuliaDUContext(const Param1& p1, const Param2& p2, bool isInstantiationContext) : BaseContext(p1, p2, isInstantiationContext) {
        static_cast<KDevelop::DUChainBase*>(this)->d_func_dynamic()->setClassId(this);
    }

    template<class Param1, class Param2, class Param3>
    JuliaDUContext(const Param1& p1, const Param2& p2, const Param3& p3) : BaseContext(p1, p2, p3) {
        static_cast<KDevelop::DUChainBase*>(this)->d_func_dynamic()->setClassId(this);
    }
    template<class Param1, class Param2>
    JuliaDUContext(const Param1& p1, const Param2& p2) : BaseContext(p1, p2) {
        static_cast<KDevelop::DUChainBase*>(this)->d_func_dynamic()->setClassId(this);
    }

    KDevelop::AbstractNavigationWidget* createNavigationWidget(KDevelop::Declaration* decl, KDevelop::TopDUContext* topContext,
                                            KDevelop::AbstractNavigationWidget::DisplayHints hints) const override;

    enum {
        Identity = IdentityT
    };
};

typedef JuliaDUContext<KDevelop::TopDUContext, 100> JuliaTopDUContext;
typedef JuliaDUContext<KDevelop::DUContext, 101> JuliaNormalDUContext;

}

#endif
