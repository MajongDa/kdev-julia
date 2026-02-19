#include "juliaducontext.h"

#include <language/duchain/topducontext.h>
#include <language/duchain/topducontextdata.h>
#include <language/duchain/duchainregister.h>
#include <language/duchain/duchainpointer.h>

#include <language/duchain/navigation/abstractnavigationwidget.h>

#include <QDebug>

using namespace KDevelop;

namespace Julia {

REGISTER_DUCHAIN_ITEM_WITH_DATA(JuliaTopDUContext, TopDUContextData);

REGISTER_DUCHAIN_ITEM_WITH_DATA(JuliaNormalDUContext, DUContextData);

template<>
AbstractNavigationWidget* JuliaTopDUContext::createNavigationWidget(Declaration* decl, TopDUContext* topContext,
                                                    AbstractNavigationWidget::DisplayHints hints) const {
    Q_UNUSED(decl);
    Q_UNUSED(topContext);
    Q_UNUSED(hints);
    return nullptr;
}

template<>
AbstractNavigationWidget* JuliaNormalDUContext::createNavigationWidget(Declaration* decl, TopDUContext* topContext,
                                                       AbstractNavigationWidget::DisplayHints hints) const {
    Q_UNUSED(decl);
    Q_UNUSED(topContext);
    Q_UNUSED(hints);
    return nullptr;
}

}
