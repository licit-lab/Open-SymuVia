#include "stdafx.h"
#include "MergingObject.h"

#include "sensors/SensorsManager.h"
#include "tuyau.h"
#include "reseau.h"
#include "DiagFonda.h"
#include "vehicule.h"
#include "voie.h"
#include "convergent.h"
#include "sensors/PonctualSensor.h"

MergingObject::MergingObject()
{
	m_pReseau = NULL;

	m_pTuyPrincipal = NULL;
	m_pTuyAval = NULL;						
	m_pTuyAmontPrioritaire = NULL;
	m_pTuyAmontnonPrioritaire = NULL;	

	m_nVoieReduite = -1;

	m_pGstCapteurs = NULL;
}

MergingObject::MergingObject(Reseau* pReseau, Tuyau* pTP, int nVoieReduite, Tuyau* pTAv, Tuyau *pTAmP, Tuyau* pTAmNP, double dbAlpha)
{
	m_pReseau = pReseau;

	m_pTuyPrincipal = pTP;
	m_pTuyAval = pTAv;						
	m_pTuyAmontPrioritaire = pTAmP;
	m_pTuyAmontnonPrioritaire = pTAmNP;	

	m_nVoieReduite = nVoieReduite;

	m_dbAlpha = dbAlpha;

    if(pReseau->GetChgtVoieMandatoryMode() == 'A')
    {
	    // Construction des capteurs utiles
	    m_pGstCapteurs = new SensorsManager(pReseau, 30, 0);

	    double dbIncPosCpt = 3;	// Paramètre de position des capteurs
			
	    m_pCptAmontNP = m_pGstCapteurs->AddCapteurPonctuel("MG_CPT1", pTAmNP, pTAmNP->GetLength() - dbIncPosCpt );
        pTAmNP->getListeCapteursConvergents().push_back(m_pCptAmontNP);

	    m_pCptAmontP = m_pGstCapteurs->AddCapteurPonctuel("MG_CPT2", pTAmP, pTAmP->GetLength() - dbIncPosCpt );
        pTAmP->getListeCapteursConvergents().push_back(m_pCptAmontP);
	    
	    m_pCptAval = m_pGstCapteurs->AddCapteurPonctuel("MG_CPT3", pTAv, dbIncPosCpt );
        pTAv->getListeCapteursConvergents().push_back(m_pCptAval);
    }
}

MergingObject::~MergingObject()
{
	if(m_pGstCapteurs)
		delete m_pGstCapteurs;
}

void MergingObject::UpdateInfoCapteurs()
{
	if(m_pGstCapteurs)
		m_pGstCapteurs->UpdateTabNbVeh();
}
void MergingObject::ProbabilityCalculation(double dbInstant)
{
	// Calcul de l'offre du tronçon aval
	double dbOffre = 0;
	dbOffre = std::min<double>( m_pReseau->GetMaxDebitMax(), m_pGstCapteurs->GetDebitMoyen( m_pCptAval, 0));

	// Calcul de la demande du tronçon amont prioritaire
	double dbDemandeTrP = 0;
	//dbDemandeTrP = min( m_pReseau->GetMaxDebitMax(), m_pGstCapteurs->GetDebitMoyen( m_pCptAmontP, 0));
	
    bool bTPFreeFlow;
	boost::shared_ptr<Vehicule> pVeh = m_pReseau->GetVehiculeAmont((TuyauMicro*)m_pTuyAmontPrioritaire, ((TuyauMicro*)m_pTuyAmontPrioritaire)->GetMaxRapVitSurQx(dbInstant),0, dbInstant );
    if(pVeh)
    {
        if( pVeh->GetVit(1) < std::min<double>( pVeh->GetVitMax(), pVeh->GetLink(1)->GetVitRegByTypeVeh(pVeh->GetType(),dbInstant, pVeh->GetPos(1), pVeh->GetVoie(1)->GetNum())) )
            bTPFreeFlow = false;
        else
            bTPFreeFlow = true;
    }
    else
        bTPFreeFlow = true;

    if(bTPFreeFlow)
        dbDemandeTrP = std::min<double>(m_pReseau->GetMaxDebitMax(), m_pGstCapteurs->GetDebitMoyen( m_pCptAmontP, 0) );
    else
        dbDemandeTrP = m_pReseau->GetMaxDebitMax();

	// Calcul de la demande du tronçon amont non prioritaire

	// Modif du 19/07/10
	double dbDemandeTrNP = 0;
    bool bTNPFreeFlow;
	pVeh.reset();
	pVeh = m_pReseau->GetVehiculeAmont((TuyauMicro*)m_pTuyAmontnonPrioritaire, ((TuyauMicro*)m_pTuyAmontnonPrioritaire)->GetMaxRapVitSurQx(dbInstant),0, dbInstant );
    if(pVeh)
    {
        if( pVeh->GetVit(1) < std::min<double>( pVeh->GetVitMax(), pVeh->GetLink(1)->GetVitRegByTypeVeh(pVeh->GetType(),dbInstant,pVeh->GetPos(1),pVeh->GetVoie(1)->GetNum())) )
            bTNPFreeFlow = false;
        else
            bTNPFreeFlow = true;
    }
    else
        bTNPFreeFlow = true;

    if(bTNPFreeFlow)
        dbDemandeTrNP = std::min<double>(m_pReseau->GetMaxDebitMax(), m_pGstCapteurs->GetDebitMoyen( m_pCptAmontNP, 0) );
    else
        dbDemandeTrNP = m_pReseau->GetMaxDebitMax();

	//dbDemandeTrNP = min( m_pReseau->GetMaxDebitMax(), m_pGstCapteurs->GetDebitMoyen( m_pCptAmontNP, 0));

	// Calcul de q2
	double dbq2 = 0;
		
	if( (dbDemandeTrP + dbDemandeTrNP) > dbOffre && dbDemandeTrP > ( 1 / (1+m_pReseau->GetFAlphaMandatory() ) * dbOffre ))
		dbq2 = std::min<double>( dbDemandeTrNP, m_pReseau->GetFAlphaMandatory() / (1+m_pReseau->GetFAlphaMandatory() ) * dbOffre );
	else
		dbq2 = dbDemandeTrNP;

	// Calcul du nombre de véhicule sur la voie d'insertion
	int nNbVeh = 0;
	nNbVeh = ((VoieMicro*)(m_pTuyPrincipal->GetLstLanes()[m_nVoieReduite]))->GetNbVeh();

	// Déduction de la probabilité
	if( nNbVeh > 0)
		m_dbProbaliteInsertion = dbq2 * m_pReseau->GetTimeStep() / nNbVeh;
	else
		m_dbProbaliteInsertion = 0;

	// TRACE
	//if( m_pReseau->IsDebug() )
	{
		//*Reseau::m_pFicSimulation<< "Merging Object : " << m_pTuyPrincipal->GetLabel() << " - offre : " << dbOffre;
		//*Reseau::m_pFicSimulation<< " - demande prio. : " << dbDemandeTrP;
		//*Reseau::m_pFicSimulation<< " - demande non prio. : " << dbDemandeTrNP;
		//*Reseau::m_pFicSimulation<< " - q2 : " << dbq2;
		//*Reseau::m_pFicSimulation<< " - nb. veh. ins. : " << nNbVeh;
		//*Reseau::m_pFicSimulation<< " - proba. : " << m_dbProbaliteInsertion << std::endl;

		//*Reseau::m_pFicSimulation<< " " << dbOffre;
		//*Reseau::m_pFicSimulation<< " " << dbDemandeTrP;
		//*Reseau::m_pFicSimulation<< " " << dbDemandeTrNP;
		//*Reseau::m_pFicSimulation<< " " << dbq2;
		//*Reseau::m_pFicSimulation<< " " << nNbVeh;
		//*Reseau::m_pFicSimulation<< " " << m_dbProbaliteInsertion;
	}
}


