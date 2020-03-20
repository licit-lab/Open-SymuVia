#include "stdafx.h"
#include "Connection.h"

#include "reseau.h"
#include "tuyau.h"
#include "TuyauMeso.h"
#include "voie.h"
#include "vehicule.h"
#include "Xerces/XMLUtil.h"
#include "SystemUtil.h"
#include "ControleurDeFeux.h"
#include "regulation/RegulationBrique.h"
#include "regulation/RegulationModule.h"
#include "Logger.h"
#include "SymuCoreManager.h"
#include "usage/SymuViaTripNode.h"

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

using namespace std;

XERCES_CPP_NAMESPACE_USE

Connexion::Connexion():CMesoNode()
{	
	m_pReseau = NULL;
	m_nZLevel = 0;
    m_pCtrlDeFeux = NULL;

	m_nID = -1;

    m_nNbMaxVoies = 0;

    Abscisse = 0;
    Ordonne = 0;
}

Connexion::Connexion(std::string strID, Reseau *pReseau):CMesoNode()
{
	m_strID = strID;
	m_pReseau = pReseau;
	m_nZLevel = 0;
    m_pCtrlDeFeux = NULL;

	m_nID = pReseau->m_nIncCnx++;

    m_nNbMaxVoies = 0;

    Abscisse = 0;
    Ordonne = 0;
}

Connexion::~Connexion()
{
    for (std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionFlux> >* >::iterator iter = m_mapLstRepartition.begin(); iter != m_mapLstRepartition.end(); ++iter)
    {
        EraseListOfVariation(iter->second);
        delete iter->second;
    }   
}

MouvementsSortie::MouvementsSortie()
{
    m_pReseau = NULL;
    m_bHasVitMax = false;
    m_dbVitMax = DBL_MAX;
    m_dbTaux = 0;
}

// Teste si un tronçon est un tronçon amont de la connexion
bool Connexion::IsTuyauAmont(Tuyau *pTAm)
{
    if( m_LstTuyAm.size()>0)
    {
    
	    return std::find( m_LstTuyAm.begin(), m_LstTuyAm.end(), pTAm ) != m_LstTuyAm.end();
    }
    return false;
}

// Teste si un tronçon est un tronçon aval de la connexion
bool Connexion::IsTuyauAval(Tuyau *pTAv)
{
    if( m_LstTuyAv.size() >0)
    {
	    return std::find( m_LstTuyAv.begin(), m_LstTuyAv.end(), pTAv ) != m_LstTuyAv.end();
    }
    return false;
}

// Teste si un mouvement est autorisé
bool Connexion::IsMouvementAutorise(Voie *pVAm, Tuyau *pTAv, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
	if( ! IsTuyauAmont( (Tuyau*)pVAm->GetParent()) )
		return false;

	if( !IsTuyauAval(pTAv) )
		return false;	

	std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itVoieAmont;
	itVoieAmont = m_mapMvtAutorises.find( pVAm );
	if( itVoieAmont != m_mapMvtAutorises.end() )
	{
		std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator itTuyauAval;
		itTuyauAval = itVoieAmont->second.find(pTAv);
		if( itTuyauAval != itVoieAmont->second.end() )
		{
			std::map<int, boost::shared_ptr<MouvementsSortie> > & pmapMvtSort = itTuyauAval->second;
            std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator iter;
            for(iter = pmapMvtSort.begin(); iter != pmapMvtSort.end(); iter++)
            {
                if(!(iter->second->IsTypeExclu(pTV, pSousType)))
                    return true;
            }
		}
	}

	return false;
}

// Teste si un mouvement est autorisé
bool Connexion::IsMouvementAutorise(Tuyau *pTAm, Voie *pVAv, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
    if (!IsTuyauAmont(pTAm))
		return false;

    if (!IsTuyauAval((Tuyau*)pVAv->GetParent()))
		return false;	

	std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itVoieAmont;
	for(itVoieAmont = m_mapMvtAutorises.begin(); itVoieAmont != m_mapMvtAutorises.end(); ++itVoieAmont)
	{
		std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator itTuyauAval;
        itTuyauAval = itVoieAmont->second.find((Tuyau*)pVAv->GetParent());
		if( itTuyauAval != itVoieAmont->second.end() )
		{
			std::map<int, boost::shared_ptr<MouvementsSortie> > & pmapMvtSort = itTuyauAval->second;
            std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator iter = itTuyauAval->second.find(pVAv->GetNum());
            if (iter != itTuyauAval->second.end())
            {
                if(!(iter->second->IsTypeExclu(pTV, pSousType)))
                    return true;
            }
		}
	}

	return false;
}

// Teste si un mouvement est autorisé
bool Connexion::IsMouvementAutorise(Voie *pVAm, Voie *pVAv, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
    if (!IsTuyauAmont((Tuyau*)pVAm->GetParent()))
        return false;

    if (!IsTuyauAval((Tuyau*)pVAv->GetParent()))
        return false;

    std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itVoieAmont;
    itVoieAmont = m_mapMvtAutorises.find(pVAm);
    if (itVoieAmont != m_mapMvtAutorises.end())
    {
        std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator itTuyauAval;
        itTuyauAval = itVoieAmont->second.find((Tuyau*)pVAv->GetParent());
        if (itTuyauAval != itVoieAmont->second.end())
        {
            std::map<int, boost::shared_ptr<MouvementsSortie> > & pmapMvtSort = itTuyauAval->second;
            std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator iter = pmapMvtSort.find(pVAv->GetNum());
            if (iter != itTuyauAval->second.end())
            {
                if (!(iter->second->IsTypeExclu(pTV, pSousType)))
                    return true;
            }
        }
    }

    return false;
}

// Accesseur (ajouté pour accès depuis python)
 std::map<int, boost::shared_ptr<MouvementsSortie> > & Connexion::GetMovement(Voie * pVoie, Tuyau * pTuyau)
 {
     return m_mapMvtAutorises[pVoie][pTuyau];
 }

 bool Connexion::HasReservedLaneForMove(Tuyau * pUpstreamLink, Tuyau * pDownstreamLink)
 {
     std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itVoieAmont;
     for (itVoieAmont = m_mapMvtAutorises.begin(); itVoieAmont != m_mapMvtAutorises.end(); ++itVoieAmont)
     {
         if (itVoieAmont->first->GetParent() == pUpstreamLink)
         {
             std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator itTuyauAval = itVoieAmont->second.find(pDownstreamLink);
             if (itTuyauAval != itVoieAmont->second.end() && itVoieAmont->second.size() == 1)
             {
                 // La voie amont a pour unique tuyau aval le tuyau cible.
                 return true;
             }
         }
     }
     return false;
 }

// Teste si un mouvement est autorisé
bool Connexion::IsMouvementAutorise(Tuyau *pTAm, Tuyau *pTAv, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
	if( ! IsTuyauAmont( pTAm ) )
		return false;

	if( !IsTuyauAval(pTAv) )
		return false;

	if( pTAv->GetBriqueAmont() )
		if( pTAv->GetBriqueAmont()->GetType() == 'G' )
			return true;

	for(int i=0; i<pTAm->getNb_voies(); i++)	
    {
		if( IsMouvementAutorise(pTAm->GetLstLanes()[i], pTAv, pTV, pSousType) )		
			return true;		
    }

	return false;
}

bool Connexion::GetCoeffsMouvementAutorise(Voie *pVAm, Tuyau *pTAv, std::map<int, boost::shared_ptr<MouvementsSortie> > &mapCoeffs, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
	if( !this->IsMouvementAutorise(pVAm, pTAv, pTV, pSousType) )
		return false;

	mapCoeffs = m_mapMvtAutorises.find( pVAm )->second.find(pTAv)->second;

	return true;
}

void Connexion::GetCoeffsMouvementAutorise(Voie *pVAm, Tuyau *pTAv, std::map<int, boost::shared_ptr<MouvementsSortie> > &mapCoeffs)
{
    std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator iter = m_mapMvtAutorises.find( pVAm );
    if(iter != m_mapMvtAutorises.end())
    {
        std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator iter2 = iter->second.find(pTAv);
        if(iter2 != iter->second.end())
        {
            mapCoeffs = iter2->second;
        }
    }
}

//================================================================
    int Connexion::GetNoEltAmont
//----------------------------------------------------------------
// Fonction   :  Renvoie l'indice de l'élément amont passé en paramètre
// Version du :  07/02 /2005
// Historique :  07/02/2005 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    Tuyau   *pTuyau
)
{
    for(int i=0; i < (int)m_LstTuyAm.size(); i++)
    {
        if( pTuyau == m_LstTuyAm[i] )
            return i;
    }

    return -1;
}

//================================================================
    int Connexion::GetNoEltAval
//----------------------------------------------------------------
// Fonction   :  Renvoie l'indice de l'élément aval passé en paramètre
// Version du :  07/02 /2005
// Historique :  07/02/2005 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    Tuyau   *pTuyau
)
{
    for(int i=0; i < (int)m_LstTuyAv.size(); i++)
    {
        if( pTuyau == m_LstTuyAv[i] )
            return i;
    }

    return -1;
}

//================================================================
    double Connexion::GetRepartition
//----------------------------------------------------------------
// Fonction  : Retourne la répartition du flux de l'élément amont
//             vers l'élément aval passé en paramètre à un instant
//             donné
// Version du: 29/09/2006
// Historique: 29/09/2006 (C.Bécarie - Tinea)
//              Si l'instant simulé est supérieur à la somme des durées
//              des répartitions définies, on retourne la dernière
//              répartition
//             11/01/2005 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    TypeVehicule * pTypeVeh,// Type de véhicule
    int nAmont,             // Numéro du tronçon amont
    int nVoieAmont,         // Numéro de la voie amont
    int nAval,              // Numéro du tronçon aval
    int nVoieAval,          // Numéro de la voie val
    double dbTime           // Instant où on veut connaître la répartition
)
{    
    RepartitionFlux*    pRF;

    // Vérification
	if( nAmont >= (int)m_LstTuyAm.size() || nAval >= (int)m_LstTuyAv.size() )
        return 0;
    if(  nVoieAmont >= m_nNbMaxVoies || nVoieAval >= m_nNbMaxVoies )
        return 0;

    std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionFlux> >* >::const_iterator iterTypeVeh = m_mapLstRepartition.find(pTypeVeh);
    if (iterTypeVeh == m_mapLstRepartition.end())
        return 0;

    pRF = GetVariation(dbTime, iterTypeVeh->second, m_pReseau->GetLag());

    if(!pRF)
        return 0;

    return pRF->pCoefficients[nAmont*m_nNbMaxVoies + nVoieAmont][nAval*m_nNbMaxVoies + nVoieAval];
}

void Connexion::AddEltAmont(Tuyau *pT)
{
	m_LstTuyAm.push_back( pT );
    
};

void Connexion::AddEltAval(Tuyau *pT)
{
    m_LstTuyAv.push_back( pT );
};

/*bool Connexion::LoadDirectionalCoefficients(DOMNode *xmlNodeDirCoefs)
{
}*/

bool Connexion::LoadMouvementsAutorises(XERCES_CPP_NAMESPACE::DOMNode *xmlNodeMvtsAutorises, Logger * pLogger, bool bInsertion, std::string strTAvPrincipal, int nbVoiesEch)
{
	int nVAm, nVAv;
	std::string sId;
	double dbTaux;
	std::string slibelle;
	double dbLigneFeu;
    double dbVitMax;
    bool bHasVitMax;

	if( !xmlNodeMvtsAutorises)
		return false;

	XMLSize_t counti = xmlNodeMvtsAutorises->getChildNodes()->getLength();
	for(XMLSize_t i=0; i<counti; i++)
	{
		DOMNode * xmlChildi = xmlNodeMvtsAutorises->getChildNodes()->item(i);
		if (xmlChildi->getNodeType() != DOMNode::ELEMENT_NODE) continue;

		// Chargement du libellé du tronçon amont
		::GetXmlAttributeValue( xmlChildi, "id_troncon_amont", sId, pLogger);
		Tuyau *pTAm = m_pReseau->GetLinkFromLabel( sId );

       
		// Vérifications
		if(!pTAm)
			return false;
     
		if(!this->IsTuyauAmont(pTAm) )
			return false;
		
		// Chargement de la voie amont
		::GetXmlAttributeValue( xmlChildi, "num_voie_amont", nVAm, pLogger);

		// Vérification
		if(pTAm->getNb_voies() < nVAm )
			return false;

       
		Voie *pVAm = pTAm->GetLstLanes()[nVAm-1];
		if(!pVAm)
			return false;

		// Chargement de l'aval des mouvements
        DOMNode *xmlNodeMvtAvals = NULL;
        XMLSize_t countMvtAutoChilds = xmlChildi->getChildNodes()->getLength();
        for (XMLSize_t iChild = 0; iChild < countMvtAutoChilds; iChild++)
        {
            DOMNode * pXMLChild = xmlChildi->getChildNodes()->item(iChild);
            if (pXMLChild->getNodeType() != DOMNode::ELEMENT_NODE) continue;

            std::string childNodeName = US(pXMLChild->getNodeName());
            if (!childNodeName.compare("MOUVEMENT_SORTIES"))
            {
                xmlNodeMvtAvals = pXMLChild;
                break;
            }
        }

		XMLSize_t count = xmlNodeMvtAvals->getChildNodes()->getLength();
		for(XMLSize_t j=0; j< count; j++)
		{
			DOMNode *xmlNodeMvtAval = xmlNodeMvtAvals->getChildNodes()->item(j);
			if (xmlNodeMvtAval->getNodeType() != DOMNode::ELEMENT_NODE) continue;

			// Chargement du libellé du tronçon aval
			::GetXmlAttributeValue( xmlNodeMvtAval, "id_troncon_aval", sId, pLogger);

            // Cas particulier de l'insertion
            // EVOLUTION n°70 : si plusieurs tronçon aval cas de "l'échangeur", il faut incrémenter le numéro de voie
            // du nombre de voies du tuyau aval secondaire.
            int nDecalVoie = 0;
            if(bInsertion)
            {
                if(!sId.compare(strTAvPrincipal))
                {
                    nDecalVoie = nbVoiesEch;
                }
                sId = "T_" + GetID().substr(2,GetID().find("_INSERTION_")-2) + "_INSERTION";
            }

			Tuyau *pTAv = m_pReseau->GetLinkFromLabel( sId );

			// Vérifications
			if(!pTAv)
				return false;

			if(!this->IsTuyauAval(pTAv) )
				return false;
			
			// Chargement de la voie aval
			::GetXmlAttributeValue( xmlNodeMvtAval, "num_voie_aval", nVAv, pLogger);

            // EVOLUTION n°70 - décalage des numéros de voie du tronçon principal (qui n'est plus le tronçn le plus à droite)
            if(bInsertion)
            {
                nVAv+=nDecalVoie;
            }


			// Vérification
			if(pTAv->getNb_voies() < nVAv)
				return false;

			// Chargement du taux d'utilisation
			::GetXmlAttributeValue( xmlNodeMvtAval, "taux_utilisation", dbTaux, pLogger);

			// Chargement du libellé
			::GetXmlAttributeValue( xmlNodeMvtAval, "libelle", slibelle, pLogger);

			// Chargement de la ligne de feu
			::GetXmlAttributeValue( xmlNodeMvtAval, "ligne_feu", dbLigneFeu, pLogger);

            bool bTmp = false;
            GetXmlAttributeValue(xmlNodeMvtAval, "ligne_feu_relative", bTmp, pLogger);
            if(bTmp)
            {
                dbLigneFeu *= pTAm->GetLength();
            }

            // Chargement de la vitesse maximum
            dbVitMax = DBL_MAX;
			bHasVitMax = GetXmlAttributeValue( xmlNodeMvtAval, "vit_max", dbVitMax, pLogger);

            // Chargement des types de véhicules exclus
            std::string strTmp;
            GetXmlAttributeValue(xmlNodeMvtAval, "exclusion_types_vehicules", strTmp, pLogger); 
            std::deque<std::string> split = SystemUtil::split(strTmp, ' ');
            std::vector<TypeVehicule*> pVectTypesInterdits;
            for(size_t typeIdx = 0; typeIdx < split.size(); typeIdx++)
            {
                pVectTypesInterdits.push_back(m_pReseau->GetVehicleTypeFromID(split[typeIdx]));
            }
		
			// Création ou mise à jour
			std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itVAm = this->m_mapMvtAutorises.find( pVAm );
			if( itVAm != this->m_mapMvtAutorises.end() )
			{
				std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator itTAv = (*itVAm).second.find( pTAv );
				if( itTAv != (*itVAm).second.end() )
				{
					boost::shared_ptr<MouvementsSortie> MS(new MouvementsSortie());
                    MS->m_pReseau = m_pReseau;
					MS->m_dbTaux = dbTaux;
					MS->m_strLibelle = slibelle;
					MS->m_dbLigneFeu = dbLigneFeu;
                    MS->m_dbVitMax = dbVitMax;
                    MS->m_bHasVitMax = bHasVitMax;
                    MS->m_LstForbiddenVehicles[NULL] = pVectTypesInterdits;
					(*itTAv).second.insert( pair<int, boost::shared_ptr<MouvementsSortie> >(nVAv-1, MS) );
				}
				else
				{
					std::map<int, boost::shared_ptr<MouvementsSortie> > mapTaux;
					boost::shared_ptr<MouvementsSortie> MS(new MouvementsSortie());
                    MS->m_pReseau = m_pReseau;
					MS->m_dbTaux = dbTaux;
					MS->m_strLibelle = slibelle;
					MS->m_dbLigneFeu = dbLigneFeu;
                    MS->m_dbVitMax = dbVitMax;
                    MS->m_bHasVitMax = bHasVitMax;
                    MS->m_LstForbiddenVehicles[NULL] = pVectTypesInterdits;
					mapTaux.insert( pair<int, boost::shared_ptr<MouvementsSortie> >(nVAv-1, MS) );
					(*itVAm).second.insert( pair<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >>( pTAv, mapTaux ) );
				}
			}
			else
			{
				std::map<int, boost::shared_ptr<MouvementsSortie> > mapTaux;
				boost::shared_ptr<MouvementsSortie> MS(new MouvementsSortie());
                MS->m_pReseau = m_pReseau;
				MS->m_dbTaux = dbTaux;
				MS->m_strLibelle = slibelle;
				MS->m_dbLigneFeu = dbLigneFeu;
                MS->m_dbVitMax = dbVitMax;
                MS->m_bHasVitMax = bHasVitMax;
                MS->m_LstForbiddenVehicles[NULL] = pVectTypesInterdits;
				mapTaux.insert( pair<int, boost::shared_ptr<MouvementsSortie> >(nVAv-1, MS) );
				std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > mapTAv;
				mapTAv.insert( pair<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >>(pTAv, mapTaux));

				this->m_mapMvtAutorises.insert( pair<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >>(pVAm, mapTAv) );
			}	
		}

		// Vérification que la somme des taux d'utilisation pour un tronçon aval soit égale à 1 (ou 0)
		std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itVAm = this->m_mapMvtAutorises.find( pVAm );		
		if( itVAm != this->m_mapMvtAutorises.end() )
		{			
			//std::map<Tuyau*, std::map<int, double>>::iterator itTAv;
			std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator itTAv;
			for( itTAv = m_mapMvtAutorises.find( pVAm )->second.begin(); itTAv != m_mapMvtAutorises.find( pVAm )->second.end(); itTAv++)
			{
				std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator itTaux;
				double dbSum = 0;
                int nSum = 0;

                // Si on a plusieurs voies aval et que tous els taux vallent 1 (par défaut), on remplace par une équirépartition des taux des voies
                if((*itTAv).second.size() > 1)
                {
                    bool bAllOnes = true;
                    for(itTaux = (*itTAv).second.begin(); itTaux != (*itTAv).second.end() && bAllOnes; itTaux++)
                    {
					    if((*itTaux).second->m_dbTaux != 1)
                        {
                            bAllOnes = false;
                        }
                    }

                    if(bAllOnes)
                    {
                        for(itTaux = (*itTAv).second.begin(); itTaux != (*itTAv).second.end(); itTaux++)
                        {
                            (*itTaux).second->m_dbTaux = 1.0 / (double)(*itTAv).second.size();
                        }
                    }
                }


				for(itTaux = (*itTAv).second.begin(); itTaux != (*itTAv).second.end(); itTaux++)
                {
					dbSum += (*itTaux).second->m_dbTaux;
                    nSum++;
                }

				if( dbSum!=0 && fabs(dbSum-1) > nSum*std::numeric_limits<double>::epsilon())
					return false;
			}
		}
	}

	return true;
}

// Chargement des données d'affectation prédéfinies
bool Connexion::LoadAffectation(DOMNode *xmlNodeAffectation, bool bAllType, Logger *pChargement)
{
	DOMNode *xmlNodeTypeVehAffectation;
	std::string sTypeVeh;

	if( !xmlNodeAffectation)
		return false;

	if( !bAllType )		// Affectation par type de véhicule
	{
		//int count = xmlNodeAffectation->ChildNodes->Count;
		XMLSize_t count = xmlNodeAffectation->getChildNodes()->getLength();
		for(XMLSize_t i=0; i<count; i++)	// Boucle sur les enfants de AFFECTATIONS_TYPE_VEHICULE
		{
			//xmlNodeTypeVehAffectation = xmlNodeAffectation->ChildNodes[i];
			xmlNodeTypeVehAffectation = xmlNodeAffectation->getChildNodes()->item(i);
			if (xmlNodeTypeVehAffectation->getNodeType() != DOMNode::ELEMENT_NODE) continue;

			// Type de véhicule
			::GetXmlAttributeValue( xmlNodeTypeVehAffectation, "id_type_vehicule", sTypeVeh, pChargement);

			TypeVehicule *pTV = m_pReseau->GetVehicleTypeFromID( sTypeVeh );
			if( !pTV )
				continue;

            if (!LoadAffectationEx(m_pReseau->GetXMLUtil()->SelectSingleNode("./AFFECTATIONS", xmlNodeTypeVehAffectation->getOwnerDocument(), (DOMElement*)xmlNodeTypeVehAffectation), pTV, pChargement))
				return false;
		}
	}
	else	// Affectation commune à tous les types de véhicule
	{
		std::deque<TypeVehicule*>::iterator itTV;
		for( itTV = m_pReseau->m_LstTypesVehicule.begin(); itTV != m_pReseau->m_LstTypesVehicule.end(); itTV++)
		{
			if( !LoadAffectationEx( xmlNodeAffectation, (*itTV), pChargement) )
				return false;
		}
	}

	return true;
}



bool Connexion::LoadAffectationEx(DOMNode *xmlNodeAffectations, TypeVehicule *pTV, Logger *pChargement)
{
	DOMNode *xmlNodeVarTempAffectation;
	DOMNode *xmlNodeCoupleEntreeDestination;
	std::string sEntree, sSortie;
	std::string sDst;

	if( !xmlNodeAffectations)
		return false;	

	// Boucle sur les variantes temporelles
	XMLSize_t countj = xmlNodeAffectations->getChildNodes()->getLength();
	for(XMLSize_t j=0; j<countj; j++)
	{
		xmlNodeVarTempAffectation = xmlNodeAffectations->getChildNodes()->item(j);
		if (xmlNodeVarTempAffectation->getNodeType() != DOMNode::ELEMENT_NODE) continue;

		typeVarianteTemporelleAffectation VTAffectation;
        typePlageTemporelleAffectation PTAffectation;

		// Chargement de la durée de la variante		
        vector<PlageTemporelle*> plages; 
        if(!GetXmlDuree( xmlNodeVarTempAffectation, m_pReseau, VTAffectation.first, plages, pChargement))
        {
            return false;
        }

		// Boucle sur les couples entrée/destination
		std::map< typeCoupleEntreeDestination, typeCoeffAffectation> mpCoupleCoeff;

        DOMNode * xmlNodeCoupleEntreeDestinationList = m_pReseau->GetXMLUtil()->SelectSingleNode("./COUPLES_ENTREE_DESTINATION", xmlNodeVarTempAffectation->getOwnerDocument(), (DOMElement*)xmlNodeVarTempAffectation);
		XMLSize_t countk = xmlNodeCoupleEntreeDestinationList->getChildNodes()->getLength();
		for(XMLSize_t k=0; k<countk; k++)
		{
			xmlNodeCoupleEntreeDestination = (DOMElement*)xmlNodeCoupleEntreeDestinationList->getChildNodes()->item(k);
			if (xmlNodeCoupleEntreeDestination->getNodeType() != DOMNode::ELEMENT_NODE) continue;

			typeCoupleEntreeDestination cplEntreeDestination;

			::GetXmlAttributeValue( xmlNodeCoupleEntreeDestination, "id_troncon_amont", sEntree, pChargement);	// Tronçon amont

			Tuyau *pTAm = m_pReseau->GetLinkFromLabel( sEntree );
			if( !pTAm )
				continue;
			if( !this->IsTuyauAmont(pTAm) )
				continue;

			::GetXmlAttributeValue( xmlNodeCoupleEntreeDestination, "id_destination", sDst, pChargement);			// Destination
            char cDestType;
			SymuViaTripNode *pDest = m_pReseau->GetDestinationFromID( sDst, cDestType );
			if(!pDest)
				continue;

			cplEntreeDestination = ::make_pair( pTAm, pDest);

			// Boucle sur les sorties possibles
			typeCoeffAffectation coeffAffect;
            DOMNode * xmlNodeSortieConnexions = m_pReseau->GetXMLUtil()->SelectSingleNode("./SORTIES_CONNEXION", xmlNodeCoupleEntreeDestination->getOwnerDocument(), (DOMElement*)xmlNodeCoupleEntreeDestination);
			XMLSize_t countl = xmlNodeSortieConnexions->getChildNodes()->getLength();
			for(XMLSize_t l=0; l<countl; l++)
			{
				DOMNode * xmlNodeSortieConnexion = xmlNodeSortieConnexions->getChildNodes()->item(l);

				if (xmlNodeSortieConnexion->getNodeType() != DOMNode::ELEMENT_NODE) continue;

				::GetXmlAttributeValue( xmlNodeSortieConnexion, "id_troncon_aval", sSortie, pChargement);	// Tronçon aval
				Tuyau *pTAv = m_pReseau->GetLinkFromLabel( sSortie );
				if( !pTAv )
					continue;
				if( !this->IsTuyauAval(pTAv) )
					continue;

				double dbCoeff;
				::GetXmlAttributeValue( xmlNodeSortieConnexion, "coeff", dbCoeff, pChargement);				// Coefficient d'affectation					
								
				coeffAffect.insert( std::make_pair( pTAv, dbCoeff ) );
			}			

			// Vérification somme des coefficients d'affectation pour le couple entrée/destination = 1
			typeCoeffAffectation::iterator it = coeffAffect.begin();
			double dbSum = 0;
            int nSum = 0;
            while (it != coeffAffect.end())
			{
				dbSum += it->second;
                nSum++;
				it++;
			}

			if( fabs(dbSum - 1) > nSum*std::numeric_limits<double>::epsilon() )
			{									
				return false;
			}
			
			mpCoupleCoeff.insert( std::make_pair(cplEntreeDestination, coeffAffect) );						
			
		}

        if(!m_pReseau->m_bTypeProfil && plages.size() > 0) // rmq : si pas de plage, on utilise une variante temporelle couvrant toute la simulation
        {
            PTAffectation.second = mpCoupleCoeff;

            if( this->m_mapCoeffAffectationPT.find(pTV) == this->m_mapCoeffAffectationPT.end() )
			    m_mapCoeffAffectationPT.insert( std::make_pair(pTV, std::deque<typePlageTemporelleAffectation>() ) );
		
            for(size_t iPlage = 0; iPlage < plages.size(); iPlage++)
            {
                PTAffectation.first = plages[iPlage];
		        m_mapCoeffAffectationPT.find(pTV)->second.push_back( PTAffectation );
            }
        }
        else
        {
		    VTAffectation.second =  mpCoupleCoeff;

            if( this->m_mapCoeffAffectation.find(pTV) == this->m_mapCoeffAffectation.end() )
			    m_mapCoeffAffectation.insert( std::make_pair(pTV, std::deque<typeVarianteTemporelleAffectation>() ) );
		
		    m_mapCoeffAffectation.find(pTV)->second.push_back( VTAffectation );
        }

	}	

    // vérification des plages temporelles (couverture de toute la simu)
    if( !m_pReseau->m_bTypeProfil && (this->m_mapCoeffAffectationPT.find(pTV) != this->m_mapCoeffAffectationPT.end()) )
    {
        vector<PlageTemporelle*> plages;
        deque<typePlageTemporelleAffectation> & variantes = this->m_mapCoeffAffectationPT.find(pTV)->second;
        for(size_t iVar = 0; iVar < variantes.size(); iVar++)
        {
            plages.push_back(variantes[iVar].first);
        }
        if(!CheckPlagesTemporellesEx(m_pReseau->GetDureeSimu(), plages))
        {
            if( pChargement)
            {
                *pChargement << Logger::Error << "ERROR : The temporal periods defined for the assignment of connection " << m_strID << " doesn't cover the whole simulation duration !" << std::endl;
                *pChargement << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            }
            return false;
        }
    }


	return true;
}


bool Connexion::GetCoeffsAffectation(double dbInst, TypeVehicule* pTypeVeh, Tuyau *pTAm, SymuViaTripNode *pDst, std::map<Tuyau*, double, LessPtr<Tuyau>> &mapCoeffs)
{
    // Recherche en priorité des plages temporelles définies
    std::deque<typePlageTemporelleAffectation> dqPTAffectation;
	std::deque<typePlageTemporelleAffectation>::iterator itPTAffectation;
    std::map< typeCoupleEntreeDestination, typeCoeffAffectation> mapCoupleCoeffs;

    bool plageFound = false;
    if( this->m_mapCoeffAffectationPT.find( pTypeVeh ) != m_mapCoeffAffectationPT.end() )
    {
        dqPTAffectation = this->m_mapCoeffAffectationPT.find( pTypeVeh )->second;

        for(size_t i = 0; i < dqPTAffectation.size(); i++)
        {
            if(dqPTAffectation[i].first->m_Debut <= dbInst && dqPTAffectation[i].first->m_Fin >= dbInst)
            {
                mapCoupleCoeffs = dqPTAffectation[i].second;
                plageFound = true;
            }
        }
    }

    if(!plageFound) 
    {
	    std::deque<typeVarianteTemporelleAffectation> dqVTAffectation;
	    std::deque<typeVarianteTemporelleAffectation>::iterator itVTAffectation;
	

	    double dbDureeCum = - m_pReseau->GetLag();

	    if( this->m_mapCoeffAffectation.find( pTypeVeh ) == m_mapCoeffAffectation.end() )
		    return false;

	    dqVTAffectation = this->m_mapCoeffAffectation.find( pTypeVeh )->second;

	    // Recherche de la variante temporelle courante
	    itVTAffectation = dqVTAffectation.begin();
	    dbDureeCum += (*itVTAffectation).first;
	    while ( dbInst >= dbDureeCum && ++itVTAffectation != dqVTAffectation.end())
	    {		
		    dbDureeCum += (*itVTAffectation).first;		
	    }	
	    if( itVTAffectation == dqVTAffectation.end())			// si variante temporelle non définie, on prend la dernière
		    mapCoupleCoeffs = dqVTAffectation.back().second;
	    else
		    mapCoupleCoeffs = itVTAffectation->second;
    }

	// Recherche du couple entrée/destination
	if( mapCoupleCoeffs.find( std::make_pair(pTAm, pDst )) == mapCoupleCoeffs.end() )
		return false;
	else
		mapCoeffs = mapCoupleCoeffs.find( std::make_pair(pTAm, pDst ))->second;

	//*(Reseau::m_pFicSimulation) << "map : " << mapCoeffs.begin()->first << " " << mapCoeffs.begin()->second << std::endl;
	//*(Reseau::m_pFicSimulation) << "      " << mapCoeffs.rbegin()->first << " " << mapCoeffs.rbegin()->second << std::endl;

	return true;
}

// MAJ du cout d'une direction
void Connexion::SetCoutAffectation(TypeVehicule *pTV, Tuyau* pTAm, SymuViaTripNode *pDest, Tuyau* pTAv, double dbCout)
{
	std::map< typeCoupleEntreeDestination, typeCoeffAffectation> mapCout;
	typeCoupleEntreeDestination cplEntreeDestination = make_pair(pTAm, pDest);
	typeCoeffAffectation mapData;

	if( m_mapCoutAffectation.find( pTV ) != m_mapCoutAffectation.end() )
		mapCout = m_mapCoutAffectation.find( pTV )->second;

	if( mapCout.find(cplEntreeDestination) != mapCout.end() )
		mapData = mapCout.find(cplEntreeDestination)->second;	

	if( mapData.find( pTAv ) != mapData.end() )
		mapData.find( pTAv )->second = min( mapData.find( pTAv )->second, dbCout);
	else
	{
		mapData.insert( make_pair(pTAv, dbCout) );
	}

	if( mapCout.find(cplEntreeDestination) != mapCout.end() )
		mapCout.find(cplEntreeDestination)->second = mapData;
	else
		mapCout.insert( make_pair(cplEntreeDestination, mapData ) );	

	if( m_mapCoutAffectation.find( pTV ) != m_mapCoutAffectation.end() )
		m_mapCoutAffectation.erase( m_mapCoutAffectation.find( pTV ) );

	m_mapCoutAffectation.insert( make_pair(pTV, mapCout) );

	//Trace
	//*(Reseau::m_pFicSimulation) << "Cout affectation " << this->GetID() << " pour " << pTV->GetLibelle() << " : " << pTAm->GetLabel() << " " << pS->GetID() << " " << pTAv->GetLabel() << " " << dbCout << std::endl;
}

// MAJ des coefficients d'affectation à partir des coûts (à l'aide d'un logit)
void Connexion::UpdateAffectation(TypeVehicule *pTV, double dbTetaLogit)
{	
	std::deque<typeVarianteTemporelleAffectation> dqAffectation;
	typeVarianteTemporelleAffectation vtAffectation;
	typeCoeffAffectation mapCoeff;
	std::map< typeCoupleEntreeDestination, typeCoeffAffectation> mapCoupleEntreeDestination;

	// Remise à zéro des coefficients d'affectation
	if( m_mapCoeffAffectation.find( pTV ) != m_mapCoeffAffectation.end() )
		m_mapCoeffAffectation.find(pTV)->second.clear();

    // Suppression des variantes temporelles définies par plages (de toute façon pas utilisées car Affectation calculée automatiquement ...)
    if( m_mapCoeffAffectationPT.find( pTV ) != m_mapCoeffAffectationPT.end() )
		m_mapCoeffAffectationPT.find(pTV)->second.clear();
		
	if( m_mapCoutAffectation.find( pTV) != m_mapCoutAffectation.end() )
	{
		// Boucle sur les couples entree/destination avec coût non nul
		std::map< typeCoupleEntreeDestination, typeCoeffAffectation>::iterator itCpl;
		for( itCpl = m_mapCoutAffectation.find( pTV)->second.begin(); itCpl != m_mapCoutAffectation.find( pTV)->second.end(); itCpl++)
		{
			std::map<Tuyau*, double, LessPtr<Tuyau>>::iterator itTCout;
			double dbSum = 0;
			mapCoeff.clear();
			// Boucle sur les tronçons avals possible

			// 
			if( dbTetaLogit > 10 )		// Traitement d'un teta grand (sinon passage à la limite mal traité numériquement)
				dbTetaLogit = 10;			
			
			for( itTCout = (*itCpl).second.begin(); itTCout != (*itCpl).second.end(); itTCout++)
			{
				dbSum += exp( - dbTetaLogit * (*itTCout).second );	// Calcul de la somme exponentielle des coût
			}
			// Calcul du rapport logit pour chaque tronçon aval possible
			for( itTCout = (*itCpl).second.begin(); itTCout != (*itCpl).second.end(); itTCout++)
			{
				double dbCoeff = exp( - dbTetaLogit * (*itTCout).second ) / dbSum ;
				mapCoeff.insert( make_pair( (*itTCout).first, dbCoeff ));

        	    m_pReseau->log() << "Assignment coeff. " << this->GetID() << " for " << pTV->GetLabel() << " : " << (*itCpl).first.first->GetLabel() << " " << (*itCpl).first.second->GetInputID() << " " << (*itTCout).first->GetLabel() << " " << dbCoeff << std::endl;
			}
			mapCoupleEntreeDestination.insert( make_pair( (*itCpl).first, mapCoeff ) );
		}
	}
	vtAffectation = make_pair( DBL_MAX, mapCoupleEntreeDestination);
	dqAffectation.push_back( vtAffectation );
	
	if( m_mapCoeffAffectation.find( pTV ) != m_mapCoeffAffectation.end() )
		m_mapCoeffAffectation.find(pTV)->second = dqAffectation;
	else
		m_mapCoeffAffectation.insert( make_pair(pTV, dqAffectation) );
}

// MAJ des coefficients d'affectation en utilisant MSA
void Connexion::UpdateAffectationMSA(TypeVehicule *pTV, int nIteration)
{
	double dbCoef, dbCoefPrev;

	m_pReseau->log() << std::endl << "UpdateAffectationMSA of " << this->GetID() << " for type " << pTV->GetLabel() << std::endl ;

	if( nIteration == 1)
		return;

	if( m_mapCoeffAffectation.find(pTV) == m_mapCoeffAffectation.end() )
		return;

	if( m_mapCoeffAffectationPrev.find(pTV) == m_mapCoeffAffectationPrev.end() )
		return;

	std::deque<typeVarianteTemporelleAffectation> dqAffectation = m_mapCoeffAffectation.find(pTV)->second;
	std::deque<typeVarianteTemporelleAffectation> dqAffectationPrev = m_mapCoeffAffectationPrev.find(pTV)->second;

	std::map< typeCoupleEntreeDestination, typeCoeffAffectation>::iterator itAf;
	std::map< typeCoupleEntreeDestination, typeCoeffAffectation>::iterator itAfPrev;

	for( itAf = dqAffectation.begin()->second.begin(); itAf != dqAffectation.begin()->second.end(); itAf++)
	{		
		itAfPrev = dqAffectationPrev.begin()->second.find( (*itAf).first );

        m_pReseau->log() << " upstream link : " <<(*itAf).first.first->GetLabel() << " - destination : " << (*itAf).first.second->GetInputID() << std::endl;

		if( itAfPrev != dqAffectationPrev.begin()->second.end() )
		{
			std::map<Tuyau*, double, LessPtr<Tuyau>>::iterator itCoef;
			std::map<Tuyau*, double, LessPtr<Tuyau>>::iterator itCoefPrev;

			for( itCoef = (*itAf).second.begin(); itCoef != (*itAf).second.end(); itCoef++)
			{
				m_pReseau->log() << " downstream link : " << (*itCoef).first->GetLabel();

				itCoefPrev = (*itAfPrev).second.find( (*itCoef).first );

				if( itCoefPrev != (*itAfPrev).second.end() )			
				{
					dbCoef = (*itCoef).second;
					dbCoefPrev = (*itCoefPrev).second;

					(*itCoef).second = 1./(float)nIteration*(*itCoef).second + (1-1./(float)nIteration)*(*itCoefPrev).second;	// MSA									
    			    m_pReseau->log() << " -> Computed assignment coeff. : " <<dbCoef;
	                m_pReseau->log() << " - Prev. assignment coeff. : " <<dbCoefPrev;
					m_pReseau->log() << " - Used assignment coeff. : " << (*itCoef).second << std::endl;
				}
				else
				{
                    m_pReseau->log() << " -> Computed assignment coeff. : " << (*itCoef).second;
    			    m_pReseau->log() << " - Prev. assignment coeff. : 0";

					(*itCoef).second = 1./(float)nIteration*(*itCoef).second;	// MSA

				    m_pReseau->log() << " - Used assignment coeff. : " << (*itCoef).second << std::endl;
				}
			}
		}		
	}
}

// Ménage
void Connexion::ClearMapAssignment()
{
	m_mapAssignment.clear();
};

// Incrémentation du nombre de véhicules
void Connexion::IncNbVehAff(TypeVehicule *pTV, Tuyau *pTAm, SymuViaTripNode* pDest, const std::vector<Tuyau*> & Ts)
{	
	std::pair<Tuyau*, SymuViaTripNode*> pairTS(pTAm, pDest);
	std::map< std::pair<Tuyau*, SymuViaTripNode*>, deque<AssignmentData> > mapTSAss;	
	std::deque<AssignmentData>::iterator itData;

	if( m_mapAssignment.find(pTV) != m_mapAssignment.end() )
	{
		mapTSAss = m_mapAssignment.find(pTV)->second;
	}
	else
		return;

	if( mapTSAss.find( pairTS ) != mapTSAss.end() )
	{
		for( itData = mapTSAss.find( pairTS )->second.begin(); itData != mapTSAss.find( pairTS )->second.end(); itData++)
		{
			// Recherche de l'itinéraire ou du tronçon aval correspondant
			if( (*itData).dqTuyaux == Ts )
			{
				(*itData).nNbVeh++;
				break;
			}
		}
	}
	else
		return;

};

// Initialisation des variables de simulation de la connexion
void Connexion::InitSimuTrafic()
{
    ClearMapAssignment();
}


bool Connexion::IsAnOrigine() const 
{
    return GetNbElAmont()+GetNbElAssAmont() == 0;
}
bool Connexion::IsADestination() const
{
    return GetNbElAval() + GetNbElAssAval() == 0;
}

bool Connexion::IsTotalyMesoNode() const
{
    bool bIsMeso = true;
	size_t i = 0 ;
    while( bIsMeso &&
        i < this->m_LstTuyAm.size() )
    {
        bIsMeso = bIsMeso && (this->m_LstTuyAm.at(i))->GetType() == Tuyau::TT_MESO;
        ++i;
    }
    i = 0;
    while( bIsMeso &&
        i < this->m_LstTuyAv.size() )
    {
        bIsMeso = bIsMeso && (this->m_LstTuyAv.at(i))->GetType() == Tuyau::TT_MESO;
        ++i;
    }
   
    return bIsMeso;
 }
    
bool Connexion::HasMesoTroncon( ) const
{
    bool bHasMeso = false;

    size_t i = 0 ;
    while(!bHasMeso &&
        i < this->m_LstTuyAm.size() )
    {
        bHasMeso = (this->m_LstTuyAm.at(i))->GetType() == Tuyau::TT_MESO;
        ++i;
    }
    i = 0;

    while( !bHasMeso &&
        i < this->m_LstTuyAv.size() )
    {
        bHasMeso =  (this->m_LstTuyAv.at(i))->GetType() == Tuyau::TT_MESO;
        ++i;
    }


    return bHasMeso;

}

// Calcul le barycentre de la connexion à partir des coordonnées des tronçons amont et aval
void    Connexion::calculBary(void)
{
	double a=0,b=0;

	for( int i=0; i< (int) m_LstTuyAv.size();i++)
	{
		a+=m_LstTuyAv[i]->  GetAbsAmont();
		b+=m_LstTuyAv[i]->GetOrdAmont();
	}
	for( int i=0; i< (int) m_LstTuyAm.size();i++)
	{
		a+=m_LstTuyAm[i]->GetAbsAval();
		b+=m_LstTuyAm[i]->GetOrdAval();
	}

	Abscisse =a/ ((int) m_LstTuyAm.size()+(int) m_LstTuyAv.size()) ;
	Ordonne =b/ ((int) m_LstTuyAv.size()+(int) m_LstTuyAm.size());

}

#ifdef USE_SYMUCORE
double Connexion::GetMarginal(SymuCore::MacroType* pMacroType, Tuyau* pTuyauAmont, Tuyau * pTuyauAval)
{
    // On choisit ici de définir un marginal de 0 pour les connexions ponctuelles plutôt que NaN
    return 0;
}
#endif // USE_SYMUCORE


// ************************
// *** MouvementsSortie
// *************************

bool MouvementsSortie::IsTypeExclu(TypeVehicule* pTV, SousTypeVehicule * pSousType)
{
    bool bResult;
    bool bDefaultResult = false;
    for(size_t i = 0; i < m_LstForbiddenVehicles[NULL].size() && !bDefaultResult; i++)
    {
        if(m_LstForbiddenVehicles[NULL][i] == pTV)
        {
            bDefaultResult = true;
        }
    }
    if(pSousType)
    {
        // Dans ce cas, on regarde s'il y a des briques qui régulent ce mouvement.
        // Si oui, on renvoie vrai si le type est interdit par au moins une brique que le véhicule respecte, et faux sinon
        // Si non, on renvoie la valeur par défaut bDefaultResult
        map<RegulationBrique*, vector<TypeVehicule*> >::iterator iter;
        bool bAtLeastOneRegul = false;
        // Pour chaque brique pour lesquelles on a une consigne d'interdiction ...
        for(iter = m_LstForbiddenVehicles.begin(); iter != m_LstForbiddenVehicles.end(); iter++)
        {
            if(iter->first != NULL)
            {
                bAtLeastOneRegul = true;

                // on interdit si le véhicule respecte cette consigne
                if(pSousType->checkRespect(iter->first))
                {
                    for(size_t i = 0; i < iter->second.size(); i++)
                    {
                        if(iter->second[i] == pTV)
                        {
                            return true;
                        }
                    }
                }
            }
        }
        if(!bAtLeastOneRegul)
        {
            bResult = bDefaultResult;
        }
        else
        {
            bResult = false;
        }
    }
    else
    {
        bResult = bDefaultResult;
        if(!bResult)
        {
            // rmq. : la régulation de zone du tonkin ne fonctionnait plus (régression suite à la mise en place de la notion
            // de respect de la consigne (voir les sous types de véhicules) : on regarde donc, dans le cas où
            // aucun sous type n'est défini, si au moins une brique de régulation interdit le mouvement,
            // en esperant ne pas introduire de régression dans la fonctionnalité de taux de respect de la consigne...
            map<RegulationBrique*, vector<TypeVehicule*> >::iterator iter;
            for(iter = m_LstForbiddenVehicles.begin(); iter != m_LstForbiddenVehicles.end(); ++iter)
            {
                if(iter->first != NULL)
                {
                    for(size_t i = 0; i < iter->second.size(); i++)
                    {
                        if(iter->second[i] == pTV)
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return bResult;
}

void MouvementsSortie::AddTypeExclu(TypeVehicule* typeVehicule)
{
    RegulationBrique * pBrique = m_pReseau->GetModuleRegulation()->getBriqueActive();
    map<RegulationBrique*, vector<TypeVehicule*> >::iterator iter = m_LstForbiddenVehicles.find(pBrique);
    if(iter == m_LstForbiddenVehicles.end())
    {
        m_LstForbiddenVehicles[pBrique] = m_LstForbiddenVehicles[NULL];
    }
    vector<TypeVehicule*> & lstTypes = m_LstForbiddenVehicles[pBrique];
    if(std::find(lstTypes.begin(), lstTypes.end(), typeVehicule) == lstTypes.end())
    {
        lstTypes.push_back(typeVehicule);
    }

    // Le graphe d'affectation a changé ...
    m_pReseau->GetSymuScript()->Invalidate();
}

void MouvementsSortie::RemoveTypeExclu(TypeVehicule* typeVehicule)
{
    RegulationBrique * pBrique = m_pReseau->GetModuleRegulation()->getBriqueActive();
    map<RegulationBrique*, vector<TypeVehicule*> >::iterator iter = m_LstForbiddenVehicles.find(pBrique);
    if(iter == m_LstForbiddenVehicles.end())
    {
        m_LstForbiddenVehicles[pBrique] = m_LstForbiddenVehicles[NULL];
    }

    vector<TypeVehicule*> & lstTypes = m_LstForbiddenVehicles[pBrique];
    vector<TypeVehicule*>::iterator iterVect = std::find(lstTypes.begin(), lstTypes.end(), typeVehicule);
	if(iterVect != lstTypes.end())
		lstTypes.erase(iterVect);

    // Le graphe d'affectation a changé ...
    m_pReseau->GetSymuScript()->Invalidate();
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void AssignmentData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void AssignmentData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void AssignmentData::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dqTuyaux);
    ar & BOOST_SERIALIZATION_NVP(nNbVeh);
    ar & BOOST_SERIALIZATION_NVP(dbCout);
    ar & BOOST_SERIALIZATION_NVP(dbCoutPenalise);
    ar & BOOST_SERIALIZATION_NVP(dbCoeff);
    ar & BOOST_SERIALIZATION_NVP(dbCoeffPrev);
    ar & BOOST_SERIALIZATION_NVP(bPredefini);
    ar & BOOST_SERIALIZATION_NVP(strRouteId);
    ar & BOOST_SERIALIZATION_NVP(pJunction);
}

template void RepartitionFlux::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RepartitionFlux::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RepartitionFlux::serialize(Archive & ar, const unsigned int version)
{
    SerialiseTab2D<Archive, double>(ar, "pCoefficients", pCoefficients, nNbVoiesAmont, nNbVoiesAval);
    SerialiseTab2D<Archive, int>(ar, "nNbVeh", nNbVeh, nNbVoiesAmont, nNbVoiesAval);
}

template void MouvementsSortie::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void MouvementsSortie::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void MouvementsSortie::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_dbTaux);
    ar & BOOST_SERIALIZATION_NVP(m_strLibelle);
    ar & BOOST_SERIALIZATION_NVP(m_dbLigneFeu);
    ar & BOOST_SERIALIZATION_NVP(m_bHasVitMax);
    ar & BOOST_SERIALIZATION_NVP(m_dbVitMax);
    ar & BOOST_SERIALIZATION_NVP(m_LstForbiddenVehicles);
}

template void Connexion::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Connexion::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Connexion::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(CMesoNode);

    ar & BOOST_SERIALIZATION_NVP(m_strID);
	ar & BOOST_SERIALIZATION_NVP(m_nID);
    ar & BOOST_SERIALIZATION_NVP(m_pCtrlDeFeux);
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_nNbMaxVoies);
    ar & BOOST_SERIALIZATION_NVP(m_nZLevel);

    ar & BOOST_SERIALIZATION_NVP(Abscisse);
    ar & BOOST_SERIALIZATION_NVP(Ordonne);

    ar & BOOST_SERIALIZATION_NVP(m_LstTuyAm);
    ar & BOOST_SERIALIZATION_NVP(m_LstTuyAv);

    ar & BOOST_SERIALIZATION_NVP(m_LstTuyAssAm);
    ar & BOOST_SERIALIZATION_NVP(m_LstTuyAssAv);

    ar & BOOST_SERIALIZATION_NVP(m_mapMvtAutorises);
    ar & BOOST_SERIALIZATION_NVP(m_mapLstRepartition);

    ar & BOOST_SERIALIZATION_NVP(m_mapCoeffAffectation);
    ar & BOOST_SERIALIZATION_NVP(m_mapCoeffAffectationPrev);
    ar & BOOST_SERIALIZATION_NVP(m_mapCoeffAffectationPT);
    ar & BOOST_SERIALIZATION_NVP(m_mapCoutAffectation);

    ar & BOOST_SERIALIZATION_NVP(m_mapAssignment);
    ar & BOOST_SERIALIZATION_NVP(m_LstCoutsAVide);
}

