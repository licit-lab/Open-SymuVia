#include "stdafx.h"
#include "TraceDocAcoustique.h"

#include "EVEDocAcoustique.h"

TraceDocAcoustique::TraceDocAcoustique(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument * pXMLDocument)
: XMLDocAcoustique(pNetwork, pXMLDocument)
{
	m_pDocEveDocAcoustique = new EVEDocAcoustique();
}

TraceDocAcoustique::~TraceDocAcoustique(void) 
{
	delete m_pDocEveDocAcoustique;
	m_pDocEveDocAcoustique = NULL;
}

// Obtention des informations du trafic
eveShared::TrafficState * TraceDocAcoustique::GetTrafficState()
{
	if (m_pDocEveDocAcoustique == NULL)
	{
		return NULL;
	}
	else
	{
		return m_pDocEveDocAcoustique->GetTrafficState();
	}
}

// Ajout d'un instant
TraceDocAcoustique::PINSTANT	TraceDocAcoustique::AddInstant(double dbInstant, int nNbVeh)
{
	TraceDocAcoustique::PINSTANT pInst = XMLDocAcoustique::AddInstant(dbInstant, 0 );
	if (m_pDocEveDocAcoustique != NULL)
	{
		m_pDocEveDocAcoustique->AddInstant(dbInstant,0);
	}
	return pInst;
}

// Ajout des données acoustique d'une source ponctuelle à l'instant considéré
void TraceDocAcoustique::AddSourcePonctuelle(int nID, double dbAbsDeb, double dbOrdDeb, double dbHautDeb, double dbAbsFin, double dbOrdFin, double dbHautFin, double * arEmissions)
{
	XMLDocAcoustique::AddSourcePonctuelle(nID, dbAbsDeb, dbOrdDeb, dbHautDeb, dbAbsFin, dbOrdFin, dbHautFin, arEmissions);
	if (m_pDocEveDocAcoustique != NULL)
	{
		m_pDocEveDocAcoustique->AddSourcePonctuelle(nID, dbAbsDeb, dbOrdDeb, dbHautDeb, dbAbsFin, dbOrdFin, dbHautFin, arEmissions);
	}
}

// Ajout des données acoustiques d'une cellule à l'instant considéré
void TraceDocAcoustique::AddCellSimuEx(int nID, double * arEmissions, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav )
{
	XMLDocAcoustique::AddCellSimuEx(nID, arEmissions, strTuyau, dbXam, dbYam, dbZam, dbXav, dbYav, dbZav);
	if (m_pDocEveDocAcoustique != NULL)
	{
		m_pDocEveDocAcoustique->AddCellSimuEx(nID, arEmissions, strTuyau, dbXam, dbYam, dbZam, dbXav, dbYav, dbZav);
	}
}
