#ifndef ASTBUILDER_H_INCLUDED
#define ASTBUILDER_H_INCLUDED

#include "julia.h"

#include <QString>
#include <QUrl>
namespace Julia
{
class AstBuilder{
    QString parse(QUrl *file);

};
}

#endif // ASTBUILDER_H_INCLUDED
