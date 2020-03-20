#include "stdafx.h"
#include "TronconDestination.h"

#include "tuyau.h"

TronconDestination::TronconDestination(Tuyau * pTuyau, double dbPosition)
    : SymuViaTripNode(pTuyau->GetLabel(), pTuyau->GetReseau())
{
    m_pTuyau = pTuyau;

    m_InputPosition.SetLink(pTuyau);
    m_InputPosition.SetPosition(dbPosition);
}

TronconDestination::~TronconDestination()
{
}