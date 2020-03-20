#pragma once

#include "DocAcoustique.h"
#include "TrafficState.h"

class EVEDocAcoustique :
	public DocAcoustique
{
public:
	EVEDocAcoustique(void);
	virtual ~EVEDocAcoustique(void);

private:
	eveShared::TrafficState * m_pTrafState;

public:

	// Obtention des informations du trafic
	eveShared::TrafficState * GetTrafficState();

	// Fin
	virtual void Terminate();

    // Ajout d'un instant
	virtual PINSTANT	AddInstant(double dbInstant, int nNbVeh);

    // Sauvegarde des données acoustiques d'un instant
	virtual void SaveLastInstant();

    // Ajout de la description d'une cellule acoustique
	virtual void AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav);

    // Ajout des données acoustique d'une source ponctuelle à l'instant considéré
	virtual void AddSourcePonctuelle(int nID, double dbAbsDeb, double dbOrdDeb, double dbHautDeb, double dbAbsFin, double dbOrdFin, double dbHautFin, double * arEmissions);

    // Ajout des données acoustiques d'une cellule à l'instant considéré
	virtual void AddCellSimu(int nID, double * arEmissions ) ;

    // Ajout des données acoustiques d'une cellule à l'instant considéré
	virtual void AddCellSimuEx(int nID, double * arEmissions, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav );

};
