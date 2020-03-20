#pragma once
#ifndef StaticElementH
#define StaticElementH

#include "symUtil.h"

class Reseau;

class StaticElement
{
public:
    // Constructeurs, destructeurs
    StaticElement();
    StaticElement(Point ptPos);
    virtual ~StaticElement(); // Destructeur

    const Point& GetPos() const;

protected:
    Point m_ptPos;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif
