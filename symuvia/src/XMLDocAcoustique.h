#pragma once
#ifndef XMLDocAcoustiqueH
#define XMLDocAcoustiqueH

#include "DocAcoustique.h"

#include <xercesc/util/XercesVersion.hpp>

class STime;
class DOMLSSerializerSymu;
namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMDocument;
};
class Reseau;

/*===========================================================================================*/
/* Classe de modélisation du document XML des données acoustiques       					 */
/*===========================================================================================*/
class XMLDocAcoustique : public DocAcoustique
{
private:

    Reseau *                m_pNetwork;

	std::string			    m_strFilename;

	DOMLSSerializerSymu		* m_XmlWriter;

    bool                    m_bSave;                // Indique si le document XML est sauvegardé

	XERCES_CPP_NAMESPACE::DOMNode   *m_XmlNodeInstants;	
	XERCES_CPP_NAMESPACE::DOMNode	*m_XmlNodeCellules;
	XERCES_CPP_NAMESPACE::DOMNode	*m_XmlNodeSimulation;	
    XERCES_CPP_NAMESPACE::DOMNode	*m_XmlNodeSRCS;	
    XERCES_CPP_NAMESPACE::DOMNode	*m_XmlNodeCELLS;	

    // Constructeurs
	//XMLDocAcoustique()
	//{
	//};

	// Le document XML
	XERCES_CPP_NAMESPACE::DOMDocument * pXMLDoc;

    bool                    m_bReinitialized;       // Indique si le writer courant est issu d'un restore de snapshot
    bool                    m_bAtLeastOneInstant;   // pour reprise du snapshot : permet de savoir comment refermer le noeud instant

public:

    XMLDocAcoustique() : m_bReinitialized(false), m_bAtLeastOneInstant(false)
    {
        m_pNetwork = NULL;
        pXMLDoc = NULL;
    }

    XMLDocAcoustique(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument * pXMLDocument) : m_bReinitialized(false), m_bAtLeastOneInstant(false)
	{
        m_pNetwork = pNetwork;
		pXMLDoc = pXMLDocument;
	}

	virtual ~XMLDocAcoustique();

	void setSave(bool bSave)
	{
		m_bSave = bSave;
	}

	//XMLDocAcoustique(bool bSave)
	//{
 //       //m_strLibOctave = gcnew array<String^>(9){"63Hz","125Hz","250Hz","500Hz","1000Hz","2000Hz","4000Hz","8000Hz","Glob"};
 //       m_bSave = bSave;
	//};

    // Initialisation
	void Init(const std::string &strFilename, const STime &dtDebut, const std::string &strVersionExe, const std::string &strTimeTrafic, const std::string &strVersionDB, const std::string &strTitre, const std::string &strDate);

    // Re-initialisation (après une désérialisation par exemple)
    virtual void ReInit(std::string result);

	// Fin
	virtual void Terminate();

    // Mise en veille (prise snapshot pour reprise plus tard)
    virtual void Suspend();

    // Retourne le flux XML du dernier instant
	XERCES_CPP_NAMESPACE::DOMNode*  GetLastInstant();

    // libère le DOMNode associé au dernier instant (il est sauvegardé)
    void        ReleaseLastInstant();

    // Ajout d'un instant
	virtual /*DOMNode**/XMLDocAcoustique::PINSTANT	AddInstant(double dbInstant, int nNbVeh);

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif


