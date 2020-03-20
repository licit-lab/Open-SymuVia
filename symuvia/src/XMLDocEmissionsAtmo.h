#pragma once
#ifndef XMLDocEmissionAtmoH
#define XMLDocEmissionAtmoH

#include <xercesc/util/XercesVersion.hpp>

#include <string>

class DOMLSSerializerSymu;

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMElement;
    class DOMDocument;
};
class STime;
class Reseau;

/*===========================================================================================*/
/* Classe de modélisation du document XML des émissions atmosphériques                       */
/*===========================================================================================*/
class XMLDocEmissionsAtmo
{
private:

	std::string				            m_strFilename;
	DOMLSSerializerSymu		            * m_XmlWriter;

	XERCES_CPP_NAMESPACE::DOMNode		* m_XmlNodeInstants;
	XERCES_CPP_NAMESPACE::DOMNode		* m_XmlNodeSimulation;
	XERCES_CPP_NAMESPACE::DOMElement	* m_XmlNodeVehicules;

	// Le document XML
	XERCES_CPP_NAMESPACE::DOMDocument   * pXMLDoc;

    Reseau                              * m_pNetwork;

public:
    // Constructeurs
	XMLDocEmissionsAtmo(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument* pXMLDocument)
	{
        m_pNetwork = pNetwork;
		pXMLDoc = pXMLDocument;
	}
	~XMLDocEmissionsAtmo();

    // Initialisation
	void Init(const std::string & strFilename, const STime &dtDebut, const std::string & strVersionExe, const std::string & strTimeTrafic, const std::string & strVersionDB, const std::string & strTitre, const std::string & strDate);

    // Fin
	void Terminate();

    // Retourne le flux XML du dernier instant
	XERCES_CPP_NAMESPACE::DOMNode *	GetLastInstant();

    // Ajout d'un instant
	XERCES_CPP_NAMESPACE::DOMNode *	AddInstant(double dbInstant, int nNbVeh);

    // Sauvegarde des données atmosphériques d'un instant
	void SaveLastInstant();
    void ReleaseLastInstant();
	void Addval(double dbVCO2, double dbVNOx, double dbVPM, double dbCumCO2, double dbCumNOx, double dbCumPM );

	// Sauvegarde des données atmosphériques d'un véhicule (cumul)
	void AddVehicule(int nID, const std::string & strType, const std::string & strEntree, double dbInstCreation, const std::string & strSortie, double dbInstSortie, double dbCumCO2, double dbCumNOx, double dbCumPM, double dbDstParcourue, bool bAddToCreationNode) ;

    // ajout la description d'un véhicule sur sa création
    virtual void AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif) {};

	// ajout la description d'un véhicule lors de la sortie d'un véhicule du réseau
	virtual void AddVehiculeToSortieNode(int nID, const std::string & strSortie, double dbInstSortie, double dbVit) {};

    // Sauvegarde des donénes atmosphériques d'un véhicule sur l'instant courant
	void AddVehiculeInst(int nID, double dbCO2, double dbNOx, double dbPM);




};

#endif