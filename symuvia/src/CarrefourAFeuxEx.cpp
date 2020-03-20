#include "stdafx.h"
#include "CarrefourAFeuxEx.h"

#include "tuyau.h"
#include "repartiteur.h"
#include "reseau.h"
#include "vehicule.h"
#include "voie.h"
#include "divergent.h"
#include "convergent.h"
#include "ControleurDeFeux.h"
#include "SystemUtil.h"
#include "Xerces/XMLUtil.h"
#include "Logger.h"

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include <boost/make_shared.hpp>

#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

using namespace std;

XERCES_CPP_NAMESPACE_USE

//================================================================
    CarrefourAFeuxEx::CarrefourAFeuxEx
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 11/05/2009
// Historique: 11/05/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
    char    *strID, 
    double  dbVitMax, 
    double  dbTagreg, 
    double  dbGamma, 
    double  dbMu ,     
    double  dbLong,
    bool    bTraversees,
    Reseau *pReseau
):BriqueDeConnexion(strID, pReseau, 'C', bTraversees)
{
    m_dbVitMax = dbVitMax;
    m_dbTagreg = dbTagreg;
    m_dbGamma = dbGamma;
    m_dbMu = dbMu;    
    m_dbLongueurCellAcoustique = dbLong;	
};

//================================================================
    CarrefourAFeuxEx::~CarrefourAFeuxEx
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 11/05/2009
// Historique: 11/05/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
)
{
    std::deque<EntreeCAFEx*>::iterator itE;
    std::deque<SortieCAFEx*>::iterator itS;
    std::deque<MvtCAFEx*>::iterator itM;    
    std::deque<GrpPtsConflitTraversee*>::iterator itGrpPt;

    for(itE=m_LstEntrees.begin(); itE != m_LstEntrees.end(); itE++)
        delete( (*itE) );

    for(itS=m_LstSorties.begin(); itS != m_LstSorties.end(); itS++)
        delete( (*itS) );

    for(itE=m_LstEntreesBackup.begin(); itE != m_LstEntreesBackup.end(); itE++)
        delete( (*itE) );

    for(itS=m_LstSortiesBackup.begin(); itS != m_LstSortiesBackup.end(); itS++)
        delete( (*itS) );
	
    for(itM=m_LstMouvements.begin(); itM != m_LstMouvements.end(); itM++)
    {

		for(itGrpPt=(*itM)->lstGrpPtsConflitTraversee.begin(); itGrpPt != (*itM)->lstGrpPtsConflitTraversee.end(); itGrpPt++)
            delete( (*itGrpPt) );		
        delete( (*itM) );
    }

    for(itM=m_LstMouvementsBackup.begin(); itM != m_LstMouvementsBackup.end(); itM++)
    {

		for(itGrpPt=(*itM)->lstGrpPtsConflitTraversee.begin(); itGrpPt != (*itM)->lstGrpPtsConflitTraversee.end(); itGrpPt++)
            delete( (*itGrpPt) );		
        delete( (*itM) );
    }
}

//================================================================
    EntreeCAFEx*    CarrefourAFeuxEx::AddEntree
//----------------------------------------------------------------
// Fonction  : Ajout d'une entrée de CAF
// Version du: 11/05/2009
// Historique: 11/05/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    Tuyau*     pTAm,
    int             nVAm
)
{
    EntreeCAFEx* entree;

    entree = new EntreeCAFEx;

    entree->pTAm = (TuyauMicro *) pTAm;
    entree->nVoie = nVAm;
    entree->nPriorite = 0; // Par defaut, priorité par défaut (à droite)
    
    m_LstEntrees.push_back(entree);

    // Ajout dans la liste des tronçons amont de la brique
    if( std::find( m_LstTuyAm.begin(), m_LstTuyAm.end(), pTAm ) == m_LstTuyAm.end())
		m_LstTuyAm.push_back( pTAm );

    return entree;
}

//================================================================
    SortieCAFEx*    CarrefourAFeuxEx::AddSortie
//----------------------------------------------------------------
// Fonction  : Ajout d'une sortie de CAF
// Version du: 19/05/2009
// Historique: 19/05/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    Tuyau*     pTAv,
    int             nAv
)
{  
    std::deque<SortieCAFEx*>::iterator itS;

    // On regarde si cette sortie existe déjà
    for(itS = m_LstSorties.begin(); itS != m_LstSorties.end(); itS++)
    {
        if( (*itS)->pTAv == pTAv && (*itS)->nVoie == nAv )
            return (*itS);
    }

    SortieCAFEx* sortie = new SortieCAFEx();

    sortie->pTAv = (TuyauMicro *)pTAv;
    sortie->nVoie = nAv;
    
    m_LstSorties.push_back(sortie);

    // Ajout dans la liste des tronçons aval de la brique
    if( std::find( m_LstTuyAv.begin(), m_LstTuyAv.end(), pTAv ) == m_LstTuyAv.end())
		m_LstTuyAv.push_back( pTAv );

    return sortie;
}

//================================================================
    MvtCAFEx*    CarrefourAFeuxEx::AddMouvement
//----------------------------------------------------------------
// Fonction  : Ajout d'un mouvement
// Version du: 11/05/2009
// Historique: 11/05/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    TuyauMicro*     pTAm,
    int             nVAm,
    TuyauMicro*     pTAv,
    int             nVAv,
    boost::shared_ptr<MouvementsSortie> pMvtSortie
)
{
    MvtCAFEx* mvt = new MvtCAFEx;

    std::deque<EntreeCAFEx*>::iterator itE;
    std::deque<SortieCAFEx*>::iterator itS;

    for(itE=m_LstEntrees.begin(); itE!=m_LstEntrees.end(); itE++)
    {
        if( (*itE)->pTAm == pTAm && (*itE)->nVoie == nVAm )
        {
            mvt->ent = (*itE);
            break;
        }
    }

    for(itS=m_LstSorties.begin(); itS!=m_LstSorties.end(); itS++)
    {
        if( (*itS)->pTAv == pTAv && (*itS)->nVoie == nVAv )
        {
            mvt->sor = (*itS);
            break;
        }
    }

    mvt->pMvtSortie = pMvtSortie;

    m_LstMouvements.push_back( mvt );

    return mvt;
}

    

//================================================================
    bool CarrefourAFeuxEx::Init
//----------------------------------------------------------------
// Fonction  : Initialisation du carrefour à feux (construction des 
// objets -tronçons et connexions- internes) à partir du noeud XML
// de description 
// Version du: 11/05/2009
// Historique: 11/05/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	DOMNode					*pXmlNodeCAF,
    Logger                  *ofLog    
)
{    
    DOMElement *pXMLEntreesCAF, *pXMLMouvementsCAF, *pXMLSortiesCAF;
	DOMNode *pXMLEntreeCAF,  *pXMLSortieCAF, *pXMLMouvementCAF;
	
	std::string strId, strPriorite;    
    std::string strTmp;
    Point pt;
    Divergent *pDvgtAv, *pDvgtAm;    
    double dbAmX, dbAmY, dbAmZ;
    double dbAvX, dbAvY, dbAvZ;
	std::string sTmp;       
    int    nVAm, nVAv;
    std::deque<Convergent*> lstConvergentsInternes;
    std::deque<Convergent*>::iterator itCvg;
    Convergent *pCvg;
    double dbAlpha;
    TuyauMicro *pTAm, *pTAv, *pT;
    double dbVitMax;

    if(!pXmlNodeCAF)
        return false;

	pXMLMouvementsCAF = NULL;
	pXMLEntreesCAF = NULL;
	pXMLEntreeCAF = NULL;
	pXMLMouvementCAF = NULL;

    // Initialisation des mouvements autorisés du CAF et construction des tronçons et connexions internes
	std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itMvtAutorises;
	
	pXMLEntreesCAF = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode( "./ENTREES_CAF", pXmlNodeCAF->getOwnerDocument(), (DOMElement*)pXmlNodeCAF);
    pXMLSortiesCAF = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode("./SORTIES_CAF", pXmlNodeCAF->getOwnerDocument(), (DOMElement*)pXmlNodeCAF);

    // Correction bug n°38 : vérification de la validité des mouvements définis dans ENTREES_CAP par rapport à ceux définis dans MOUVEMENTS_AUTORISES
    if(pXMLEntreesCAF)
    {
        std::string strTronconAmont, strTronconAval;
        int iNumVoieAmont, iNumVoieAval;
        XMLSize_t countEntrees = pXMLEntreesCAF->getChildNodes()->getLength();
		for(XMLSize_t iEntree=0; iEntree<countEntrees; iEntree++)
		{
			DOMNode * xmlNode = pXMLEntreesCAF->getChildNodes()->item(iEntree);
			if (xmlNode->getNodeType() != DOMNode::ELEMENT_NODE) continue;
            
            
            GetXmlAttributeValue(xmlNode, "id_troncon_amont", strTronconAmont, ofLog);			
			GetXmlAttributeValue(xmlNode, "num_voie_amont", iNumVoieAmont, ofLog);	

            std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > * pMvtSortie = NULL;
            for(itMvtAutorises = m_mapMvtAutorises.begin(); itMvtAutorises != m_mapMvtAutorises.end() && !pMvtSortie; itMvtAutorises++)
	        {		
		        pTAm = (TuyauMicro*)(*itMvtAutorises).first->GetParent();
		        nVAm = ((*itMvtAutorises).first)->GetNum();

                if(!pTAm->GetLabel().compare(strTronconAmont))
                {
                    if(nVAm == iNumVoieAmont-1)
                    {
                        pMvtSortie = &(*itMvtAutorises).second;
                    }
                }
            }

            if(!pMvtSortie)
            {
                *ofLog << Logger::Error << "ERROR : the node ENTREE_CAF for lane n°" << iNumVoieAmont << " of link " << strTronconAmont << " matches an undefined authorized movement input in the node MOUVEMENTS_AUTORISES of the junction " << m_strID << std::endl;
                *ofLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                return false;
            }
            else
            {
                // Pour chaque mouvement ...
                DOMNode * pXMLMouvements = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode("./MOUVEMENTS", xmlNode->getOwnerDocument(), (DOMElement*)xmlNode);
                XMLSize_t countMvts = pXMLMouvements->getChildNodes()->getLength();
		        for(XMLSize_t iMvt = 0; iMvt<countMvts; iMvt++)
		        {
			        DOMNode * xmlNodeMvt = pXMLMouvements->getChildNodes()->item(iMvt);
			        if (xmlNodeMvt->getNodeType() != DOMNode::ELEMENT_NODE) continue;

                    GetXmlAttributeValue(xmlNodeMvt, "id_troncon_aval", strTronconAval, ofLog);			
			        GetXmlAttributeValue(xmlNodeMvt, "num_voie_aval", iNumVoieAval, ofLog);	

                    // test de la présence de cette voie dans pMvtSortie
                    bool bIsThere = false;
                    std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >::iterator posMvt = pMvtSortie->find(m_pReseau->GetLinkFromLabel(strTronconAval));
                    if(posMvt != pMvtSortie->end())
                    {
                        if(posMvt->second.find(iNumVoieAval-1) != posMvt->second.end())
                        {
                            bIsThere = true;
                        }
                    }
                    if(!bIsThere)
                    {
                        *ofLog << Logger::Error << "ERROR : the node MOUVEMENT of the lane n°" << iNumVoieAval << " of link " << strTronconAval << " matches an undefined authorized move output in the node MOUVEMENTS_AUTORISES of the junction " << m_strID << std::endl;
                        *ofLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                        return false;
                    }
                }
            }
        }
    }
    // Fin vérification validité du noeud ENTREES_CAF
	for(itMvtAutorises = m_mapMvtAutorises.begin(); itMvtAutorises != m_mapMvtAutorises.end(); itMvtAutorises++)
	{		
		pTAm = (TuyauMicro*)(*itMvtAutorises).first->GetParent();
		nVAm = ((*itMvtAutorises).first)->GetNum();

		// Le noeud 'ENTREE_CAF' correspondant a t'il été précisé
		pXMLEntreeCAF = NULL;		
		if(pXMLEntreesCAF)
		{
            // Evolution 34 : vérification de la cohérence des numéros de vois
            if (m_pReseau->GetXMLUtil()->SelectSingleNode("./ENTREE_CAF[@id_troncon_amont=\"" + pTAm->GetLabel() + "\" and @num_voie_amont>" + SystemUtil::ToString(pTAm->getNbVoiesDis()) + "]", pXMLEntreesCAF->getOwnerDocument(), (DOMElement*)pXMLEntreesCAF))
            {
                *ofLog << Logger::Error << "ERROR : the node ENTREE_CAF has an invalid lane number for link " << pTAm->GetLabel() << std::endl;
                *ofLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                return false;
            }

			std::string sXPath = "./ENTREE_CAF[@id_troncon_amont=\"" +  pTAm->GetLabel() + "\" and @num_voie_amont=" + SystemUtil::ToString(nVAm+1) + "]";			
            pXMLEntreeCAF = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode(sXPath.c_str(), pXMLEntreesCAF->getOwnerDocument(), pXMLEntreesCAF);

			if(pXMLEntreeCAF)
                pXMLMouvementsCAF = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode("./MOUVEMENTS", pXMLEntreeCAF->getOwnerDocument(), (DOMElement*)pXMLEntreeCAF);
		}

		EntreeCAFEx* entreeCAF = AddEntree( (TuyauMicro*)pTAm, nVAm );

		// Type de priorité si défini
		if(pXMLEntreesCAF)
		{
			GetXmlAttributeValue(pXMLEntreeCAF, "priorite", strPriorite, ofLog); 
			entreeCAF->nPriorite = 0;
			if( strPriorite == "cedez_le_passage" )
				entreeCAF->nPriorite = 1;
			else if( strPriorite == "stop" )
				entreeCAF->nPriorite = 2;
		}

		// Construction du premier divergent à l'extrémité aval de l'entrée si ce n'est pas déjà fait
		if( !pTAm->getConnectionAval() )
		{
			sTmp = GetID() + "_" + "D0" + "_" + pTAm->GetLabel();                                                
            
			pDvgtAm = new Divergent( sTmp, m_pReseau );
			m_pReseau->AddDivergent(sTmp, pDvgtAm);  

			pDvgtAm->AddEltAmont( pTAm );

			pTAm->setConnectionAval( pDvgtAm );
			pTAm->SetTypeAval('D');
            
		}
		else
		{
			pDvgtAm = (Divergent*)pTAm->getConnectionAval();            
		}

		dbAmX = pTAm->GetLstLanes()[nVAm]->GetExtAval()->dbX;
		dbAmY = pTAm->GetLstLanes()[nVAm]->GetExtAval()->dbY;
		dbAmZ = pTAm->GetLstLanes()[nVAm]->GetExtAval()->dbZ;

		// Lecture des divergents monovoie de l'entrée
		if(pXMLEntreeCAF)
		{
            DOMNode *pXMLDivergentsCAF = m_pReseau->GetXMLUtil()->SelectSingleNode("./DIVERGENTS", pXMLEntreeCAF->getOwnerDocument(), (DOMElement*)pXMLEntreeCAF);

			if( pXMLDivergentsCAF )
			{
				XMLSize_t countk = pXMLDivergentsCAF->getChildNodes()->getLength();
				for(XMLSize_t k=0; k<countk;k++)    
				{
					DOMNode * xmlChildk = pXMLDivergentsCAF->getChildNodes()->item(k);
					if (xmlChildk->getNodeType() != DOMNode::ELEMENT_NODE) continue;

					GetXmlAttributeValue(xmlChildk, "coord", pt, ofLog);        // coord. du divergent

					GetXmlAttributeValue(xmlChildk, "id", strId, ofLog);       //identifiant du divergent                            
					char szID[128];
#ifdef WIN32
					strcpy_s(szID, 128, strId.c_str());
#else
                    strcpy(szID, strId.c_str());
#endif
	                
					// Construction du divergent correspondant
					pDvgtAv = new Divergent( szID, m_pReseau );
					m_pReseau->AddDivergent(strId, pDvgtAv);  

					// Construction du tronçon inter-divergent
					sTmp = pDvgtAm->GetID() + "_" + pDvgtAv->GetID();               

                    std::vector<double> largeursVoiesTmp;
                    largeursVoiesTmp.push_back(pTAm->getLargeursVoies()[0]);
					pT = new TuyauMicro(m_pReseau, sTmp, 'D', 'D', pDvgtAv, pDvgtAm, 'M', pTAm->GetRevetement(), largeursVoiesTmp
						, dbAmX, dbAmY, 
						pt.dbX, pt.dbY, dbAmZ, pt.dbZ,
						1, m_pReseau->GetTimeStep(), 0
						, m_dbVitMax, DBL_MAX, m_dbLongueurCellAcoustique, "");

					entreeCAF->lstTBr.push_back(pT);

					m_pReseau->GetLstTuyauxMicro()->push_back(pT);
					m_pReseau->GetLstTuyaux()->push_back(pT);
                    m_pReseau->m_mapTuyaux[pT->GetLabel()] = pT;

					pT->SetBriqueParente(this);

					pDvgtAm->AddEltAval(pT);  
					pDvgtAv->AddEltAmont(pT);

					pT->AddTypeVeh(m_pReseau->GetLstTypesVehicule()[0]); 

					    // Lecture des points internes
					Point   pt;
                    DOMNode* XMLPts = m_pReseau->GetXMLUtil()->SelectSingleNode("./POINTS_INTERNES", pXMLDivergentsCAF->getOwnerDocument(), (DOMElement*)(xmlChildk));
					if(XMLPts)
					{
						XMLSize_t countl = XMLPts->getChildNodes()->getLength();
						for(XMLSize_t l=0; l<countl; l++)
						{
							DOMNode * xmlChildl = XMLPts->getChildNodes()->item(l);
							if (xmlChildl->getNodeType() != DOMNode::ELEMENT_NODE) continue;

							GetXmlAttributeValue(xmlChildl, "coordonnees", pt, ofLog);   
							pT->AddPtInterne(pt.dbX, pt.dbY, pt.dbZ);
						}
					}
					pT->CalculeLongueur(0);   
					pT->Segmentation();

					pDvgtAm = pDvgtAv;
					dbAmX = pT->GetExtAval()->dbX;
					dbAmY = pT->GetExtAval()->dbY;
					dbAmZ = pT->GetExtAval()->dbZ;
				}
			}
		}

		std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > ::iterator itSorties;
		for(itSorties = (*itMvtAutorises).second.begin(); itSorties != (*itMvtAutorises).second.end(); itSorties++)            
        {               
            // Tronçon de sortie                            
			pTAv = (TuyauMicro*)((*itSorties).first);

			std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator itVoiesSortie;
			for(itVoiesSortie = (*itSorties).second.begin(); itVoiesSortie != (*itSorties).second.end(); itVoiesSortie++)        
			{
				nVAv = (*itVoiesSortie).first;

                // Vitesse maximum
                dbVitMax = (*itVoiesSortie).second->m_bHasVitMax?(*itVoiesSortie).second->m_dbVitMax:m_dbVitMax;

				SortieCAFEx* sortieCAF = AddSortie( pTAv, nVAv );

				// Construction du premier convergent à l'extrémité aval de la sortie si ce n'est pas déjà fait
				Convergent *pCvgtAv;
				if( !pTAv->getConnectionAmont() )
				{       
                    strTmp = GetID() + "_C0_"+ pTAv->GetLabel();
					pCvgtAv = new Convergent(strTmp, m_dbTagreg, m_dbGamma, m_dbMu, m_pReseau );
                    m_pReseau->AddConvergent(strTmp, pCvgtAv); 
                    lstConvergentsInternes.push_back(pCvgtAv);

					pCvgtAv->AddEltAval(pTAv);                

					pCvgtAv->m_LstTf = m_LstTfCvg;

					pCvgtAv->Init(pTAv);
	                
					pTAv->setConnectionAmont( pCvgtAv );
					pTAv->SetTypeAmont('C');

					dbAvX = pTAv->GetExtAmont()->dbX;
					dbAvY = pTAv->GetExtAmont()->dbY;
					dbAvZ = pTAv->GetExtAmont()->dbZ;
				}
				else
					pCvgtAv = (Convergent*)pTAv->getConnectionAmont();

				dbAvX = pTAv->GetLstLanes()[nVAv]->GetExtAmont()->dbX;
				dbAvY = pTAv->GetLstLanes()[nVAv]->GetExtAmont()->dbY;
				dbAvZ = pTAv->GetLstLanes()[nVAv]->GetExtAmont()->dbZ;


				// Le noeud 'MOUVEMENT' correspondant a t'il été précisé ?
				pXMLMouvementCAF = NULL;	
				if(pXMLMouvementsCAF)
				{
					std::string sXPath = "./MOUVEMENT[@id_troncon_aval=\"" + pTAv->GetLabel() + "\" and @num_voie_aval=" + SystemUtil::ToString(nVAv+1) + "]";
                    pXMLMouvementCAF = m_pReseau->GetXMLUtil()->SelectSingleNode(sXPath, pXMLMouvementsCAF->getOwnerDocument(), pXMLMouvementsCAF);
				}
				
                MvtCAFEx* mvt = AddMouvement(pTAm, nVAm, pTAv, nVAv, itVoiesSortie->second); // ???

				// Un divergent a t'il été défini pour cette entrée ?
				Divergent *pDiv ;
				std::string strDiv;
				char cType;
				if( pXMLMouvementCAF && GetXmlAttributeValue(pXMLMouvementCAF, "divergent", strDiv, ofLog) )
				{                    
					pDiv = (Divergent*)m_pReseau->GetConnectionFromID( strDiv, cType);   
					dbAmX = pDiv->m_LstTuyAm.front()->GetExtAval()->dbX;
					dbAmY = pDiv->m_LstTuyAm.front()->GetExtAval()->dbY;
					dbAmZ = pDiv->m_LstTuyAm.front()->GetExtAval()->dbZ;
				}
				else    // le divergent n'a pas été renseigné, le mouvement diverge donc de l'extrémité aval du tronçon d'entrée
				{
					pDiv = (Divergent*)pTAm->getConnectionAval();
					dbAmX = pTAm->GetLstLanes()[nVAm]->GetExtAval()->dbX;
					dbAmY = pTAm->GetLstLanes()[nVAm]->GetExtAval()->dbY;
					dbAmZ = pTAm->GetLstLanes()[nVAm]->GetExtAval()->dbZ;
				}      

				// Lecture des convergents de la sortie
				pXMLSortieCAF = NULL;	
				if(pXMLSortiesCAF)
				{
                    // Evolution 34 : vérification de la cohérence des numéros de vois
                    if (m_pReseau->GetXMLUtil()->SelectSingleNode("./SORTIE_CAF[@id_troncon_aval=\"" + pTAv->GetLabel() + "\" and @num_voie_aval>" + SystemUtil::ToString(pTAv->getNbVoiesDis()) + "]", pXMLSortiesCAF->getOwnerDocument(), (DOMElement*)pXMLSortiesCAF))
                    {
                        if( ofLog )
                        {
                            *ofLog << Logger::Error << "ERROR : the node SORTIE_CAF has an invalid lane number for link " << pTAv->GetLabel() << std::endl;
                            *ofLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                        }
                        return false;
                    }

					std::string sXPath = "./SORTIE_CAF[@id_troncon_aval=\"" +  pTAv->GetLabel() + "\" and @num_voie_aval=" + SystemUtil::ToString(nVAv+1) + "]";			
                    pXMLSortieCAF = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode(sXPath.c_str(), pXMLSortiesCAF->getOwnerDocument(), pXMLSortiesCAF);
				}

				if( pXMLSortieCAF )
				{
                    DOMNode* pXMLConvergentsCAF = (DOMElement*)m_pReseau->GetXMLUtil()->SelectSingleNode("./CONVERGENTS", pXMLSortieCAF->getOwnerDocument(), (DOMElement*)pXMLSortieCAF);

					if( pXMLConvergentsCAF )
					{
						for(XMLSize_t k=0; k<pXMLConvergentsCAF->getChildNodes()->getLength();k++)    
						{
							GetXmlAttributeValue(pXMLConvergentsCAF->getChildNodes()->item(k), "coord", pt, ofLog);        // coord. du convergent

							GetXmlAttributeValue(pXMLConvergentsCAF->getChildNodes()->item(k), "id", strId, ofLog);       //identifiant du divergent                            
							char szID[128];
#ifdef WIN32
					        strcpy_s(szID, 128, strId.c_str());
#else
                            strcpy(szID, strId.c_str());
#endif
							
							char cType;
							if( !m_pReseau->GetConnectionFromID(strId, cType ) )
							{
								// Construction du convergent correspondant
								Convergent *pCvgtAm = new Convergent( szID, m_dbTagreg, m_dbGamma, m_dbMu, m_pReseau );
								m_pReseau->AddConvergent(strId, pCvgtAm);  
                                lstConvergentsInternes.push_back(pCvgtAm);
								   
								pCvgtAm->m_LstTf = m_LstTfCvg;

								// Construction du tronçon inter-convergent 
                                std::vector<double> largeursVoiesTmp;
                                largeursVoiesTmp.push_back(pTAv->getLargeursVoies()[nVAv]);
								pT = new TuyauMicro(m_pReseau, pCvgtAm->GetID() + "_" + pCvgtAv->GetID(), 'C', 'C', pCvgtAv, pCvgtAm, 'M', pTAv->GetRevetement(), largeursVoiesTmp
									, pt.dbX, pt.dbY, dbAvX, dbAvY, pt.dbZ,
									dbAvZ, 
									1, m_pReseau->GetTimeStep(), 0
									, dbVitMax, DBL_MAX, m_dbLongueurCellAcoustique, "");

								sortieCAF->lstTBr.push_front(pT);

								m_pReseau->GetLstTuyauxMicro()->push_back(pT);
								m_pReseau->GetLstTuyaux()->push_back(pT);
                                m_pReseau->m_mapTuyaux[pT->GetLabel()] = pT;

								pT->SetBriqueParente(this);
			                    
								pCvgtAv->AddEltAmont(pT);       
								pCvgtAm->AddEltAval(pT);  					

								pT->AddTypeVeh(m_pReseau->GetLstTypesVehicule()[0]);                     
								
								// Lecture des points internes
								Point   pt;
                                DOMNode* XMLPts = m_pReseau->GetXMLUtil()->SelectSingleNode("./POINTS_INTERNES", pXMLConvergentsCAF->getOwnerDocument(), (DOMElement*)(pXMLConvergentsCAF->getChildNodes()->item(k)));
								if(XMLPts)
								{
									XMLSize_t countl = XMLPts->getChildNodes()->getLength();
									for(XMLSize_t l=0; l<countl; l++)
									{
										DOMNode * xmlChildl = XMLPts->getChildNodes()->item(l);
										if (xmlChildl->getNodeType() != DOMNode::ELEMENT_NODE) continue;

										GetXmlAttributeValue(xmlChildl, "coordonnees", pt, ofLog);   
										pT->AddPtInterne(pt.dbX, pt.dbY, pt.dbZ);
									}
								}
								pT->CalculeLongueur(0);  
								pT->Segmentation();

								pCvgtAm->Init(pT);

                                // récupération de la bonne voie aval du convergent considéré
                                TuyauMicro * pTuyAval = (TuyauMicro*)pCvgtAv->m_LstTuyAv[0];
                                VoieMicro * pVoieAval = NULL;
                                if(pTuyAval == pTAv)
                                {
                                    pVoieAval = (VoieMicro*)pTAv->GetLstLanes()[nVAv];
                                }
                                else
                                {
                                    pVoieAval = (VoieMicro*)pTuyAval->GetLstLanes()[0];
                                }
								pCvgtAv->AddMouvement( (VoieMicro*)pT->GetLstLanes()[0], pVoieAval);
								pCvgtAv = pCvgtAm;
								dbAvX = pT->GetExtAmont()->dbX;
								dbAvY = pT->GetExtAmont()->dbY;
								dbAvZ = pT->GetExtAmont()->dbZ;
							}
						}
					}					
				}

				// Convergent
				Convergent *pCvg ;
				std::string strCvg;
				if( pXMLMouvementCAF && GetXmlAttributeValue(pXMLMouvementCAF, "convergent", strCvg, ofLog) )
				{                    
					pCvg = (Convergent*)m_pReseau->GetConnectionFromID( strCvg, cType);          

					dbAvX = pCvg->m_LstTuyAv.front()->GetExtAmont()->dbX;
					dbAvY = pCvg->m_LstTuyAv.front()->GetExtAmont()->dbY;
					dbAvZ = pCvg->m_LstTuyAv.front()->GetExtAmont()->dbZ;
				}
				else    // le convergent n'a pas été renseigné, le mouvement converge donc vers l'extrémité amont du tronçon de sortie
				{
					pCvg = (Convergent*)pTAv->getConnectionAmont();

					if(!pCvg) // le convergent n'existe pas encore car cette sortie n'a pas été renseignée par l'utilisateur
					{           
                        strTmp = GetID()+"_C0_" + pTAv->GetLabel();
						pCvg = new Convergent(strTmp, m_dbTagreg, m_dbGamma, m_dbMu, m_pReseau );
                        m_pReseau->AddConvergent(strTmp, pCvg);  
                        lstConvergentsInternes.push_back(pCvg);
						pCvg->AddEltAval(pTAv);   
						pCvg->Init(pTAv);

						pCvg->m_LstTf = m_LstTfCvg;
	                    
						pTAv->setConnectionAmont( pCvg );
						pTAv->SetTypeAmont('C');                        
					}      

					dbAvX = pTAv->GetLstLanes()[nVAv]->GetExtAmont()->dbX;
					dbAvY = pTAv->GetLstLanes()[nVAv]->GetExtAmont()->dbY;
					dbAvZ = pTAv->GetLstLanes()[nVAv]->GetExtAmont()->dbZ;
				}    

	            
				// Construction du tronçon intermédiaire  
                std::vector<double> largeursVoiesTmp;
                
                largeursVoiesTmp.push_back(pTAv->getLargeursVoies()[nVAv]);
               
				pT = new TuyauMicro(m_pReseau, pDiv->GetID() + "_" + pCvg->GetID() + "_" + to_string(nVAm+1) + "_" + to_string(nVAv+1),
									'D', 'C', pCvg, pDiv, 'M', pTAv->GetRevetement(), largeursVoiesTmp,
									dbAmX, dbAmY,
									dbAvX, dbAvY,
									dbAmZ, dbAvZ,
									1, m_pReseau->GetTimeStep(), 0,
									dbVitMax, DBL_MAX, m_dbLongueurCellAcoustique, "");            

				m_pReseau->GetLstTuyauxMicro()->push_back(pT);
				m_pReseau->GetLstTuyaux()->push_back(pT);
                m_pReseau->m_mapTuyaux[pT->GetLabel()] = pT;

				pT->SetBriqueParente(this);

				pDiv->AddEltAval(pT);  
				pCvg->AddEltAmont(pT);  
 
                
				    // Lecture des points internes
				if(pXMLMouvementCAF)
				{
					Point   pt;
                    DOMNode * XMLPts = m_pReseau->GetXMLUtil()->SelectSingleNode("./POINTS_INTERNES", pXMLMouvementCAF->getOwnerDocument(), (DOMElement*)pXMLMouvementCAF);
					if(XMLPts)
					{
						XMLSize_t countl = XMLPts->getChildNodes()->getLength();
						for(XMLSize_t l=0; l<countl; l++)
						{
							DOMNode * xmlChildl = XMLPts->getChildNodes()->item(l);
							if (xmlChildl->getNodeType() != DOMNode::ELEMENT_NODE) continue;

							GetXmlAttributeValue(xmlChildl, "coordonnees", pt, ofLog);   
							pT->AddPtInterne(pt.dbX, pt.dbY, pt.dbZ);
						}
					}      
				}

				pT->CalculeLongueur(0);   
				pT->AddTypeVeh(m_pReseau->GetLstTypesVehicule()[0]); 

				pT->Segmentation();

                // récupération de la bonne voie aval du convergent considéré
                TuyauMicro * pTuyAval = (TuyauMicro*)pCvg->m_LstTuyAv[0];
                VoieMicro * pVoieAval = NULL;
                if(pTuyAval == pTAv)
                {
                    pVoieAval = (VoieMicro*)pTAv->GetLstLanes()[nVAv];
                }
                else
                {
                    pVoieAval = (VoieMicro*)pTuyAval->GetLstLanes()[0];
                }
				pCvg->AddMouvement( (VoieMicro*)pT->GetLstLanes()[0], pVoieAval);
	                     
				// Construction de la liste des tronçons du mouvement
				TuyauMicro *pTMvt;
				size_t i = 0;

				// Branche d'entrée
				if( mvt->ent->lstTBr.size() > 0)
				{
					pTMvt = mvt->ent->lstTBr[i];
					if( pTMvt->getConnectionAmont() != pDiv )
					{
						mvt->lstTMvt.push_back( pTMvt );
						if( pTMvt->getConnectionAval() != pDiv )
						{
							i++;
							pTMvt = mvt->ent->lstTBr[i];   
							while( pTMvt->getConnectionAval() != pDiv )
							{
								mvt->lstTMvt.push_back( pTMvt );
								i++;
								pTMvt = mvt->ent->lstTBr[i];                       
							} 
							if( pTMvt->getConnectionAval() == pDiv )
								mvt->lstTMvt.push_back( pTMvt );
						}
					}
				}
				// Tronçon intermédiaire
				mvt->lstTMvt.push_back( pT );

				// Branche de sortie
				i = 0;
				if( mvt->sor->lstTBr.size() > 0)
				{
					pTMvt = mvt->sor->lstTBr[i];     
					while( pTMvt->getConnectionAmont() != pCvg )
					{       
						i++;
                        if(i >= mvt->sor->lstTBr.size())
                        {
                            break;
                        }
						pTMvt = mvt->sor->lstTBr[i];                        
					}                        

					for(size_t l=i; l < mvt->sor->lstTBr.size(); l++)
					{
						pTMvt = mvt->sor->lstTBr[l];   
						mvt->lstTMvt.push_back( pTMvt );  
					}
				}

				// Affectation des voies précédentes des tronçons internes
				std::deque<TuyauMicro*>::iterator itT;
				for(itT = mvt->lstTMvt.begin(); itT !=  mvt->lstTMvt.end(); itT++)
				{
					if( itT == mvt->lstTMvt.begin() )
						((VoieMicro*)((*itT)->GetLstLanes()[0]))->SetPrevVoie((VoieMicro*)pTAm->GetLstLanes()[nVAm]);
					else
						((VoieMicro*)((*itT)->GetLstLanes()[0]))->SetPrevVoie((VoieMicro*)((*(itT-1))->GetLstLanes()[0]));
				}

				// Chargement des points d'attente du mouvement
				DOMNode*        pXmlNodePtsAttente;   
				double          dbVal;
				int				nVal;
				std::deque<double>::iterator itPos;
				std::deque<int>::iterator itNbMax;
	            
				// Il en existe au moins un à l'extrémité aval de l'entrée du mouvement
				mvt->lstPtAttente.push_back(-1);
				mvt->lstNbVehMax.push_back(0);

				if( pXMLMouvementCAF )
				{
                    pXmlNodePtsAttente = m_pReseau->GetXMLUtil()->SelectSingleNode("./POINTS_D_ATTENTE", pXMLMouvementCAF->getOwnerDocument(), (DOMElement*)pXMLMouvementCAF);
					if(pXmlNodePtsAttente)
					{
						XMLSize_t countl = pXmlNodePtsAttente->getChildNodes()->getLength();
						for(XMLSize_t l=0; l<countl; l++)
						{
							DOMNode* xmlChildl = pXmlNodePtsAttente->getChildNodes()->item(l);
							if (xmlChildl->getNodeType() != DOMNode::ELEMENT_NODE) continue;

							GetXmlAttributeValue( xmlChildl, "position", dbVal, ofLog);
							GetXmlAttributeValue( xmlChildl, "nb_veh_max", nVal, ofLog);
							itNbMax = mvt->lstNbVehMax.begin();
							for( itPos = mvt->lstPtAttente.begin(); itPos != mvt->lstPtAttente.end(); itPos++)
							{
								if( dbVal < (*itPos) )
								{
									mvt->lstPtAttente.insert( itPos, dbVal );
									mvt->lstNbVehMax.insert( itNbMax, nVal );
									break;
								}
								itNbMax++;
							}
							if( itPos == mvt->lstPtAttente.end() )
							{
								mvt->lstPtAttente.push_back( dbVal );
								mvt->lstNbVehMax.push_back( nVal );
							}
						}
	                
					}	
				}
            }
        }
    } // rof each sortie
 

    // tri des convergents (conversion en répartiteurs pour les convergents à un tronçon amont)
    for(size_t i = 0; i < lstConvergentsInternes.size(); i++)
    {
        // Si le convergent n'a qu'un tronçon amont, pas la peine d'effectuer les traitements spécifiques convergent ...
        pCvg = lstConvergentsInternes[i];
        if(pCvg->m_LstTuyAm.size() == 1)
        {
            // création d'un répartiteur semblable au convergent et modif des éléments voisins (tronçons poru les types de connexions et le pointeur 
            // vers la connexion,
            Repartiteur * pRep = new Repartiteur(pCvg->GetID(), 'H', pCvg->GetReseau()); 
            for(size_t iTuyAmont = 0; iTuyAmont < pCvg->m_LstTuyAm.size(); iTuyAmont++)
            {
                assert(pCvg->m_LstTuyAm[iTuyAmont]->getConnectionAval() == pCvg);
                pCvg->m_LstTuyAm[iTuyAmont]->setConnectionAval(pRep);
                pCvg->m_LstTuyAm[iTuyAmont]->SetTypeAval('R');
                pRep->AddEltAmont(pCvg->m_LstTuyAm[iTuyAmont]);
            }
            for(size_t iTuyAval= 0; iTuyAval < pCvg->m_LstTuyAv.size(); iTuyAval++)
            {
                assert(pCvg->m_LstTuyAv[iTuyAval]->getConnectionAmont() == pCvg);
                pCvg->m_LstTuyAv[iTuyAval]->setConnectionAmont(pRep);
                pCvg->m_LstTuyAv[iTuyAval]->SetTypeAmont('R');
                pRep->AddEltAval(pCvg->m_LstTuyAv[iTuyAval]);
            }

            // Suppression du convergent de la liste des converegnts du réseau
            m_pReseau->RemoveConvergent(pCvg);
            delete pCvg;

            // Calcul du nb max de voie pour le répartiteur
            int nNbmaxVoies = 1;
            for(int j=0; j < pRep->getNbAmont(); j++)
            {
                if( pRep->m_LstTuyAm[j]->GetLanesManagement() == 'D' && pRep->m_LstTuyAm[j]->getNb_voies() > nNbmaxVoies)
                    nNbmaxVoies = pRep->m_LstTuyAm[j]->getNb_voies();
            }
            for(int j=0; j < pRep->GetNbElAval(); j++)
            {
                if( pRep->m_LstTuyAv[j]->GetLanesManagement() == 'D' &&  pRep->m_LstTuyAv[j]->getNb_voies() > nNbmaxVoies)
                    nNbmaxVoies = pRep->m_LstTuyAv[j]->getNb_voies();
            }
            pRep->SetNbMaxVoies(nNbmaxVoies);

            pRep->calculBary();
            pRep->calculRayon();
            pRep->Constructeur_complet(pRep->GetNbElAmont(),pRep->GetNbElAval(),false);

            // Ajout du répartiteur à la liste des répartiteurs du réseau
            m_pReseau->GetLstRepartiteurs()->insert(std::make_pair(pRep->GetID(), pRep));
        }
        else
        {
            // Il s'agit d'un "vrai" convergent ...
            m_LstCvgs.push_back(pCvg);
        }
    }

    // Initialisation des convergents
    std::deque<Convergent*>::iterator itCvgt;
    for(itCvgt = m_LstCvgs.begin(); itCvgt != m_LstCvgs.end(); itCvgt++)
        (*itCvgt)->FinInit( (*itCvgt)->m_LstTuyAv.front(), 5);

    // Calcul des points de conflit de type traversée     
    std::deque<MvtCAFEx*>::iterator itMvt1;
    std::deque<MvtCAFEx*>::iterator itMvt2;
    std::deque<TuyauMicro*>::iterator itT1;
    std::deque<TuyauMicro*>::iterator itT2;    
    Point ptAm, ptAv;
    size_t i;
    double dbDst1, dbDst2;
    size_t iSegment1, iSegment2;

    for(itMvt1=m_LstMouvements.begin(); itMvt1 != m_LstMouvements.end(); itMvt1++)
    {        
        for(itMvt2=itMvt1; itMvt2 != m_LstMouvements.end(); itMvt2++)
        {            
            if( (*itMvt1)->ent != (*itMvt2)->ent && (*itMvt1)->sor != (*itMvt2)->sor )  // Exclusion des mouvements d'entrées ou de sorties identiques
            {
                dbDst1 = 0;
                dbDst2 = 0;
                for(itT1 = (*itMvt1)->lstTMvt.begin(); itT1 != (*itMvt1)->lstTMvt.end(); itT1++)    // Boucle sur les tronçons du mouvement 1
                {                    
                    for(itT2 = (*itMvt2)->lstTMvt.begin(); itT2 != (*itMvt2)->lstTMvt.end(); itT2++)  // Boucle sur les tronçons du mouvement 2
                    {
                        if( CalculIntersection( (*itT1), (*itT2), pt, iSegment1, iSegment2 ) )
                        {
                            // Création du point de conflit
                            boost::shared_ptr<PtConflitTraversee> ptc = boost::make_shared<PtConflitTraversee>();
                            // hypothèse : les tronçons internes au CAF ont été créés à une voie uniquement.
                            // on peut donc associer au conflit la voie d'indice 0 de chaque troncon interne
                            ptc->pT1 = (VoieMicro*)(*itT1)->GetLstLanes()[0];
                            ptc->pT2 = (VoieMicro*)(*itT2)->GetLstLanes()[0];     

                            ptc->mvt1 = *itMvt1;
                            ptc->mvt2 = *itMvt2;

                            // calcul de l'abscisse curviligne du point de conflit sur le tronçon 1
                            ptc->dbPos1  = 0;
                            if( (*itT1)->GetLstPtsInternes().size() == 0)
                            {
                                ptc->dbPos1 = GetDistance(*ptc->pT1->GetExtAmont(), pt);   
                            }
                            else // Tronçon linéaire par morceaux
                            {
                                ptAm = *ptc->pT1->GetExtAmont();
                                for( i=0; i<iSegment1; i++)
                                {
                                    ptAv = *(*itT1)->GetLstPtsInternes()[i];
                                    ptc->dbPos1 += GetDistance(ptAm, ptAv); 
                                    ptAm = ptAv;
                                }
                                ptc->dbPos1 += GetDistance(ptAm, pt);
                            }
                            // calcul de la distance à parcourir par le véhicule à partir de l'entrée dans le CAF pour atteindre le point de conflit traversée
                            ptc->dbDst1 = dbDst1 + ptc->dbPos1;

                            // calcul de l'abscisse curviligne du point de conflit sur le tronçon 2
                            ptc->dbPos2 = 0;
                            if( (*itT2)->GetLstPtsInternes().size() == 0)
                            {
                                ptc->dbPos2 = GetDistance(*ptc->pT2->GetExtAmont(), pt);   
                            }
                            else // Tronçon linéaire par morceaux
                            {
                                ptAm = *ptc->pT2->GetExtAmont();
                                for( i=0; i<iSegment2; i++)
                                {
                                    ptAv = *(*itT2)->GetLstPtsInternes()[i];
                                    ptc->dbPos2 += GetDistance(ptAm, ptAv); 
                                    ptAm = ptAv;
                                }
                                ptc->dbPos2 += GetDistance(ptAm, pt);
                            }
                            // calcul de la distance à parcourir par le véhicule à partir de l'entrée dans le CAF pour atteindre le point de conflit traversée
                            ptc->dbDst2 = dbDst2 + ptc->dbPos2;

                            // Priorité
                            if( (*itMvt1)->ent->nPriorite < (*itMvt2)->ent->nPriorite )
                            {
                                ptc->pTProp = (VoieMicro*)(*itT1)->GetLstLanes()[0];
                            }
                            else if( (*itMvt2)->ent->nPriorite < (*itMvt1)->ent->nPriorite)
                            {
                                ptc->pTProp = (VoieMicro*)(*itT2)->GetLstLanes()[0];
                            }
                            else // Priorité à droite
                            {
                                if( !IsCounterClockWise( (*itMvt1)->ent->pTAm->GetExtAmont(), (*itMvt1)->ent->pTAm->GetExtAval(), 
                                                        (*itMvt2)->ent->pTAm->GetExtAmont(), (*itMvt2)->ent->pTAm->GetExtAval() ) )
                                    ptc->pTProp = (VoieMicro*)(*itT2)->GetLstLanes()[0];
                                else
                                    ptc->pTProp = (VoieMicro*)(*itT1)->GetLstLanes()[0];
                            }

                            m_LstPtsConflitTraversee.push_back( ptc );                                

                            break;
                        }
                        dbDst2 += (*itT2)->GetLength();
                    }
                    dbDst1 += (*itT1)->GetLength();
                }                
            }
        }
    }

    // Calcul des groupes de points de conflit (ordonnés)
    GrpPtsConflitTraversee *pGrpPtsCT, *pGrp;
    std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itPtsCT;
    std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itTmp; 
    std::deque<GrpPtsConflitTraversee*>::iterator itGrp;
    double dbPos;

    for(itMvt1=m_LstMouvements.begin(); itMvt1 != m_LstMouvements.end(); itMvt1++)
    {
        // Un seul groupe avec un seul point d'attente pour l'instant
        pGrpPtsCT = new GrpPtsConflitTraversee;       
        pGrpPtsCT->dbDstPtAttente = 0;

        // Recherche des points de conflit sur les tronçons du mouvement et tri selon leur position par rapport au mouvement
        for(itPtsCT = m_LstPtsConflitTraversee.begin(); itPtsCT != m_LstPtsConflitTraversee.end(); itPtsCT++)
        {
            // Correction bug n°44 : on ne prend pas en compte les points de conflits qui ne correspondent pas au mouvement en cours de traitement (itMvt1)
            // (dans un carrefour complexe, on peut très bien avoir deux points de conflits avec les même tronçons T1 et T2 mais liés à des mouvements différents.)
            if( (*itPtsCT)->mvt1 == *itMvt1
                || (*itPtsCT)->mvt2 == *itMvt1 ) 
            {
                if(  (*itPtsCT)->pT1 != (*itPtsCT)->pTProp )
                    dbPos = (*itPtsCT)->dbDst1;
                else
                    dbPos = (*itPtsCT)->dbDst2;

				bool bEnd = false;
                for( itTmp = pGrpPtsCT->lstPtsConflitTraversee.begin(); itTmp != pGrpPtsCT->lstPtsConflitTraversee.end(); itTmp++)
                {
                    if( (*itPtsCT)->pT1 != (*itPtsCT)->pTProp  && dbPos < (*itTmp)->dbDst1 )
                    {
                        pGrpPtsCT->lstPtsConflitTraversee.insert( itTmp, *itPtsCT);
						bEnd = true;	// Sortie de boucle
                        break;
                    }
                    if( (*itPtsCT)->pT2 != (*itPtsCT)->pTProp  && dbPos < (*itTmp)->dbDst2 )
                    {
                        pGrpPtsCT->lstPtsConflitTraversee.insert( itTmp, *itPtsCT);
						bEnd = true;   // Sortie de boucle
                        break;
                    }
                }
                if( !bEnd)
                    pGrpPtsCT->lstPtsConflitTraversee.push_back( *itPtsCT );
            }                
        }
        // Création des groupes de traversées selon leur position par rapport aux points d'attentes
        std::deque<double>::iterator itPtA;
		std::deque<int>::iterator itNbMax = (*itMvt1)->lstNbVehMax.begin();
        for(itPtA = (*itMvt1)->lstPtAttente.begin(); itPtA != (*itMvt1)->lstPtAttente.end(); itPtA++)
        {
            pGrp = new GrpPtsConflitTraversee;
			
			pGrp->dbDstPtAttente = (*itPtA);
				
			pGrp->nNbMaxVehAttente = (*itNbMax);

            pGrp->dbInstLastTraversee = UNDEF_INSTANT;
            pGrp->LstTf = m_LstTfTra;                            

            // Calcul du tronçon d'accueil et de la position sur ce tronçon
            double dbTmp = 0;
			if( pGrp->dbDstPtAttente == -1)	// Point d'attente par défaut (à l'xtrémité aval de l'entrée du mouvement)
			{
				pGrp->dbDstPtAttente = 0;
				pGrp->pVPtAttente = (VoieMicro*)(*itMvt1)->ent->pTAm->GetLstLanes()[(*itMvt1)->ent->nVoie];
				pGrp->dbPosPtAttente = (*itMvt1)->ent->pTAm->GetLstLanes()[(*itMvt1)->ent->nVoie]->GetLength()-0.001;
			}
			else
			{
				for(itT1 = (*itMvt1)->lstTMvt.begin(); itT1 != (*itMvt1)->lstTMvt.end(); itT1++)
				{
					if( dbTmp + (*itT1)->GetLength() > pGrp->dbDstPtAttente )
						break;
					dbTmp += (*itT1)->GetLength();
				}
				pGrp->pVPtAttente = (VoieMicro*)(*itT1)->GetLstLanes()[0];	// Tronçon interne = 1 seule voie
				pGrp->dbPosPtAttente = pGrp->dbDstPtAttente - dbTmp;
			}

            // Ajout à la liste des groupes de traversée du mouvement
            (*itMvt1)->lstGrpPtsConflitTraversee.push_back(pGrp);

			itNbMax++;
        }

        // Affectation des traversées selon leur position par rapport aux points d'attente
        for(itPtsCT = pGrpPtsCT->lstPtsConflitTraversee.begin(); itPtsCT != pGrpPtsCT->lstPtsConflitTraversee.end(); itPtsCT++)
        {
            if(  (*itPtsCT)->pT1 != (*itPtsCT)->pTProp )
                dbPos = (*itPtsCT)->dbDst1;
            else
                dbPos = (*itPtsCT)->dbDst2;

            for( itGrp = (*itMvt1)->lstGrpPtsConflitTraversee.begin(); itGrp != (*itMvt1)->lstGrpPtsConflitTraversee.end(); itGrp++)
            {
                if( dbPos < (*itGrp)->dbDstPtAttente )
                    break;
            }
            itGrp--;
            (*itGrp)->lstPtsConflitTraversee.push_back( *itPtsCT );
        }

        delete pGrpPtsCT;
    }  

    std::deque<TuyauMicro*> lstTAm; // Liste des tronçons amont d'un convergent
    map<double, TuyauMicro*> mapTAm;
    map<double, TuyauMicro*>::iterator itMap;
    VoieMicro* pVAv, *pVAm;
    PointDeConvergence *pPtCvg;

    // Parcours des convergents du CAP pour mettre à jour les priorités des convergents en fonction de leur géométrie
    for(itCvg = m_LstCvgs.begin(); itCvg != m_LstCvgs.end(); itCvg++)
    {
        pCvg = (*itCvg);
        pTAv = (TuyauMicro*)pCvg->m_LstTuyAv.front();

        // Parcours des voies du tronçon aval
        for(int nV=0; nV<pTAv->getNb_voies(); nV++)
        {
            pVAv = (VoieMicro*)pTAv->GetLstLanes()[nV];
            pPtCvg = pCvg->m_LstPtCvg[nV];

            lstTAm.clear();
            mapTAm.clear();

            // On ordonne les tronçons amont en utilisant le sens géométrique
            for(int i=0; i<(int)pPtCvg->m_LstVAm.size(); i++)
            {
                pVAm = pPtCvg->m_LstVAm[i].pVAm;
                pTAm = (TuyauMicro*)pVAm->GetParent();
                dbAlpha = AngleOfView( pVAv->GetAbsAmont(), pVAv->GetOrdAmont(), pVAv->GetAbsAval(), pVAv->GetOrdAval(), pTAm->GetAbsAmont(), pTAm->GetOrdAmont());
                mapTAm.insert( pair<double, TuyauMicro*>(dbAlpha, pTAm));
            }

            // Attribution des priorités
            int nNiv = (int)mapTAm.size();
            for ( itMap=mapTAm.begin() ; itMap != mapTAm.end(); itMap++ )
            {
                pCvg->m_LstPtCvg[nV]->InitPriorite( (VoieMicro*)(*itMap).second->GetLstLanes()[0], nNiv--);                 
            }
        }
    }

    // Si carrefour à priorité, maj des niveaux de priorités en focntion de la saisie des utilisateurs
    if(!m_pCtrlDeFeux)
    {
        std::deque<MvtCAFEx*>::iterator itMvt;
        std::deque<TuyauMicro*>::iterator itT;
        PointDeConvergence *pPtCvg;

        for(itMvt = m_LstMouvements.begin(); itMvt != m_LstMouvements.end(); itMvt++)
        {
            if( (*itMvt)->ent->nPriorite > 0)
            {
                for(itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
                {
                    if( (*itT)->get_Type_aval() == 'C' )
                    {
                        Convergent *pCvg = (Convergent*)(*itT)->getConnectionAval();
                        pPtCvg = pCvg->GetPointDeConvergence( (*itMvt)->sor->nVoie );
                        pPtCvg->InitPriorite( (VoieMicro*)(*itT)->GetLstLanes()[0], pPtCvg->GetPrioriteRef((VoieMicro*)(*itT)->GetLstLanes()[0]) + (int)pPtCvg->m_LstVAm.size() );
                    }
                }
            }
        }
    }

    return true;
}

//================================================================
    void CarrefourAFeuxEx::FinInit
//----------------------------------------------------------------
// Fonction  : Post traitements en fin de chargement
// Version du: 21/06/2013
// Historique: 21/06/2013 (O.Tonck - Ipsis)
//             Création
//================================================================        
(
    Logger   *ofLog
)
{
    if(m_pCtrlDeFeux)
    {
        int    nVAv;
        TuyauMicro *pTAv;
        std::map<Voie*, std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> >, LessPtr<Voie> >::iterator itMvtAutorises;

        // Vérification de la cohérence de la définition des lignes de feux :
        // une ligne de feux à l'intérieur d'un CAF ne peut être positionnée que sur le premier tronçon interne du mouvement correspondant !
        for(itMvtAutorises = m_mapMvtAutorises.begin(); itMvtAutorises != m_mapMvtAutorises.end(); itMvtAutorises++)
	    {
            Voie* pVoieEntree = itMvtAutorises->first;
            std::map<Tuyau*, std::map<int, boost::shared_ptr<MouvementsSortie> >, LessPtr<Tuyau> > ::iterator itSorties;
		    for(itSorties = (*itMvtAutorises).second.begin(); itSorties != (*itMvtAutorises).second.end(); itSorties++)            
            {
                // Tronçon de sortie                            
			    pTAv = (TuyauMicro*)((*itSorties).first);

			    std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator itVoiesSortie;
			    for(itVoiesSortie = (*itSorties).second.begin(); itVoiesSortie != (*itSorties).second.end(); itVoiesSortie++)        
			    {
				    nVAv = (*itVoiesSortie).first;

                    LigneDeFeu ligneFeu;
                    ligneFeu.m_pTuyAv = pTAv;
                    ligneFeu.m_pTuyAm = (Tuyau*)pVoieEntree->GetParent();
                    ligneFeu.nVAm = pVoieEntree->GetNum();
                    ligneFeu.nVAv = nVAv;
                    ligneFeu.m_pControleur = m_pCtrlDeFeux;

                    if(itVoiesSortie->second->m_dbLigneFeu <= 0)
                    {
                        double dbLengthAmont = pVoieEntree->GetLength();
                        if(dbLengthAmont < -itVoiesSortie->second->m_dbLigneFeu)
                        {
                            // on ramène la ligne à la longueur du tronçon aval
                            itVoiesSortie->second->m_dbLigneFeu = -dbLengthAmont + 0.0001;
                            *ofLog << Logger::Warning << "WARNING : The traffic light lines of the junction " << GetID() << " have been pulled back to the start of the upstream link." << std::endl;
                            *ofLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.

                        }

                        ligneFeu.m_dbPosition = dbLengthAmont+itVoiesSortie->second->m_dbLigneFeu;
                        pVoieEntree->GetLignesFeux().push_back(ligneFeu);
                    }
                    else
                    {
                        std::vector<Tuyau*> lstTuyInt;
                        GetTuyauxInternes((Tuyau*)pVoieEntree->GetParent(), pVoieEntree->GetNum(), pTAv, nVAv, lstTuyInt);

                        assert(lstTuyInt.size() > 0);

                        if(lstTuyInt[0]->GetLength() < itVoiesSortie->second->m_dbLigneFeu)
                        {
                            // on ramène la ligne à la longueur du premier tronçon interne
                            itVoiesSortie->second->m_dbLigneFeu = lstTuyInt[0]->GetLength() - 0.0001;
                            *ofLog << Logger::Warning << "WARNING : The traffic light lines of the junction " << GetID() << " have been pulled back to the end of the first internal link of the authorized movements." << std::endl;
                            *ofLog << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                        }

                        ligneFeu.m_dbPosition = itVoiesSortie->second->m_dbLigneFeu;
                        lstTuyInt[0]->GetLstLanes()[0]->GetLignesFeux().push_back(ligneFeu);
                    }
                }
            }
        }
    }

    // Calcul une fois pour toutes des couts à vides pour chaque mouvement du CAF
    Tuyau * pTuyAm, * pTuyAv;
    Point  * pBary = CalculBarycentre();
    for(size_t i=0; i<m_LstTuyAm.size();i++)
    {
        pTuyAm = m_LstTuyAm[i];

        for(size_t j=0; j<m_LstTuyAv.size();j++)
        {
            pTuyAv = m_LstTuyAv[j];

            // Calcul de l'angle du mouvement
            double dbXAm1 = pTuyAm->GetAbsAval();
            double dbYAm1 = pTuyAm->GetOrdAval();
            double dbXAm2 = pBary->dbX;
            double dbYAm2 = pBary->dbY;
            double dbXAv2 = pTuyAv->GetAbsAmont();
            double dbYAv2 = pTuyAv->GetOrdAmont();
            double dbXAv1 = pBary->dbX;
            double dbYAv1 = pBary->dbY;
            double dbAngle = AngleOfView(dbXAm1, dbYAm1, dbXAm2, dbYAm2, dbXAv2 + (dbXAm1 - dbXAv1), dbYAv2 + (dbYAm1 - dbYAv1));

            // On passe en degrés car la méthode ComputeCost compare avec la fourchette d'angle définie pour les tout droit qui est exprimé en degrés
            dbAngle *= 180.0/M_PI;

            double dbPenalty;
            if(dbAngle <= m_pReseau->GetAngleToutDroit())
            {
                // Cas du tout droit légèrement à gauche
                dbPenalty = 1;
            }
            else if(dbAngle <= 180)
            {
                // Cas du tourne à gauche
                dbPenalty = m_pReseau->GetNonPriorityPenaltyFactor();
            }
            else if(dbAngle < 360 - m_pReseau->GetAngleToutDroit())
            {
                // Cas du tourne à droite
                dbPenalty = m_pReseau->GetRightTurnPenaltyFactor();
            }
            else
            {
                // Cas du tout droit légèrement à droite
                dbPenalty = 1;
            }

            std::vector<Tuyau*> lstTuyauxInternes;
            GetTuyauxInternes(pTuyAm, pTuyAv, lstTuyauxInternes);
            TypeVehicule * pTypeVeh;
            for (size_t iTypeVeh = 0; iTypeVeh < m_pReseau->m_LstTypesVehicule.size(); iTypeVeh++)
            {
                pTypeVeh = m_pReseau->m_LstTypesVehicule[iTypeVeh];
                double dbCost = 0;
                for (size_t k = 0; k < lstTuyauxInternes.size(); k++)
                {
                    double dbVitReg = std::min<double>(lstTuyauxInternes[k]->GetVitReg(), pTypeVeh->GetVx());
                    dbCost += lstTuyauxInternes[k]->GetLength() / dbVitReg;
                }
                // rmq. : ajout d'une pénalité multiplicative en fonction du type de mouvement
                dbCost *= dbPenalty;
                m_LstCoutsAVide[std::make_pair(pTuyAm, pTuyAv)][pTypeVeh] = dbCost;
            }
        }
    }
    delete pBary;
}

//================================================================
    bool CarrefourAFeuxEx::ReInit
//----------------------------------------------------------------
// Fonction  : Mise à jour du carrefour à feux (construction des 
// objets -tronçons et connexions- internes) à partir du noeud XML
// de description 
// Version du: 27/09/2011
// Historique: 27/09/2011 (O.Tonck - Ipsis)
//             Création
//================================================================
(
	DOMNode					*pXmlNodeCAF,
    Logger                  *ofLog    
)
{    
    std::deque<EntreeCAFEx*>::iterator itE;
    std::deque<SortieCAFEx*>::iterator itS;
    std::deque<MvtCAFEx*>::iterator itM;    
    std::deque<GrpPtsConflitTraversee*>::iterator itGrpPt;

    for(size_t i = 0; i < m_LstEntrees.size(); i++)
    {
        m_LstEntreesBackup.push_back(m_LstEntrees[i]);
    }
    m_LstEntrees.clear();

    for(size_t i = 0; i < m_LstSorties.size(); i++)
    {
        m_LstSortiesBackup.push_back(m_LstSorties[i]);
    }
    m_LstSorties.clear();

    // backup des anciens mouvements (à conserver pour que les véhicules puissent finir de s'écouler
    for(size_t i = 0; i < m_LstMouvements.size(); i++)
    {
        m_LstMouvementsBackup.push_back(m_LstMouvements[i]);
    }
    m_LstMouvements.clear();

    return Init(pXmlNodeCAF, ofLog);
}
//================================================================
    void CarrefourAFeuxEx::UpdateConvergents
//----------------------------------------------------------------
// Fonction  : Mise à jour des convergents (priorité) en fonction de la 
//             couleur des feux au début du pas de temps
// Version du: 19/05/2008
// Historique: 19/05/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbInstant
)
{
    std::deque<Convergent*>::iterator itCvg;
    std::deque<PointDeConvergence*>::iterator itPtCvg;
    std::deque<DesVoieAmont>::iterator itVAm;
    Convergent *pCvg;
	PointDeConvergence *pPtCvg;
	Tuyau* pTMvt;

    if(!m_pCtrlDeFeux)	// Pas de controleur de feux donc pas de modification des priorités
        return;

	// Initialisation des niveaux de priorité avec la valeur de référence
    for(itCvg = m_LstCvgs.begin(); itCvg != m_LstCvgs.end(); itCvg++)
    {
        pCvg = (*itCvg);      

        for(itPtCvg=pCvg->m_LstPtCvg.begin(); itPtCvg!=pCvg->m_LstPtCvg.end(); itPtCvg++)
        {
            for(itVAm = (*itPtCvg)->m_LstVAm.begin(); itVAm != (*itPtCvg)->m_LstVAm.end(); itVAm++ )
			{
                (*itVAm).nNiveau = (*itVAm).nNivRef;				
			}
        }
    }    

	// Boucle sur la liste des convergents internes du CAF à mettre à jour
	for(itCvg = m_LstCvgs.begin(); itCvg != m_LstCvgs.end(); itCvg++)
	{
		 pCvg = (*itCvg);   

		for(itPtCvg=pCvg->m_LstPtCvg.begin(); itPtCvg!=pCvg->m_LstPtCvg.end(); itPtCvg++)
		{
			pPtCvg = (*itPtCvg);

			if( pPtCvg->m_LstVAm.size() == 0)
				break;

			if( (*itPtCvg)->m_LstVAm.size() == 1 )
			{
				(*itPtCvg)->m_LstVAm.begin()->nNiveau = 1;		// Un seul tronçon amont, priorité absolue
				break;
			}

			// Recherche des mouvements concernés par le point de convergence et regroupement en 3 groupes
			// Groupe 1 : mouvement rouge sans véhicule entre l'entrée amont du CAF et le point de convergence étudié
			// Groupe 2 : mouvement vert
			// Groupe 3 : mouvement rouge avec véhicule entre l'entrée amont du CAF et le point de convergence étudié
			std::deque<MvtCAFEx*>::iterator itMvt;
			std::deque<TuyauMicro*>::iterator itT;

			std::deque<MvtCAFEx*> GRP1;
			std::deque<MvtCAFEx*> GRP2;
			std::deque<MvtCAFEx*> GRP3;

			// Parcours des mouvements du carrefour
			for( itMvt = m_LstMouvements.begin(); itMvt != m_LstMouvements.end(); itMvt++)
			{
				bool bMvt = false;

				MvtCAFEx* pMvt = (*itMvt);

				// Parcours des tronçons internes du mouvement
				for( itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
				{
					 pTMvt = (*itT);

					// Parcours des voies amont du point de convergence étudié
					std::deque<TuyauMicro*>::iterator itT;
					std::deque<DesVoieAmont>::iterator itVPtCvgAm;
	
					 for( itVPtCvgAm = pPtCvg->m_LstVAm.begin(); itVPtCvgAm != pPtCvg->m_LstVAm.end(); itVPtCvgAm++)
					 {
						 if( (*itVPtCvgAm).pVAm->GetParent() == pTMvt )
						 {
							 bMvt = true;		// Le mouvement passe bien bien par le point de convergence étudié
							 break;
						 }
					 }

					 if(bMvt == true )
						 break;
				}
				if( bMvt )
				{
					if( m_pCtrlDeFeux->GetStatutCouleurFeux(dbInstant, pMvt->ent->pTAm, pMvt->sor->pTAv) == FEU_VERT)
					{
						// Feu vert
						GRP2.push_back(pMvt);
					}
					else
					{
						// Feu rouge

						// Existe t'il des véhicules entre la ligne de feu et le point de convergence étudié
						// Parcours des tronçons de la liste des tronçons des mouvements rouge
						bool bVehicule  = false;
						for( itT = pMvt->lstTMvt.begin(); itT != pMvt->lstTMvt.end(); itT++)
						{
							if( m_pReseau->HasVehicule( (VoieMicro*)(*itT)->GetLstLanes()[0]) )	
							{
								bVehicule = true;
								break;
							}
							if( (*itT) == pTMvt )	// Dernier tronçon à étudier atteind, on sort
								break;
						}

                        // rmq. : bug n°50 : problème : ne correspondant pas à la définition des groupes commentée ci-dessus
						if(  bVehicule )
							GRP1.push_back(pMvt);
						else
							GRP3.push_back(pMvt);
					}
				}
			}
			// Construction des priorités du point de convergence en fonction des groupes
			if( GRP1.size() == 0)
			{
				// Priorité au groupe feu vert 
				// Rien à faire puisque tout a été initialisé

				// Non priorité au goupe feu rouge GRP3
				for( itMvt = GRP3.begin(); itMvt != GRP3.end(); itMvt++)
				{
					// Recherche du tronçon amont du point de convergence concerné
					for( itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
					{
						 pTMvt = (*itT);

						// Parcours des voies amont du point de convergence étudié
						std::deque<TuyauMicro*>::iterator itT;
						std::deque<DesVoieAmont>::iterator itVPtCvgAm;
		
						 for( itVPtCvgAm = pPtCvg->m_LstVAm.begin(); itVPtCvgAm != pPtCvg->m_LstVAm.end(); itVPtCvgAm++)
						 {
							 if( (*itVPtCvgAm).pVAm->GetParent() == pTMvt )
							 {			
								(*itVPtCvgAm).nNiveau = (*itVPtCvgAm).nNivRef + ((int)pPtCvg->m_LstVAm.size() + 1);	// En ajoutant le nombre de tronçons amont du point de convergence, on est sur d'être au dela des niveaux de prioriotés des autres tronçons
								 break;
							 }
						 }
					 }					
				}
			}
			else
			{
				// Priorité au groupe feu rouge avec véhicule 
				// Rien à faire puisque tout a été initialisé

				// Priorité de niveau suivant au goupe feu rouge GRP2
				for( itMvt = GRP2.begin(); itMvt != GRP2.end(); itMvt++)
				{
					// Recherche du tronçon amont du point de convergence concerné
					for( itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
					{
						 pTMvt = (*itT);

						// Parcours des voies amont du point de convergence étudié
						std::deque<TuyauMicro*>::iterator itT;
						std::deque<DesVoieAmont>::iterator itVPtCvgAm;
		
						 for( itVPtCvgAm = pPtCvg->m_LstVAm.begin(); itVPtCvgAm != pPtCvg->m_LstVAm.end(); itVPtCvgAm++)
						 {
							 if( (*itVPtCvgAm).pVAm->GetParent() == pTMvt )
							 {			
								(*itVPtCvgAm).nNiveau = (*itVPtCvgAm).nNivRef + ((int)pPtCvg->m_LstVAm.size() + 1);	// En ajoutant le nombre de tronçons amont du point de convergence, on est sur d'être au dela des niveaux de prioriotés des autres tronçons
								 break;
							 }
						 }
					 }					
				}

				// Non priorité au goupe feu rouge GRP3 sans véhicule
				for( itMvt = GRP3.begin(); itMvt != GRP3.end(); itMvt++)
				{
					// Recherche du tronçon amont du point de convergence concerné
					for( itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
					{
						 pTMvt = (*itT);

						// Parcours des voies amont du point de convergence étudié
						std::deque<TuyauMicro*>::iterator itT;
						std::deque<DesVoieAmont>::iterator itVPtCvgAm;
		
						 for( itVPtCvgAm = pPtCvg->m_LstVAm.begin(); itVPtCvgAm != pPtCvg->m_LstVAm.end(); itVPtCvgAm++)
						 {
							 if( (*itVPtCvgAm).pVAm->GetParent() == pTMvt )
							 {			
								(*itVPtCvgAm).nNiveau = (*itVPtCvgAm).nNivRef + (2*(int)pPtCvg->m_LstVAm.size() + 1);	// En ajoutant deux fois le nombre de tronçons amont du point de convergence, on est sur d'être au dela des niveaux de prioriotés des autres tronçons
								 break;
							 }
						 }
					 }					
				}
			}
		}
	}
    
}

//================================================================
    bool    CarrefourAFeuxEx::GetTuyauxInternes
//----------------------------------------------------------------
// Fonction  : Retourne la liste des tuyaux internes du CAF
//             définissant l'itinéraire de pTAm à pTAv sans
//             connaissances des voies des tronçons amont et aval
//             Retourne faux si le mouvement n'est pas défini
// Remarque  : Renvoie le premier mouvement rencontré si il en existe
//             plusieurs
// Version du: 18/06/2009
// Historique: 18/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    Tuyau *pTAm,     
    Tuyau *pTAv,     
    std::vector<Tuyau*> &dqTuyaux
)
{
    for(int i=0; i<pTAm->getNb_voies(); i++)
    {
        for(int j=0; j<pTAv->getNb_voies(); j++)
        {
            if( GetTuyauxInternes(pTAm, i, pTAv, j, dqTuyaux) )
                return true;
        }
    }
    return false;
}

//================================================================
    bool    CarrefourAFeuxEx::GetTuyauxInternes
//----------------------------------------------------------------
// Fonction  : Retourne la liste des tuyaux internes du CAF
//             définissant l'itinéraire de pTAm à pTAv avec
//             connaissance des voies des tronçons amont et aval
//             Retourne faux si le mouvement n'est pas défini
// Version du: 30/09/2008
// Historique: 30/09/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    Tuyau *pTAm, 
    int nVAm,
    Tuyau *pTAv, 
    int nVAv,
    std::vector<Tuyau*> &dqTuyaux
)
{
    std::deque<MvtCAFEx*>::iterator itMvt;
    std::deque<TuyauMicro*>::iterator itT;

    // Recherche du mouvement
    for(itMvt=m_LstMouvements.begin(); itMvt!=m_LstMouvements.end(); itMvt++)
    {
        if( (*itMvt)->ent->pTAm == pTAm && (*itMvt)->ent->nVoie == nVAm && (*itMvt)->sor->pTAv == pTAv && (*itMvt)->sor->nVoie == nVAv )
        {            
            for( itT =  (*itMvt)->lstTMvt.begin();  itT != (*itMvt)->lstTMvt.end(); itT++)
            {
                dqTuyaux.push_back( (*itT) );                
            }
			return true;
        }
    }
    return false;
}

//================================================================
bool CarrefourAFeuxEx::GetTuyauxInternesRestants
//----------------------------------------------------------------
// Fonction  : Retourne la liste de tous les tuyaux internes
//             restants à partir d'un tuyau interne donné
// Version du: 30/11/2015
// Historique: 30/11/2015 (O. Tonck - Ipsis)
//             Création
//================================================================
(
    TuyauMicro *pTInt,
    Tuyau *pTAv,
    std::vector<Tuyau*> &dqTuyaux
)
{
    std::deque<MvtCAFEx*>::iterator itMvt;
    std::deque<TuyauMicro*>::iterator itT;

    // Recherche du mouvement
    for(itMvt=m_LstMouvements.begin(); itMvt!=m_LstMouvements.end(); ++itMvt)
    {
        if( (*itMvt)->sor->pTAv == pTAv)
        {            
            for( itT =  (*itMvt)->lstTMvt.begin();  itT != (*itMvt)->lstTMvt.end(); ++itT)
            {
                if(*itT == pTInt)
                {
                    dqTuyaux.insert(dqTuyaux.end(), itT, (*itMvt)->lstTMvt.end());
                    return true;
                }
            }
        }
    }
    return false;
}

//================================================================
    set<Tuyau*>    CarrefourAFeuxEx::GetAllTuyauxInternes
//----------------------------------------------------------------
// Fonction  : Retourne la liste de tous les tuyaux internes
//             potentiellement utilisables entre le tuyau aval
//             et amont donnés en paramètres
// Version du: 22/05/2013
// Historique: 22/05/2013 (O. Tonck - Ipsis)
//             Création
//================================================================
( 
    Tuyau	*pTAm, 
    Tuyau	*pTAv
)
{
    set<Tuyau*> result;
    vector<Tuyau*> tuyauxInt;
    for(int i=0; i<pTAm->getNb_voies(); i++)
    {
        for(int j=0; j<pTAv->getNb_voies(); j++)
        {
            tuyauxInt.clear();
            if( GetTuyauxInternes(pTAm, i, pTAv, j, tuyauxInt) )
            {
                result.insert(tuyauxInt.begin(), tuyauxInt.end());
            }
        }
    }
    return result;
}

VoieMicro* CarrefourAFeuxEx::GetNextTroncon(TuyauMicro *pTAm, int nVAm, TuyauMicro* pTAv, int nVAv, TuyauMicro *pTI)
{
    // Recherche du mouvement dans la liste des mouvements
    std::deque<MvtCAFEx*>::iterator itM;
    std::deque<TuyauMicro*>::iterator itT;

    for(itM=m_LstMouvements.begin(); itM!=m_LstMouvements.end(); itM++)
    {
        if( (*itM)->ent->pTAm == pTAm && (*itM)->ent->nVoie == nVAm &&
            (*itM)->sor->pTAv == pTAv && (*itM)->sor->nVoie == nVAv )
        {
            if( !pTI )
                return (VoieMicro*)(*itM)->lstTMvt[0]->GetLstLanes()[0]; // Premier tronçon interne du CAF

            for(itT=(*itM)->lstTMvt.begin(); itT!= (*itM)->lstTMvt.end(); itT++)
            {
                if( (*itT) == pTI )
                {
                    if( ++itT != (*itM)->lstTMvt.end() )
                        return (VoieMicro*)(*itT)->GetLstLanes()[0]; // Tronçon interne suivant

                    return (VoieMicro*)pTAv->GetLstLanes()[nVAv]; // On sort du CAF
                }
            }
        }
    }
    return NULL;
}

VoieMicro* CarrefourAFeuxEx::GetNextTroncon(TuyauMicro* pTAv, int nVAv, TuyauMicro *pTI)
{
    // Recherche du mouvement dans la liste des mouvements
    std::deque<MvtCAFEx*>::iterator itM;
    std::deque<TuyauMicro*>::iterator itT;

    for(itM=m_LstMouvements.begin(); itM!=m_LstMouvements.end(); itM++)
    {
        if( (*itM)->sor->pTAv == pTAv && (*itM)->sor->nVoie == nVAv )
        {
            for(itT=(*itM)->lstTMvt.begin(); itT!= (*itM)->lstTMvt.end(); itT++)
            {
                if( (*itT) == pTI )
                {
                    if( ++itT != (*itM)->lstTMvt.end() )
                        return (VoieMicro*)(*itT)->GetLstLanes()[0]; // Tronçon interne suivant

                    return (VoieMicro*)pTAv->GetLstLanes()[nVAv]; // On sort du CAF
                }
            }
        }
    }
    return NULL;
}

int CarrefourAFeuxEx::GetVoieAmont( Tuyau* pTInterne, Tuyau* pTAm, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
    // Recherche du mouvement dans la liste des mouvements
    std::deque<MvtCAFEx*>::iterator itM;
    std::deque<TuyauMicro*>::iterator itT;

    for(itM=m_LstMouvements.begin(); itM!=m_LstMouvements.end(); itM++)
    {
        if( (*itM)->ent->pTAm == pTAm && !(*itM)->IsTypeExclu(pTV, pSousType))
        {
            for(itT=(*itM)->lstTMvt.begin(); itT!= (*itM)->lstTMvt.end(); itT++)
            {
                if( (*itT) == pTInterne )
                {
                    return (*itM)->ent->nVoie;
                }
            }
        }
    }

    return -1;
}

VoieMicro* CarrefourAFeuxEx::GetNextVoieBackup( Tuyau* pTInterne, Tuyau* pTAm, TypeVehicule * pTV, SousTypeVehicule * pSousType)
{
    // Recherche da la voie amont et du mouvement correspondant
    std::deque<MvtCAFEx*>::iterator itM;
    std::deque<TuyauMicro*>::iterator itT;

    for(itM=m_LstMouvementsBackup.begin(); itM!=m_LstMouvementsBackup.end(); itM++)
    {
        if( (*itM)->ent->pTAm == pTAm && !(*itM)->IsTypeExclu(pTV, pSousType))
        {
            for(itT=(*itM)->lstTMvt.begin(); itT!= (*itM)->lstTMvt.end(); itT++)
            {
                if( (*itT) == pTInterne && pTAm == (*itM)->ent->pTAm)
                {
                    if(++itT != (*itM)->lstTMvt.end())
                        return (VoieMicro*)(*itT)->GetLstLanes()[0]; // Tronçon interne suivant

                    return (VoieMicro*)((*itM)->sor->pTAv)->GetLstLanes()[(*itM)->sor->nVoie]; // On sort du CAF
                }
            }
        }
    }

    return NULL;
}

bool isBriqueAvalCAF(Tuyau * pTuyau, void * pArg)
{
    BriqueDeConnexion * pBrique = (BriqueDeConnexion*)pArg;
    return pTuyau->GetBriqueAval() == pBrique;
}

void CarrefourAFeuxEx::CalculTraversee(Vehicule *pVeh, double dbInstant, std::vector<int> & vehiculeIDs)
{
    if(m_bSimulationTraversees)
    {
        std::deque<Vehicule*>::iterator itVeh;    
        std::deque<MvtCAFEx*>::iterator itMvt;    
        std::deque<TuyauMicro*>::iterator itT;
        std::deque<Voie*>::iterator itVoie;
        MvtCAFEx* pMvt = NULL;     
        TuyauMicro *pTInterne, *pTTra;
        VoieMicro *pVTra;
        VoieMicro * pVoieInterne;
        int nVoieSor;
        double dbDstDeb, dbDstFin, dbDstTra, dbPosTra;        

        // Recherche du mouvement emprunté
        int iTuyIdx = pVeh->GetItineraireIndex(isBriqueAvalCAF, this);
        if(iTuyIdx != -1)
        {
            Tuyau * pNextLink = NULL;
            if (iTuyIdx + 1 < (int)pVeh->GetItineraire()->size())
            {
                pNextLink = pVeh->GetItineraire()->at(iTuyIdx + 1);
            }
            else if (m_pReseau->IsCptDirectionnel() && pVeh->GetNextTuyau() && this->IsTuyauAval(pVeh->GetNextTuyau()))
            {
                pNextLink = pVeh->GetNextTuyau();
            }
            if (pNextLink)
            {
                pTInterne = NULL;
                if (pVeh->GetLink(1) && pVeh->GetLink(1)->GetBriqueParente() == this) {
                    pTInterne = (TuyauMicro*)pVeh->GetLink(1);   // véhicule à l'intérieur du CAF au début du pas de temps
                    pVoieInterne = pVeh->GetVoie(1);
                }
                else if (pVeh->GetLink(0) && pVeh->GetLink(0)->GetBriqueParente() == this) {
                    pTInterne = (TuyauMicro*)pVeh->GetLink(0);   // véhicule à l'intérieur du CAF à la fin du pas de temps  
                    pVoieInterne = pVeh->GetVoie(0);
                }
                else {                               // véhicule traversant le CAF durant le pas de temps
                    for (itVoie = pVeh->m_LstUsedLanes.begin(); itVoie != pVeh->m_LstUsedLanes.end(); itVoie++)
                    {
                        if (((Tuyau*)((*itVoie)->GetParent()))->GetBriqueParente() == this)
                        {
                            pTInterne = (TuyauMicro*)(*itVoie)->GetParent();
                            pVoieInterne = (VoieMicro *)*itVoie;
                            break;
                        }
                    }
                }

                if (pTInterne)
                {
                    // Correction du bug n°51 : on rajoute également un test sur la voie aval du mouvement ! (rmq : possibilité de rajouter un test
                    // sur la voie amont ? à rajouter si besoin)
                    // recherche parmis les voies suivantes si déjà calculées
                    do
                    {
                        pVoieInterne = (VoieMicro*)pVeh->CalculNextVoie(pVoieInterne, dbInstant);
                    } while (pVoieInterne && ((Tuyau*)pVoieInterne->GetParent())->GetBriqueParente());
                    if (pVoieInterne)
                    {
                        nVoieSor = pVoieInterne->GetNum();
                    }

                    for (itMvt = m_LstMouvements.begin(); itMvt != m_LstMouvements.end(); itMvt++)
                    {
                        if ((*itMvt)->ent->pTAm == pVeh->GetItineraire()->at(iTuyIdx) && (*itMvt)->sor->pTAv == pNextLink
                            && (!pVoieInterne || (nVoieSor == (*itMvt)->sor->nVoie)) // test de la voie de sortie si possible
                            && !(*itMvt)->IsTypeExclu(pVeh->GetType(), &pVeh->GetSousType()))
                        {
                            for (itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
                            {
                                if ((*itT) == pTInterne)
                                {
                                    pMvt = (*itMvt);
                                    break;
                                }
                            }

                            // Si la voie de sortie du mouvement est interdite au type du véhicule, on continue pour voir si on en trouve un pour lequel la voie sortant n'est pas interdite
                            // (ce mouvement sera plus vraissemblablement celui emprunté par le véhicule, surtout dans les cas ou la voie aval n'a pas pu 
                            // être déterminée, cf. commentaire bug n°51 plus haut)
                            if (pMvt && !pMvt->sor->pTAv->IsVoieInterdite(pVeh->GetType(), pMvt->sor->nVoie, dbInstant))
                                break;
                        }
                    }
                }
            }
        }
        if( pMvt )
        {
            dbDstDeb = 0;
            dbDstFin = 0;

            // Calcul de la distance déja parcourue par le véhicule à l'intérieur du CAF
            if( !(pVeh->GetLink(1) && pVeh->GetLink(1)->GetBriqueParente() == this))
                dbDstDeb = 0;
            else
                for(itT=pMvt->lstTMvt.begin(); itT!= pMvt->lstTMvt.end(); itT++)
                {
                    if( (*itT) != pVeh->GetLink(1) )
                        dbDstDeb += (*itT)->GetLength();
                    else
                    {
                        dbDstDeb += pVeh->GetPos(1);
                        break;
                    }
                }

            for(itT=pMvt->lstTMvt.begin(); itT!= pMvt->lstTMvt.end(); itT++)
            {
                if( (*itT) != pVeh->GetLink(0) )
                    dbDstFin += (*itT)->GetLength();
                else
                {
                    dbDstFin += pVeh->GetPos(0);
                    break;
                }
            }

            std::deque<GrpPtsConflitTraversee*>::iterator itGrpTra;
            std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itTra;

            // Boucle sur les groupes de traversée du mouvement
            for( itGrpTra = pMvt->lstGrpPtsConflitTraversee.begin(); itGrpTra != pMvt->lstGrpPtsConflitTraversee.end(); itGrpTra++)
            {
                if( dbDstFin > (*itGrpTra)->dbDstPtAttente  )  
                {
                    // Boucle sur les points de conflit du groupe
                    for( itTra = (*itGrpTra)->lstPtsConflitTraversee.begin(); itTra != (*itGrpTra)->lstPtsConflitTraversee.end(); itTra++)
                    {
                        // le mouvement est-il prioritaire par rapport à ce point de conflit ? Si oui, on ne va pas plus loin
                        for(itT=pMvt->lstTMvt.begin(); itT!= pMvt->lstTMvt.end(); itT++)
                        {
                            if( (*itT) == (*itTra)->pTProp->GetParent() )
                                break;
                        }
                        if( itT != pMvt->lstTMvt.end() )
						    continue;

                        if( (*itTra)->pT1 == (*itTra)->pTProp )
                        {
                            dbDstTra = (*itTra)->dbDst2;
                            dbPosTra = (*itTra)->dbPos2;
                            pVTra = (*itTra)->pT2;
                            pTTra = (TuyauMicro*)pVTra->GetParent();
                        }
                        else
                        {
                            dbDstTra = (*itTra)->dbDst1;
                            dbPosTra = (*itTra)->dbPos1;
                            pVTra = (*itTra)->pT1;
                            pTTra = (TuyauMicro*)pVTra->GetParent();
                        }

					    // Existe t'il une traversée suivante ? Si oui, a t'elle atteind son nombre max de veh en attente ?
					    if( (itGrpTra+1) != pMvt->lstGrpPtsConflitTraversee.end())
					    {
						    if( (*(itGrpTra+1))->nNbMaxVehAttente > 0)	// Y a t'il une limite du nombre de vehicule en attente ?
						    {
							    if( (*(itGrpTra+1))->nNbMaxVehAttente <= (int) (*(itGrpTra+1))->listVehEnAttente.size() )	// la limite est-elle atteinte ?
							    {
								    // Le véhicule s'arrête au point d'attente									                                  
                                    pVeh->SetPos( (*itGrpTra)->dbPosPtAttente );     
								    pVeh->SetTuyau( (TuyauMicro*)(*itGrpTra)->pVPtAttente->GetParent(), dbInstant );
                                    pVeh->SetVoie( (*itGrpTra)->pVPtAttente );   

                                    double dbVit = pVeh->GetDstParcourueEx() / m_pReseau->GetTimeStep();
                                    pVeh->SetVit(dbVit);      

                                    double dbAcc = (pVeh->GetVit(0) - pVeh->GetVit(1)) / m_pReseau->GetTimeStep();
                                    pVeh->SetAcc( dbAcc );            

								    // Ajout du véhicule à la liste des véhicules en attente									
								    if( std::find ( (*itGrpTra)->listVehEnAttente.begin(), (*itGrpTra)->listVehEnAttente.end(), pVeh->GetID()) == (*itGrpTra)->listVehEnAttente.end())									
									    (*itGrpTra)->listVehEnAttente.push_back( pVeh->GetID() );

								    dbVit = pVeh->GetDstParcourueEx() / m_pReseau->GetTimeStep();
								    pVeh->SetVit(dbVit);      

								    dbAcc = (pVeh->GetVit(0) - pVeh->GetVit(1)) / m_pReseau->GetTimeStep();
								    pVeh->SetAcc( dbAcc ); 

								    break;
							    }
						    }
					    }
                        
                        // Algorithme de traversée
                        if( dbDstDeb < dbDstTra  )
                        {
                            bool bOK = m_pReseau->CalculTraversee( pVeh, vehiculeIDs, (*itTra).get(), (*itGrpTra), GetTfTra(pVeh->GetType()), dbInstant );
                            if(!bOK || !pVeh->IsRegimeFluide() )
                            {
                                if( dbDstDeb > (*itGrpTra)->dbDstPtAttente )  // Le véhicule a dépassé le point d'attente (au début du pas de temps)
                                {                                             // il s'arrête donc au niveau de la traversée
                                    if( dbDstFin >  dbDstTra )
								    {
									    pVeh->SetPos( dbPosTra );     
									    pVeh->SetTuyau( pTTra, dbInstant );
									    pVeh->SetVoie( pVTra );       
								    }
								    else
									    break;		// La traversée n'est pas atteinte, on ne fait rien
                                }
                                else
                                {
                                    // Le véhicule est stoppé au point d'attente du groupe                                   
                                    if(dbDstDeb == (*itGrpTra)->dbDstPtAttente && pVeh->GetLink(1) && pVeh->GetLink(1)->GetBriqueParente() == this)
                                    {
                                        // Cas particulier où le véhicule est déjà sur le point d'attente,
                                        // pour éviter un problème dans le cas où ce point d'attente est à la jonction de deux tronçons
                                        // et où le véhicule est à la coordonnée amont du second tronçon (il ne faut pas
                                        // que le véhicule soit replacé à la coordonnée aval du premier tronçon, ca équivaut à un retour en arrière du véhicule...)

                                        //rmq. condition pVeh->GetLink(1) && pVeh->GetLink(1)->GetBriqueParente() == this pour éviter le cas ou le véhicule
                                        // n'est pas encore sur la brique, auquel cas dbDstDeb = 0 mais devrait valoir moins quelquechose...

                                        pVeh->SetPos( pVeh->GetPos(1) );     
								        pVeh->SetTuyau( pVeh->GetLink(1), dbInstant );
                                        pVeh->SetVoie( pVeh->GetVoie(1) );  
                                    }
                                    else
                                    {
                                        pVeh->SetPos( (*itGrpTra)->dbPosPtAttente );     
								        pVeh->SetTuyau( (TuyauMicro*)(*itGrpTra)->pVPtAttente->GetParent(), dbInstant );
                                        pVeh->SetVoie( (*itGrpTra)->pVPtAttente );   
                                    }

                                    double dbVit = pVeh->GetDstParcourueEx() / m_pReseau->GetTimeStep();
                                    pVeh->SetVit(dbVit);      

                                    double dbAcc = (pVeh->GetVit(0) - pVeh->GetVit(1)) / m_pReseau->GetTimeStep();
                                    pVeh->SetAcc( dbAcc );            

								    // Incrémentation du nombre de véhicule en attente
								    if( std::find ( (*itGrpTra)->listVehEnAttente.begin(), (*itGrpTra)->listVehEnAttente.end(), pVeh->GetID()) == (*itGrpTra)->listVehEnAttente.end())									

								    // Ajout du véhicule à la liste des véhicules en attente									
								    if( std::find ( (*itGrpTra)->listVehEnAttente.begin(), (*itGrpTra)->listVehEnAttente.end(), pVeh->GetID()) == (*itGrpTra)->listVehEnAttente.end())									
									    (*itGrpTra)->listVehEnAttente.push_back( pVeh->GetID() );
                                }
                                double dbVit = pVeh->GetDstParcourueEx() / m_pReseau->GetTimeStep();
                                pVeh->SetVit(dbVit);      

                                double dbAcc = (pVeh->GetVit(0) - pVeh->GetVit(1)) / m_pReseau->GetTimeStep();
                                pVeh->SetAcc( dbAcc );  

                                break;
                            }  
						    else
						    {
							    // Suppresion du véhicule de la liste des véhicules en attente
							    if( std::find ( (*itGrpTra)->listVehEnAttente.begin(), (*itGrpTra)->listVehEnAttente.end(), pVeh->GetID()) != (*itGrpTra)->listVehEnAttente.end())									
								    (*itGrpTra)->listVehEnAttente.remove( pVeh->GetID() );
						    }
                        }
                    }
                }
            }
        }
    }
}

//================================================================
    void CarrefourAFeuxEx::UpdatePrioriteTraversees
//----------------------------------------------------------------
// Fonction  : Mise à jour des priorités des traversées en fonction
//             de la couleur des feux pour le pas de temps courant
// Version du: 16/06/2009
// Historique: 16/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbInstant
)
{
    std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itPtCT;
    std::deque<MvtCAFEx*>::iterator itMvt, itMvt2;
    std::deque<TuyauMicro*>::iterator itT, itT2;
    bool bFeu1, bFeu2;

    if(!m_pCtrlDeFeux)  // Les priorités changent uniquement dans le cas de contrôleur de feux
        return;
    
    for(itPtCT = m_LstPtsConflitTraversee.begin(); itPtCT != m_LstPtsConflitTraversee.end(); itPtCT++)
    {
        bFeu1 = true;
        bFeu2 = true;

        // Recherche des mouvements impliquant le tronçon T1 du point de conflit
        for(itMvt = m_LstMouvements.begin(); itMvt!=m_LstMouvements.end(); itMvt++)
        {
            for(itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
            {
                if( (*itT) == (*itPtCT)->pT1->GetParent() )
                {
                    // Calcul de la couleur de feux du mouvement 
                    bFeu1 = bFeu1 && ( m_pCtrlDeFeux->GetStatutCouleurFeux( dbInstant, (*itMvt)->ent->pTAm, (*itMvt)->sor->pTAv) == FEU_VERT );

                }

                if( (*itT) == (*itPtCT)->pT2->GetParent() )
                {
                    // Calcul de la couleur de feux du mouvement 
                    bFeu2 = bFeu2 & ( m_pCtrlDeFeux->GetStatutCouleurFeux( dbInstant, (*itMvt)->ent->pTAm, (*itMvt)->sor->pTAv) == FEU_VERT );
                }
            }
        }                 

        if( bFeu1 != bFeu2 )    // Deux couleurs de feux différentes : priorité du mouvement rouge sur le vert
        {
            //if( !bFeu1 )
            if( bFeu1 )
                (*itPtCT)->pTProp = (*itPtCT)->pT1;
            else
                (*itPtCT)->pTProp = (*itPtCT)->pT2;
        }
        else // Deux même couleurs de feux -> priorité à droite
        {
            if( !IsCounterClockWise( (*itPtCT)->pT1->GetExtAmont(), (*itPtCT)->pT1->GetExtAval(), 
                                (*itPtCT)->pT2->GetExtAmont(), (*itPtCT)->pT2->GetExtAval() ) )
                (*itPtCT)->pTProp = (*itPtCT)->pT2;
            else
                (*itPtCT)->pTProp = (*itPtCT)->pT1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void EntreeCAFEx::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void EntreeCAFEx::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void EntreeCAFEx::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTAm);
    ar & BOOST_SERIALIZATION_NVP(nVoie);
    ar & BOOST_SERIALIZATION_NVP(nPriorite);
    ar & BOOST_SERIALIZATION_NVP(lstTBr);
}

template void SortieCAFEx::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SortieCAFEx::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SortieCAFEx::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTAv);
    ar & BOOST_SERIALIZATION_NVP(nVoie);
    ar & BOOST_SERIALIZATION_NVP(lstTBr);
}

template void MvtCAFEx::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void MvtCAFEx::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void MvtCAFEx::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(ent);
    ar & BOOST_SERIALIZATION_NVP(sor);
    ar & BOOST_SERIALIZATION_NVP(lstTMvt);
    ar & BOOST_SERIALIZATION_NVP(lstGrpPtsConflitTraversee);
    ar & BOOST_SERIALIZATION_NVP(lstPtAttente);
    ar & BOOST_SERIALIZATION_NVP(lstNbVehMax);
    ar & BOOST_SERIALIZATION_NVP(pMvtSortie);
}

template void CarrefourAFeuxEx::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CarrefourAFeuxEx::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CarrefourAFeuxEx::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(BriqueDeConnexion);

    ar & BOOST_SERIALIZATION_NVP(m_LstEntrees);
    ar & BOOST_SERIALIZATION_NVP(m_LstSorties);
    ar & BOOST_SERIALIZATION_NVP(m_LstEntreesBackup);
    ar & BOOST_SERIALIZATION_NVP(m_LstSortiesBackup);
    ar & BOOST_SERIALIZATION_NVP(m_LstMouvements);
    ar & BOOST_SERIALIZATION_NVP(m_LstMouvementsBackup);
    ar & BOOST_SERIALIZATION_NVP(m_LstCvgs);

    ar & BOOST_SERIALIZATION_NVP(m_dbTagreg);
    ar & BOOST_SERIALIZATION_NVP(m_dbGamma);
    ar & BOOST_SERIALIZATION_NVP(m_dbMu);
    ar & BOOST_SERIALIZATION_NVP(m_dbLongueurCellAcoustique);
}