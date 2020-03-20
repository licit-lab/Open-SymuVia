#include "stdafx.h"
#include "loi_emission.h"

#include "reseau.h"
#include "SystemUtil.h"
#include "Logger.h"

using namespace std;

const int Loi_emission::Nb_Octaves=8;
const std::string Loi_emission::nom_octave[8] = { "63", "125", "250", "500", "1000", "2000", "4000", "8000" };

//---------------------------------------------------------------------------
// constructeurs et destructeurs
//--------------------------------------------------------------------------


// constructeur
Loi_emission::Loi_emission()
{
    Init();
    m_pNetwork = NULL;
}


// constructeur
Loi_emission::Loi_emission(Reseau * pNetwork)
{
    Init();
    m_pNetwork = pNetwork;
}

void Loi_emission::Init()
{
    m_bLoaded = false;
}

//================================================================
    bool    Loi_emission::LoadDatabase
//----------------------------------------------------------------
// Fonction  : Chargement des données de la base Loi_emission
// Version du: 12/10/2006 (d'après MQ - 2004)
// Historique: 12/10/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    m_pNetwork->log()<<"LoiEmission Database connection...";

    // CONNECTION BASE DE DONNEES******************************************

	try
	{
		m_DBLoiEmission.Open("DSNLoiEmission");


        // Chargement de la version de la BD
		m_DBVersion.sqlConnect(m_DBLoiEmission);
		m_DBVersion.sqlPrepare("select VERSION from VERSION where ID_VERSION = (select Max(ID_VERSION) from VERSION)");

        // Chargement des revêtements
		m_DBRevetements.sqlConnect(m_DBLoiEmission);
		m_DBRevetements.sqlPrepare("select ID_REVETEMENT from REVETEMENT where REVETEMENT = ? ");

		// Chargement des allures
		m_DBAllures.sqlConnect(m_DBLoiEmission);
		m_DBAllures.sqlPrepare("select ID_ALLURE from ALLURE where ALLURE = ? ");
       
        // Chargement des types de véhicule
		m_DBTypeVehicules.sqlConnect(m_DBLoiEmission);
		m_DBTypeVehicules.sqlPrepare("select ID_TYPE from TYPE_VEHICULE where TYPE = ? ");


        // Chargement des vitesses
		m_DBVitesses.sqlConnect(m_DBLoiEmission);
		m_DBVitesses.sqlPrepare("select ID_VITESSE from VITESSE where VITESSE = ? ");

        // Préparation de la requête des puissances
		m_DBPuissances.sqlConnect(m_DBLoiEmission);
		m_DBPuissances.sqlPrepare("select Puissance from DATAS where ID_ALLURE = ? AND ID_REVETEMENT = ? AND ID_TYPE = ? AND ID_VITESSE = ? AND ID_TIERS = ? ");
	}
    catch(ExceptionSymuvia& e)
    {
        m_pNetwork->log()<<endl;
        m_pNetwork->log()<< Logger::Error << " Error loading data from LoiEmission database --> "<<endl;
		m_pNetwork->log()<< Logger::Error << " Error message : "<< e.m_strError << endl;
        m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    m_dbFactPuis = pow((double)10,(double)-12);
    
    m_pNetwork->log()<<" ok"<<endl;
    m_bLoaded = true;
    return true;
}

//================================================================
std::string    Loi_emission::GetVersion
//----------------------------------------------------------------
// Fonction  : Retourne le numéro de version de la BD 'LoEmission'
// Version du: 21/11/2006
// Historique: 21/11/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(	
)
{
	std::string strVersion;
	m_DBVersion.sqlExec();

	if (m_DBVersion.sqlFetch())
	{
		strVersion = m_DBVersion.getResultStr(0);
		m_DBVersion.sqlSkipData();
	}

	return strVersion;
}

// destructeur
Loi_emission::~Loi_emission()
{
	m_DBRevetements.sqlDeconnect();
	m_DBAllures.sqlDeconnect();
	m_DBTypeVehicules.sqlDeconnect();
	m_DBVitesses.sqlDeconnect();
	m_DBPuissances.sqlDeconnect();
	m_DBVersion.sqlDeconnect();

	m_DBLoiEmission.Close();
}

//================================================================
    void Loi_emission::CalculEmissionBruit
