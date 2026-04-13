#include <QDebug>

#include <language/duchain/topducontext.h>
#include <language/duchain/topducontextdata.h>
#include <language/duchain/duchainregister.h>
#include <language/duchain/duchainpointer.h>

#include <language/duchain/navigation/abstractnavigationwidget.h>
#include <language/duchain/navigation/abstractdeclarationnavigationcontext.h>


#include "navigation/navigationwidget.h"
#include "juliaducontext.h"
#include "juliadebug.h"

using namespace KDevelop;

namespace Julia {

REGISTER_DUCHAIN_ITEM_WITH_DATA(JuliaTopDUContext, TopDUContextData);

REGISTER_DUCHAIN_ITEM_WITH_DATA(JuliaNormalDUContext, DUContextData);

template<>
AbstractNavigationWidget* JuliaTopDUContext::createNavigationWidget(Declaration* decl, TopDUContext* topContext,
                                                    AbstractNavigationWidget::DisplayHints hints) const {
    if ( ! decl ) {
        qCDebug(KDEV_JULIA) << "no declaration, not returning navigationwidget";
        return nullptr;
    }
    return new NavigationWidget(DeclarationPointer(decl), TopDUContextPointer(topContext), hints);
}

template<>
AbstractNavigationWidget* JuliaNormalDUContext::createNavigationWidget(Declaration* decl, TopDUContext* topContext,
                                                       AbstractNavigationWidget::DisplayHints hints) const {
    if ( ! decl ) {
        return nullptr;
    }
    return new NavigationWidget(DeclarationPointer(decl), TopDUContextPointer(topContext), hints);
}

}
