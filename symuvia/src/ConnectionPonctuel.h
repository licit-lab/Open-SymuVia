#pragma once
#ifndef connectionPonctuelH
#define connectionPonctuelH 

//#define NB_ELT_MAX 10

#include "Connection.h"

#include <boost/shared_ptr.hpp>

#include <deque>

class Reseau;
class Vehicule;
class VoieMicro;

// La classe connexion est la classe mère de toutes les liaisons entre les tronçons.
// Ses fils sont entrées, sorties et répartiteurs

class ConnectionPonctuel : public Connexion
{
protected:

    std::deque<boost::shared_ptr<Vehicule>> m_LstInsVeh;        // Liste des véhicules suceptibles de traverser la connexion au cours du pas de temps (décelé en appliquant la loi de poursuite classique)

public:

        // Constructeurs, destructeurs et assimilés
	    virtual ~ConnectionPonctuel(void); // Destructeur
        ConnectionPonctuel(void); // constructeur par défaut
		ConnectionPonctuel(std::string, Reseau *pReseau); // constructeur normal

        bool Init( XERCES_CPP_NAMESPACE::DOMNode *pXmlNodeCnx, Logger *pOfLog);        

        void        AddInsVeh(boost::shared_ptr<Vehicule> pV){ m_LstInsVeh.push_back(pV); }
        void        DelInsVeh(boost::shared_ptr<Vehicule> pV);

        void        ClearLstInsVeh(){m_LstInsVeh.clear();}

        void        CopyFrom( ConnectionPonctuel *pCnx);          

		void	    InitSimuTrafic(){}

        virtual bool ComputeTraffic(boost::shared_ptr<Vehicule> pV, VoieMicro * pUpstreamLane, double dbInstant, double dbPasTemps, double &dbMinimizedGoalDistance) {return false;}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif
