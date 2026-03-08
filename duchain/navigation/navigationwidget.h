/*
    SPDX-FileCopyrightText: 2024 Julia language plugin for KDevelop

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef JULIA_NAVIGATIONWIDGET_H
#define JULIA_NAVIGATIONWIDGET_H

#include <language/duchain/navigation/abstractnavigationwidget.h>
#include <language/duchain/declaration.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/duchainpointer.h>
#include <language/util/includeitem.h>

namespace Julia {

class NavigationWidget : public KDevelop::AbstractNavigationWidget
{
    Q_OBJECT

public:
    NavigationWidget(KDevelop::DeclarationPointer declaration, KDevelop::TopDUContextPointer topContext,
                     KDevelop::AbstractNavigationWidget::DisplayHints hints = KDevelop::AbstractNavigationWidget::NoHints);
    NavigationWidget(const KDevelop::IncludeItem& includeItem, KDevelop::TopDUContextPointer topContext,
                     KDevelop::AbstractNavigationWidget::DisplayHints hints = KDevelop::AbstractNavigationWidget::NoHints);

    static QString shortDescription(const KDevelop::IncludeItem&) { return QString(); };
};

}

#endif // JULIA_NAVIGATIONWIDGET_H
