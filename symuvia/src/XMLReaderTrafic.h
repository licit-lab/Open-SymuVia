#pragma once
#ifndef XMLReadTraficH
#define XMLReadTraficH

#include <string>
#include <fstream>
#include <map>

class XmlReaderSymuvia;
class Logger;


struct VehTraficData {
    std::string type;
    std::string entree;
    std::string sortie;
    std::string instC;
    std::string instE;
    std::string instS;
    std::string dstParcourue;
};


/*===========================================================================================*/
/* Classe de gestion de la lecture du document XML des données trafic					     */
/*===========================================================================================*/
class XMLReaderTrafic
{
private:
	std::string			m_strFilename;

	XmlReaderSymuvia*	m_XmlReaderTraj;	// Curseur des trajectoires des véhicules au cours de la simu	
	XmlReaderSymuvia*	m_XmlReaderVeh;		// Curseur de la liste des véhicules
    XmlReaderSymuvia*	m_XmlReaderSimuCell;// Curseur des variables de simulation des cellules 
    std::map<std::string, VehTraficData> m_mapVehData; // copie mémoire des données du noeud <VEHS>

    Logger* pFicSimulation;

    // Constructeur
	XMLReaderTrafic()
	{
		m_XmlReaderTraj = NULL;
		m_XmlReaderVeh = NULL;
		m_XmlReaderSimuCell = NULL;
	}

public:
    XMLReaderTrafic(Logger* pFS);

    ~XMLReaderTrafic();

    // Initialisation
	bool Init(const std::string & strFilename, std::string & sTimeExecTraf);
    
    // Routine permettant la lecture des trajectoires
	int ReadNextTrajectoire(double dbInstant, int nID);

    // Retourne la valeur d'un attribut pour la trajectoire courante
	std::string GetValueAttrFromTrajectoire(const std::string & ssAttr);

    // Lecture des données des véhicules
    void ReadVehicules();

    // Retourne les données pour le véhicule demandé
	VehTraficData & GetAttrFromVehicule(const std::string & id);

    // Routine de lecture des cellules au cours de la simulation
    int ReadNextSimuCell(double dbInstant, int nID);

    // Retourne la valeur d'un attribut pour la cellule de simulation courante
	std::string GetValueAttrFromSimuCell(const std::string & ssAttr);

};

#endif