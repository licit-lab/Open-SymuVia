#pragma once
#ifndef PonctualSensorH
#define PonctualSensorH

#include "AbstractSensor.h"
#include "../tools.h"

#include <map>
#include <deque>
#include <vector>

class TypeVehicule;
class Vehicule;
class Voie;


/*===========================================================================================*/
/* Structure PonctualSensorData                                                              */
/*===========================================================================================*/
struct PonctualSensorData
{
    std::vector<int>	nCumNbVehFranchissant;		// Vecteur du nombre de véhicule ayant franchi le capteur depuis le début
                                                    // de la simulation détaillé voie par voie    

    std::vector<int>     nNbVehFranchissant;		// Vecteur du nombre de véhicule ayant franchi le capteur depuis le début
                                                    // de la période courante détaillé voie par voie

    std::vector<double>  dbCumulInvVit;				// Vecteur du cumul des inverses de vitesse pour les véhicules ayant
                                                    // franchi le capteur depuis le début de la période courante
                                                    // détaillé voie par voie
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};


class PonctualSensorSnapshot : public AbstractSensorSnapshot
{
public:
	std::map<TypeVehicule*, PonctualSensorData>	data;						// Map de la liste des états capteur utilsateur

    std::deque<int>         nIDVehFranchissant;     // Liste des ID des véhicules ayant franchi le capteur depuis le début
												    // de la période courante
    std::deque<std::string> cTypeVehFranchissant;   // Liste des types de véhicules ayant franchi le capteur depuis le début
												    // de la période courante
    std::deque<double>      dbInstVehFranchissant;  // Instants de franchissement des véhicules depuis le
												    // début de la période courante

    std::deque<int>         dbVoieVehFranchissant;  // Voies de franchissement des véhicules depuis le
												    // début de la période courante

	// Informations dynamiques sur une période glissante
	std::map<TypeVehicule*, std::map<Voie*, std::vector<double>, LessPtr<Voie>>, LessPtr<TypeVehicule>> pNbTypeVehInst;	// Tableau du nombre de véhicule classé par type ayant franchi le capteur à chaque instant
													// sur une période glissante égale à la période d'agrégation	

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


class PonctualSensor : public AbstractSensor
{
public:
    PonctualSensor();
    PonctualSensor(Tuyau * pTuyau, double dbPosition);
    PonctualSensor(const std::string & strNom, Tuyau * pTuyau, double dbPosition);
    virtual ~PonctualSensor();

    virtual std::string GetSensorPeriodXMLNodeName() const;

    void InitData(Reseau * pNetwork, double dbPeriod);

    virtual void ResetData();

    virtual void CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh);

    virtual void AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink = DBL_MAX);

    void CalculFranchissementMeso(double dbInstant, Vehicule * pVeh, Tuyau * pLink, double dbLengthInLink, bool & bFranchissement, double & dbInstFr, double & dbInvVit);

    void CalculFranchissement(Vehicule * pVeh, bool & bFranchissement, double & dbDst, int & nVoie, bool & bAmont);

    virtual void WriteDef(DocTrafic* pDocTrafic);
    virtual void Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput);

    virtual void PrepareNextPeriod();

    Tuyau * GetTuyau();
    double GetPosition();
    PonctualSensorSnapshot & GetData();
    std::deque<int> & GetNIDVehFranchissant();
    std::deque<double> & GetInstVehFranchissant();
    std::deque<std::string> & GetTypeVehFranchissant();
    std::deque<int> & GetVoieVehFranchissant();
    std::map<TypeVehicule*, std::map<Voie*, std::vector<double>, LessPtr<Voie>>, LessPtr<TypeVehicule>> & GetNbTypeVehInst();

    int GetLastInd();
    void SetLastInd(int);

    virtual AbstractSensorSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup);

    void SetTuyauAmont(Tuyau * pTuyAm);

private:
    bool CheckTuyauAmont(Vehicule * pVeh);

    void AddVehicleInfos(PonctualSensor * pCpt, Vehicule * pVeh, int nVoie, double dbInstFr, double dbInvVit);

protected:
    Tuyau * m_pTuyau;           // Troncon sur lequel est positionné le capteur ponctuel
    double  m_dbPosition;       // Positionnement en m du capteur (à partir du début du tuyau)
    int     nLastInd;           // Index auquel inscrire les données du pas de temps courant (utilisé pour convergents uniquement, non sérialisé car non persistent entre les PDT)

    Tuyau * m_pTuyauAmont;      // Troncon amont éventuel. Si renseigné, les véhicules pris en compte par le capteur ne sont que ceux dont le tronçon non interne précédent est ce tronçon

    // Données de travail du capteur
    PonctualSensorSnapshot m_Data;

    // Variable temporaire utilisée pour gérer le chevauchement d'un pas de temps sur deux périodes d'agrégation
    PonctualSensor * m_pNextPeriodSensor;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

#endif // PonctualSensorH