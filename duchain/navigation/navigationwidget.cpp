/*
    SPDX-FileCopyrightText: 2024 Julia language plugin for KDevelop

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "navigationwidget.h"

#include <language/duchain/navigation/abstractdeclarationnavigationcontext.h>
#include <language/duchain/navigation/abstractnavigationcontext.h>

using namespace KDevelop;

namespace Julia {

NavigationWidget::NavigationWidget(DeclarationPointer declaration, TopDUContextPointer topContext,
                                   AbstractNavigationWidget::DisplayHints hints)
    : AbstractNavigationWidget()
{
    auto* declNav = new AbstractDeclarationNavigationContext(declaration, topContext, nullptr);
    NavigationContextPointer ctx(declNav);
    setContext(ctx, 400);
    setDisplayHints(hints);
}

NavigationWidget::NavigationWidget(const IncludeItem& includeItem, TopDUContextPointer topContext,
                                   AbstractNavigationWidget::DisplayHints hints)
    : AbstractNavigationWidget()
{
    // For include items, we still create a declaration navigation context
    // Just pass null declaration - it will show the include path
    DeclarationPointer nullDecl;
    auto* declNav = new AbstractDeclarationNavigationContext(nullDecl, topContext, nullptr);
    NavigationContextPointer ctx(declNav);
    setContext(ctx, 400);
    setDisplayHints(hints);
}

}
