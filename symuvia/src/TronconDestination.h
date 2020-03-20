#pragma once
#ifndef TronconDestinationH
#define TronconDestinationH

#include "usage/SymuViaTripNode.h"

class Tuyau;

#include <deque>

class TronconDestination : public SymuViaTripNode
{
public:
    TronconDestination(Tuyau * pTuyau, double dbPosition = UNDEF_LINK_POSITION);
    virtual ~TronconDestination();

    Tuyau * GetTuyau() {return m_pTuyau;}

private:
    Tuyau* m_pTuyau;
};

#endif //TronconDestinationH