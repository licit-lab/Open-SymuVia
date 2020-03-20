#pragma once

#ifndef DocAcoustiqueHeader
#define DocAcoustiqueHeader

#include <boost/serialization/assume_abstract.hpp>

#include <string>

namespace boost {
    namespace serialization {
        class access;
    }
}

class DocAcoustique
{
public:
	typedef void * PINSTANT;

public:

    virtual ~DocAcoustique();

	// Fin
	virtual void Terminate() = 0;

    // Ajout d'un instant
	/*DOMNode**/virtual PINSTANT	AddInstant(double dbInstant, int nNbVeh) = 0;

    // Sauvegarde des données acoustiques d'un instant
	virtual void SaveLastInstant() = 0;

    // Libération des données acoustiques d'un instant
    virtual void ReleaseLastInstant() {};

    // Ajout de la description d'une cellule acoustique
	virtual void AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav) = 0;

    // Ajout des données acoustique d'une source ponctuelle à l'instant considéré
	virtual void AddSourcePonctuelle(int nID, double dbAbsDeb, double dbOrdDeb, double dbHautDeb, double dbAbsFin, double dbOrdFin, double dbHautFin, double * arEmissions) = 0;

    // Ajout des données acoustiques d'une cellule à l'instant considéré
	virtual void AddCellSimu(int nID, double * arEmissions )  = 0;

    // Ajout des données acoustiques d'une cellule à l'instant considéré
	virtual void AddCellSimuEx(int nID, double * arEmissions, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav ) = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(DocAcoustique)

#endif