//----------------------------------------------------------------
// Fonction  : Retourne l'emissiond de bruit pour chaque octave
//             plus le niveau global
// Version du: 23/10/2006
// Historique: 23/10/2006 (C.Bécarie - Tinea)
//             Reprise
//================================================================
(
    double* pW,         // Tableau des puissances calculées
    double  vit,         // Vitesse
    double  acc,         // Accélération
    std::string nom_rev,     // revêtement
    std::string type_veh     // Type de véhicule
)
{    
	bool	bOk;
	int		nVit;
    double  dbVit = 3.6*vit;   // On travaille avec une vitesse en km/h
    int     nOctave;
	int		DBAllures_Id;
	int		DBRevetements_Id;
	int		DBTypeVehicules_Id;
	int		DBVitesses_Id;

    if(dbVit<0 && dbVit>-0.0001)
        dbVit=0;

	try
	{
		//Récupération des données de revêtement
		m_DBRevetements.AddParamStr(0, nom_rev, 255);
		m_DBRevetements.sqlExec();
		bOk = m_DBRevetements.sqlFetch();
		if (!bOk)
		{
		    m_pNetwork->log()<<"Road surface '" << nom_rev << "' not found in the LoiEmission database"<<endl;
			return;
		}
		DBRevetements_Id = m_DBRevetements.getResultInt(0);
		m_DBRevetements.sqlSkipData();

		//Récupération des données de véhicule
 		m_DBTypeVehicules.AddParamStr(0, type_veh, 255);
		m_DBTypeVehicules.sqlExec();
		bOk = m_DBTypeVehicules.sqlFetch();
		if (!bOk)
		{
		    m_pNetwork->log()<<"Vehicle type '" << type_veh << "' not found in the LoiEmission database"<<endl;
			return;
		}
		DBTypeVehicules_Id = m_DBTypeVehicules.getResultInt(0);
		m_DBTypeVehicules.sqlSkipData();

		//Récupération des données d'allure
		std::string strAllure;

		// ID de l'allure
		if (acc >= 1)
			strAllure = "A";
		if (acc <= -0.2)
			strAllure = "D";
		if (acc > -0.2 && acc < 1)
			strAllure = "S";

 		m_DBAllures.AddParamStr(0, strAllure, 50);
		m_DBAllures.sqlExec();
		bOk = m_DBAllures.sqlFetch();
		if (!bOk)
		{
		    m_pNetwork->log() << "Speed category '" << strAllure << "' not found in the LoiEmission database"<<endl;
			return;
		}
		DBAllures_Id = m_DBAllures.getResultInt(0);
		m_DBAllures.sqlSkipData();

		//Récupération des données de vitesse
		// ID de la vitesse
		// Rmq : de 0 à 20, la vitesse est répertoriée de 1 en 1 et de 20 à 100, de 2 en 2
		nVit = (int)dbVit;

		if( dbVit> 20)
		{
			if (div(nVit,2).rem > 0)
				nVit--;
		}	

		m_DBVitesses.AddParamInt(0, nVit);
		m_DBVitesses.sqlExec();
		bOk = m_DBVitesses.sqlFetch();
		if (!bOk)
		{
		    m_pNetwork->log()<<"Vitesse '" << nVit << "' not found in the LoiEmission database"<<endl;
			return;
		}
		DBVitesses_Id = m_DBVitesses.getResultInt(0);
		m_DBVitesses.sqlSkipData();

		// Chargement des puissances et des niveaux de puissance concernés
		for(nOctave = 0; nOctave<Nb_Octaves+1;nOctave++)
		{
			m_DBPuissances.AddParamInt(0, DBAllures_Id);
			m_DBPuissances.AddParamInt(1, DBRevetements_Id);
			m_DBPuissances.AddParamInt(2, DBTypeVehicules_Id);
			m_DBPuissances.AddParamInt(3, DBVitesses_Id);
			m_DBPuissances.AddParamInt(4, nOctave+1);
			m_DBPuissances.sqlExec();
			bOk = m_DBPuissances.sqlFetch();
			if (!bOk)
			{
				m_pNetwork->log()<<"Puissance not found in the LoiEmission database for the following characteristics : "<<endl;
				m_pNetwork->log()<<"Road surface      : "<< nom_rev <<endl;
				m_pNetwork->log()<<"Speed category    : "<< strAllure << endl;
				m_pNetwork->log()<<"Vehicle type      : "<< type_veh << endl;
				m_pNetwork->log()<<"Speed             : "<< dbVit << endl;
				if( nOctave < Nb_Octaves)
					m_pNetwork->log()<<"Octave              : " << nom_octave[nOctave] << " Hz" << endl;
				else
					m_pNetwork->log()<<"Octave              : Global " << endl;            
			}
			else
			{
				double dbPuissance = m_DBPuissances.getResultDouble(0);
				pW[nOctave] = dbPuissance * m_dbFactPuis;
				m_DBPuissances.sqlSkipData();
			}
		}
	}
	catch(ExceptionSymuvia &e)
	{
        m_pNetwork->log() << endl;
        m_pNetwork->log() << Logger::Error << " Error requesting the LoiEmission database --> "<<endl;
		m_pNetwork->log() << Logger::Error << " Error message : "<< e.m_strError << endl;
        m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
	}

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Loi_emission::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Loi_emission::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Loi_emission::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_dbFactPuis);
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);

    // rechargement de la liaison avec la BDD si besoin
    if(Archive::is_loading::value && !m_bLoaded)
    {
        LoadDatabase();
    }
}