double	MergingObject::GetProbaliteInsertion(Vehicule * pVehicule)
{
    // rmq. : valider le fait que le calcul se fasse ici (au niveau de CalculChangementVoie et pas en fin du pas de temps comme fait avant) ?

    // ***********************************************************************************
    // ANCIENNE METHODE DE CALCUL. On utilise la proba calculée en fin de pas de temps
    // ***********************************************************************************
    //return m_dbProbaliteInsertion;
    // ***********************************
    // FIN ANCIENNE METHODE DE CALCUL
    // ***********************************


    // ******************************
    // NOUVEL ALGORITHME A TESTER
    // ******************************

    // ETAPE 1 : calcul de la demande sur la voie origine (amont non prioritaire)
    double dbDemandeOrigine = CalculDemandeOrigine(pVehicule);

    // ETAPE 2 : Calcul du débit latéral
    double dbDebitLateral = CalculDebitLateral(pVehicule, dbDemandeOrigine);

    // ETAPE  3 : Calcul de la consigne (proba d'insertion)
    double dbConsigne = CalculConsigne(pVehicule, dbDebitLateral);

    return dbConsigne;
}

double  MergingObject::CalculDemandeOrigine(Vehicule * pVehicule)
{
    double dbDemandeOrig;

    // *************************************
    // METHODE 1 : utilisation des capteurs
    // *************************************
    //dbDemandeOrig = m_pGstCapteurs->GetDebitMoyen( m_pCptAmontNP, 0);

    // *****************************************
    // METHODE 2 : calcul demande invididuelle
    // *****************************************
    //dbDemandeOrig = CalculLambai(pVehicule);

    // *************************************
    // METHODE 3 : calcul demande globale
    // *************************************
    double dbLambaiMoy = 0.0;
    int nVehiculesNP = 0;
    for(size_t i = 0; i < m_pReseau->m_LstVehicles.size(); i++)
    {
        if( m_pReseau->m_LstVehicles[i]->GetVoie(1) == pVehicule->GetVoie(1) )
        {
            dbLambaiMoy += CalculLambai(m_pReseau->m_LstVehicles[i].get());
            nVehiculesNP++;
        }
    }
    dbLambaiMoy = dbLambaiMoy / nVehiculesNP;
    dbDemandeOrig = dbLambaiMoy;

    return dbDemandeOrig;
}

double  MergingObject::CalculDebitLateral(Vehicule * pVehicule, double dbDemandeOrigine)
{
    double dbDebit;
    double mu, lambda;
    
    // ****************************
    // METHODE 1 : formule globale 
    // ****************************
    VoieMicro * pVoieInsertion = pVehicule->GetVoie(1);

    // Calcul de la longueur de la zone d'insertion (longueurEx - position du premier véhicule sur le tronçon)
    double dbLongueurInsertion = pVoieInsertion->GetLongueurEx(pVehicule->GetType());
    boost::shared_ptr<Vehicule> pLastVehicule = m_pReseau->GetNearAvalVehicule(pVoieInsertion, 0);
    // rmq. : vérifier que le calcul est bon (si le seul véhicule est le véhicule courant, la longueur d'insertion est la longueur
    // qu'il lui reste par exemple. Doit-on bien le prendre en compte ?).
    if(pLastVehicule)
    {
        dbLongueurInsertion = dbLongueurInsertion - pLastVehicule->GetPos(1);
    }

    // Calcul de l'offre de la voie cible pour le véhicule considéré
    CalculOffreDemandeDest(pVehicule, mu, lambda);
    double dbOffre = mu;

    // Calcul du débit
    double dbAlpha = m_pReseau->GetFAlphaMandatory();
    double dbQ = std::max<double>(dbOffre*dbAlpha/(1+dbAlpha),dbOffre-dbDemandeOrigine);
    dbDebit = dbQ/dbLongueurInsertion;
    
    /*
    // **********************************
    // METHODE 2 : formule individuelle
    // *********************************
    double dbLambdai = CalculLambai(pVehicule);
    double dbSumSi = 0;
    for(size_t i = 0; i < m_pReseau->m_LstVehicles.size(); i++)
    {
        if( m_pReseau->m_LstVehicles[i]->GetVoie(1) == pVehicule->GetVoie(1) )
        {
            dbSumSi += CalculSi(m_pReseau->m_LstVehicles[i].get());
        }
    }
    double dbAlpha = m_pReseau->GetFAlphaMandatory();
    
    CalculOffreDemandeDest(pVehicule, mu, lambda);
    dbDebit = min(1, mu/lambda) * dbLambdai * (dbAlpha/(1.0 + dbAlpha)) * (1.0/dbSumSi);
    */

    return dbDebit;
}

double  MergingObject::CalculConsigne(Vehicule * pVehicule, double dbDebitLateral)
{
    double dbConsigne;

    // ****************************
    // METHODE 1 : formule globale 
    // ****************************
    // Calcul du Si moyen sur les véhicules de la voie d'insertion
    double dbSiMoy = 0.0;
    int nVehiculesNP = 0;
    for(size_t i = 0; i < m_pReseau->m_LstVehicles.size(); i++)
    {
        if( m_pReseau->m_LstVehicles[i]->GetVoie(1) == pVehicule->GetVoie(1) )
        {
            dbSiMoy += CalculSi(m_pReseau->m_LstVehicles[i].get());
            nVehiculesNP++;
        }
    }
    dbSiMoy = dbSiMoy / nVehiculesNP;

    // Calcul de la probabilité d'insertion associée
    dbConsigne = dbDebitLateral*dbSiMoy*m_pReseau->GetTimeStep();

    // *********************************
    // METHODE 2 : formule individuelle 
    // *********************************
    //dbConsigne = dbDebitLateral*CalculSi(pVehicule)*m_pReseau->GetPasTemps();

    /*
    // **************************************************************
    // METHODE 3 : calcul de l'instant d'insertion (formule globale)
    // **************************************************************
    // on utilise alors le Si moyen
    double dbSiMoy = 0.0;
    int nVehiculesNP = 0;
    for(size_t i = 0; i < m_pReseau->m_LstVehicles.size(); i++)
    {
        if( m_pReseau->m_LstVehicles[i]->GetVoie(1) == pVehicule->GetVoie(1) )
        {
            dbSiMoy += CalculSi(m_pReseau->m_LstVehicles[i].get());
            nVehiculesNP++;
        }
    }
    dbSiMoy = dbSiMoy / nVehiculesNP;
    double dbQi = dbDebitLateral*dbSiMoy;
    double dbInstantInsertion = 1.0/dbQi;
    // l'insertion a lieu à ce pas de temps si l'instant déterminé est avant la fin du pas de temps
    dbConsigne = dbInstantInsertion<m_pReseau->GetPasTemps()?1:0; */
    
    /*
    // *******************************************************************
    // METHODE 4 : calcul de l'instant d'insertion (formule individuelle)
    // *******************************************************************
    // on utilise alors le Si individuel
    double dbSi = CalculSi(pVehicule);
    double dbQi = dbDebitLateral*dbSi;
    double dbInstantInsertion = 1.0/dbQi;
    // l'insertion a lieu à ce pas de temps si l'instant déterminé est avant la fin du pas de temps
    dbConsigne = dbInstantInsertion<m_pReseau->GetPasTemps()?1:0; 
    */

    return dbConsigne;
}

// Calcul de l'interdistance entre le véhicule et son leader
double  MergingObject::CalculSi(Vehicule * pVehicule)
{
    double dbSi;

    boost::shared_ptr<Vehicule> pLeader = m_pReseau->GetPrevVehicule(pVehicule);

    // Si pas de véhicule, on utilise la distance restante sur la voie
    if(pLeader)
    {
        dbSi = pLeader->GetPos(1)-pVehicule->GetPos(1);
    }
    else
    {	
        dbSi = std::max<double>( 1 / pVehicule->GetDiagFonda()->GetKMax(), pVehicule->GetVoie(1)->GetLongueurEx(pVehicule->GetType()) - pVehicule->GetPos(1));
    }

    return dbSi;
}

// Calcul de la demande individuelle d'un véhicule sur la voie origine
double MergingObject::CalculLambai(Vehicule * pVehicule)
{
    double dbVit = std::min<double>( pVehicule->GetVitRegTuyAct(), pVehicule->GetDiagFonda()->GetVitesseLibre() );
    double dbSi = CalculSi(pVehicule);
    return std::min<double>( dbVit / dbSi, pVehicule->GetDiagFonda()->GetDebitMax() );
}


// Calcul de l'offre individuelle d'un véhicule sur la voie destination
void   MergingObject::CalculOffreDemandeDest(Vehicule * pVehicule, double & mu, double & lambda)
{
    VoieMicro * pVoieCible = (VoieMicro*)m_pTuyAmontPrioritaire->GetVoie(0);
    boost::shared_ptr<Vehicule>   pVehFollower = m_pReseau->GetNearAmontVehicule(pVoieCible, pVoieCible->GetLength() / pVehicule->GetVoie(1)->GetLength() * pVehicule->GetPos(1) + 0.001, -1.0, pVehicule);
    boost::shared_ptr<Vehicule>   pVehLeader = m_pReseau->GetNearAvalVehicule(pVoieCible, pVoieCible->GetLength() / pVehicule->GetVoie(1)->GetLength() * pVehicule->GetPos(1) + 0.001, pVehicule);

    // Si pas de leader, on cherche un leader sur la voie en aval de la voie cible
    if( !pVehLeader && pVehicule->GetVoie(1))
    {
        bool bContinueRecherche = true;             // Si convergent et tuyau non prioritaire, on ne s'intéresse pas au leader sur les tuyaux en aval
        if(pVehicule->GetLink(1)->get_Type_aval() == 'C')
        {
            Convergent *pCvg = (Convergent *)pVehicule->GetLink(1)->getConnectionAval();
            if( pCvg->IsVoieAmontNonPrioritaire( pVehicule->GetVoie(1) ) )
                bContinueRecherche = false;
        }
       
        if(bContinueRecherche)
        {
            Tuyau *pDest = NULL;
            Voie* pNextVoie = pVehicule->CalculNextVoie(pVoieCible, m_pReseau->m_dbInstSimu);
            if(pNextVoie)
                pVehLeader = m_pReseau->GetNearAvalVehicule((VoieMicro*)pNextVoie, 0);
        }
    }

    // Calcul de l'inter-distance entre le véhicule follower et le vehicule leader de la voie cible
    double dbSf = 0.0;
    if(pVehFollower && pVehLeader)
	{
        dbSf = m_pReseau->GetDistanceEntreVehicules(pVehLeader.get(), pVehFollower.get());
		//dbSf = dbSf / pVehFollower->GetDeltaN(); // Demande modif ludo du 22/06/10
	}
    else
    {        
		dbSf = 1/pVehicule->GetDiagFonda()->GetKCritique();
    }

    // Offre de la voie cible
    if(pVehLeader)
        mu = std::min<double>( (pVehLeader->GetDiagFonda()->GetKMax() - (1/dbSf))*(-1)*pVehLeader->GetDiagFonda()->GetW(), pVehLeader->GetDiagFonda()->GetDebitMax() );
    else
        mu = pVehicule->GetDiagFonda()->GetDebitMax();

    // Demande de la voie cible
    if(pVehFollower)
    {
        lambda = std::min<double>( pVehFollower->GetDiagFonda()->GetVitesseLibre() / dbSf, pVehFollower->GetDiagFonda()->GetDebitMax() );
    }
    else        
    {
        double dbVit = std::min<double>( pVehicule->GetVitRegTuyAct(), pVehicule->GetDiagFonda()->GetVitesseLibre() );
        lambda = std::min<double>( dbVit / dbSf, pVehicule->GetDiagFonda()->GetDebitMax() );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void MergingObject::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void MergingObject::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void MergingObject::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyPrincipal);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyAval);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyAmontPrioritaire);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyAmontnonPrioritaire);
    ar & BOOST_SERIALIZATION_NVP(m_nVoieReduite);
    ar & BOOST_SERIALIZATION_NVP(m_dbAlpha);

    ar & BOOST_SERIALIZATION_NVP(m_pGstCapteurs);
    ar & BOOST_SERIALIZATION_NVP(m_pCptAval);
    ar & BOOST_SERIALIZATION_NVP(m_pCptAmontP);
    ar & BOOST_SERIALIZATION_NVP(m_pCptAmontNP);

    ar & BOOST_SERIALIZATION_NVP(m_dbProbaliteInsertion);
}