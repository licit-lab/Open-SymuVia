#pragma once

#ifndef SYMUCORE_POINT_H
#define SYMUCORE_POINT_H

#include "SymuCoreExports.h"

namespace SymuCore {

class SYMUCORE_DLL_DEF Point {

public:

    Point();
    Point(double x, double y);

    //destructor
    virtual ~Point();

    // getters.
    double getX();
    double getY();

    // setters.
    void setX(double x);
    void setY(double y);

    double distanceTo(Point * pOtherPoint) const;

private:
    double m_X;
    double m_Y;
};
}

#endif // SYMUCORE_POINT_H
