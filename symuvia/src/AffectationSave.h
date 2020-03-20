#pragma once
#ifndef affectationSaveH
#define affectationSaveH

#include "TimeUtil.h"

namespace boost {
    namespace serialization {
        class access;
    }
}

#include <deque>
#include <vector>
#include <map>
#include <set>

class Vehicule;
class TypeVehicule;
class Tuyau;
class DOMLSSerializerSymu;
class SymuCoreManager;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Classes aidant à la sauvegarde des affectations
//////////////////////////////////////////////////////////////////////////////////////////////////

// Structure pour la sauvegarde des itinéraires prédits et réalisés
class CItineraire
{
public:
	CItineraire()
	{
		coeffs = 0.0;
		tpsparcours = 0.0;
        distance = 0.0;
        predefini = false;
	}
public:
	std::vector<Tuyau*>		 tuyaux;		    // liste des objets Tuyau constituant l'itinéraire
	std::vector<std::string> elements;			// liste des noms des Tuyaux et des connexions constituant l'itinéraire
    std::set<int>           vehicleIDs;         // Ensemble des identifiants de véhicules ayant été affectés à cet itinéraire
	double coeffs;								// coeficient d'affectation de l'itinéraire
	double tpsparcours;							// temps de parcours de l'itinéraire de la période courante, prédit si itération, réalisé si période convergente
    double distance;                            // Longueur de l'itinéraire
    bool predefini;                             // Flag indiquant si l'itinéraire est prédéfini ou calculé
public:
	void write(DOMLSSerializerSymu * writer, bool bIsRealized);	// écriture d'un itinéraire dans le fichier de sauvegarde des affectations

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure pour la sauvegarde des itérations prédites
class CIteration
{
public:
	CIteration()
	{
		num = 0;
		temps_calcul = 0;
		indice_proximite = 0.0;		
	}
public :
	size_t num;								// numéro de l'itération
	int temps_calcul;						// temps de calcul de l'itération
	double indice_proximite;				// indice de proximité de la convergence
	std::deque<CItineraire> itineraires;	// liste des itinéraires

	STime temps_debut_calcul;					// temps du début du calcul
public:
	CItineraire * addItineraire(const std::vector<Tuyau*> & tuyaux, const std::vector<std::string> & elements, double coeffs, double tpsparcours,
                                bool bPredefini, double dbLength); // ajout d'un itinéraire
	void write(DOMLSSerializerSymu * writer);	// écriture d'un itinéraire dans le fichier de sauvegarde des affectations

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure pour la sauvegarde des couples OD
class CCoupleOD
{
public:
	CCoupleOD()
	{
		nb_veh_emis = 0;
		nb_veh_recus = 0;
	}
	CCoupleOD(const std::string & origineA, const std::string & destinationA)
	{
		origine = origineA;
		destination = destinationA;
		nb_veh_emis = 0;
		nb_veh_recus = 0;
	}
public:
	std::string origine;					// nom de l'origine
	std::string destination;				// nom de la destination
	int nb_veh_emis;						// nombre de véhicules entrants
	int nb_veh_recus;						// nombre de véhicules sortants
	std::deque<CIteration> predit;			// liste des itérations des itinéraires prédits
	std::deque<CItineraire> realise;		// liste des itinéraires réalisés

public:
	CIteration * AddPreditIteration(size_t num);//, double indice_proximite = 0.0);	// Ajout d'in itinéraire prédit

	CItineraire * AddRealiseItineraire(const std::vector<Tuyau*> & tuyaux, const std::vector<std::string> &, bool bPredefini, int vehID, double dbLength);   // Ajout d'un itinéraire réalisé

	void write(DOMLSSerializerSymu * writer);	// écriture d'une itération dans le fichier de sauvegarde des affectations

private:
	CItineraire * GetRealiseItineraire(const std::vector<std::string> &elements);	// Obtention d'un itinéraire réalisé

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure pour la sauvegarde d'une période
class CPeriode
{
public:
	CPeriode()
	{
		type.clear();
		debut = 0;
		fin = 0;
		temps_calcul = 0;
		nb_iteration = 0;
		indicateur_proximite = 1.0;
		seuil_tempsparcours = 1.0;
		temps_debut_calcul = STime();
	}
	~CPeriode()
	{
		free();
	}
public:
	std::string type;						// "periodique" ou "evenementiel" 
	double debut;							// instant du début
	double fin;								// instant de fin
	int temps_calcul;						// durée du calcul de la période
	int nb_iteration;						// nombre d'itérations pour la convergence
	double indicateur_proximite;			// indicateur de proximité
	double seuil_tempsparcours;				// seuil des temps de parcours

											// liste de couples origine/destination par type de véhicule
											// la liste de couples est indexée sur la paire (origine, destination)
	std::map<TypeVehicule*, std::map<std::pair<std::string, std::string>, CCoupleOD>> periode_type_vehicule;
	STime temps_debut_calcul;				// Instant où les calculs ont commencé.
public:
	void InitPeriode();							// Initialisation de la période
    void FinPeriode(SymuCoreManager * pSymuScript, const std::string & sType, int nb_iterationA, double indicateur_proximiteA, double finA, double seuil_tempsparcoursA, bool bForceGraphReset); // Traitements de fin de période
	CCoupleOD * getCoupleOD(TypeVehicule* pTVeh, const std::string & sOrig, const std::string & sDest); // Obtention d'un couple OD
	std::map<std::pair<std::string, std::string>, CCoupleOD> * addTypeVehicule(TypeVehicule* pTVeh); // Ajout d'un type de véhicule
	CCoupleOD * addCoupleOD(TypeVehicule* pTVeh, const std::string & sOrig, const std::string & sDest); // Ajout d'un couple O/D
	CCoupleOD * addCoupleOD(std::map<std::pair<std::string, std::string>, CCoupleOD> * pListCoupleOD, const std::string & sOrig, const std::string & sDest); // Ajout d'un couple O/D
	void SetLastPreditIterationIndProx(double dbIndProx);	// Positionnement de l'indice de proximité et du temps de calcul de la dernière itération
	void free()									// Libération des données de la période
	{
		type.clear();
		debut = 0;
		fin = 0;
		temps_calcul = 0;
		nb_iteration = 0;
		indicateur_proximite = 1.0;
		seuil_tempsparcours = 1.0;
		periode_type_vehicule.clear();
		temps_debut_calcul = STime();
	}
	void	CleanPeriode(bool bClear);						// Nettoyage de la période avant de calculer une nouvelle période

	void write(DOMLSSerializerSymu * writer);	// Ecriture d'une période dans le fichier de sauvegarde des affectations

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif