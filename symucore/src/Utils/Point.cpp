#include "Point.h"

#include <math.h>

using namespace SymuCore;

Point::Point()
{
    m_X = 0;
    m_Y = 0;
}

Point::Point(double x, double y)
{
    m_X = x;
    m_Y = y;
}

Point::~Point()
{

}

void Point::setX(double x)
{
    m_X = x;
}

double Point::getX()
{
    return m_X;
}

double Point::getY()
{
    return m_Y;
}

void Point::setY(double y)
{
    m_Y = y;
}

double Point::distanceTo(Point * pOtherPoint) const
{
    double dbResult;
    dbResult = sqrt((m_X - pOtherPoint->m_X)*(m_X - pOtherPoint->m_X) + (m_Y - pOtherPoint->m_Y)*(m_Y - pOtherPoint->m_Y));
    return dbResult;
}
