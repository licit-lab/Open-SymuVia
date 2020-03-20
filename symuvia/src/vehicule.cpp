#include "stdafx.h"
#include "vehicule.h"

#include "ControleurDeFeux.h"
#include "MergingObject.h"
#include "Affectation.h"
#include "DocAcoustique.h"
#include "CarrefourAFeuxEx.h"
#include "RandManager.h"
#include "Parking.h"
#include "ZoneDeTerminaison.h"
#include "DiagFonda.h"
#include "voie.h"
#include "tuyau.h"
#include "TuyauMeso.h"
#include "convergent.h"
#include "arret.h"
#include "loi_emission.h"
#include "SystemUtil.h"
#include "reseau.h"
#include "Logger.h"
#include "regulation/RegulationBrique.h"
#include "carFollowing/CarFollowingFactory.h"
#include "carFollowing/AbstractCarFollowing.h"
#include "carFollowing/CarFollowingContext.h"
#include "carFollowing/NewellContext.h"
#include "carFollowing/NewellCarFollowing.h"
#include "usage/Trip.h"
#include "usage/TripLeg.h"
#include "usage/AbstractFleet.h"
#include "usage/SymuViaFleet.h"
#include "usage/PublicTransportFleet.h"
#include "usage/logistic/DeliveryFleet.h"
#include "usage/AbstractFleetParameters.h"
#include "usage/SymuViaFleetParameters.h"
#include "usage/Schedule.h"
#include "sensors/SensorsManager.h"
#include "sensors/TankSensor.h"
#include "TravelTimesOutputManager.h"
#include "Plaque.h"
#ifdef USE_SYMUCOM
#include "ITS/Stations/DerivedClass/Simulator.h"
#endif //USE_SYMUCOM

#include <boost/make_shared.hpp>

#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include <boost/python/dict.hpp>

#include <boost/thread/lock_guard.hpp>

using namespace std;


bool isTuyauAmont(Tuyau * pTuyau, void * pArg)
{
    BriqueDeConnexion * pBrique = (BriqueDeConnexion*)pArg;
    return pBrique->IsTuyauAmont(pTuyau);
}

bool isBriqueAval(Tuyau * pTuyau, void * pArg)
{
    BriqueDeConnexion * pBrique = (BriqueDeConnexion*)pArg;
    return pTuyau->GetBriqueAval() == pBrique;
}

bool isSameTuyau(Tuyau * pTuyau, void * pArg)
{
    Tuyau * pTuyauRef = (Tuyau*)pArg;
    return pTuyauRef == pTuyau;
}

TypeVehicule::TypeVehicule()
{
    m_bVxDistr = false;
    m_bWDistr = false;
    m_bInvKxDistr = false;

    m_dbVx = 0;
    m_dbVxDisp = 0;
    m_dbVxMin = 0;
    m_dbVxMax = 0;

    m_dbW = 0;
    m_dbWDisp = 0;
    m_dbWMin = 0;
    m_dbWMax = 0;

    m_dbKx = 0;
    m_dbInvKxDisp = 0;
    m_dbKxMin = 0;
    m_dbKxMax = 0;


    m_dbEspacementArret = 0;
    m_dbDeceleration = 0;
    m_dbDecelerationEcartType = 0;

    m_dbValAgr = 0;

    m_dbTauChgtVoie = 0;
    m_dbPiRabattement = 0;

    m_dbVitLaterale = 0;

    m_dbTauDepassement = 0;

    m_dbAgressiveProportion = 0;
    m_dbShyProportion = 0;
    m_dbAgressiveEtaT = 0;
    m_dbShyEtaT = 0;
    m_epsylon0 = 0;
    m_epsylon1 = 0;
}

//================================================================
void TypeVehicule::PushPlageAcc
//----------------------------------------------------------------
// Fonction  : Ajoute une nouvelle plage d'accélération à la liste
// Version du: 01/04/2008
// Historique: 01/04/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbAcc,       // accélération
    double dbVitSup     // borne sup de la vitesse de la plage
)
{
    PlageAcceleration structPA;

    structPA.dbAcc = dbAcc;
    structPA.dbVitSup = dbVitSup;

    m_LstPlagesAcceleration.push_back(structPA);
}

double TypeVehicule::CalculDeceleration(Reseau * pNetwork)
{
    if (m_dbDecelerationEcartType == 0)
    {
        return m_dbDeceleration;
    }
    else
    {
        double minDec = std::max<double>(0,m_dbDeceleration-2*m_dbDecelerationEcartType);
        double maxDec = m_dbDeceleration+2*m_dbDecelerationEcartType;
        return pNetwork->GetRandManager()->myNormalRand(m_dbDeceleration, m_dbDecelerationEcartType, minDec, maxDec);
    }
}

//================================================================
double TypeVehicule::GetAccMax
//----------------------------------------------------------------
// Fonction  : Retourne la valeur d'accélération en fonction de la vitesse
// Version du: 01/04/2008
// Historique: 01/04/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(    
    double dbVit     // vitesse
)
{    
    for(int i=0; i<(int)m_LstPlagesAcceleration.size(); i++)    
    {        
        if( dbVit <= m_LstPlagesAcceleration[i].dbVitSup + 0.000001 )
            return m_LstPlagesAcceleration[i].dbAcc;
    }

    // Si vitesse au dessus de la dernière plage, on prend la dernière de la liste
    return m_LstPlagesAcceleration[m_LstPlagesAcceleration.size()-1].dbAcc; 
}

//================================================================
void TypeVehicule::AddSrcAcoustiques
//----------------------------------------------------------------
// Fonction  : Ajoute une source acoustique à la liste
// Version du: 06/10/2008
// Historique: 06/10/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbPos, 
    char *strIDDB
)
{
    SrcAcoustique srcAcous;

    srcAcous.dbPosition = dbPos;
    srcAcous.strIDDataBase = strIDDB;

    m_LstSrcAcoustiques.push_back(srcAcous);
}

//================================================================
Vehicule::Vehicule
//----------------------------------------------------------------
// Fonction  : Constructeur par défaut
// Version du: 17/07/2006
// Historique: 17/07/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    m_pReseau = NULL;

    m_pTypeVehicule = NULL;

    m_pTrip = NULL;
    m_pLastTripNode = NULL;
    m_pFleet = NULL;
    m_pFleetParameters = NULL;
    m_pScheduleElement = NULL;

    m_pPos  = NULL;
    m_pVoie = NULL;
    m_pTuyau = NULL;
    m_pVit  = NULL;
    m_pAcc  = NULL;

    m_pWEmission = NULL;

    m_pDF = new CDiagrammeFondamental();

    m_pNextTuyau = NULL;
    m_pNextVoie = NULL;
    m_pPrevVoie = NULL;

    m_bAgressif = false;
    m_bJorgeAgressif = false;
    m_bJorgeShy = false;
    m_bArretAuFeu = false;

    m_nID = -999;

    m_dbCumCO2 = 0;
    m_dbCumNOx = 0;
    m_dbCumPM = 0;  

    m_dbInstantEntree = -1;
    m_dbInstantSortie = -1;

    m_dbDstParcourue = 0;

    m_pGhostVoie = NULL;
    m_pVoieOld = NULL;

    m_bDriven = false;
    m_bForcedDriven = false;
    m_bDriveSuccess = false;

    m_bAttenteInsertion = false;

    m_dbDeceleration = -1;
    m_bDeceleration = false;

    m_pCurrentTuyauMeso = NULL;
	m_pNextTuyauMeso = NULL;

    m_pCarFollowing = NULL;

#ifdef USE_SYMUCOM
	m_pConnectedVehicle = NULL;
#endif // USE_SYMUCOM

    m_dbCreationPosition = 0;
    m_dbGenerationPosition = 0;

    m_dbInstInsertion = -1;
    m_dbTpsArretRestant = 0;
    m_nVoieInsertion = 0;

    m_bEndCurrentLeg = false;

    m_ExternalID = -1;

    InitChgtVoie();
}


//================================================================
Vehicule::Vehicule
//----------------------------------------------------------------
// Fonction  : Constructeur par défaut
// Version du: 07/11/2017
// Historique: 07/11/2017 (O.Tonck - Ipsis)
//             Création
// Nécessité de présence d'un constructeur par copie car utilisé
// par le wrapper python, et la présence d'un mutex non copyable
// dans la classe
//================================================================
(
    const Vehicule & other
)
{
    m_pReseau = other.m_pReseau;
    m_dbVitMax = other.m_dbVitMax;
    m_nNbPasTempsHist = other.m_nNbPasTempsHist;
    m_pTypeVehicule = other.m_pTypeVehicule;
    m_pDF = other.m_pDF;
    m_pPos = other.m_pPos;
    m_pVoie = other.m_pVoie;
    m_pTuyau = other.m_pTuyau;
    m_pVit = other.m_pVit;
    m_pAcc = other.m_pAcc;

    m_dbVitRegTuyAct = other.m_dbVitRegTuyAct;
    m_nID = other.m_nID;
    m_pNextVoie = other.m_pNextVoie;
    m_pNextTuyau = other.m_pNextTuyau;
    m_pPrevVoie = other.m_pPrevVoie;

    m_bDejaCalcule = other.m_bDejaCalcule;
    m_bDriven = other.m_bDriven;
    m_bForcedDriven = other.m_bForcedDriven;
    m_bDriveSuccess = other.m_bDriveSuccess;
    m_bAttenteInsertion = other.m_bAttenteInsertion;

    m_dbInstantCreation = other.m_dbInstantCreation;
    m_dbInstantEntree = other.m_dbInstantEntree;
    m_dbInstantSortie = other.m_dbInstantSortie;

    m_bAccMaxCalc = other.m_bAccMaxCalc;
    m_dbResTirFoll = other.m_dbResTirFoll;
    m_dbResTirFollInt = other.m_dbResTirFollInt;

    m_bOutside = other.m_bOutside;
    m_bArretAuFeu = other.m_bArretAuFeu;
    m_bChgtVoie = other.m_bChgtVoie;
    m_bVoiesOK = other.m_bVoiesOK;
    m_pWEmission = other.m_pWEmission;
    m_dbHeureEntree = other.m_dbHeureEntree;
    m_nVoieDesiree = other.m_nVoieDesiree;

    m_bRegimeFluide = other.m_bRegimeFluide;
    m_bRegimeFluideLeader = other.m_bRegimeFluideLeader;

    m_bAgressif = other.m_bAgressif;
    m_bJorgeAgressif = other.m_bJorgeAgressif;
    m_bJorgeShy = other.m_bJorgeShy;
    m_bDeceleration = other.m_bDeceleration;
    m_dbDeceleration = other.m_dbDeceleration;

    m_dbCumCO2 = other.m_dbCumCO2;
    m_dbCumNOx = other.m_dbCumNOx;
    m_dbCumPM = other.m_dbCumPM;

    m_dbDstParcourue = other.m_dbDstParcourue;
    m_nGhostRemain = other.m_nGhostRemain;
    m_pGhostFollower = other.m_pGhostFollower;
    m_pGhostLeader = other.m_pGhostLeader;
    m_pGhostVoie = other.m_pGhostVoie;

    m_pVoieOld = other.m_pVoieOld;
    m_TuyauxParcourus = other.m_TuyauxParcourus;

    m_bTypeChgtVoie = other.m_bTypeChgtVoie;
    m_TypeChgtVoie = other.m_TypeChgtVoie;
    m_bVoieCible = other.m_bVoieCible;
    m_nVoieCible = other.m_nVoieCible;
    m_bPi = other.m_bPi;
    m_dbPi = other.m_dbPi;
    m_bPhi = other.m_bPhi;
    m_dbPhi = other.m_dbPhi;
    m_bRand = other.m_bRand;
    m_dbRand = other.m_dbRand;

    m_MouvementsInsertion = other.m_MouvementsInsertion;
    m_VehiculeDepasse = other.m_VehiculeDepasse;
    m_SousType = other.m_SousType;
    m_NextVoieTirage = other.m_NextVoieTirage;
    m_pCarFollowing = other.m_pCarFollowing;
    m_dbCreationPosition = other.m_dbCreationPosition;
    m_dbGenerationPosition = other.m_dbGenerationPosition;

    m_pTrip = other.m_pTrip;
    m_pLastTripNode = other.m_pLastTripNode;

    m_pFleet = other.m_pFleet;
    m_pFleetParameters = other.m_pFleetParameters;
    m_pScheduleElement = other.m_pScheduleElement;

    m_dbTpsArretRestant = other.m_dbTpsArretRestant;
    m_dbInstInsertion = other.m_dbInstInsertion;
    m_nVoieInsertion = other.m_nVoieInsertion;

    m_bEndCurrentLeg = other.m_bEndCurrentLeg;
    m_pDestinationEnterLane = other.m_pDestinationEnterLane;
    m_dbInstDestinationReached = other.m_dbInstDestinationReached;
    m_dbDestinationTimeStep = other.m_dbDestinationTimeStep;
    m_ExternalID = other.m_ExternalID;

    m_nextTuyaux = other.m_nextTuyaux;
    m_LstUsedLanes = other.m_LstUsedLanes;
    m_LstVoiesSvt = other.m_LstVoiesSvt;

    m_pCurrentTuyauMeso = other.m_pCurrentTuyauMeso;
    m_timeCode = other.m_timeCode;
    m_pNextTuyauMeso = other.m_pNextTuyauMeso;

#ifdef USE_SYMUCOM
	m_pConnectedVehicle = other.m_pConnectedVehicle;
#endif // USE_SYMUCOM

}

//================================================================
Vehicule::Vehicule
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 17/07/2006
// Historique: 17/07/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    Reseau*	pReseau,	
    int     nID,                // Identifiant unique
    double  dbVitInit,          // Vitesse initiale du véhicule
    TypeVehicule*   pTV,        // Type du véhicule
    double  dbPasTemps          // Pas de temps de la simulation
)
{
    double dbW, dbVx;

    m_pReseau = pReseau;

    m_nID = nID;

    m_pTypeVehicule = pTV;        

    if( pTV->IsWDistr() )
    {
        dbW = pReseau->GetRandManager()->myNormalRand(pTV->GetW(), pTV->GetWDisp(), pTV->GetWMin(), pTV->GetWMax());
    }
    else
        dbW  = pTV->GetW();

    double dbKx = pTV->GetKx();

    if( pTV->IsVxDistr() )
    {
        dbVx = pReseau->GetRandManager()->myNormalRand(pTV->GetVx(), pTV->GetVxDisp(), pTV->GetVxMin(), pTV->GetVxMax());
    }
    else
        dbVx = pTV->GetVx();

    if( pTV->IsInvKxDistr() )
    {
        dbKx = 1 / pReseau->GetRandManager()->myNormalRand(1 / pTV->GetKx(), pTV->GetInvKxDisp(), 1 / pTV->GetKxMax(), 1 / pTV->GetKxMin());
    }
    else
        dbKx = pTV->GetKx();

    m_pDF = new CDiagrammeFondamental( dbW, dbKx, dbVx, NULL );
    m_dbVitMax = m_pDF->GetVitesseLibre();

    // Une fois le type du véhicule, il est possible de calculer le nombre de pas de temps à
    // historiser
    m_nNbPasTempsHist = (int)ceil( (- 1. / (GetDiagFonda()->GetW() * GetDiagFonda()->GetKMax())) / dbPasTemps )+1;

    // Allocation des tableau des variables caractéristiques du trafic
    m_pPos  = new double[m_nNbPasTempsHist+1];
    m_pVoie = new VoieMicro*[m_nNbPasTempsHist+1];
    m_pTuyau = new Tuyau*[m_nNbPasTempsHist+1];
    m_pVit  = new double[m_nNbPasTempsHist+1];
    m_pAcc  = new double[m_nNbPasTempsHist+1];

    for( int i=0; i<m_nNbPasTempsHist+1; i++)
    {
        m_pPos[i]       = UNDEF_POSITION;
        m_pVoie[i]      = NULL;
        m_pTuyau[i]      = NULL;
        m_pVit[i]       = m_dbVitMax;
        m_pAcc[i]       = 0;
    }

    m_bDejaCalcule = false;
    m_bChgtVoie = false;
    m_dbInstantCreation = 0;
    m_bArretAuFeu = false;

    m_pNextVoie = NULL;

    m_pWEmission = new double[Loi_emission::Nb_Octaves+1];

    for(int i=0; i<Loi_emission::Nb_Octaves+1; i++)
        m_pWEmission[i] = 0;

    // ATTENTION : EN DUR !!!
    m_bAccMaxCalc = false;

    m_nVoieDesiree = -1;

    m_LstUsedLanes.clear();

    m_dbResTirFoll = -1;
    m_dbResTirFollInt = -1;

    m_bRegimeFluide = true;
    m_bRegimeFluideLeader = true;

    m_bOutside = false;

    m_pPrevVoie = NULL;

    m_bAgressif = false;
    m_bJorgeAgressif = false;
    m_bJorgeShy = false;

    m_pNextTuyau = NULL;

    m_dbCumCO2 = 0;
    m_dbCumNOx = 0;
    m_dbCumPM = 0;  

    m_dbInstantEntree = -1;
    m_dbInstantSortie = -1;

    m_dbDstParcourue = 0;

    m_nGhostRemain = 0;			
    m_pGhostVoie = NULL;
    m_pVoieOld = NULL;

    m_pTrip = NULL;
    m_pLastTripNode = NULL;
    m_pFleet = NULL;
    m_pFleetParameters = NULL;
    m_pScheduleElement = NULL;

    m_bDriven = false;
    m_bForcedDriven = false;
    m_bDriveSuccess = false;

    m_bAttenteInsertion = false;

    m_dbDeceleration = -1;
    m_bDeceleration = false;

    m_pCurrentTuyauMeso = NULL;
	m_pNextTuyauMeso = NULL;

    m_pCarFollowing = NULL;

    m_dbCreationPosition = 0;
    m_dbGenerationPosition = 0;

    m_dbInstInsertion = -1;
    m_dbTpsArretRestant = 0;
    m_nVoieInsertion = 0;

    m_bEndCurrentLeg = false;

    m_ExternalID = -1;

    InitChgtVoie();

#ifdef USE_SYMUCOM
    m_pConnectedVehicle = NULL;
#endif //USE_SYMUCOM
}

//================================================================
Vehicule::~Vehicule
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 17/07/2006
// Historique: 17/07/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    if(m_pPos)
        delete [] m_pPos;

    if(m_pVoie)
        delete [] m_pVoie;

    if(m_pTuyau)
        delete [] m_pTuyau;

    if(m_pVit)
        delete [] m_pVit;

    if(m_pAcc)
        delete [] m_pAcc;   

    if(m_pWEmission)
        delete [] m_pWEmission;
    m_pWEmission = NULL;

    if(m_pDF)
        delete m_pDF;
    m_pDF = NULL;

    if(m_pCarFollowing)
        delete m_pCarFollowing;

    if(m_pTrip)
    {
        delete m_pTrip;
    }

    if(m_pFleetParameters)
        delete m_pFleetParameters;

#ifdef USE_SYMUCOM
	if (m_pConnectedVehicle)
		m_pReseau->m_pSymucomSimulator->RemoveConnectedVehicle(m_pConnectedVehicle);
#endif //USE_SYMUCOM
}

void Vehicule::SetTrip(Trip * pTrip)
{
    assert(!m_pTrip);
    m_pTrip = pTrip;
}

Trip* Vehicule::GetTrip()
{
    return m_pTrip;
}

void Vehicule::SetFleet(AbstractFleet* pFleet)
{
    m_pFleet = pFleet;
}

AbstractFleet * Vehicule::GetFleet()
{
    return m_pFleet;
}

AbstractFleetParameters *Vehicule::GetFleetParameters()
{
    return m_pFleetParameters;
}

void Vehicule::SetFleetParameters(AbstractFleetParameters * pParams)
{
    m_pFleetParameters = pParams;
}

ScheduleElement *Vehicule::GetScheduleElement()
{
    return m_pScheduleElement;
}

void Vehicule::SetScheduleElement(ScheduleElement * pScheduleElement)
{
    m_pScheduleElement = pScheduleElement;
}

TripNode * Vehicule::GetOrigine()
{
    return m_pTrip->GetOrigin();
}

TripNode * Vehicule::GetDestination()
{
    return m_pTrip->GetInitialDestination();
}

std::vector<Tuyau*>* Vehicule::GetItineraire()
{
    return m_pTrip->GetFullPath();
}

bool Vehicule::IsAtTripNode()
{
    TripNode * pDestination = GetTrip()->GetCurrentLeg()->GetDestination();

    if (!pDestination)
        return false;

    return pDestination->HasVehicle(shared_from_this());
}

void Vehicule::MoveToNextLeg()
{
    // Sortie du TripNode précédent
    if(GetTrip())
    {
        bool bIsTripOrigin = false;
        TripNode * pOrigin = GetTrip()->GetCurrentDestination();
        if(!pOrigin)
        {
            // si pas de leg courant, le véhicule sort de l'origine du Trip
            pOrigin = GetTrip()->GetOrigin();
            bIsTripOrigin = true;
        }

        // Il peut arriver qu'on n'ait pas d'origine ici (chargement des véhicules pour calcul d'émissions par exemple : pas grave car one ne s'en sert pas dans ce cas)
        if (pOrigin)
        {
            // Cas particulier : le leg précédent allait vers un parking, mais plein, le véhicule a été réaffecté : ne pas faire de VehiculeExit sur le parking, qui n'a
            // jamais été atteint, et d'où le véhicule ne sort donc pas, sinon décrémentation du stock...
            bool bIgnoredParking = !bIsTripOrigin && dynamic_cast<Parking*>(pOrigin);
            if (!bIgnoredParking)
            {
                pOrigin->VehiculeExit(shared_from_this());
            }
        }
    }

    // si le leg courant est terminé avant d'en avoir parcouru tous les tronçons,
    // on supprime les tronçons non parcourus et on invalide le FullPath.
    // Ce cas peut se produire lorsqu'on a affecté à un véhicule un itinéraire pour arriver en zone, et 
    // que cet itinéraire est plus long que le strict minimum pour atteindre la zone (cas de routes prédéfinies vers le barycente d'une zone par exemple)
    GetTrip()->EndCurrentPath();

    // Passage au leg suivant
    GetTrip()->IncCurrentLegIndex();
}

//================================================================
void Vehicule::InitializeCarFollowing
//----------------------------------------------------------------
// Fonction  :  Initialise la loi de poursuite à utiliser pour le
//              véhicule.
// Version du:  18/12/2014
// Historique:  18/12/2014 (O.Tonck - IPSIS)
//              Création
//================================================================
(
)
{
    // Cette méthode ne peut être appelée qu'une fois (sinon, il
    // faudrait deleter l'ancienne loi
    assert(m_pCarFollowing == NULL);

    m_pCarFollowing = m_pReseau->GetCarFollowingFactory()->buildCarFollowing(this);
}

void Vehicule::TirageJorge()
{
    double dbRand = (double)m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;

    if(dbRand < GetType()->GetAgressiveProportion())
    {
        m_bJorgeAgressif = true;
    }
    else if(dbRand < GetType()->GetAgressiveProportion() + GetType()->GetShyProportion())
    {
        m_bJorgeShy = true;
    }    
}

//================================================================
AbstractCarFollowing * Vehicule::GetCarFollowing
//================================================================
()
{
    return m_pCarFollowing;
}

//================================================================
double Vehicule::GetLength()
//================================================================
{
    return m_pCarFollowing->GetVehicleLength();
}

//================================================================
std::string Vehicule::GetUnifiedID
//================================================================
()
{
    return SystemUtil::ToString(m_nID);
}

//================================================================
double Vehicule::GetPos
//----------------------------------------------------------------
// Fonction  :  Retourne la valeur stockée dans N correspondant à
//              l'indice nDiffTemps
// Remarque  :  l'indice 0 correspond à l'instant traité
// Version du:  29/06/2006
// Historique:  29/06/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    int nDiffTemps
)
{
    if(!m_pPos)
        return DBL_MAX;

    if( nDiffTemps > m_nNbPasTempsHist )
        return DBL_MAX;

    return m_pPos[nDiffTemps];
}

//================================================================
VoieMicro* Vehicule::GetVoie
//----------------------------------------------------------------
// Fonction  :  Retourne la valeur stockée dans Voie correspondant à
//              l'indice nDiffTemps
// Remarque  :  l'indice 0 correspond à l'instant traité
// Version du:  29/06/2006
// Historique:  29/06/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    int nDiffTemps
)
{
    if(!m_pVoie)
        return NULL;

    if( nDiffTemps > m_nNbPasTempsHist )
        return NULL;

    return m_pVoie[nDiffTemps];
}

//================================================================
Tuyau* Vehicule::GetLink
//----------------------------------------------------------------
// Fonction  :  Retourne la valeur stockée dans Tuyau correspondant à
//              l'indice nDiffTemps
// Remarque  :  l'indice 0 correspond à l'instant traité
// Version du:  29/06/2006
// Historique:  29/06/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    int nDiffTemps
)
{
    if(!m_pTuyau)
        return NULL;

    if( nDiffTemps > m_nNbPasTempsHist )
        return NULL;

    return m_pTuyau[nDiffTemps];
}

//================================================================
double Vehicule::GetVit
//----------------------------------------------------------------
// Fonction  :  Retourne la valeur stockée dans Vit correspondant à
//              l'indice nDiffTemps
// Remarque  :  l'indice 0 correspond à l'instant traité
// Version du:  03/07/2006
// Historique:  03/07/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    int nDiffTemps
)
{
    if(!m_pVit)
        return DBL_MAX;

    if( nDiffTemps > m_nNbPasTempsHist )
        return DBL_MAX;

    return m_pVit[nDiffTemps];
}

//================================================================
double Vehicule::GetAcc
//----------------------------------------------------------------
// Fonction  :  Retourne la valeur stockée dans Acc correspondant à
//              l'indice nDiffTemps
// Remarque  :  l'indice 0 correspond à l'instant traité
// Version du:  03/07/2006
// Historique:  03/07/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    int nDiffTemps
)
{
    if(!m_pAcc)
        return DBL_MAX;

    if( nDiffTemps > m_nNbPasTempsHist )
        return DBL_MAX;

    return m_pAcc[nDiffTemps];
}

//================================================================
double Vehicule::GetAccMax
//----------------------------------------------------------------
// Fonction  :  Retourne la valeur max de l'accélération
// Remarque  :
// Version du:  04/11/2008
// Historique:  21/12/2006 (C.Bécarie - Tinea)
//               Création
//              01/04/2008 (C.Bécarie - Tinea)
//               L'accélération est définie par plage de vitesse
//              04/11/2008 (C.Bécarie - Tinea)
//               Si la simulation est de type accélération non bornée,
//               l'accélération retournée est infinie
//================================================================
(
    double  dbVit
)
{
    if(m_pVoie[1])
        if( !m_pReseau->IsAccBornee())
            return DBL_MAX;

    if(m_bAccMaxCalc)
        return 3.4*(1-m_pVit[1]*3.6/123.8);
    else
        return GetType()->GetAccMax(dbVit);
}

//================================================================
void Vehicule::SetVoieAnt
//----------------------------------------------------------------
// Fonction  :  Met à jour la voie du véhicule au début du pas de
//              temps
// Remarque  :
// Version du:  12/06/2013
//              12/06/2013 (O.Tonck - ipsis)
//               Ajout de la gestion dfes listes de véhicules
//               pour chaque voie au début du pas de temps (pour
//               optimisation des perfs)
//================================================================
(
    VoieMicro* pVoie
)
{

    // mise à jour de la voie précédente
    if(m_pVoie[1] != NULL)
    {
        m_pVoie[1]->GetLstVehiculesAnt().erase(shared_from_this());
    }
    m_pVoie[1] = pVoie;
    // mise à jour de la liste des véhicules du début du pas de temps pour la nouvelle voie
    if(m_pVoie[1] != NULL && !IsMeso())
    {
        m_pVoie[1]->GetLstVehiculesAnt().insert(shared_from_this());
    }
}

//================================================================
void Vehicule::DecalVarTrafic
//----------------------------------------------------------------
// Fonction  :  Remet à jour les valeurs des variables caractéristiques
//              du trafic de façon à ce que la première valeur 
//              corresponde à l'instant traité et la dernière valeur à
//              l'instant ( Instant - PasTemps* |w|) où w est la pente
//              de remontée de congestion du diagramme fondamental
// Version du:  06/10/2008
// Historique:  29/06/2006 (C.Bécarie - Tinea)
//              Création
//             06/10/2008 (C.Bécarie - INEO TINEA)
//             Ajout gestion plusieurs sources acoustiques pour un 
//             type de véhicule (notion de voie précédente)
//================================================================
(
)
{
    int i;


    if(!m_pPos & !m_pVit & !m_pAcc)
        return;

    m_bDejaCalcule = false;  
    m_bAttenteInsertion = false;
    m_bDeceleration = false;
    m_bEndCurrentLeg = false;

    //  if( (!GetLink(1)  ||( GetLink(1)->GetType() != Tuyau::TT_MESO)))/* &&
    //      (GetCurrentTuyauMeso() == NULL  || GetCurrentTuyauMeso()->GetType() !=  Tuyau::TT_MESO))*/
    { 
        if(m_pVoie[0] != m_pVoie[1])
            m_pPrevVoie = m_pVoie[1];               // MAJ voie précédente

        for(i=m_nNbPasTempsHist-2;i>=0;i--)
        {
            m_pPos[i+1] = m_pPos[i];                // Position
            if( m_pPos[i+1] != UNDEF_POSITION && fabs(m_pPos[i+1] ) < 0.00001 )     // Correction des erreurs numériques
                m_pPos[i+1] = 0;

            m_pVoie[i+1]= m_pVoie[i];               // Voie
            m_pTuyau[i+1]= m_pTuyau[i];             // Voie
            m_pVit[i+1] = m_pVit[i];                // Vitesse
            m_pAcc[i+1] = m_pAcc[i];                // Accélération
        }

        m_pVoie[0]  = NULL;
        m_pTuyau[0] = NULL;
    }
    //   else
    //   { 
    //       m_pVoie[0]  = NULL;
    //   }
    m_pPos[0]   = 0;
    m_pVit[0]   = 0;
    m_pAcc[0]   = 0;

}

double Vehicule::CalculDeceleration(double dbPasTemps, double dbLimiteArret)
{
    double dbMinimizedGoalDistance = dbLimiteArret;

    // on n'applique pas de décélération particulière si le véhicule est en régime congestionné (le leader gère la décélération qui la répercute sur les followers)
    if(m_pCarFollowing->GetCurrentContext()->IsFreeFlow())
    {
        m_bDeceleration = true;

        // on ne refait un tirage que s'il s'agit d'une nouvelle phase de décélération
        if(m_dbDeceleration == -1)
        {
            m_dbDeceleration = GetType()->CalculDeceleration(m_pReseau);
        }
        double dbDistanceDeceleration = 0;

        if(m_dbDeceleration != 0)
        {
            double dbPasTempsComplet = m_pReseau->GetTimeStep();				
            double N = floor(m_pVit[1] / (m_dbDeceleration * dbPasTempsComplet));	// Nombre de pas de temps où la décélération est appliquée
            dbDistanceDeceleration = N * m_pVit[1] * dbPasTempsComplet - ((N+1) * N/2) * m_dbDeceleration * dbPasTempsComplet * dbPasTempsComplet;
        }
        else
        {
            return dbMinimizedGoalDistance;
        }

        // on prévient la division par zero potentielle
        if(m_pVit[1] == 0)
        {
            // on recherche la plus grande position permettant de s'arrêter en douceur à l'arrêt
            double dbTime = sqrt(2.0*dbLimiteArret/m_dbDeceleration);
            double dbSpeed = m_dbDeceleration*dbTime;
            // on calcule donc la position avec la vitesse dbSpeed
            dbMinimizedGoalDistance = std::min<double>(dbMinimizedGoalDistance, dbSpeed*dbPasTemps);
        }
        else
        {
            // calcul de dbT : "fraction" du pas de temps pendant laquelle on ne décélère pas encore
            double dbLimiteDeceleration = dbLimiteArret - dbDistanceDeceleration;
            double dbT = dbLimiteDeceleration/m_pVit[1];
            if(dbT <= dbPasTemps)
            {
                if(dbT<0)
                {
                    // trop tard pour décélérer : on décélère sur tout le pas de temps
                    dbT = 0;
                }

                // rmq. pas moyen d'utiliser ici une vraie formule en x1 = x0 + dt * v0 + 0.5 * a * dt² car la vitesse m_pVit[1]
                // est la vitesse moyenne sur le pas de temps précédent et pas la vitesse au début du pas de temps.
                dbMinimizedGoalDistance = std::max<double>(std::min<double>(dbMinimizedGoalDistance, m_pVit[1]*dbPasTemps - m_dbDeceleration*(dbPasTemps-dbT)), 0);
            }
        }
    }

    return dbMinimizedGoalDistance;
}


double Vehicule::GetLigneDeFeu(VoieMicro * pVoiePrev, Connexion * pCnx, Tuyau * pTuyauNext, int NumVoieNext)
{
    double dbLdF = 0.0;
    std::map<int, boost::shared_ptr<MouvementsSortie> > mapCoeffs;
    if (pCnx == NULL)
        return dbLdF;
    if( pCnx->GetCoeffsMouvementAutorise( pVoiePrev, pTuyauNext, mapCoeffs, GetType(), &m_SousType ) )
    {
        if( mapCoeffs.find(NumVoieNext)!= mapCoeffs.end() )
            dbLdF = mapCoeffs[NumVoieNext]->m_dbLigneFeu;
    }
    return dbLdF;
}

void Trace(Vehicule * pVeh, double dbInstant, char * message)
{
    std::ofstream * fic;

    fic = new ofstream("c:\\TestT.txt", std::ios_base::app);

    *fic << "--------------------------------------------------------";
    *fic << message << std::endl;
    *fic << "    Instant=" << dbInstant << std::endl;
    *fic << "    ID=" << pVeh->GetID() << std::endl;
    *fic << "    Voie[0]=" << pVeh->GetVoie(0)->GetLabel() << std::endl;
    *fic << "    Voie[1]=" << pVeh->GetVoie(1)->GetLabel() << std::endl;
    *fic << "    Pos[0]=" << pVeh->GetPos(0) << std::endl;
    *fic << "    Pos[1]=" << pVeh->GetPos(1) << std::endl;
    boost::shared_ptr<Vehicule> pLeader = pVeh->GetLeader();
    if (!pLeader)
        *fic << "    Leader=<Pas de Leader>" << std::endl;
    else
        *fic << "    Leader=" << pLeader->GetID() << std::endl;

    *fic << "--------------------------------------------------------";

    fic->close();
}

bool Vehicule::IsMeso()
{
	return m_pNextTuyauMeso != NULL || // si le véhicule est en attente pour s'insérer sur un tuyau meso, on ne le traite plus en tant que véhicule micro (sinon il est pris en compte dans les leaders, etc...)
		(GetLink(0) && GetLink(0)->GetType() == Tuyau::TT_MESO) || // tuyau suivant est meso
           (GetCurrentTuyauMeso() && GetCurrentTuyauMeso()->GetType() == Tuyau::TT_MESO);
}

void Vehicule::SetInsertionMesoEnAttente(Tuyau * pNextTuyauMeso)
{
	m_pNextTuyauMeso = pNextTuyauMeso;
}

bool Vehicule::CheckInsertionFromLink(double dbInstant, VoieMicro * pPreviousLane)
{
    VoieMicro * pVoieCible = pPreviousLane;

    boost::shared_ptr<Vehicule> pVehFollower = m_pReseau->GetNearAmontVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001, -1.0, this);

    bool bInsertionOk = true;

    if (pVehFollower)								// Si le follower sur la voie cible est ghosté, pas de changement de voie
    {
        if (pVehFollower->GetGhostVoie() == pVoieCible)
        {
            bInsertionOk = false;
        }
    }

    if (bInsertionOk)
    {
        boost::shared_ptr<Vehicule> pVehLeader = m_pReseau->GetNearAvalVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001, this);
        // Si pas de leader et si on se trouve sur une voie où le changement est obligatoire, on
        // cherche un leader sur la voie en aval de la voie cible
        if (!pVehLeader && pPreviousLane && (pPreviousLane->IsChgtVoieObligatoire(GetType()) || IsVoieDesiree()))
        {
            bool bContinueRecherche = true;             // Si convergent et tuyau non prioritaire, on ne s'intéresse pas au leader sur les tuyaux en aval
            if (m_pTuyau[1]->get_Type_aval() == 'C')
            {
                Convergent *pCvg = (Convergent *)((Tuyau*)pPreviousLane->GetParent())->getConnectionAval();
                if (pCvg->IsVoieAmontNonPrioritaire(pPreviousLane))
                    bContinueRecherche = false;
            }

            if (bContinueRecherche)
            {
                Tuyau *pDest = NULL;
                Voie* pNextVoie = CalculNextVoie(pVoieCible, dbInstant);
                if (pNextVoie)
                    pVehLeader = m_pReseau->GetNearAvalVehicule((VoieMicro*)pNextVoie, 0);
            }
        }

        bInsertionOk = m_pCarFollowing->TestLaneChange(dbInstant, pVehFollower, pVehLeader, NULL, pVoieCible, true, false, true);
    }

    return bInsertionOk;
}

//================================================================
void Vehicule::CalculTraficEx
//----------------------------------------------------------------
// Fonction  :  Calcule les variables caractéristiques du trafic
//              pour le véhicule pour un pas de temps
// Remarque  :  
// Version du:  
// Historique:  
//================================================================
(
    double  dbInstant
)
{
    std::vector<int> vehiculeIDs;
    CalculTraficEx(dbInstant, vehiculeIDs);
}


void Vehicule::CalculTraficEx
(
    double  dbInstant,
    std::vector<int> & vehiculeIDs
)
{
    if(m_bDejaCalcule)
        return;    

    // Si le vehicule apparait deja dans la pile de appels, on est dans un cas de boucle infinie : on
    // produit un message d'erreur et on termine
    for(size_t i = 0; i < vehiculeIDs.size(); i++)
    {
        if(vehiculeIDs[i] == m_nID)
        {
            // Evolution n°26 : en mode loi de poursuite exacte, on casse la boucle (la loie approchée sera utilisée alors ponctuellement) au lieu de quitter brutalement
            // Dans les autres lois de poursuite que Newell, on n'affiche pas l'avertissement : ca ne change rien puisque seule la loi Newell à besoin de calculer le leader avant le follower.
            if( dynamic_cast<NewellCarFollowing*>(GetCarFollowing()) )
            {
                m_pReseau->log() << Logger::Warning << std::endl << "Infinite loop detected (recursive call to Vehicule::CalculTraficEx() for vehicle " << m_nID << "). Using estimated car following to break the loop.";
                m_pReseau->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            }
            return;
        }
    }
    vehiculeIDs.push_back(m_nID);

    // Traitement du cas particulier des véhicules qui ne sont pas sur le réseau au début du pas de temps
    VoieMicro * pPreviousLane = GetVoie(1);
    if(!pPreviousLane)
    {
        pPreviousLane = GetVoie(0);
        this->SetVoieAnt(pPreviousLane);

        // Gestion particulière de la création des véhicules qui s'insèrent
        if(IsOutside())
        {
            m_bDejaCalcule = true;
            return;
        }
    }

    bool bCreatedVehicle;
    if(GetPos(1) <= UNDEF_POSITION)
    {
        bCreatedVehicle = true;
        if (m_dbGenerationPosition > 0)
        {
            SetPosAnt(m_dbGenerationPosition);

            // Pour les véhicules créés au milieu d'un tronçon, on teste l'insertion comme pour un changement de voie (en plus de la possibilité d'avancer via la loi de poursuite plus tard) :
            if (!CheckInsertionFromLink(dbInstant, pPreviousLane))
            {
                // Pas d'insertion possible
                m_pTuyau[0] = m_pTuyau[1];
                m_pVoie[0] = m_pVoie[1];
                m_pPos[0] = UNDEF_POSITION;
                m_pPos[1] = UNDEF_POSITION;
                m_pAcc[0] = 0;
                m_bDejaCalcule = true;
                return;
            }
        }
        else
        {
            if (GetPos(0) <= UNDEF_POSITION)
            {
                // Si pas de position particulière définie, le véhicule est créé au début du tronçon
                SetPosAnt(0);
            }
            else
            {
                // Mais si on a une position de création spécifique définie, on l'utilise
                SetPosAnt(GetPos(0));
            }
        }
    }
    else
    {
        bCreatedVehicle = false;
        m_dbCreationPosition = 0;
    }

    // Calcul du pas de temps durant lequel le véhicule avance
    // rmq : mise en place du max pour le cas où le véhicule n'a pas pu être créé depuis longtemps (par de stock dans un parking par exemple)
    // auquel cas m_dbInstantCreation prend une grande valeur négative. Hors, on ne peut pas remonter le temps au delà du début du pas de temps.
    double dbTimeStep = m_pReseau->GetTimeStep() - std::max<double>(0, m_dbInstantCreation);

    // Cas particulier des véhicules à l'arrêt à un TripNode
    // (la durée de l'arret peut être terminée et le bus non encore sorti de son arrêt car il n'arrive pas à s'insérer)
    if((IsAtTripNode() && !GetTrip()->GetCurrentLeg()->GetDestination()->ManageStop(shared_from_this(), dbInstant, dbTimeStep)) || m_bOutside)
    {
		
        if(m_bOutside)
        {
            m_pTuyau[0] = m_pTuyau[1];
            m_pVoie[0] = (VoieMicro*)m_pTuyau[0]->GetLstLanes()[GetNumVoieInsertion()];
            m_pPos[0] = m_pPos[1];
            m_pVit[0] = 0;                
            m_pAcc[0] = (-m_pVit[1]) / pPreviousLane->GetPasTemps();
        }
        m_bDejaCalcule = true;
        m_bAttenteInsertion = true;
        return;
    }

    // Détermination du la distance jusqu'à laquelle construire le contexte de calul de la loi de poursuite du véhicule
    double contextRange = ComputeContextRange(dbTimeStep);

    // Construction du nouveau contexte pour le pas de temps à calculer
    m_pCarFollowing->BuildContext(dbInstant, contextRange);

    // Si un leader peut influencer le véhicule, on le calcule en premier (récursion)
    const vector<boost::shared_ptr<Vehicule> > & leaders = m_pCarFollowing->GetCurrentContext()->GetLeaders();
    if(leaders.size() != 0)
    {
        leaders[0]->CalculTraficEx(dbInstant, vehiculeIDs);
    }

    // Calcul de la loi de poursuite
    m_pCarFollowing->Compute(dbTimeStep, dbInstant);
    m_bRegimeFluide = m_pCarFollowing->GetCurrentContext()->IsFreeFlow();
    // copie de l'indicateur fluide congestionné ne prenant en compte qu'un éventuel leader et pas les feux, par exemple
    m_bRegimeFluideLeader = m_bRegimeFluide;

    // Impact des éléments du réseau traversés sur la position calculée par la loi de poursuite
    m_bArretAuFeu = false;
    double dbStartPosition = GetPos(1) + m_dbCreationPosition;
    double dbGoalDistance = m_pCarFollowing->GetComputedTravelledDistance();
    double dbMinimizedGoalDistance = dbGoalDistance+1;
    double dbTravelledDistance = 0;
    const vector<VoieMicro*> lstLanes = m_pCarFollowing->GetCurrentContext()->GetLstLanes();
    for(size_t iLane = 0; iLane < lstLanes.size(); iLane++)
    {
        VoieMicro * pLane = lstLanes[iLane];
        double startPositionOnLane = iLane == 0 ? dbStartPosition : 0;
        double laneLength = pLane->GetLongueurEx(GetType());
        double endPositionOnLane = dbGoalDistance - dbTravelledDistance + startPositionOnLane;

        // Prise en compte des éléments du réseau traversés sur la voie courante et sur la connexion ponctuelle aval
        double dbMinimizedGoalDistanceForLane;
        if(CheckNetworkInfluence(dbInstant, dbTimeStep, pLane, laneLength, iLane, lstLanes, dbTravelledDistance, dbGoalDistance, startPositionOnLane, endPositionOnLane, dbMinimizedGoalDistanceForLane))
        {
            if(dbMinimizedGoalDistanceForLane < dbMinimizedGoalDistance)
            {
                dbMinimizedGoalDistance = dbMinimizedGoalDistanceForLane;
            }
        }
        dbTravelledDistance += std::min<double>(endPositionOnLane,laneLength) - startPositionOnLane;
    }

    dbGoalDistance = std::min<double>(dbGoalDistance, dbMinimizedGoalDistance);

    // CP du véhicule qui vient d'être créé mais ne peut pas s'insérer. On note sa position négative afin d'avoir la bonne distance
    bool bCancelInsertion = false;
    if(bCreatedVehicle && dbGoalDistance + m_dbCreationPosition <= 0)
    {
        bCancelInsertion = true;
        m_dbCreationPosition = m_dbCreationPosition + dbGoalDistance;
    }

    // on ne permet pas les déplacements négatifs !
    dbGoalDistance = std::max<double>(0, dbGoalDistance);

    // Positionnement de la vitesse
    // CP du véhicule qui vient d'être créé : En régime congestionné, afin d'avoir une vitesse réaliste, celle-ci est égale à la vitesse du leader au début du pas de temps
    if(bCreatedVehicle && !m_bRegimeFluide && leaders.size() != 0)
    {
        SetVit(std::min<double>(GetVitMax(), leaders[0]->GetVit(1)));
    }
    else
    {
        SetVit(dbTimeStep != 0 ? dbGoalDistance / dbTimeStep : GetVitMax());
    }

    // Positionnement de l'accélération
    if(bCreatedVehicle)
    {
        SetAcc(0);
    }
    else
    {
        SetAcc(dbTimeStep != 0 ? (GetVit(0)-GetVit(1)) / dbTimeStep : 0);
    }

    m_LstUsedLanes.clear();
    dbTravelledDistance = 0;
    double dbTravelledDistanceAtDestination;
    TripLeg * pCurrentLeg = GetTrip()->GetCurrentLeg();
    bool bDestinationReached = false;
    VoieMicro * pDestinationEnterLane = NULL;
    bool bContinueDirectlyAfterDestination = GetTrip()->ContinueDirectlyAfterDestination();
    for(size_t iLane = 0; iLane < lstLanes.size(); iLane++)
    {
        VoieMicro * pLane = lstLanes[iLane];
        double startPositionOnLane = iLane == 0 ? dbStartPosition : 0;
        double laneLength = pLane->GetLongueurEx(GetType());
        double endPositionOnLane = dbGoalDistance - dbTravelledDistance + startPositionOnLane;
        dbTravelledDistance += std::min<double>(endPositionOnLane,laneLength) - startPositionOnLane;
        char pNextConnexionType = ((Tuyau*)pLane->GetParent())->get_Type_aval();
        bool bIsNextConnectionExit = pNextConnexionType == 'P' || pNextConnexionType == 'S';
        bool bIsLastLane = iLane == lstLanes.size()-1;

        // Correction des erreurs d'arrondis dus à la precision double (sinon, on peut avoir un véhicule qui s'arrête à la fin 
        // d'un tronçon, mais avec une position très légèrement supérieure ce qui n'est pas cohérent).
        if((bIsLastLane && !bIsNextConnectionExit && endPositionOnLane > laneLength)
            // Correction d'un problème de précision numérique identique à celui ci-dessus mais 
            // qui provoque le passage aux feux rouges (le test ci-dessus ne couvre pas ce cas à cause du bIsLastLane
            || (m_bArretAuFeu && endPositionOnLane > laneLength && endPositionOnLane < laneLength + 0.00001))
        {
            endPositionOnLane = laneLength;
        }

        // Si la destination de l'étape courante est atteinte ...
		//std::cout << this->m_nID << " " << pLane->GetLabel() << " " << laneLength << " " << endPositionOnLane << std::endl;
        bDestinationReached = GetFleet()->IsCurrentLegDone(this, pCurrentLeg, dbInstant, pLane, laneLength, startPositionOnLane, endPositionOnLane, bContinueDirectlyAfterDestination);
		
		if(bDestinationReached)
        {
			//m_pReseau->log() << std::endl << "bDestinationReached - id " << GetID() << std::endl;
            pDestinationEnterLane = pLane;
            dbTravelledDistanceAtDestination = dbTravelledDistance;
        }

        // Cas de l'arrêt du déplacement (sortie, arrêt de bus)
        if(bDestinationReached && !bContinueDirectlyAfterDestination)
        {
            m_pTuyau[0] = (Tuyau*)pLane->GetParent();
            m_pVoie[0] = pLane;
            m_pPos[0] = endPositionOnLane;
            break;
        }
        // Si on ne va pas sur les voies suivantes, on sort de la boucle
        else if(endPositionOnLane <= laneLength)
        {
            m_pTuyau[0] = (Tuyau*)pLane->GetParent();
            m_pVoie[0] = pLane;
            m_pPos[0] = endPositionOnLane;
            break;
        }
        // Cas de l'entrée des véhicules sur un tronçon meso
        else if(!bIsLastLane && ((Tuyau*)lstLanes[iLane+1]->GetParent())->IsMeso())
        {
            CTuyauMeso * pTuyauMeso = (CTuyauMeso*)lstLanes[iLane+1]->GetParent();
            double dbInstantSortie = dbInstant - dbTimeStep;
            if(dbGoalDistance != 0)
            {
                dbInstantSortie += dbTimeStep * dbTravelledDistance / dbGoalDistance; 
            }
            bool bInserted = pTuyauMeso->GetCnxAssAm()->AddEnteringVehiculeFromOutside((Tuyau*)pLane->GetParent(),pTuyauMeso, dbInstantSortie, this);
            if(!bInserted)
            {
                m_pTuyau[0] = (Tuyau*)pLane->GetParent();
                m_pVoie[0] = pLane;
                m_pPos[0] = std::max<double>(0, laneLength - 1/GetDiagFonda()->GetKCritique());

                // On empêche le véhicule de reculer !
                if (m_pTuyau[0] == m_pTuyau[1] && m_pPos[0] < m_pPos[1])
                {
                    m_pPos[0] = m_pPos[1];
                    SetVit(0);
                }
                else
                {
                    SetVit(dbTimeStep != 0 ? (dbTravelledDistance - std::max<double>(1 / GetDiagFonda()->GetKCritique(), laneLength)) / dbTimeStep : GetVitMax());
                }
                
                SetAcc(dbTimeStep != 0 ? (GetVit(0)-GetVit(1)) / dbTimeStep : 0);
                m_bDejaCalcule = true;
                m_dbInstantCreation = 0;
                m_nVoieDesiree = -1;
                return;
            }
            else
            {
                // Le véhicule s'est inséré dans le tuyau meso, on sort de la boucle : le calcul meso prendra le relai.
				SetInsertionMesoEnAttente(pTuyauMeso);
                m_pTuyau[0] = pTuyauMeso;
                m_pVoie[0] = lstLanes[iLane+1];
                m_pPos[0] = 0;
                m_bDejaCalcule = true;
                m_dbInstantCreation = 0;
                if(iLane > 0)
                {
                    m_LstUsedLanes.push_back(pLane);
                }
                m_nVoieDesiree = -1;

                return;
            }
        }

        // Ajout du véhicule à la liste des véhicules à post-traiter pour l'insertion
        ConnectionPonctuel * pPunctualDownstreamConnection = ((Tuyau*)pLane->GetParent())->getConnectionAval();
        if (pPunctualDownstreamConnection)
        {
            // on ne le fait pas pour les entrées/sorties, qui sont bien des COnnectionPonctuel mais qui n'utilisent pas la liste
            // des véhicules qui s'insèrent. De plus, ca provoquait une grosse conso mémoire, problématique en cas de rollbacks
            // car on gardait des shared_ptr<Vehicule> de véhicules d'itérations précédentes car on ne vide jamais la liste
            // pour les entrées/sorties.
            if (((Tuyau*)pLane->GetParent())->get_Type_aval() != 'E' && ((Tuyau*)pLane->GetParent())->get_Type_aval() != 'S')
            {
                pPunctualDownstreamConnection->AddInsVeh(shared_from_this());
            }
        }

        // m_LstUsedLanes ne comprend que les voies intermédiaires
        if(iLane > 0)
        {
            m_LstUsedLanes.push_back(pLane);
        }

        // En cas de changement de voie, RAZ de la voie désirée
        m_nVoieDesiree = -1;
    }


    // gestion de la sortie du réseau
    // rmq. : on teste pDestinationEnterLane plutôt que bDestinationReached car bDestinationReached peut repasser à false si plusieurs tronçons sont traversées
    // pendant un même pas de temps. Limite actuelle de l'algo : on ne peut pas finir plusieurs TripLeg en un même pas de temps (a priori assez improbable)
    if (pDestinationEnterLane != NULL)
    {
        // Calcul du moment exact d'atteinte de l'objectif.
        double dbInstDestinationReached;
        if( dbGoalDistance > 0.0001)
            dbInstDestinationReached = dbInstant - dbTimeStep + dbTimeStep * (dbTravelledDistanceAtDestination / dbGoalDistance);
        else
            dbInstDestinationReached = dbInstant - dbTimeStep;

        // On déclenche l'évènement correspondant pour cette destination, mais seulement dans le FinCalculTrafic, car on a besoin
        // d'une liste des tuyaux déjà parcourus mise à jour pour le bon traitements des itinéraires.
        m_bEndCurrentLeg = true;
        m_pDestinationEnterLane = pDestinationEnterLane;
        m_dbInstDestinationReached = dbInstDestinationReached;
        m_dbDestinationTimeStep = dbTimeStep;
    }
    else if (m_pReseau->IsModeDepassementChgtDir() && !m_bRegimeFluideLeader)
    {
        TuyauMicro * pLink = (TuyauMicro*)m_pTuyau[0];

        // Uniquement pour les tronçons monovoie non internes
        if (pLink->getNb_voies() == 1 && !pLink->GetBriqueParente())
        {
            // Uniquement pour les véhicules qui restent sur un tronçon au court du pas de temps (pour ne pas avoir à gérer les cas compliqués avec une liste des voiesi ntermédiaires, etc...
            if ((pLink == m_pTuyau[1]) && (pLink != NULL))
            {
                // Uniquement à la fin du tronçon
                if (m_pPos[0] >= pLink->GetLength() - m_pReseau->GetDistanceDepassementChgtDir())
                {
                    Tuyau * pNextLink = CalculNextTuyau(pLink, dbInstant);
                    if (pNextLink)
                    {
                        std::map<Tuyau*, Vehicule*>::iterator iterNextLinkCandidate = pLink->m_mapVehiclesForDirChangeOvertakeMode.find(pNextLink);
                        if (iterNextLinkCandidate == pLink->m_mapVehiclesForDirChangeOvertakeMode.end()
                            || iterNextLinkCandidate->second->GetPos(0) < m_pPos[0])
                        {
                            pLink->m_mapVehiclesForDirChangeOvertakeMode[pNextLink] = this;
                        }
                    }
                }
            }
        }
    }

    // Pour l'instant, on ne sait pas gérer en même temps une arrivée à destination et un ralentissement du à une traversée
    // (il faudrait mettre à jour m_bEndCurrentLeg en fonction de si le ralentissement du à la traversée empêche finalement d'arriver à la destination...)
    // On s'assure de plus que le véhicule n'est pas sorti du réseau m_pTuyau[0] non NULL. remarque : possibilité d'améliorer GestionTraversees
    // en utilisant le tuyau de sortie ?
    if (!m_bEndCurrentLeg && m_pTuyau[0])
    {
        // Gestion des traversées
        GestionTraversees(dbInstant, vehiculeIDs);
    }

    // gestion de la non insertion des véhicules créés
    if(bCancelInsertion)
    {
        SetPosAnt(UNDEF_POSITION);
        SetPos(UNDEF_POSITION);
    }

    m_bDejaCalcule = true;
    m_dbInstantCreation = 0;

    // DEBUG
    if(leaders.size() != 0)
    {
        if(GetLink(0) && leaders[0]->GetLink(0) == GetLink(0))
        {
            if(GetPos(0)>leaders[0]->GetPos(0))
            {
                m_pReseau->log() << Logger::Warning << std::endl << "Vehicle " << m_nID << " went through its leader...";
                m_pReseau->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            }
        }
    }
}


bool Vehicule::CheckNetworkInfluence(double dbInstant, double dbTimeStep, VoieMicro * pLane, double laneLength, size_t iLane, const std::vector<VoieMicro*> & lstLanes,
    double travelledDistance, double dbGoalDistance, double startPos, double endPos,
    double & dbMinimizedGoalDistanceForLane)
{
    bool bHasInfluence = false;
    dbMinimizedGoalDistanceForLane = DBL_MAX;

    Tuyau * pLink = (Tuyau*)pLane->GetParent();

    // 1 - Cas de l'influence du franchissement de la fin de la voie
    // ***************************************************************
    if(endPos > laneLength)
    {
        // Arrêt en fin de voie dans les cas suivants :
        // - changement de voie obligatoire
        // - absence de voie suivante (itinéraire impossible, par exemple)
        bool bStopAtLaneEnd = pLane->IsChgtVoieObligatoire(GetType())
            || ((iLane == lstLanes.size() - 1) && pLink->get_Type_aval()!='S' && pLink->get_Type_aval()!='P' );

        if(bStopAtLaneEnd)
        {
            m_bRegimeFluide = false;
            bHasInfluence = true;
            dbMinimizedGoalDistanceForLane = std::min<double>(dbMinimizedGoalDistanceForLane, travelledDistance + laneLength - startPos);
        }
    }

    // 2 - Cas de l'influence du prochain TripNode
    // ************************************************
    // Gestion de la décélération : seulement si le véhicule doit s'arrêter à la fin de l'étape courante
    if(!GetTrip()->ContinueDirectlyAfterDestination())
    {
        TripNode * pNextTripNode;
        double dbDstArret = GetNextTripNodeDistance(1, &pNextTripNode);
        if(dbDstArret < DBL_MAX)
        {
            double dbMinimizedGoalDistanceForBusStop = CalculDeceleration(dbTimeStep, dbDstArret);
            if(dbMinimizedGoalDistanceForBusStop < dbGoalDistance)
            {
                bHasInfluence = true;
                dbMinimizedGoalDistanceForLane = std::min<double>(dbMinimizedGoalDistanceForLane, dbMinimizedGoalDistanceForBusStop);
            }
        }
    }
    // Gestion d'un éventuelle restriction de capacité
    if( pLink->get_Type_aval() == 'S' || pLink->get_Type_aval() == 'P')
    {
        double dbPosCrit = laneLength - GetCarFollowing()->GetMaxInfluenceRange();
        if( endPos > dbPosCrit)
        {                        
            // L'instant de sortie du véhicule suivant permet de calculer sa vitesse                         
            double dbts = pLane->GetNextInstSortie()[GetNextRouteID()];
            // Pour stopper les véhicules qui ne peuvent pas rentrer dans le parking :
            bool bBlockVehicle = false;
            TripNode * pTripNode = GetTrip()->GetCurrentLeg()->GetDestination();
            // rmq. : finalement, on autorise quand même le gars à rentrer dans un parking malgré la limite, sinon on a des problèmes avec SymuMaster ou si impossible de faire demi tour
            if (pTripNode && !pTripNode->IsAllowedVehiculeType(GetType()))
            {
                bBlockVehicle = true;
            }
            if( dbts < 0 || bBlockVehicle) // L'instant de sortie est infini, le véhicule s'arrête à 1/Kcx
            {
                bHasInfluence = true;
                dbMinimizedGoalDistanceForLane = std::min<double>(dbMinimizedGoalDistanceForLane, travelledDistance + dbPosCrit - startPos);
            }
            else    
            {
                if(dbts > dbInstant-dbTimeStep)
                {
                    double dbVit = (travelledDistance + laneLength - startPos) / (dbts - (dbInstant-dbTimeStep) );
                    bHasInfluence = true;
                    dbMinimizedGoalDistanceForLane = std::min<double>(dbMinimizedGoalDistanceForLane, dbVit*dbTimeStep);
                }
            }
        }
    }

    // 3 - Cas de l'influence de la connexion ponctuelle aval
    // ***************************************************************
    double dbMinimizedGoalDistanceForConnection = dbGoalDistance+1;
    if(pLink->getConnectionAval() && pLink->getConnectionAval()->ComputeTraffic(shared_from_this(), pLane, dbInstant, dbTimeStep, dbMinimizedGoalDistanceForConnection))
    {
        if(dbMinimizedGoalDistanceForConnection < dbGoalDistance)
        {
            bHasInfluence = true;
            dbMinimizedGoalDistanceForLane = std::min<double>(dbMinimizedGoalDistanceForLane, dbMinimizedGoalDistanceForConnection);
        }
    }

    // 4 - Cas de l'influence des feux tricolores
    // **************************************************
    double dbMinimizedGoalDistanceForLights = dbGoalDistance+1;
    bool bArretAuFeu = false;
    if(CalculTraficFeux(dbInstant, dbTimeStep, travelledDistance, startPos, endPos, dbGoalDistance, pLane, iLane, lstLanes, dbMinimizedGoalDistanceForLights, bArretAuFeu))
    {
        // gestion de la décélération au feu rouge le cas échéant
        if(bArretAuFeu)
        {
            double dbMinimizedDeceleratedGoalDistanceForLights = CalculDeceleration(dbTimeStep, dbMinimizedGoalDistanceForLights);
            dbMinimizedGoalDistanceForLights = std::min<double>(dbMinimizedGoalDistanceForLights, dbMinimizedDeceleratedGoalDistanceForLights);
        }
        if(dbMinimizedGoalDistanceForLights < dbGoalDistance)
        {
            bHasInfluence = true;
            if(dbMinimizedGoalDistanceForLights < dbMinimizedGoalDistanceForLane)
            {
                dbMinimizedGoalDistanceForLane = dbMinimizedGoalDistanceForLights;
                if(bArretAuFeu)
                {
                    m_bRegimeFluide = false;
                    m_bArretAuFeu = true;
                }
            }
        }
    }

    return bHasInfluence;
}

double Vehicule::ComputeContextRange(double dbTimeStep)
{
    // on se limite pour le contexte à la distance maximale que peut parcourir le véhicule,
    // plus la distance d'influence d'un leader éventuel
    return m_pCarFollowing->GetMaxInfluenceRange() + dbTimeStep*this->GetVitMax();
}


// gestion des traversées
void Vehicule::GestionTraversees(double dbInstant, std::vector<int> & vehiculeIDs)
{
    // Détection des véhicules ayant traversé une brique de connexion au cours du pas de temps afin de leur effectuer le traitement des traversées
    if( m_pReseau->GetSimulationTraversees() && m_pVit[0] > 0)
    {
        if( m_pTuyau[0]->GetBriqueParente() && (m_pTuyau[0]->GetBriqueParente()->GetType()=='C' || m_pTuyau[0]->GetBriqueParente()->GetType()=='G') )
        {            
            (m_pTuyau[0]->GetBriqueParente())->CalculTraversee(this, dbInstant, vehiculeIDs);            
        }
        else if( m_pTuyau[1] &&  m_pTuyau[1]->GetBriqueParente() && (m_pTuyau[1]->GetBriqueParente()->GetType()=='C' || m_pTuyau[1]->GetBriqueParente()->GetType()=='G') )
        {            
            m_pTuyau[1]->GetBriqueParente()->CalculTraversee(this, dbInstant, vehiculeIDs);            
        }
        else
        {
            std::deque<Voie*>::iterator itVoie;
            for( itVoie=m_LstUsedLanes.begin(); itVoie!=m_LstUsedLanes.end(); itVoie++)
            {
                if( ((Tuyau*)((*itVoie)->GetParent()))->GetBriqueParente() && (((Tuyau*)((*itVoie)->GetParent()))->GetBriqueParente()->GetType()=='C' || ((Tuyau*)((*itVoie)->GetParent()))->GetBriqueParente()->GetType()=='G') )
                {                    
                    (((Tuyau*)( (*itVoie)->GetParent() ))->GetBriqueParente())->CalculTraversee(this, dbInstant, vehiculeIDs);                    
                    break;
                }
            }
        }
    }	

    // mise à jour de la liste des voies empruntées (véhicule pouvant être ralenti suite aux traversées
    UpdateLstUsedLanes();
}

// Recalcule la liste des voies intermédiaires empruntées au cours du pas de temps suite à une modification de la position du véhicule
void Vehicule::UpdateLstUsedLanes()
{
    if(m_LstUsedLanes.size() > 0)
    {
        // Si une des voies est la voie finale, on l'enlève ainsi que les voies suivantes
        std::deque <Voie*>::iterator courant;
        std::deque <Voie*>::iterator debut = m_LstUsedLanes.begin();
        std::deque <Voie*>::iterator fin = m_LstUsedLanes.end();

        for (courant=debut;courant!=fin;courant++)
        {
            if( (VoieMicro*)(*courant) == m_pVoie[0])
            {
                m_LstUsedLanes.erase( courant, m_LstUsedLanes.end());
                break;
            }
        }

        // Si on n'a pas changé de voie, on vide la liste
        if(m_pTuyau[0] && m_pTuyau[0] == m_pTuyau[1])
        {
            m_LstUsedLanes.clear();
        }
    }
}


// récupération du tirage pour le choix de la voie suivante (récup en mémoire si déjà tiré, sinon tirage)
double Vehicule::GetTirage(Voie* pVoieAmont)
{
    double dbResult;

    map<Voie*, double>::iterator iterTAv = m_NextVoieTirage.find(pVoieAmont);
    if(iterTAv != m_NextVoieTirage.end())
    {
        dbResult = iterTAv->second;
    }
    else
    {
        dbResult = m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
        m_NextVoieTirage[pVoieAmont] = dbResult;
    }

    return dbResult;
}


//================================================================
bool Vehicule::CalculTraficFeux
//----------------------------------------------------------------
// Fonction  :  traite les arrêts aux lignes de feux
// Version du:  21/06/2013
// Historique:  21/06/2013 (O.Tonck - IPSIS)
//               version initiale
//================================================================
(
    double dbInstant,
    double dbTimeStep,
    double dbStartPosOfLane,
    double dbStartPosOnLane,
    double dbEndPosOnLane,
    double dbGoalDistance,
    VoieMicro * pLane,
    size_t iLane,
    const std::vector<VoieMicro*> & lstLanes,
    double &dbMinimizedGoalDistance,
    bool   &bArretAuFeu
)
{
    bool bHasInfluence = false;

    VoieMicro * pVoieAmont;
    VoieMicro * pVoieAval;
    double dbTpsSortie;

    pVoieAval = NULL;
    vector<LigneDeFeu> & lignes = pLane->GetLignesFeux();
    // traitement de chaque ligne de feu potentielle
    for(size_t iLigneFeu = 0; iLigneFeu < lignes.size(); iLigneFeu++)
    {
        LigneDeFeu & ligneFeu = lignes[iLigneFeu];

        double dbPosFeu = ligneFeu.m_dbPosition;

        if( dbPosFeu >= dbStartPosOnLane)   // le véhicule est avant la ligne de feux au début du pas de temps
        {
            // vérification de la correspondance entre la ligne de feux et le mouvement emprunté par le véhicule

            // détermination de la voie amont du mouvement
            if(((Tuyau*)pLane->GetParent())->GetBriqueParente())
            {
                pVoieAmont = pLane->GetPrevVoie();
            }
            else
            {
                pVoieAmont = pLane;
            }

            // Pas la peine de continuer si la ligne de feu ne correspond pas à la voie amont
            if(ligneFeu.m_pTuyAm == pVoieAmont->GetParent()
                && ligneFeu.nVAm == pVoieAmont->GetNum())
            {
                // détermination de la voie aval du mouvement si pas déjà calculée
                if(!pVoieAval)
                {
                    // recherche parmis les voies suivantes si déjà calculées
                    for(size_t iNextVoie = iLane+1; iNextVoie < lstLanes.size() && !pVoieAval; iNextVoie++)
                    {
                        if(!((Tuyau*)lstLanes[iNextVoie]->GetParent())->GetBriqueParente())
                        {
                            pVoieAval = lstLanes[iNextVoie];
                        }
                    }
                    // Si pas trouvé, calcul des voies suivantes
                    if(!pVoieAval)
                    {
                        pVoieAval = lstLanes.back();
                        do
                        {
                            pVoieAval = (VoieMicro*)CalculNextVoie(pVoieAval, dbInstant);
                        }
                        while(pVoieAval && ((Tuyau*)pVoieAval->GetParent())->GetBriqueParente());
                    }
                }

                // la voie aval doit correspondre à celle associée à la ligne de feux
                // rmq. : il se peut qu'on ne connaisse pas la voie aval (CalculNextVoie renvoie NULL) (cas d'un changement de voie obligatoire par exemple
                // pour disposer d'un mouvement autorisé permettant d'aller où le véhicule souhaite). Dans ce cas, on ne traite pas la ligne de feux
                // (le véhicule doit de toute façon changer de voie avant de franchir le carrefour)
                if(pVoieAval &&
                    (ligneFeu.m_pTuyAv == pVoieAval->GetParent()
                    && ligneFeu.nVAv == pVoieAval->GetNum()))
                {
                    // récupération de la couleur du feu
                    double dbTempsVertOrange = 0;
                    eCouleurFeux Couleur = ligneFeu.m_pControleur->GetStatutCouleurFeux( dbInstant, ligneFeu.m_pTuyAm, ligneFeu.m_pTuyAv, NULL, &dbTempsVertOrange);

                    double dbDstLoc = (dbStartPosOfLane-dbStartPosOnLane) + dbPosFeu;

                    // Le feu est rouge (durant tous le pas de temps), on ne va pas plus loin...
                    if((Couleur == FEU_ROUGE) && (fabs(dbTempsVertOrange) < 0.00001))
                    {
                        dbMinimizedGoalDistance = dbDstLoc;
                        bArretAuFeu = true;
                        return true;
                    }
                    
                    if(GetVit(1)>0)
                    {						
                        dbTpsSortie = dbDstLoc / GetVit(1);
                    }
                    else
                    {
                        dbTpsSortie = dbDstLoc / (dbEndPosOnLane/dbTimeStep);
                    }

                    // Le feu est vert au début du pas de temps et rouge à la fin
                    if((Couleur != FEU_VERT) && (dbTempsVertOrange > 0))
                    {
                        if(dbTpsSortie > dbTempsVertOrange*dbTimeStep)
                        {
                            // On n'a pas le temps de sortir, on s'arrête à la ligne de feu     
                            dbMinimizedGoalDistance = dbDstLoc;
                            bArretAuFeu = true;
                            return true;
                        }

                    }

                    // Le feu est vert à la fin du pas de temps				
                    if((Couleur == FEU_VERT) && (dbTempsVertOrange < 1))
                    {
                        if( dbTpsSortie > dbTimeStep*(1-dbTempsVertOrange) );   // Pas de ralentissement, le véhicule passe
                        else
                        {
                            bHasInfluence = true;
                            if( GetVit(1)>0 )
                            {
                                dbMinimizedGoalDistance = dbGoalDistance - (1-dbTempsVertOrange)*m_pVit[1]*dbTimeStep;
                            }
                            else
                            {
                                dbMinimizedGoalDistance = dbGoalDistance*dbTempsVertOrange;
                            }
                        }
                    }

                    break; // il ne peut y avoir qu'une ligne de feux correspondant au mouvement du véhicule... pas la peine de continuer pour cette voie
                }
            }
        }
    }

    return bHasInfluence;
}

//================================================================
void Vehicule::CalculXYZ
//----------------------------------------------------------------
// Fonction  :  Calcule les coordonnées absolues du véhicule à la fin
//              ou au début du pas de temps (fin par défaut)
// Remarque  :  
// Version du:  
// Historique:  
//================================================================
(
    double &dbX, 
    double &dbY, 
    double &dbZ, 
    bool bFin   /* = true */ // Indique si le calcul s'effectue à la fin du pas de temps
)
{    
    Voie*		pVoie;
    Tuyau*		pTuyau;
    double      dbPos;

    if(bFin)
    {
        pVoie = GetVoie(0);
        pTuyau = GetLink(0);
        dbPos = m_pPos[0];
    }
    else
    {
        pVoie = GetVoie(1);
        pTuyau = GetLink(1);
        dbPos = m_pPos[1];
    }


    if(!pVoie)
        return;

    if(!pTuyau)
        return;

	// Comment on peut avoir des positions négatives ici (scenario grand lyon SG) 
	// -> insertion à l'aide d'un SymCreateVehicle au niveau d'une connexion interne et le véhicule ne peut pas s'insérer à cause de la présence d'autres véhicules
    CalculAbsCoords(pVoie, std::max<double>(0, dbPos), m_bOutside, dbX, dbY, dbZ);	
}

//================================================================
void Vehicule::SortieTrafic
//----------------------------------------------------------------
// Fonction  :  Ecriture des résultats de simulation à la fin du pas
//              de temps
// Remarque  :
// Version du:  18/07/2006
// Historique:  18/07/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    DocTrafic *pXMLDocTrafic,
    bool bChgtVoieDebug, 
    double dInstant
)
{
    double		dbAbs;
    double		dbOrd;
    double		dbHaut;    
    std::string strLibelle;
    int			nNumVoie;       
    std::string sLib;
    std::string sTypeVehicule;
    double      dbLongueur;
    double      dbVitMax;
    int         nCurrentLoad = -1;

    if(m_pPos[0] <= UNDEF_POSITION &&  (!m_pTuyau[0] || m_pTuyau[0]->get_Type_amont()=='E') )
	{ 		
        return;     // Pas de sortie résultat pour les véhicules accumulés à l'entrée
	}

    // Ecriture du document XML		    
    std::string ssTuyau;
    // Intégration EVE
    std::string ssTuyauEx;
    std::string ssNextTuyauEx;


    if(GetVoie(0) )
    {
        CalculXYZ(dbAbs, dbOrd, dbHaut);

        if( m_pReseau->m_pCoordTransf )
            m_pReseau->m_pCoordTransf->Transform(1, &dbAbs, &dbOrd);

        Voie* pVoie0 = m_pVoie[0];

        double dbPos0 = std::max<double>(m_pPos[0], 0); // comment on peut avoir des positions négatives ici (scenario grand lyon SG): insertion à l'aide d'un SymCreateVehicle au niveau d'une connexion interne et le véhicule ne peut pas s'insérer à cause de la présence d'autres véhicules

        // Si existence d'un ghost pour ce véhicule, la sortie est celle du ghost (sauf pour les coordonnées absolues X, Y et Z)
        if( m_pReseau->IsChgtVoieGhost() )
        {
            if( m_nGhostRemain > 0 && m_pGhostVoie)
            {
                pVoie0 = m_pGhostVoie;	// Voie du ghost
                dbPos0 = dbPos0 * m_pGhostVoie->GetLength() / m_pVoie[0]->GetLength();	// Position du ghost
            }
        }

        if(IsOutside() && GetInstInsertion()==-1)
            return;

        nNumVoie = pVoie0->GetNum()+1;


        sTypeVehicule = GetType()->GetLabel();
        sLib = GetFleet()->GetVehicleIdentifier(shared_from_this());
        if(GetFleet()->OutputVehicleLoad(this))
        {
            nCurrentLoad = GetFleetParameters()->GetUsageParameters().GetLoadUsageParameters().GetCurrentLoad();
        }

        // Document et Trace XML
        ssTuyau = pVoie0->GetParent()->GetLabel();		

        // Intégration EVE
        if( m_pReseau->GetSymMode()==Reseau::SYM_MODE_STEP_EVE )
        {
            ssTuyauEx = m_pReseau->GetLibelleTronconEve( m_pTuyau[0], nNumVoie - 1 );	// Tronçon au sens EVE	

            // cas d'un mobile dans une brique de connexion : on indique le label du tronçon de sortie de brique
            if( m_pTuyau[0]->GetBriqueParente() != NULL
                && (m_pTuyau[0]->GetBriqueParente()->GetType() == 'C' || m_pTuyau[0]->GetBriqueParente()->GetType() == 'G'))
            {
                if(m_pNextTuyau)
                {
                    ssNextTuyauEx = m_pNextTuyau->GetLabel();
                }
            }
        }

        // Longueur du mobile
        dbLongueur = GetLength();

        // Vitesse libre du véhicule
        dbVitMax = GetDiagFonda()->GetVitesseLibre();

        int nIDVehLeader = -1;
        if(GetLeader())
        {
            nIDVehLeader = GetLeader()->GetID();
        }

        std::string strDriveState = "";
        if(m_bDriven)
        {
            if(m_bForcedDriven)
            {
                if(m_bDriveSuccess)
                {
                    strDriveState = "force (ecoulement respecte)";
                }
                else
                {
                    strDriveState = "force (ecoulement non respecte)";
                }
            }
            else
            {
                if(m_bDriveSuccess)
                {
                    strDriveState = "pilote (cible atteinte)";
                }
                else
                {
                    strDriveState = "pilote (cible non atteinte)";
                }
            }
        }

        std::map<std::string, std::string> additionalAttributes;
        if((IsAtTripNode() || IsOutside() || m_bAttenteInsertion) && m_pLastTripNode)
        {
            additionalAttributes = GetFleet()->GetOutputAttributes(this, m_pLastTripNode);
        }

        if(!GetCurrentTuyauMeso() || GetCurrentTuyauMeso()->GetType() != Tuyau::TT_MESO)
        {
            Tuyau * pCurrentLink = (Tuyau*)pVoie0->GetParent();
            // Cas particulier des véhicules micro dans les CAFs qui sont sur un mouvement reliant deux tuyaux meso : on ne sort pas les trajectoires correspondantes.
            bool bShowVehicle = true;
            if (pCurrentLink->GetBriqueParente())
            {
                int iTuyIdx = this->GetItineraireIndex(isBriqueAval, pCurrentLink->GetBriqueParente());
                if (iTuyIdx != -1)
                {
                    if (GetItineraire()->at(iTuyIdx)->GetType() == Tuyau::TT_MESO)
                    {
                        if (iTuyIdx + 1 < (int)this->GetItineraire()->size())
                        {
                            bShowVehicle = GetItineraire()->at(iTuyIdx+1)->GetType() != Tuyau::TT_MESO;
                        }
                        else
                        {
                            bShowVehicle = false;
                        }
                    }
                }
            }
            if (bShowVehicle)
            {
                NewellContext * pNewellContext = dynamic_cast<NewellContext*>(GetCarFollowing()->GetCurrentContext());
                pXMLDocTrafic->AddTrajectoire(m_nID, pCurrentLink, ssTuyau, ssTuyauEx, ssNextTuyauEx, nNumVoie, dbAbs, dbOrd, dbHaut, dbPos0, m_pVit[0], m_pAcc[0], pNewellContext ? pNewellContext->GetDeltaN() : -1, sTypeVehicule, dbVitMax, dbLongueur, sLib, nIDVehLeader, nCurrentLoad, m_bTypeChgtVoie && bChgtVoieDebug, m_TypeChgtVoie, m_bVoieCible && bChgtVoieDebug, m_nVoieCible,
                    m_bPi && bChgtVoieDebug, m_dbPi, m_bPhi && bChgtVoieDebug, m_dbPhi, m_bRand && bChgtVoieDebug, m_dbRand, m_bDriven, strDriveState, m_VehiculeDepasse.get() != NULL, m_bRegimeFluideLeader, additionalAttributes);
            }
        }
        else
        {
            pXMLDocTrafic->AddStream(m_nID, ssTuyau, ssTuyauEx);	
        }
    }
    else
    { 
        //  le vehicule n'a pas de voie micro 
        if(GetCurrentTuyauMeso() && GetCurrentTuyauMeso()->GetType() == Tuyau::TT_MESO)
        {
            ssTuyauEx = GetCurrentTuyauMeso()->GetLabel();
            ssTuyau =  ssTuyauEx ;
            pXMLDocTrafic->AddStream(m_nID, ssTuyau, ssTuyauEx);	
        }
    }
}

/// <summary>
/// Procédure de calcul de la vitesse de la voie adjacente
/// </summary>
//================================================================
double Vehicule::CalculVitesseVoieAdjacente
//----------------------------------------------------------------
// Fonction  :  Calcule de la vitesse de la voie adjacente
// Remarque  :	Si voie inatteignable, la vitesse retorunée est 0.0
// Version du:  28/06/2010
// Historique:  28/06/2010 (C.Bécarie)
//              Création
//================================================================
(
    int     nVoieCible,
    double	dbInst
)
{
    VoieMicro* pVoieCible = NULL;

    if(!m_pTuyau[1])
        return 0.0;	

    if(!m_pVoie[1])
        return 0.0;

    if( nVoieCible < 0 || nVoieCible >= m_pTuyau[1]->getNbVoiesDis() )
        return 0.0;

    pVoieCible = (VoieMicro*)m_pTuyau[1]->GetLstLanes()[nVoieCible];
    if(!pVoieCible)
        return 0.0;

    double dbVitVoieCible = GetCarFollowing()->ComputeLaneSpeed(pVoieCible);

    return std::min<double>(dbVitVoieCible, ((Tuyau*)pVoieCible->GetParent())->GetVitRegByTypeVeh(this->m_pTypeVehicule, dbInst, GetPos(1), pVoieCible->GetNum()));
}

/// <summary>
/// Procédure d'initialisation des variables utilisées pour le changement de voie
/// </summary>
//================================================================
void Vehicule::InitChgtVoie
//----------------------------------------------------------------
// Fonction  :  Initialise les variables utilisées pour le 
//              changement de voie.
// Remarque  :
// Version du:  21/06/2011
// Historique:  21/06/2011 (O. Tonck - IPSIS)
//              Passage du .h au .cpp + ajout d'autres vairables
//              18/08/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
)
{
    m_bChgtVoie=false;
    m_bTypeChgtVoie=false;
    m_bPi=false;
    m_bPhi=false;
    m_bRand=false;
    m_bVoieCible=false;
    m_TypeChgtVoie = eConfort;
    m_nVoieCible = 0;
    m_dbPhi = 0;
    m_dbPi = 0;
    m_dbRand = 0;
}

/// <summary>
/// Procédure de calcul de changement de voie
/// </summary>
//================================================================
bool Vehicule::CalculChangementVoie
//----------------------------------------------------------------
// Fonction  :  Calcule si il y a changement de voie ou non du
//              véhicule considéré de la voie initiale vers la
//              voie passée en paramètre
// Remarque  :
// Version du:  21/06/2011
// Historique:  21/06/2011 (O.Tonck - IPSIS)
//              Ajout de variables de sortie
//              18/08/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    int     nVoieCible,					// Voie cible de changement de voie
    double  dbInstant,					// Instant du changement de voie
    SensorsManagers* pGestionsCapteurs, // Capteurs pour détection changement de voie
    bool	bForce,	/*false*/			// Indique si il s'agit d'un changement de voie forcé
    bool	bRabattement /*false*/		// Indique si il s'agit d'un rabattement
)
{
    
    boost::shared_ptr<Vehicule>   pVehFollower;
    boost::shared_ptr<Vehicule>   pVehLeader;
    boost::shared_ptr<Vehicule>   pVehLeaderOrig;
    VoieMicro*  pVoieCible;

    // Précaution
    if(!m_pVoie)
        return false;

    if(!m_pReseau)
        return false;

    // Pas de changement de voie pour un bus à l'arrêt
    if(IsAtTripNode())
        return false;

    if( m_pTuyau[1]->GetType() == Tuyau::TT_MICRO && !((TuyauMicro*)m_pTuyau[1])->IsChtVoieVersDroiteAutorise() && nVoieCible == 0 		// En dur pour Hélène
        && (!(m_bOutside || IsAtTripNode()) || !m_pVoie[1])) // Correction bug n°19 : l'insertion d'un bus depuis un arrêt n'est pas un changement de voie vers la droite !
        return false;

    // CP du véhicule qui a été créé au cours du pas de temps : on lui interdit de changer de voie      
	// CP du véhicule qui n'est pas encore sur le réseau    
    if(m_pPos[1] <= UNDEF_POSITION)
        return false;
        

    // Si un changement de voie a déjà été calculé de façon positive,
    // on ne recommence pas le calcul avec cette voie cible
    if(m_bChgtVoie)
        return false;

    // Si le véhicule est leader de sa voie, il ne change pas de voie (sauf si voie avec caractéristique
    // changement de voie obligatoire)
    if(m_pVoie[1] && !bRabattement)
    {
        boost::shared_ptr<Vehicule> pV = m_pReseau->GetFirstVehicule(m_pVoie[1]);				
        if(this==pV.get() && !m_pVoie[1]->IsChgtVoieObligatoire(GetType()) && !IsVoieDesiree() )
            return false;
    }

    // Si le véhicule est sur un tronçon interne d'un giratoire et si son changement de voie n'est pas motivée 
    // par le désir de sortir, on lui interdit de changer de voie
    if(m_pTuyau[1] && m_pTuyau[1]->GetBriqueParente())
        if(m_pTuyau[1]->GetBriqueParente()->GetType()=='G' && !IsVoieDesiree() )
            return false;

    // Si changement de voie pour rabattement et vitesse nulle au pas de temps précédent, pas de changement
    if(bRabattement && m_pVit[1] <= 0.0001)
        return false;

    pVoieCible = (VoieMicro*)m_pTuyau[1]->GetLstLanes()[nVoieCible];

    if(!pVoieCible)
        return false;

    // Gestion des descripteurs de possibilités de changement de voie vers la voie cible (utile aux régulations de type BLIP par exemple)
    if(pVoieCible->IsTargetLaneForbidden(m_SousType))
    {
        return false;
    }

    if(pVoieCible->IsPullBackInInFrontOfGuidedVehicleForbidden(m_SousType))
    {
        boost::shared_ptr<Vehicule> pNearestAmontVehicule = m_pReseau->GetNearAmontVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001 );

        if(pNearestAmontVehicule && pNearestAmontVehicule->GetFleet() == m_pReseau->GetPublicTransportFleet()) return false;
    }

    // interdiction du changement de voie en cas de franchissement d'un terre-plein
    if(m_pTuyau[1] && m_pVoie[1] && m_pTuyau[1]->HasTerrePlein(dbInstant, m_pPos[1], nVoieCible, m_pVoie[1]->GetNum()))
        return false;

    // les tests préliminaires sont passés, on peut commencer à noter les sorties spécifiques au changement de voie

    // réinit des flags de présence des valeurs pour ne pas être pollué par des valeurs
    // calculées précédemment par un CalculChangementVoie qui a renvoyé false mais calculé des variables. Ainsi, on ne 
    // sortira que les valeurs calculées par le dernier appel à CalculChangementVoie
    InitChgtVoie();

    m_bTypeChgtVoie = true;
    if(bForce)
    {
        m_TypeChgtVoie = eForce;
    }
    else if(bRabattement)
    {
        m_TypeChgtVoie = eRabattement;
    }
    else
    {
        m_TypeChgtVoie = eConfort;
    }
    m_bVoieCible = true;
    m_nVoieCible = nVoieCible+1;

    // -----------------------------------------------
    // Calcul de la probabilité de changement de voie
    // -----------------------------------------------

    // Appel de la procédure de test si opportunité de traverser la ou les voies interdites le cas échéant
    if(m_pReseau->GetSimulationTraversees() && !TraverseeVoiesInterdites(dbInstant, m_pVoie[1], pVoieCible))
    {
        // Pas possible de traverser : on ne traverse pas.
        return false;
    }

	// Si 2 véhicules à la même position, il ne change pas de voie
	boost::shared_ptr<Vehicule>   pVehSamePosition;
	bool bMoveBack = false;
	pVehSamePosition = m_pReseau->GetNearAvalVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) - 0.01, this);
	if (pVehSamePosition)
		if (fabs(pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) - pVehSamePosition->GetPos(1)) < 0.01)
			if (fabs(this->GetPos(1) - m_pVoie[1]->GetLength()) < 0.01)		// en bout de tronçon
			{
				if(bForce)
					if(pVehSamePosition->GetVoieDesiree() == m_pVoie[1]->GetNum() && m_nVoieDesiree == pVehSamePosition->GetVoie(1)->GetNum())
						if (m_pVit[1] < 0.01 && pVehSamePosition->GetVit(1) < 0.01)
						{
							// Switch-switch
							VoieMicro *pVoieCibleVehSamePosition = m_pVoie[1];
							ChangementVoie(dbInstant, pVoieCible, pGestionsCapteurs, NULL, NULL, NULL);
							pVehSamePosition->ChangementVoie(dbInstant, pVoieCibleVehSamePosition, pGestionsCapteurs, NULL, NULL, NULL);

							return true;
						}

				m_pPos[1] = m_pPos[1] - 0.5;
				bMoveBack = true;

			}
			else
				return false;

    // Recherche des véhicules 'follower' et 'leader' de la voie cible
    pVehFollower = m_pReseau->GetNearAmontVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001, -1.0, this);
    pVehLeader = m_pReseau->GetNearAvalVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001, this);

    if(pVehFollower)								// Si le follower sur la voie cible est ghosté, pas de changement de voie
        if(pVehFollower->GetGhostVoie() ==  pVoieCible)
            return false;

    // Cas particulier si rabattement et 2 véhicules à la même position et même vitesse
    //if( bRabattement && pVehFollower)
    //	if( fabs(pVehFollower->GetPos(1) *pVoieCible->GetLength() / m_pVoie[1]->GetLength() - GetPos(1) ) < 0.1 )
    //		return false;

    // Si pas de leader et si on se trouve sur une voie où le changement est obligatoire, on
    // cherche un leader sur la voie en aval de la voie cible
    if( !pVehLeader && m_pVoie[1] && (m_pVoie[1]->IsChgtVoieObligatoire(GetType()) || IsVoieDesiree() ) )
    {
        bool bContinueRecherche = true;             // Si convergent et tuyau non prioritaire, on ne s'intéresse pas au leader sur les tuyaux en aval
        if( m_pTuyau[1]->get_Type_aval() == 'C')
        {
            Convergent *pCvg = (Convergent *)m_pTuyau[1]->getConnectionAval();
            if( pCvg->IsVoieAmontNonPrioritaire( m_pVoie[1] ) )
                bContinueRecherche = false;
        }

        if(bContinueRecherche)
        {
            Tuyau *pDest = NULL;
            Voie* pNextVoie = CalculNextVoie(pVoieCible, dbInstant);
            if(pNextVoie)
                pVehLeader = m_pReseau->GetNearAvalVehicule((VoieMicro*)pNextVoie, 0);
        }
    }

    // Recherche du véhicule précédent actuellement
    if(m_pVoie[1])
    {
        pVehLeaderOrig = m_pReseau->GetPrevVehicule(this);
        if(!pVehLeaderOrig)
            if(!GetVoie(1)->IsChgtVoieObligatoire(GetType()) && !IsVoieDesiree() && !bRabattement && !bForce)
                return false; // Le véhicule n'a pas de leader
    }

    if( m_pCarFollowing->TestLaneChange(dbInstant, pVehFollower, pVehLeader, pVehLeaderOrig, pVoieCible, bForce, bRabattement, false) )
        // Succès -> changement de voie
    {
        ChangementVoie(dbInstant, pVoieCible, pGestionsCapteurs, pVehLeader, pVehLeaderOrig, pVehFollower);

        return true;
    }

	if(bMoveBack)
		m_pPos[1] = m_pPos[1] + 0.5;

    return false;  // pas de changement de voie
}


//================================================================
bool Vehicule::CalculDepassement
//----------------------------------------------------------------
// Fonction  :  Positionne le véhicule en mode dépassement sur
//              tronçon opposé si les conditions s'y prettent :
//              1 - Calcul du delta de vitesse avec le leader, 
//              calcul du Pi et tirage aléatoire.
//              2 - Vérification de l'opportunité d'un dépassement
//              (tronçon opposé libre de circulation génante)
// Remarque  :  
// Version du:  16/10/2011 (O.Tonck - IPSIS)
// Historique:  Version Initiale
//              16/10/2011 (O.Tonck - IPSIS)
//================================================================
(
    double dbInstant
)
{
    // Cas du véhicule qui n'est pas (encore) en cours de dépassement
    if(!m_VehiculeDepasse)
    {
        // on ne passe sur le tronçon opposé que si le véhicule est situé sur la voie de gauche
        if(m_pVoie[1] && m_pVoie[1]->GetVoieGauche() == NULL)
        {
            // Recherche du leader
            boost::shared_ptr<Vehicule> pVehLeader = ChercheLeader(dbInstant, true);

            // Si leader, on regarde s'il est genant et si le véhicule décide de le dépasser.
            // On ne prend en compte le leader comme genant que si il est à sa vitesse libre.
            if(pVehLeader && fabs(pVehLeader->GetVit(1) - pVehLeader->GetType()->GetVx())< 0.0001)
            {
                // on utilise le delta des vitesses libre des deux véhicules
                double dbVitFollower = GetType()->GetVx();
                double dbVitLeader = pVehLeader->GetType()->GetVx();
                double dbPi = std::max<double>(0., dbVitFollower - dbVitLeader) / ( m_pTuyau[1]->GetMaxVitReg(dbInstant, m_pPos[1], m_pVoie[1]->GetNum()) * GetType()->GetTauDepassement());   

                if(dbPi > 0.000001) // Suppression erreurs d'arrondi
                {
                    double dbRand = m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
                    if(dbRand < dbPi)
                    {
                        // Le véhicule souhaite dépasser : il faut regarder si c'est possible (zone de dépassement autorisée + tronçon opposé libre)
                        if(DepassementAutorise(dbInstant, GetPos(1), pVehLeader)) // le véhicule est sur une portion de dépassement autorisé
                        {
                            m_VehiculeDepasse = pVehLeader;
                        }
                    }
                } // fin si pi positif
            } // fin si leader
        } // fin si véhicule sur la voie de gauche
    }

    return m_VehiculeDepasse.get()!=NULL;
}

//================================================================
bool Vehicule::DepassementAutorise
//----------------------------------------------------------------
// Fonction  :  Teste la zone de depassement autorisé
// Remarque  :  
// Version du:  17/10/2011 (O.Tonck - IPSIS)
// Historique:  Version Initiale
//              17/10/2011 (O.Tonck - IPSIS)
//================================================================
(
    double dbInstant,
    double dbPos,
    boost::shared_ptr<Vehicule> pVehLeader // véhicule à dépasser
)
{
    bool result = true;

    // test de la zone de dépassement autorisée
    if(GetLink(1)->IsDepassementInterdit(GetPos(1)))
    {
        result = false;
    }
    else
    {
        // vérification du fait que le tronçon opposé est libre.
        // On approxime en supposant les vitesses des véhicules constantes en cours de dépassement

        // la distance de dépassement vaut l'interdistance + la distance de sécurité pour le rabattement
        double dbDistanceDepassement = pVehLeader->GetPos(1) - GetPos(1) + GetCarFollowing()->GetMaxInfluenceRange(); 

        // Calcul du delta de vitesse pour estimation du temps pris pour parcourir la distance de dépassement
        double dbVitDepasse = pVehLeader->GetType()->GetVx();
        double dbVitDepassant = GetType()->GetVx();
        double dbDeltaVit = dbVitDepassant - dbVitDepasse;

        assert(dbDeltaVit > 0);

        double dbTempsDepassement = dbDistanceDepassement/dbDeltaVit;

        double dbDistanceParcourueEnDepassement = dbTempsDepassement*dbVitDepassant;

        // Si le dépassement ne se termine pas sur le tronçon courant : on ne dépasse pas
        if(GetPos(1) + dbDistanceParcourueEnDepassement > GetLink(1)->GetLength())
        {
            result = false;
        }
        else
        {
            // Finalement, on peut dépasser si aucun véhicule opposé ne se trouve en amont de la position courante du véhicule dépassant
            // jusqu'à une distance définie ainsi :
            double dbDistanceRecherche = dbDistanceParcourueEnDepassement + dbTempsDepassement*GetLink(1)->GetVitesseMax();

            Tuyau * pTuyauOppose = GetLink(1)->GetTuyauOppose();
            // Si le tronçon débouche sur une entrée dans la zone de recherche, on ne dépasse pas en anticipant l'arrivée potentielle d'un véhicule.
            // remarque : pour être plus correct, il faudrait aller voir plus loin que la fin du troncon courant et déetcter toute entrée
            // potentiellement genante pour le dépassement !
            if(pTuyauOppose->get_Type_amont() == 'E'
                && (pTuyauOppose->GetLength()-(GetPos(1)+dbDistanceRecherche) < 0))
            {
                result = false;
            }
            else
            {
                if(m_pReseau->GetNearAmontVehiculeEx(pTuyauOppose, pTuyauOppose->GetLength() - GetPos(1), pTuyauOppose, dbDistanceRecherche, pTuyauOppose->getNbVoiesDis()-1))
                {
                    // Si véhicule opposé genant, on ne dépasse pas.
                    result = false;
                }
            }
        }
    }

    return result;
}


//================================================================
bool Vehicule::TraverseeVoiesInterdites
//----------------------------------------------------------------
// Fonction  :  Teste la possibilité de traverser des voies
// Remarque  :  
// Version du:  10/10/2011 (O.Tonck - IPSIS)
// Historique:  Version Initiale
//              10/10/2011 (O.Tonck - IPSIS)
//================================================================
(
    double dbInstant,
    VoieMicro * pVoieOrigine,
    VoieMicro * pVoieCible
)
{
    bool result = true;
    double tempsTraversee = 0;
    double dbVitLat = GetType()->GetVitesseLaterale();
    Tuyau * pTuyau = (Tuyau*)pVoieOrigine->GetParent(); 

    // uniquement si les deux voies ne sont pas adjacentes
    if(abs(pVoieOrigine->GetNum() - pVoieCible->GetNum()) >= 2)
    {
        // Création du groupe de traversée correspondant à l'ensemble des voies entre l'ogirine et la destionation
        GrpPtsConflitTraversee *pGrpTra = new GrpPtsConflitTraversee();
        pGrpTra->dbDstPtAttente = m_pPos[1];
        pGrpTra->dbPosPtAttente = m_pPos[1];
        pGrpTra->pVPtAttente = pVoieOrigine;
        pGrpTra->nNbMaxVehAttente = 0;
        pGrpTra->dbInstLastTraversee = UNDEF_INSTANT;
        int nSens = pVoieOrigine->GetNum()<pVoieCible->GetNum()?1:-1;

        for(int iVoieInt = pVoieOrigine->GetNum()+nSens; iVoieInt != pVoieCible->GetNum(); iVoieInt = iVoieInt + nSens)
        {
            boost::shared_ptr<PtConflitTraversee> pPtTra = boost::make_shared<PtConflitTraversee>();

            pPtTra->pT1 = pVoieOrigine;
            pPtTra->pT2 = (VoieMicro*)((Tuyau*)pVoieOrigine->GetParent())->GetLstLanes()[iVoieInt];

            pPtTra->pTProp = pPtTra->pT2;

            pPtTra->dbPos1 = m_pPos[1];
            pPtTra->dbPos2 = m_pPos[1];
            pPtTra->dbDst1 = m_pPos[1];
            pPtTra->dbDst2 = m_pPos[1];

            pGrpTra->lstPtsConflitTraversee.push_back(pPtTra);

            tempsTraversee += pTuyau->getLargeurVoie(iVoieInt)/dbVitLat;
        }

        // Boucle sur les points de conflit du groupe
        for( std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itTra = pGrpTra->lstPtsConflitTraversee.begin(); result && itTra != pGrpTra->lstPtsConflitTraversee.end(); itTra++)
        {
            std::vector<int> placeHolder;
            result = result && m_pReseau->CalculTraversee( this, placeHolder, (*itTra).get(), pGrpTra, 0, dbInstant, true, tempsTraversee);
        }

        // suppression des objets aloués
        delete pGrpTra;
    }

    return result;
}

//================================================================
void Vehicule::ChangementVoie
//----------------------------------------------------------------
// Fonction  :  Efectue le changement de voie
// Remarque  :  
// Version du:  04/10/2011 (O.Tonck - IPSIS)
// Historique:  Version Initiale
//              04/10/2011 (O.Tonck - IPSIS)
//================================================================
(
    double                      dbInstant,         // Instant
    VoieMicro*                  pVoieCible,        // Voie cible
    SensorsManagers*            pGestionsCapteurs, // capteurs
    boost::shared_ptr<Vehicule> pVehLeader,        // Vehicule leader
    boost::shared_ptr<Vehicule> pVehLeaderOrig,    // Vehicule leader sur la voie d'origine
    boost::shared_ptr<Vehicule> pVehFollower       // Vehicule suivant
)
{
    m_bChgtVoie = true;

    VoieMicro * pVoieOld = m_pVoie[1];
    SetVoieAnt(pVoieCible);

    // La position sur la voie cible est recalculée pour prendre en compte la différence possible
    // de longueur entre les 2 voies (au prorata)
    m_pPos[1] = m_pPos[1] * pVoieCible->GetLength() / pVoieOld->GetLength();
    // correction bug n°31 : on empêche le véhicule de se rabattre derrière son follower 
    // sur la voie cible... (peut arriver à cause de l'arrondi pVoieCible->GetLength() / pVoieOld->GetLength())
    if(pVehFollower && pVehFollower->GetLink(1) == m_pTuyau[1])
    {
        m_pPos[1] = std::max<double>(m_pPos[1], pVehFollower->GetPos(1));
    }

    // MAJ données capteurs sur les changements de voie
    // refiler la voie old, cible, et la position du changement
    if(pGestionsCapteurs)
    {
        pGestionsCapteurs->GetGestionCapteursLongitudinaux()->AddChgtVoie(dbInstant, m_pTuyau[1], m_pPos[1], pVoieOld->GetNum(), pVoieCible->GetNum());
    }

    m_nVoieDesiree = -1;    // Permet d'empêcher un autre changement de voie si le véhicule est postionnée
    // sur la bonne voie pour aller sur le tronçon suivant

    Tuyau *pDest = NULL;
    m_pNextVoie = (VoieMicro*) CalculNextVoie( m_pVoie[1], dbInstant );  // Il faut recalculer la voie suivante

    // Application du changement de voie pour la loi de poursuite
    m_pCarFollowing->ApplyLaneChange(pVoieOld, pVoieCible, pVehLeader, pVehFollower, pVehLeaderOrig);

    if(m_bOutside)
    {
        m_dbInstInsertion = dbInstant;  // Trace de l'insertion du bus 
        m_bOutside = false;
    }
}

//================================================================
void Vehicule::ChangementVoieExt
//----------------------------------------------------------------
// Fonction  :  Efectue le changement de voie en calculant les
//              leaders et followers nécessaires
// Remarque  :  
// Version du:  04/10/2011 (O.Tonck - IPSIS)
// Historique:  Version Initiale
//              04/10/2011 (O.Tonck - IPSIS)
//================================================================
(
    double                      dbInstant,         // Instant
    VoieMicro*                  pVoieCible,        // Voie cible
    SensorsManagers*            pGestionsCapteurs  // capteurs
)
{
    // Recherche des véhicules 'follower' et 'leader' de la voie cible
    boost::shared_ptr<Vehicule> pVehFollower = m_pReseau->GetNearAmontVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001, -1.0, this);
    boost::shared_ptr<Vehicule> pVehLeader = m_pReseau->GetNearAvalVehicule(pVoieCible, pVoieCible->GetLength() / m_pVoie[1]->GetLength() * GetPos(1) + 0.001, this);

    // Si pas de leader et si on se trouve sur une voie où le changement est obligatoire, on
    // cherche un leader sur la voie en aval de la voie cible
    if( !pVehLeader && m_pVoie[1] && (m_pVoie[1]->IsChgtVoieObligatoire(GetType()) || IsVoieDesiree() ) )
    {
        bool bContinueRecherche = true;             // Si convergent et tuyau non prioritaire, on ne s'intéresse pas au leader sur les tuyaux en aval
        if( m_pTuyau[1]->get_Type_aval() == 'C')
        {
            Convergent *pCvg = (Convergent *)m_pTuyau[1]->getConnectionAval();
            if( pCvg->IsVoieAmontNonPrioritaire( m_pVoie[1] ) )
                bContinueRecherche = false;
        }

        if(bContinueRecherche)
        {
            Tuyau *pDest = NULL;
            Voie* pNextVoie = CalculNextVoie(pVoieCible, dbInstant);
            if(pNextVoie)
                pVehLeader = m_pReseau->GetNearAvalVehicule((VoieMicro*)pNextVoie, 0);
        }
    }

    // Recherche du véhicule précédent actuellement
    boost::shared_ptr<Vehicule> pVehLeaderOrig;
    if(m_pVoie[1])
    {
        pVehLeaderOrig = m_pReseau->GetPrevVehicule(this);
    }

    ChangementVoie(dbInstant, pVoieCible, pGestionsCapteurs, pVehLeader, pVehLeaderOrig, pVehFollower);
}

//================================================================
Tuyau* Vehicule::CalculNextTuyau
    //----------------------------------------------------------------
    // Fonction  :  Calcule le tuyau suivant (après la connexion aval)
    //              du véhicule
    // Remarque  :  
    // Version du:  
    // Historique:  
    //================================================================
    (
    Tuyau*           pTuyau,                   // Voie initiale
    double                dbInstant                 // Instant simulé            
    )
{
    Tuyau* pNextTuyau = NULL;
    if(m_nextTuyaux.find(std::pair<double, Tuyau*>( dbInstant, pTuyau ))  !=m_nextTuyaux.end())
    {
        return m_nextTuyaux[std::pair<double, Tuyau*>( dbInstant, pTuyau )] ;
    }

    // le tuyau suivant ne change pas si on se trouve sur un tuyau interne à un CAF ou un giratoire
    if( m_pNextTuyau && pTuyau )
        if( pTuyau->GetBriqueParente())
            if( pTuyau->GetBriqueParente()->GetType() == 'C' || pTuyau->GetBriqueParente()->GetType() == 'G')
            {
                return m_pNextTuyau;
            }

    // -- Le comportement du flux est de type ITINERAIRE ou c'est un véhicule avec un itinéraire prédéfini --   
    if( m_pReseau->GetAffectationModule() || GetFleet() != m_pReseau->GetSymuViaFleet() )
    {
        if ((m_pReseau->GetAffectationModule() && m_pReseau->GetAffectationModule()->GetLieuAffectation() == 'E') || GetTrip()->IsPathDefined())
        {
            // prise en compte le cas où le tuyau est un tuyau interne (peut arriver lors de la création d'un véhicule en fin de trouçon qui arrive sur une brique
            // dès le premier pas de temps... classiquement le nexttroncon à renvoyer est le troncon m_itineraire[1]...
            if(pTuyau->GetBriqueParente())
            {
                // on cherche le tuyau de l'itinéraire qui précéde la brique
                int lastTuyIdx = GetItineraireIndex(isTuyauAmont, pTuyau->GetBriqueParente());
                if(lastTuyIdx != -1)
                {
                    pTuyau = (TuyauMicro*)m_pTrip->GetFullPath()->at(lastTuyIdx);
                }
            }
            // le tronçon suivant est stocké dans l'itinéraire...
            int i = GetItineraireIndex(isSameTuyau, pTuyau);
            if(i != -1)
            {
                while( i < (int)m_pTrip->GetFullPath()->size()-1)
                {
                    if ( !m_pTrip->GetFullPath()->at(i+1)->GetBriqueParente() )           // Exclusion des tronçons internes des giratoires et des carrefours
                    {
                        pNextTuyau = m_pTrip->GetFullPath()->at(i+1);

                        m_nextTuyaux[std::pair<double, Tuyau*>( dbInstant, pTuyau )] = pNextTuyau;
                        return pNextTuyau;
                    }
                    i++;
                }

                return NULL;
            }
        }
    }

    // -- Le comportement du flux est de type DESTINATION --
    double dbRand;
    if( m_pReseau->IsCptDestination() )	
    {
        // Recherche des élements correspondants sur réseau d'affectation
        Connexion* pCnx = pTuyau->GetCnxAssAv();

        std::map<Tuyau*, double, LessPtr<Tuyau>> mapCoeffs;
        std::map<Tuyau*, double, LessPtr<Tuyau>>::iterator itCoeffs;		

        if( pCnx->GetNbElAval() == 1)
        {
            m_nextTuyaux[std::pair<double, Tuyau*>( dbInstant, pTuyau )] = pCnx->m_LstTuyAssAv[0];

            return pCnx->m_LstTuyAssAv[0];
        }

        //*(Reseau::m_pFicSimulation) << pTuyau->GetLabel() << " Destination " << m_pDestination->GetID() << std::endl;
        if( pCnx->GetCoeffsAffectation( dbInstant, this->GetType(), pTuyau, (SymuViaTripNode*)this->GetDestination(), mapCoeffs) )
        {
            // Tirage aléatoire pour affecter la sortie			
            dbRand = m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
            //*(Reseau::m_pFicSimulation) << "rand CalculNextTuyau " << dbRand << std::endl;

            // Détermination de la sortie éligible			        
            itCoeffs = mapCoeffs.begin();
            double dbSum = mapCoeffs.begin()->second;
            //*(Reseau::m_pFicSimulation) << dbSum << "   " << ((TuyauMicro*)itCoeffs->first)->GetLabel() << "-" ;

            while( dbRand > dbSum && itCoeffs != mapCoeffs.end() )			
            {
                itCoeffs++;
                dbSum += itCoeffs->second;		
                //*(Reseau::m_pFicSimulation) << dbSum << "   " << ((TuyauMicro*)itCoeffs->first)->GetLabel() << "-" ;
            }
            //*(Reseau::m_pFicSimulation) << " --> " << ((TuyauMicro*)itCoeffs->first)->GetLabel() << std::endl;
            m_nextTuyaux[std::pair<double, Tuyau*>( dbInstant, pTuyau )] = (TuyauMicro*)(itCoeffs->first);

            return (itCoeffs->first);
        }
    }

    // -- Le comportement du flux est de type DIRECTIONNEL --
    if (m_pReseau->IsCptDirectionnel())
    {
        // prise en compte le cas où le tuyau est un tuyau interne (peut arriver lors de la création d'un véhicule en fin de trouçon qui arrive sur une brique
        // dès le premier pas de temps... classiquement le nexttroncon à renvoyer est le troncon m_itineraire[1]...
        Tuyau * pTuyauInterne = NULL;
        if (pTuyau->GetBriqueParente())
        {
            pTuyauInterne = pTuyau;

            // on cherche le tuyau de l'itinéraire qui précéde la brique
            int lastTuyIdx = GetItineraireIndex(isTuyauAmont, pTuyau->GetBriqueParente());

            assert(lastTuyIdx != -1); // de toute façon, ca va planter à pTuyau->GetCnxAssAv(); si pTuyau reste un tuyau interne

            pTuyau = (TuyauMicro*)m_pTrip->GetFullPath()->at(lastTuyIdx);
        }

        // Recherche des élements correspondants sur réseau d'affectation
        Connexion* pCnx = pTuyau->GetCnxAssAv();
        int nTAmont = pCnx->GetNoEltAmont(pTuyau);

        // Construction de la liste des répartitions en réaggrégant par tuyau amont (on somme les coefficients
        // vers chaque voie avale pour chaque voie amont et on renormalise pour pouvoir faire un tirage indépendant de la voie amont qu'on ne connait pas encore ici)
        std::deque<double>   dqRep;
        std::deque<Voie*>    dqVoie;
        double dbSum = 0;
        for (int i = 0; i<pCnx->GetNbElAval(); i++)
        {
            Tuyau * pTAval = pCnx->m_LstTuyAv[i];

            for (int j = 0; j < pTAval->getNbVoiesDis(); j++)
            {
                // On somme le coefficient vers la voie aval pour chaque voie amont du tuyau
                double dbRep = 0;
                for (int k = 0; k < pTuyau->getNbVoiesDis(); k++)
                {
                    dbRep += pCnx->GetRepartition(m_pTypeVehicule, nTAmont, k, i, j, dbInstant);
                }
                if (dbRep > 0)
                {
                    // On ignore les tuyaux aval non accessible en fonction du tuyau interne sur lequel on se trouve, le cas échéant
                    bool bIsAccessible;
                    if (pTuyauInterne)
                    {
                        std::vector<Tuyau*> placeholder;
                        bIsAccessible = pTuyauInterne->GetBriqueParente()->GetTuyauxInternesRestants((TuyauMicro*)pTuyauInterne, pTAval, placeholder);
                    }
                    else
                    {
                        bIsAccessible = true;
                    }
                    if (bIsAccessible)
                    {
                        dqRep.push_back(dbRep);
                        dqVoie.push_back(pTAval->GetLstLanes()[j]);
                        dbSum += dbRep;
                    }
                }
            }
        }

        // Renormalisation de dqRep
        for (size_t i = 0; i < dqRep.size(); i++)
        {
            dqRep[i] /= dbSum;
        }

        // Tirage aléatoire entre 0 et 1
        dbRand = m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;

        // Détermination de la voie éligible
        dbSum = 0;

        for (unsigned int i = 0; i < dqRep.size(); i++)
        {
            dbSum += dqRep[i];

            // Précaution
            if (i == dqRep.size() - 1)
                dbSum = 1;

            if (dbRand <= dbSum)
            {
                Tuyau * pResult = (Tuyau*)(dqVoie[i])->GetParent();
                m_nextTuyaux[std::pair<double, Tuyau*>(dbInstant, pTuyau)] = pResult;
                return pResult;
            }
        }
    }

    return NULL;
}

//================================================================
int Vehicule::GetItineraireIndex(itineraryPredicate predicate, void * pArg)
// Fonction  :  Renvoie l'index du tuyau dans l'itinéraire
// qui respecte le prédicat passé en entrée
//================================================================
{
    return GetItineraireIndex(*GetItineraire(), predicate, pArg);
}

int Vehicule::GetItineraireIndex(const std::vector<Tuyau*> & itinerary, itineraryPredicate predicate, void * pArg)
{
    int result = -1;
    // pour prendre en compte les itinéraires qui passent plusieurs fois par
    // un même tronçon, on ne s'intéresse qu'à la partie de l'itinéraire non déjà parcourue, mais en 
    // prenant tout de même en compte le dernier tuyau non interne parcouru (utile pour les fonctions
    // qui doivent remonter un peu en arrière (recherche du tuyau amont au CAF sur lequel
    // se trouve le véhicule, par exemple.

    // on commence par passer les tronçons déjà parcourus, en s'arretant sur le dernier tuyau non interne parcouru
    int iStop = (int)m_TuyauxParcourus.size()-1;
    for(int i = (int)m_TuyauxParcourus.size()-1; i >= 0; i--)
    {
        if(!m_TuyauxParcourus[i]->GetBriqueParente())
        {
            iStop = i;
            break;
        }
    }
    int jStart = 0;
    size_t itiSize = itinerary.size();
    Tuyau * pLink;
    for(int i = 0; i <= iStop; i++)
    {
        pLink = m_TuyauxParcourus[i];
        for(size_t j = jStart; j < itiSize; j++)
        {
            if(itinerary[j] == pLink)
            {
                jStart = (int)j;
                break;
            }
        }
    }
    
    // Sur la suite de l'itinéraire, on cherche le tronçon voulu
    for(size_t j = jStart; j < itiSize; j++)
    {
        if( predicate(itinerary[j], pArg) )
        {
            result = (int)j;
            break;
        }
    }
    return result;
}

int Vehicule::GetItineraireIndexThreadSafe(itineraryPredicate predicate, void * pArg, std::vector<Tuyau*> & itinerary)
{
    itinerary = m_pTrip->GetFullPathThreadSafe();

    boost::lock_guard<boost::mutex> guard(m_Mutex);
    return GetItineraireIndex(itinerary, predicate, pArg);
}

//================================================================
Connexion* Vehicule::GetConnexionAval(Tuyau * pTuyau)
//----------------------------------------------------------------
// Fonction  :  Renvoie la connexion suivante du véhicule
//================================================================
{
    Connexion * pCnx = NULL;

    // Cas d'une connexion à l'entrée d'un CAF    
    if( pTuyau->GetBriqueAval() )
    {
        if( pTuyau->GetBriqueAval()->GetType() == 'C' )
        {            
            pCnx = (Connexion*)pTuyau->GetBriqueAval();        
        }
        else if( pTuyau->GetType() == Tuyau::TT_MESO)
        {
            pCnx = (Connexion*)pTuyau->GetBriqueAval();   
        }
    }   

    // Cas d'une connexion à l'intérieur d'un CAF    
    if(  pTuyau->GetBriqueParente() )
    {
        if( pTuyau->GetBriqueParente()->GetType() == 'C' )
        {            
            pCnx = (Connexion*)pTuyau->GetBriqueParente();      
        }
    }

    // Cas d'un convergent à l'entrée d'un giratoire à 2 voies
    if( (pTuyau->get_Type_aval() =='C' ) && pTuyau->GetBriqueAval() )
    {
        if( pTuyau->GetBriqueAval()->GetType() == 'G' )
        {
            pCnx = (Connexion*)pTuyau->GetBriqueAval();      
        }
    }
    return pCnx;
}

//================================================================
Voie* Vehicule::CalculNextVoie
//----------------------------------------------------------------
// Fonction  :  Calcule la voie suivante (après la connexion aval)
//              du véhicule
// Remarque  :  
// Version du:  28/09/2006
// Historique:  20/05/2009 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    Voie*           pVoie,                  // Voie initiale
    double          dbInstant              // Instant simulé
)
{
    Connexion       *pCnx = NULL;
    Tuyau           *pTAval;
    std::deque<double>   dqRep;
    std::deque<Voie*>    dqVoie;
    double          dbRep;
    int             nTAmont;
    int             nVAmont;
    double          dbRand;
    double          dbSum;
    Voie*           pNextVoie = NULL;
    Tuyau           *pTS;    
    Giratoire*      pGir = NULL;    
    Tuyau           *pDest;
    TuyauMicro *    pTInterne = NULL;
    CarrefourAFeuxEx *pCAF = NULL;	

    pNextVoie = NULL;

    if(!pVoie)
        return NULL;

    if( ((VoieMicro*)pVoie)->IsChgtVoieObligatoire(GetType()))     // Pas de voie suivante...
    {        
        return NULL;
    }

    // Voie
    nVAmont = pVoie->GetNum();
    // On vérifie que la voie actuelle se termine bien par un répartiteur micro
    TuyauMicro* pTuyau = (TuyauMicro*)pVoie->GetParent();

    // Cas des convergents avec une seule voie sur le tronçon aval
    if(pTuyau->get_Type_aval() =='C')
    {
        pCnx = pTuyau->getConnectionAval();
        nTAmont = pCnx->GetNoEltAmont(pTuyau);
        if(pCnx->GetNbElAval()==1)
            if( pCnx->m_LstTuyAv.front()->getNb_voies() == 1)
            {

                return pCnx->m_LstTuyAv.front()->GetLstLanes()[0];
            }
    }

    // Cas d'un répartiteur ou d'un divergent
    if(pTuyau->get_Type_aval() =='R' || pTuyau->get_Type_aval() =='D')
    {
        pCnx = pTuyau->getConnectionAval();       
        nTAmont = pCnx->GetNoEltAmont(pTuyau);                

        if( pCnx->GetNbElAval()==1 && pCnx->m_LstTuyAv.front()->getNbVoiesDis() == 1)       // un seul tronçon aval avec une seule voie
        {
            return pCnx->m_LstTuyAv.front()->GetLstLanes()[0];
        }
    }  

    // Cas d'une connexion à l'entrée d'un CAF    
    if( pTuyau->GetBriqueAval() )
    {
        if( pTuyau->GetBriqueAval()->GetType() == 'C' )
        {            
            pCAF = (CarrefourAFeuxEx*)pTuyau->GetBriqueAval();        
            pCnx = (Connexion*)pTuyau->GetBriqueAval();        
        }
    }   

    // Cas d'une connexion à l'intérieur d'un CAF    
    if(  pTuyau->GetBriqueParente() )
    {
        if( pTuyau->GetBriqueParente()->GetType() == 'C' )
        {            
            pCAF = (CarrefourAFeuxEx*)pTuyau->GetBriqueParente();   
            pCnx = (Connexion*)pTuyau->GetBriqueParente();      
        }
    }

    // Cas d'un convergent à l'entrée d'un giratoire à 2 voies
    if( (pTuyau->get_Type_aval() =='C' ) && pTuyau->GetBriqueAval() )
    {
        if( pTuyau->GetBriqueAval()->GetType() == 'G' )
        {
            pGir = (Giratoire*)pTuyau->GetBriqueAval();     
            pCnx = (Connexion*)pTuyau->GetBriqueAval();
            nTAmont = pCnx->GetNoEltAmont(pTuyau);
        }
    }

    // Cas d'un tronçon interne d'un CAF
    if( pTuyau->GetBriqueParente() )
    {
        if( pTuyau->GetBriqueParente()->GetType() == 'C' )
        {
            pCAF = (CarrefourAFeuxEx*)pTuyau->GetBriqueParente();

            pTInterne = pTuyau;

            // Recherche du tronçon amont du CAF
            int iTuyIdx = GetItineraireIndex(isBriqueAval, pCAF);
            if(iTuyIdx != -1)
            {
                pTuyau = (TuyauMicro*)GetItineraire()->at(iTuyIdx);
            }

            // Recherche de la voie amont du CAF d'ouù provient le véhicule
            nVAmont = pCAF->GetVoieAmont( pTInterne, pTuyau, GetType(), &m_SousType);

            // Si le mouvement autorisé associé a disparu... (update.xml par exemple)
            if(nVAmont == -1)
            {
                //return pCAF->GetNextVoieBackup(pTInterne, pTuyau, GetType(), &m_SousType);
                //nVAmont= 1;			// A REPRENDRE (cf. anomalie 78)
                nVAmont= 0; // OTK - Pourquoi 1 ? plantage si une seule voie... mettons plutôt 0. Cela dit, on ne devrait 
                            // moins passer ici suite aux modifs pour gérer les itinéraires qui comportent plusieurs fois le même tronçon
            }
        }
    }

    // Cas des tronçons internes d'un giratoire à 2 voies
    if( (pTuyau->get_Type_aval() =='C' || pTuyau->get_Type_aval() =='D') && pTuyau->GetBriqueParente() )
    {
        if( pTuyau->GetBriqueParente()->GetType() == 'G' )
        {
            pGir = (Giratoire*)pTuyau->GetBriqueParente();

            pTInterne = pTuyau;

            // récupération de la liste des tuyau internes formant l'itinéraire du véhicule dans le giratoire
            std::vector<Tuyau*> tuyauxInternes;
            if(m_pNextTuyau && pGir->IsTuyauAval(m_pNextTuyau) && pGir->GetTuyauxInternesRestants(pTInterne, m_pNextTuyau, tuyauxInternes))
            {
                // Gestion des différents cas :
                // - cas 1 : sortie du giratoire (troncon interne vers troncon externe)
                if(pTuyau == tuyauxInternes[tuyauxInternes.size()-1])
                {
                    // Plutot que de sortir en dur sur la voie de droite, on sort de préférence sur la voie de même indice que celle utilisée à l'intérieur du giratoire.
                    int nVSortie = nVAmont;
                    if(nVSortie >= m_pNextTuyau->getNbVoiesDis())
                    {
                        nVSortie = m_pNextTuyau->getNbVoiesDis()-1;
                    }
                    int nVSortiePrivilegiee = nVSortie;
                    // Anomalie n°35 : Si on peut, on évite se sortir sur une voie interdite
                    if(m_pNextTuyau->IsVoieInterdite(GetType(), nVSortiePrivilegiee, dbInstant))
                    {
                        // on prend la première voie à droite non interdite
                        nVSortie = 0;
                        while(nVSortie < m_pNextTuyau->getNbVoiesDis()-1 && m_pNextTuyau->IsVoieInterdite(GetType(), nVSortie, dbInstant))
                        {
                            nVSortie++;
                        }

                        // Si on n'a pas trouvé de voie non interdite, on reste sur la voie de sortie initialement prévue
                        if(m_pNextTuyau->IsVoieInterdite(GetType(), nVSortie, dbInstant))
                        {
                            nVSortie = nVSortiePrivilegiee;
                        }
                    }
                    return m_pNextTuyau->GetLstLanes()[nVSortie];
                }
                else
                    // - cas 2 : passage au tronçon interne suivant
                {
                    return tuyauxInternes[1]->GetLstLanes()[nVAmont];
                }
            }
        }
    }

    // -- Le comportement du flux est de type DESTINATION --
    if( m_pReseau->IsUsedODMatrix() && !m_pReseau->IsCptItineraire() )
    {        		
        std::map<int, boost::shared_ptr<MouvementsSortie> > mapCoeffs;

        bool bMvtAutorise = false;
        if( pGir )
        {
            bMvtAutorise = pGir->GetCoeffsMouvementsInsertion( this, pVoie, GetNextTuyau(), mapCoeffs );
        }
        else
        {
            bMvtAutorise = pCnx->GetCoeffsMouvementAutorise( pVoie, GetNextTuyau(), mapCoeffs, GetType(), &m_SousType );
        }

        if( bMvtAutorise )
        {
            // Tirage aléatoire entre 0 et 1
            dbRand = GetTirage(pVoie);

            // Détermination de la voie éligible
            dbSum = 0;
            for(int i=0; i<GetNextTuyau()->getNbVoiesDis(); i++)
            {
                if( mapCoeffs.find(i) != mapCoeffs.end() )
                    dbSum += mapCoeffs[i]->m_dbTaux;

                if( dbRand <= dbSum )
                {
                    return pNextVoie = (VoieMicro*)GetNextTuyau()->GetLstLanes()[i];							
                }
            }   
        }

    } // fin if( m_pReseau->IsUsedODMatrix() && !m_pReseau->IsCptItineraire() )

    // -- Le comportement du flux est de type ITINERAIRE ou DIRECTIONNEL --
    if ((m_pReseau->IsUsedODMatrix() && m_pReseau->IsCptItineraire()) || !m_pReseau->IsUsedODMatrix())
    {
        pTS = NULL;
        pDest = NULL;

        // -- Le comportement du flux est de type DIRECTIONNEL --
        if (!m_pReseau->IsUsedODMatrix())
        {
            if (!pCnx)
                return NULL;

            // Le tronçon suivant peut déjà être déterminé, sinon on faire un tirage:
            if (m_pNextTuyau && m_pNextTuyau->GetCnxAssAm() == pCnx)
            {
                pTS = m_pNextTuyau;
                pDest = pTS;
            }
            else
            {
                // Construction de la liste des répartitions
                for (int i = 0; i < pCnx->GetNbElAval(); i++)
                {
                    pTAval = pCnx->m_LstTuyAv[i];

                    for (int j = 0; j < pTAval->getNbVoiesDis(); j++)
                    {
                        dbRep = pCnx->GetRepartition(m_pTypeVehicule, nTAmont, nVAmont, i, j, dbInstant);
                        if (dbRep > 0)
                        {
                            dqRep.push_back(dbRep);
                            dqVoie.push_back(pTAval->GetLstLanes()[j]);
                        }
                    }
                }

                // Tirage aléatoire entre 0 et 1
                dbRand = GetTirage(pVoie);

                // Détermination de la voie éligible
                dbSum = 0;

                for (unsigned int i = 0; i < dqRep.size(); i++)
                {
                    dbSum += dqRep[i];

                    // Précaution
                    if (i == dqRep.size() - 1)
                        dbSum = 1;

                    if (dbRand <= dbSum)
                    {
                        pTS = (Tuyau*)(dqVoie[i])->GetParent();
                        pDest = pTS;
                        break;
                    }
                }
            }
        }
        else
        {
            // le tronçon suivant est stocké dans l'itinéraire...
            int i = GetItineraireIndex(isSameTuyau, pTuyau);
            if (i != -1)
            {
                if (i + 1 < (int)GetItineraire()->size())
                {
                    pTS = GetItineraire()->at(i + 1);
                    pDest = pTS;
                }
             /*   else
                {
                    if (pGir) {
                        m_pReseau->log() << Logger::Warning << std::endl << "Path of vehicle " << m_nID << " ends in a roundabout and not a destination !";
                        m_pReseau->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    }
                }*/
            }
        }


        // Divergent : pour l'instant, on traite une seule voie...
        if( !pCAF && pTS && pTuyau->get_Type_aval() == 'D' && pTuyau->getNb_voies() == 1 )
        {
            return (VoieMicro*)pTS->GetLstLanes()[0];        // Forcément sur la voie 1
        }

        // Convergent seul avec plusieurs voies en sortie
        if( !pGir && !pCAF && pTS && pTuyau->get_Type_aval() == 'C' )
        {
            Convergent *pCvg = (Convergent*) pTuyau->getConnectionAval();
            VoieMicro * pNextVoie = pCvg->GetNextVoie( (VoieMicro*)pVoie);
            return pNextVoie;           
        }

        // Répartiteur ou giratoire
        if(pTS)
        {
            // Calcul de la voie de sortie si besoin
            std::map<int, boost::shared_ptr<MouvementsSortie> > mapCoeffs;

            Voie* pVAm= pTuyau->GetLstLanes()[nVAmont];

            // cas du convergent d'insertion d'un giratoire à au moins 2 voies.
            // Création d'une structure mouvements autorisés par défaut pour l'instant
            bool bMvtAutorise = false;
            if( pGir )
            {
                bMvtAutorise = pGir->GetCoeffsMouvementsInsertion( this, pVAm, pDest, mapCoeffs );
            }
            else
            {
                bMvtAutorise = pCnx->GetCoeffsMouvementAutorise( pVAm, pDest, mapCoeffs, GetType(), &m_SousType );
                // correction anomalie n°78 : véhicule bloqués lorsqu'on interdit leur type de véhicule alors qu'ils sont déjà sur le mouvement autorisé
                if(!bMvtAutorise && pTInterne)
                {
                    pCnx->GetCoeffsMouvementAutorise( pVAm, pDest, mapCoeffs );
                    bMvtAutorise = mapCoeffs.size() > 0;
                }
            }


            if( bMvtAutorise )
            {
                // récupération du tirage s'il existe dans la mémoire
                dbRand = GetTirage(pVAm);

                // Détermination de la voie éligible
                dbSum = 0;

                map<int, MouvementsSortie> mapCoeffsFinal = AdaptMouvementAutorises(mapCoeffs, dbInstant, pTS);
                map<int, MouvementsSortie>::iterator iterMapCoeff;
                int nbVoiesNextTuyau = pGir ? pGir->GetNbVoie() : pTS->getNbVoiesDis();
                for(int i=0; i<nbVoiesNextTuyau; i++)
                {
                    iterMapCoeff = mapCoeffsFinal.find(i);
                    if( iterMapCoeff != mapCoeffsFinal.end() && iterMapCoeff->second.m_dbTaux > 0)
                    {
                        dbSum += iterMapCoeff->second.m_dbTaux;

                        if( dbRand <= dbSum )
                        {
                            if(!pGir)
                            {
                                pNextVoie = (VoieMicro*)pTS->GetLstLanes()[i];
                            }
                            else
                            {
                                // Récupération du premier troncon interne suivant le troncon amont
                                std::vector<Tuyau*> tuyauxInternes;
                                pGir->GetTuyauxInternes(pTuyau, pTS, tuyauxInternes);
                                pNextVoie = tuyauxInternes[0]->GetLstLanes()[i];
                            }
                            break;
                        }
                    }
                }                                                              
            }
            else
            {
                pNextVoie = NULL;
            }

            if(pCAF && pNextVoie)
            {
                // ETS 100413
                VoieMicro * pVoieMvt = pCAF->GetNextTroncon(pTuyau, nVAmont, (TuyauMicro*)pTS, pNextVoie->GetNum(), pTInterne);
                if( pVoieMvt)
                {
                    pNextVoie =pVoieMvt;
                }
            }

            return pNextVoie;
        }
        else
            return NULL;                

    } // fin if( m_pReseau->IsUsedODMatrix() && m_pReseau->IsCptItineraire() )	

    return NULL;
}


//================================================================
std::map<int, MouvementsSortie> Vehicule::AdaptMouvementAutorises
//----------------------------------------------------------------
// Fonction  : Modifie les mouvements autorisés en fonction des
//             voies réservées.
//             Remarque : on renvoie une copie et pas des 
//             pointeurs pour ne pas modifier les données
//             originales
// Version du: 28/07/2011
// Historique: 28/07/2011 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    const std::map<int, boost::shared_ptr<MouvementsSortie> > & mapCoeffs,
    double dbInstant,
    Tuyau* pTS
)
{
    // recopie de la map des coefficients pour modification à cause de voies interdites
    // (par exemple, voies réservées)
    std::map<int, MouvementsSortie> mapCoeffsFinal;
    // on met toutes les voies non interdites
    std::map<int, boost::shared_ptr<MouvementsSortie> >::const_iterator mvtIter;
    for(mvtIter = mapCoeffs.begin(); mvtIter != mapCoeffs.end(); mvtIter++)
    {
        if(!pTS->IsVoieInterdite(GetType(), mvtIter->first, dbInstant)
            && !mvtIter->second->IsTypeExclu(GetType(), &m_SousType))
        {
            mapCoeffsFinal[mvtIter->first] = *mvtIter->second;
        }
    }

    // report des voies interdites vers les voies permises adjacentes
    for(mvtIter = mapCoeffs.begin(); mvtIter != mapCoeffs.end(); mvtIter++)
    {
        if(pTS->IsVoieInterdite(GetType(), mvtIter->first, dbInstant)
            || mvtIter->second->IsTypeExclu(GetType(), &m_SousType)) 
        {
            // recherche de la première voie permise à droite
            int indexVoieDroite = -1;
            for(int iVoie = mvtIter->first-1; iVoie >= 0; iVoie--)
            {
                // Il faut qu'un mouvement vers cette voie soit possible !
                if(mapCoeffsFinal.find(iVoie) != mapCoeffsFinal.end())
                {
                    indexVoieDroite = iVoie;
                    break;
                }
            }

            // recherche de la première voie permise à gauche
            int indexVoieGauche = -1;
            for(int iVoie = mvtIter->first+1; iVoie < pTS->getNb_voies(); iVoie++)
            {
                // Il faut qu'un mouvement vers cette voie soit possible !
                if(mapCoeffsFinal.find(iVoie) != mapCoeffsFinal.end())
                {
                    indexVoieGauche= iVoie;
                    break;
                }
            }

            // si c'est la voie de droite la plus proche ou que pas de voie de gauche
            if(indexVoieDroite != -1
                && ((mvtIter->first-indexVoieDroite < indexVoieGauche - mvtIter->first)
                || (indexVoieGauche == -1)))
            {
                mapCoeffsFinal[indexVoieDroite].m_dbTaux += mvtIter->second->m_dbTaux;
            }
            // si c'est la voie de gauche la plus proche
            else if(indexVoieGauche != -1
                && ((mvtIter->first-indexVoieDroite > indexVoieGauche - mvtIter->first)
                || (indexVoieDroite == -1)))
            {
                mapCoeffsFinal[indexVoieGauche].m_dbTaux += mvtIter->second->m_dbTaux;
            }
            // si elles sont à distance égale
            else if((mvtIter->first-indexVoieDroite == indexVoieGauche - mvtIter->first)
                && (indexVoieGauche != -1 && indexVoieDroite != -1))
            {
                // sur la voie de droite
                mapCoeffsFinal[indexVoieDroite].m_dbTaux += mvtIter->second->m_dbTaux/2.0;
                // sur la voie de gauche
                mapCoeffsFinal[indexVoieGauche].m_dbTaux += mvtIter->second->m_dbTaux/2.0;
            }
            else
            {
                // Correction anomalie n°78 : si on ne peut pas reporter, on conserve le mouvement tel quel pour ne pas bloquer de véhicule
                mapCoeffsFinal[mvtIter->first] = *mvtIter->second;
            }
        }
    }
    return mapCoeffsFinal;
}


//================================================================
void Vehicule::CalculEmission
//----------------------------------------------------------------
// Fonction  : Calcul des émissions
// Version du: 06/10/2008 (d'après MQ - 2004)
// Historique: 12/10/2006 (C.Bécarie - Tinea)
//             Création
//             06/10/2008 (C.Bécarie - INEO TINEA)
//             Ajout gestion plusieurs sources acoustiques pour un 
//             type de véhicule
//================================================================
(
    Loi_emission  * Loi,
    std::string     strType,    // Identifiant base de données
    std::string     typ_rev     // Type de revêtement de la voie
)
{    
    bool    bAcc;
    int     nVitOld, nVitNew;

    if(m_pTuyau[1] )
    {
        bAcc = (m_pAcc[0] >= 1 && m_pAcc[1] >= 1) || (m_pAcc[0] <= -0.2 && m_pAcc[1] >= -0.2) || ( (m_pAcc[0] > -0.2 && m_pAcc[0] < 1) && (m_pAcc[1] > -0.2 && m_pAcc[1] < 1) );

        nVitNew = (int)(m_pVit[0] *3.6);
        if( m_pVit[0]*3.6 > 20)
        {
            if (div(nVitNew,2).rem > 0)
                nVitNew--;
        }

        nVitOld = (int)(m_pVit[1] *3.6);
        if( m_pVit[1]*3.6 > 20)
        {
            if (div(nVitOld,2).rem > 0)
                nVitOld--;
        }

        // Si même classe de vitesse, même allure et même revêtement, il est inutile
        // de refaire le calcul !
        if( nVitNew == nVitOld && bAcc && m_pTuyau[0]->GetRevetement()==m_pTuyau[1]->GetRevetement() )
            return;
    }

    Loi->CalculEmissionBruit(m_pWEmission, m_pVit[0], m_pAcc[0], typ_rev, strType);
}

//================================================================
void Vehicule::CalculEmissionAtmos
//----------------------------------------------------------------
// Fonction  : Calcul des émissions atmosphériques
// Version du: 
// Historique: 
//================================================================
(
    double  dbPasTemps,
    double &dbValCO2,
    double &dbValNOx,
    double &dbValPM
)
{
    int nType, nIndex;

    double VITO[12][7] =      {  // VL Diesel
        {0,	    0.324,	    0.0859, 	0.00496,	-0.0586,	0.448,	    0.23        },  // CO2  - a >=-0.5
        {0,     0.324,      0.0859,	    0.00496,	-0.0586,	0.448,	    0.23        },  // CO2  - a < -0.5
        {0,	    0.00241,	-0.000411,	0.0000673,	-0.00307,	0.00214,	0.0015      },  // NOx  - a >=-0.5
        {0,	    0.00168,	-0.0000662,	0.000009,	0.00025,	0.000291,	0.00012     },  // NOx  - a < -0.5
        {0,	    0,	        0.000313,	-0.0000184,	0,	        0.00075,	0.000378    },  // PM   - a >=-0.5
        {0,	    0,	        0.000313,	-0.0000184,	0,	        0.00075,	0.000378    },  // PM   - a < -0.5
        // BUS
        {0,	    0.904,	    1.13,	    -0.0427,	2.81,	    3.45,	    1.22},          // CO2  - a >=-0.5
        {0,	    0.904,	    1.13,	    -0.0427,	2.81,	    3.45,	    1.22},          // CO2  - a < -0.5
        {0,	    0.0236,	    0.00651,	-0.00017,	0.0217,	    0.00894,	0.00757},       // NOx  - a >=-0.5
        {0,	    0.0236,	    0.00651,	-0.00017,	0.0217,	    0.00894,	0.00757},       // NOx  - a < -0.5
        {0,	    0.000223,	0.000347,	-0.0000238,	0.00208,	0.00176,	0.000223},      // PM   - a >=-0.5
        {0,	    0.000223,	0.000347,	-0.0000238,	0.00208,	0.00176,	0.000223} };    // PM   - a < -0.5

        dbValCO2 = 0;
        dbValNOx = 0;
        dbValPM = 0;

        double dbAcc = m_pAcc[0];
        if( m_pAcc[0] < -2)
            dbAcc = -2;

        //if(m_pPos[0] < 0)   // Les véhicules hors du réseau ne sont pas considérés
        //  return;

        if(m_pVit[0] > GetDiagFonda()->GetVitesseLibre() + 0.1)
            return;

        if(!m_pTypeVehicule->GetLabel().compare("VL"))
            nType = 0;
        else if (!m_pTypeVehicule->GetLabel().compare("TypeBus"))
            nType = 6;
        else
            return;

        /*dbAcc = (m_pVit[1]-m_pVit[0]) / dbPasTemps;
        if( !m_pTuyau[1] )
        dbAcc = 0;*/

        // CO2
        if(m_pAcc[0] >= -0.5)
            nIndex = nType;
        else
            nIndex = nType+1;

        dbValCO2 = std::max<double>(VITO[nIndex][0], VITO[nIndex][1] + VITO[nIndex][2]*m_pVit[0] + VITO[nIndex][3]*pow(m_pVit[0], 2) + VITO[nIndex][4]*dbAcc +  VITO[nIndex][5]*pow(dbAcc, 2) + VITO[nIndex][6]*m_pVit[0]*dbAcc);

        // Ajout au cumul du véhicule
        m_dbCumCO2 += dbValCO2;

        // NOx
        if(m_pAcc[0] >= -0.5)
            nIndex = nType + 2;
        else
            nIndex = nType + 3;

        dbValNOx = std::max<double>(VITO[nIndex][0], VITO[nIndex][1] + VITO[nIndex][2]*m_pVit[0] + VITO[nIndex][3]*pow(m_pVit[0], 2) + VITO[nIndex][4]*dbAcc +  VITO[nIndex][5]*pow(dbAcc, 2) + VITO[nIndex][6]*m_pVit[0]*dbAcc);

        // Ajout au cumul du véhicule
        m_dbCumNOx += dbValNOx;

        // PM
        if(m_pAcc[0] >= -0.5)
            nIndex = nType + 4;
        else
            nIndex = nType + 5;

        dbValPM = dbPasTemps * std::max<double>(VITO[nIndex][0], VITO[nIndex][1] + VITO[nIndex][2]*m_pVit[0] + VITO[nIndex][3]*pow(m_pVit[0], 2) + VITO[nIndex][4]*dbAcc +  VITO[nIndex][5]*pow(dbAcc, 2) + VITO[nIndex][6]*m_pVit[0]*dbAcc);

        // Ajout au cumul du véhicule
        m_dbCumPM += dbValPM;

}

//================================================================
void Vehicule::SortieEmission
//----------------------------------------------------------------
// Fonction  : Ecriture des fichiers émission acoustique
// Version du: 12/10/2006
// Historique: 12/10/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(        
)
{
    double dbAbs;
    double dbOrd;
    double dbHaut;
    double dbAbsAnt;
    double dbOrdAnt;
    double dbHautAnt;
    char   strName[50];
    char   strLigne[50];
    char   strID[12];
    double  dbLwEmission;

    if( !m_pVoie[0] )     // Le véhicule vient juste d'être créé, on ne le prend pas encore en compte (pas dans le trafic)
        return;

    CalculXYZ(dbAbs, dbOrd, dbHaut);

    if( m_pTuyau[1] )
    {
        CalculXYZ(dbAbsAnt, dbOrdAnt, dbHautAnt, false);    
    }
    else  // le véhicule vient d être créé
    {
        // Abscisse du véhicule au début du pas de temps
        dbAbsAnt = m_pVoie[0]->GetAbsAmont();

        // Ordonnée du véhicule au début du pas de temps
        dbOrdAnt = m_pVoie[0]->GetOrdAmont();

        // Hauteur du véhicule au début du pas de temps
        dbHautAnt = m_pVoie[0]->GetHautAmont();
    }

    snprintf(strID,12,"%d",m_nID);
    strcpy(strName, m_pTypeVehicule->GetLabel().c_str());
    strcat(strName, "_");
    strcat(strName, strID );
    if( GetFleet() != m_pReseau->GetSymuViaFleet() )
    {
        strcpy(strLigne, GetTrip()->GetID().c_str());
    }
    else
    {
        strcpy(strLigne, "VEH");
    }

    double * dbVal;
    dbVal = new double[Loi_emission::Nb_Octaves+1];

    for(int i=0; i<=Loi_emission::Nb_Octaves; i++)
    {
        if( m_pWEmission[i] > 0)
            dbLwEmission = 10 * log10 (m_pWEmission[i] / pow((double)10, (double)-12) );
        else
            dbLwEmission = 0;

        dbVal[i] = dbLwEmission;    
    }    

    std::string ssName = strName;
    std::string ssType = m_pTypeVehicule->GetLabel();
    std::string ssLigne = strLigne;

    m_pReseau->m_XmlDocAcoustique->AddSourcePonctuelle( m_nID, dbAbsAnt, dbOrdAnt, dbHautAnt, dbAbs ,dbOrd, dbHaut, dbVal);

}

//================================================================
Segment* Vehicule::GetCellAcoustique
//----------------------------------------------------------------
// Fonction  : Retourne la cellule acoustique sur laquelle est positionnée
//             le véhicule à la fin du pas de temps
// Version du: 29/09/2008
// Historique: 17/10/2006 (C.Bécarie - Tinea)
//             Création
//             29/09/2008
//             La source acoustique est définie par sa position par
//              rapport à l'avant du véhicule
//================================================================
(
    double dbPos    // Position de la source acoustique du véhicule
)
{
    int i;
    int nCell;

    if(!m_pVoie[0] || m_pTuyau[0]->GetType() != Tuyau::TT_MICRO)
        return NULL;

    nCell = -1;

    if( m_pPos[0] + dbPos >=0 ) // Sur le tronçon à la fin du pas de temps
    {
        for(i=0; i< ((TuyauMicro*)m_pTuyau[0])->GetNbCellAcoustique() && nCell < 0 ;i++ )
        {
            if( m_pPos[0] + dbPos <= ((i+1)*m_pVoie[0]->GetLength() / 
                ((TuyauMicro*)m_pTuyau[0])->GetNbCellAcoustique()) || fabs(m_pPos[0] + dbPos - ((i+1)*m_pVoie[0]->GetLength() /  ((TuyauMicro*)m_pTuyau[0])->GetNbCellAcoustique()))<0.1 )
                nCell = i;
        }

        if( nCell >= 0 )
            return m_pVoie[0]->GetLstCellAcoustique()[nCell];        
    } // Sur la voie précédente
    else
    {        
        if(m_pPrevVoie)
        {
            for(i=0; i< ((TuyauMicro*)m_pPrevVoie->GetParent())->GetNbCellAcoustique() && nCell < 0 ;i++ )
            {
                if( m_pPrevVoie->GetLength() + m_pPos[0] + dbPos <= ((i+1)*m_pPrevVoie->GetLength() /  ((TuyauMicro*)m_pPrevVoie->GetParent())->GetNbCellAcoustique()) || fabs(m_pPrevVoie->GetLength() + m_pPos[0] + dbPos - ((i+1)*m_pPrevVoie->GetLength() /  ((TuyauMicro*)m_pPrevVoie->GetParent())->GetNbCellAcoustique())) < 0.1)
                    nCell = i;
            }

            if( nCell >= 0 )
                return m_pPrevVoie->GetLstCellAcoustique()[nCell];            
        }
    }
    return NULL;
}


//================================================================
Segment* Vehicule::GetCellSirane
//----------------------------------------------------------------
// Fonction  : Retourne la cellule "Sirane" sur laquelle est
//             positionnée le véhicule à la fin du pas de temps
// Version du: 06/04/2012
// Historique: 06/04/2012 (O. Tonck - IPSIS)
//             Création
//================================================================
(
)
{
    int nCell;

    if(!m_pTuyau[0] || m_pTuyau[0]->GetType() != Tuyau::TT_MICRO)
        return NULL;	

    // cas de la brique de connexion
    if(m_pTuyau[0]->GetBriqueParente())
    {
        return m_pTuyau[0]->GetBriqueParente()->GetCellSirane();
    }

    nCell = -1;

    const std::vector<Segment*> & cells = ((TuyauMicro*)m_pTuyau[0])->GetSiraneCells();


    for(int i=0; i< (int)cells.size() && nCell < 0 ;i++ )
    {
        if( m_pPos[0] <= ((i+1)*m_pTuyau[0]->GetLength() /  cells.size()) || fabs(m_pPos[0] - ((i+1)*m_pTuyau[0]->GetLength() /  cells.size()))<0.1 )
            nCell = i;
    }

    if( nCell >= 0 )
        return cells[nCell];        

    return NULL;
}


//================================================================
double Vehicule::GetDstParcourueEx
//----------------------------------------------------------------
// Fonction  : Retourne la distance parcourue durant le pas de temps
// Remarque  : Cette fonction doit être appelée après CalculTrafic
//             Elle remplace la fonction GetDstParcourueEx qui
//             imposait que le véhicule soit au moins sur 2
//             tronçons consécutifs
// Version du: 28/07/2008
// Historique: 28/07/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    double dbDst;

    if( m_pTuyau[0] == m_pTuyau[1])
        return (m_pPos[0] - m_pPos[1]);

    if(!m_pTuyau[1] && m_pVoie[1] == m_pVoie[0])    // le véhicule vient d'être créé et il reste sur son tronçon de création
        return m_pPos[0];

    if(m_pVoie[1])
    {
        dbDst = m_pVoie[1]->GetLength() - m_pPos[1];
    }
    else
    {
        // rmq. : peut ariver pour les véhicules qui sortent d'un troncon meso. Du coup on estime avec la vitesse
		return m_pVit[0] * m_pReseau->GetTimeStep();
    }

    for(int i=0; i<(int)m_LstUsedLanes.size(); i++)
        if( m_pVoie[0] != m_LstUsedLanes[i] )
            dbDst += m_LstUsedLanes[i]->GetLength();

    return dbDst + m_pPos[0];
}

double Vehicule::GetNextTripNodeDistance(int timeIdx, TripNode ** pNextTripNode)
{
    double result = 0;
    TripLeg * pCurrentLeg = GetTrip()->GetCurrentLeg();
    const std::vector<Tuyau*> & remainingFullPath = m_pReseau->ComputeItineraireComplet(pCurrentLeg->GetRemainingPath());
    Tuyau * pLink, *pCurrentLink = m_pTuyau[timeIdx];
    for(int iLink = (int)remainingFullPath.size()-1; iLink >= 0; iLink--)
    {
        pLink = remainingFullPath[iLink];
        result += pLink->GetLength();

        // S'il s'agit du tronçon sur lequel se trouve le véhicule, on arrête
        if(pLink == pCurrentLink)
        {
            // On retranche la partie du troçon courant déjà parcourue par le véhicule
            result -= m_pPos[timeIdx];
            break;
        }
        // Si on arrive à la brique dans laquelle est engagé le véhicule le cas échéant,
        // On ne se fie plus au 
        else if(pCurrentLink && pCurrentLink->GetBriqueParente() && pCurrentLink->GetBriqueParente() == pLink->GetBriqueParente())
        {
            vector<Tuyau*> lstInternalLinks;
            pLink->GetBriqueParente()->GetTuyauxInternesRestants((TuyauMicro*)pCurrentLink, remainingFullPath[iLink+1], lstInternalLinks);
            for(size_t i = 0; i < lstInternalLinks.size(); i++)
            {
                result += lstInternalLinks[i]->GetLength();
            }
            // On retranche la partie du troçon courant déjà parcourue par le véhicule
            result -= m_pPos[timeIdx];
            break;
        }
    }

    *pNextTripNode = pCurrentLeg->GetDestination();

    // On retranche éventuellement la position après la fin du dernier tronçon dans le cas d'une position en milieu de tronçon
    if (*pNextTripNode)
    {
        result -= (*pNextTripNode)->GetInputPosition().GetRemainingLinkLength();
    }

    // Problème de précision numérique lorsque la longueur restante est la longueur du tronçon (ce qui arrive si la destination
    // est un tronçon sans position définie sur ce tronçon), on peut avoir ici des valeurs très légèrement négatives, qu'on ramène à 0 :
    result = std::max<double>(0.0, result);


    return result;
}

//================================================================
boost::shared_ptr<Vehicule> Vehicule::ChercheLeader
//----------------------------------------------------------------
// Fonction  :  Retourne le leader du véhicule si il existe au début 
//              du pas de temps
// Remarque  :  Le leader est cherché sur la voie du véhicule puis sur
//              les voies suivantes de son itinéraire (à calculer si 
//              besoinjusqu'à que l'existence d'une leader n'influence 
//              pas le calcul de la nouvelle position du véhicule
// Version du:  22/11/2007
// Historique:  22/11/2007 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    double  dbInstant,   // Instant de simulationh
    bool    bIgnoreVehiculesEnDepassement /* = false */
)
{    
    boost::shared_ptr<Vehicule> pVeh;
    double      dbDstMax, dbDstCum;    
    VoieMicro*  pVoieSvt;
    VoieMicro*  pPredVoie;    

    //ETS 101013
    if( m_pCurrentTuyauMeso != NULL ) // le vehicule est sur un meso ->pas de leader possible
        return boost::shared_ptr<Vehicule>(); 

    // Distance d'influence d'un leader quel que soit son type
    dbDstMax = ComputeContextRange(m_pReseau->GetTimeStep());   

    // Recherche sur la voie courante
    pVeh = m_pReseau->GetNearAvalVehicule(m_pVoie[1], m_pPos[1]+1, NULL, bIgnoreVehiculesEnDepassement);
    if(pVeh)
        return pVeh;    


    dbDstCum = m_pVoie[1]->GetLength() - m_pPos[1];

    if(dbDstMax < dbDstCum)
        return boost::shared_ptr<Vehicule>();        // Aucun leader n'a été trouvé sur la zone d'influence

    //CalculNextVoie(m_pVoie[1], dbInstant);

    // Recherche sur les voies suivantes    
    /*nVoie = 0;
    pPredVoie = m_pVoie[1];

    // Recherche de la voie courante
    for(int i=0; i<(int)m_LstVoiesSvt.size(); i++)
    {
    if(m_LstVoiesSvt[i] ==  m_pVoie[1])
    break;
    else
    nVoie++;
    }*/
    pPredVoie = m_pVoie[1];

    while(1)
    {
        // Traitement de la voie suivante

        // Déjà déterminée ?
        //if( (int)m_LstVoiesSvt.size() < nVoie+1)
        //{        
        pVoieSvt = (VoieMicro*)CalculNextVoie(pPredVoie, dbInstant);    // Détermination de la voie suivante       
        //    m_LstVoiesSvt.push_back(pVoieSvt);                  // MAJ de la liste des voies suivantes
        //}
        //else
        //    pVoieSvt = m_LstVoiesSvt[nVoie];

        // Sortie ?
        if(!pVoieSvt || dynamic_cast<CTuyauMeso*>(pVoieSvt->GetParent() ) != NULL )
        {
            return boost::shared_ptr<Vehicule>();    // Pas de leader
        }

        // Existence d'une leader sur cette voie ?
        pVeh = m_pReseau->GetNearAvalVehicule(pVoieSvt, 0, NULL, bIgnoreVehiculesEnDepassement);
        if(pVeh)
            return pVeh;    

        // Distance max d'étude atteinte ?
        dbDstCum += pVoieSvt->GetLength();
        if(dbDstCum > dbDstMax)
            return boost::shared_ptr<Vehicule>();

        pPredVoie = pVoieSvt;
    }
}


//================================================================
bool Vehicule::IsItineraire
//----------------------------------------------------------------
// Fonction  :  Indique si le tronçon passé en paramètre fait parti
//              de son itinéraire
// Remarque  :  
// Version du:  16/05/08
// Historique:  16/05/08 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    Tuyau   *pT
)
{
    std::vector<Tuyau*>::iterator itIt;

    std::vector<Tuyau*>* pIti = NULL;
    std::vector<Tuyau*> pseudoPath;
    if (m_pReseau->IsCptDirectionnel() && GetFleet() == m_pReseau->GetSymuViaFleet())
    {
        // Cas d'un véhicule non guidé en mode directionel : on intégre le tuyau suivant à l'itinéraire pour gérer le cas du tronçon aval à la brique
        pseudoPath = GetTuyauxParcourus();
        if (m_pNextTuyau && !pseudoPath.empty() && m_pNextTuyau != pseudoPath.back() && m_pNextTuyau->GetCnxAssAm() == pseudoPath.back()->GetCnxAssAv())
        {
            pseudoPath.push_back(m_pNextTuyau);
        }
        pIti = &pseudoPath;
    }
    else
    {
        pIti = GetItineraire();
    }

    for (itIt = pIti->begin(); itIt != pIti->end(); itIt++)
    {
        if(pT== (*itIt) )
            return true;
    }

    // Cas particulier d'un tronçon interne d'un carrefour ou d'un giratoire (non connu par l'itinéraire)
    Tuyau   *pTAm = NULL;
    Tuyau   *pTAv = NULL;

    if(pT->GetBriqueParente())
    {
        if( pT->GetBriqueParente()->GetType() == 'C' || pT->GetBriqueParente()->GetType() == 'G')
        {
            BriqueDeConnexion *pBrique = pT->GetBriqueParente();

            // Recherche du tronçon amont du carrefour (et du tronçon aval)
            for (itIt = pIti->begin(); itIt != pIti->end(); itIt++)
            {
                if( (*itIt)->GetBriqueAval() == pBrique )
                {
                    pTAm = (*itIt);
                    pTAv = NULL;
                    // Cas particulier introduit avec le parcours en zone : un itinéraire peut se terminer par une brique !
                    // on récupère le prochain tuyau non interne (cas des bus : l'itinéraire contient les tronçons internes !)
                    std::vector<Tuyau*>::iterator itItAv = itIt;
                    itItAv++;
                    while (itItAv != pIti->end())
                    {
                        if(!(*itItAv)->GetBriqueParente())
                        {
                            pTAv = *itItAv;
                            break;
                        }
                        itItAv++;
                    }

                    // modif pour gérer les itinéraires qui bouclent
                    if(pTAm && pTAv)
                    {
                        // Vérification que le tronçon interne pT appartient bien à un mouvement allant de pTAm vers pTAv
                        if(pBrique->GetType() == 'C')
                        {
                            CarrefourAFeuxEx *pCAF = (CarrefourAFeuxEx*)pBrique;
                            std::deque<MvtCAFEx*>::iterator itMvt;
                            std::deque<TuyauMicro*>::iterator itT; 
                            for(itMvt = pCAF->m_LstMouvements.begin(); itMvt != pCAF->m_LstMouvements.end(); itMvt++)
                            {
                                if( (*itMvt)->ent->pTAm == pTAm && (*itMvt)->sor->pTAv == pTAv && !(*itMvt)->IsTypeExclu(GetType(), &m_SousType))
                                {
                                    for(itT = (*itMvt)->lstTMvt.begin(); itT != (*itMvt)->lstTMvt.end(); itT++)
                                    {
                                        if( (*itT) == pT )
                                            return true;
                                    }
                                }
                            }
                        }
                        else if(pBrique->GetType() == 'G')
                        {
                            Giratoire *pGir = (Giratoire*)pBrique;
                            std::vector<Tuyau*> dqTuyaux;
                            pGir->GetTuyauxInternes( pTAm, pTAv, dqTuyaux);
                            for(size_t i = 0; i < dqTuyaux.size(); i++)
                            {
                                if(dqTuyaux[i] == pT)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}


//================================================================
bool Vehicule::IsPasse
//----------------------------------------------------------------
// Fonction  :  Indique si le véhicule est passé par la position du 
//              tronçon indiqué au cours du pas de temps
// Remarque  :  Attention, ne prend pas en compte les tronçons internes des CAF
// Version du:  22/05/08
// Historique:  22/05/08 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    Tuyau *pT, 
    double dbPos
)
{
    if(m_pTuyau[1] == m_pTuyau[0] && m_pTuyau[1] == pT)
    {
        if( m_pPos[1] <= dbPos && m_pPos[0] >= dbPos )
            return true;
        else
            return false;
    }
    else
    {
        for(size_t iLane = 0; iLane < m_LstUsedLanes.size() + 2; iLane++)
        {
            if(iLane == 0)
            {
                if(m_pTuyau[1] == pT)
                {
                    return m_pPos[1] <= dbPos;
                }
            }
            else if(iLane == m_LstUsedLanes.size()+1)
            {
                if(m_pTuyau[0] == pT)
                {
                    return m_pPos[0] >= dbPos;
                }
            }
            else
            {
                return m_LstUsedLanes[iLane-1]->GetParent() == pT;
            }
        }
    }
    return false;
}

//================================================================
bool Vehicule::IsDejaPasse
//----------------------------------------------------------------
// Fonction  :  Indique si le véhicule est déjà passé par la position du 
//              tronçon indiqué avant le début du pas de temps
// Remarque  :  
// Version du:  15/06/2009
// Historique:  15/06/2009 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    Tuyau *pT, 
    double dbPos
)
{
    // Cas où le véhicule est sur le tronçon testé
    if( pT == m_pTuyau[1] )
        return ( dbPos < m_pPos[1] );

    // Sinon

    // Recherche du tronçon au début du pas de temps dans son itinéraire
    for(size_t nDeb = 0; nDeb < m_TuyauxParcourus.size(); nDeb++)
    {
        if( m_TuyauxParcourus[nDeb] == pT)
            return true;
    }

    return false;
}

//================================================================
void Vehicule::FinCalculTrafic
//----------------------------------------------------------------
// Fonction  :  Méthode à appeler à la fin d'un calcul de la nouvelle
//				position du véhicule
// Remarque  :  
// Version du:  07/01/2010
// Historique:  29/08/08 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    double dbInst
)
{
    boost::shared_ptr<Vehicule> pVehFollower;

    // Procédure ghost de changement de voie
    if( m_pReseau->IsChgtVoieGhost() )
    {
        if( m_nGhostRemain > 0)	// Un ghost existe t'il ?
        {
            m_nGhostRemain--;	// Il se rapproche de sa mort...
            if( m_nGhostRemain == 0 || m_pTuyau[0] != m_pTuyau[1] ) // Il est mort (de sa belle mort ou par changement de tronçon), on ne le prend plus en compte
            {
                if( m_pGhostFollower.lock() )
                {
                    m_pGhostFollower.reset();		
                    m_pGhostLeader.reset();
                }
                m_pGhostVoie = NULL;
                m_nGhostRemain = 0;
            }
        }
    }

    if(m_pVit[0] < 0 && fabs(m_pVit[0]) > 0.0001 )      // Rustine pour régler le problème des véhicules qui reculent à cause
    {                                                   // d'un changement de priorité au sein d'un CAF
        if( m_pTuyau[1] && m_pTuyau[1]->get_Type_aval() == 'C')
        {
            Convergent* pCvg = (Convergent*)m_pTuyau[1]->getConnectionAval();
            BriqueDeConnexion* pBrique = (BriqueDeConnexion*)m_pTuyau[1]->GetBriqueParente();
            if( pBrique && pBrique->GetType()=='C' && pCvg->IsVoieAmontPrioritaire(m_pVoie[1]) )
            {
                m_pPos[0] = m_pPos[1];
                m_pTuyau[0] = m_pTuyau[1];
                m_pVoie[0] = m_pVoie[1];
                m_pVit[0] = 0;

                SetAcc((m_pVit[0] - m_pVit[1])/m_pReseau->GetTimeStep() );                                
            }
        }
    }

    // Si le véhicule a changé de tuyau, on indique qu'il faut recalculer le tuyau suivant
    // remarque : la condition "|| m_pTuyau[0] == m_pNextTuyau" a été rajoutée afin de détecter corectement
    // les cas où le véhicule a changé de tuyau mais où m_pTuyau[1] == m_pTuyau[0] à cause de l'appel à SetTuyauAnt
    if( m_pTuyau[1] != m_pTuyau[0] || m_pTuyau[0] == m_pNextTuyau)
    {
        // Le tronçon suivant n'est pas modifié si on se trouve à l'intérieur d'un CAF
        if( m_pTuyau[0] && m_pTuyau[0]->GetBriqueParente() ) 
        {
            // Bug dans le cas où on a un tuyau amont à la brique suffisamment court pour être "sauté"
            // par le véhicule (observé avec un giratoire, à voir dans le cas des CAFs). m_pNextTuyau peut alors rester positionné sur ce tuyau amont.
            // Patch : si le tuyau suivant n'est pas un tuyau aval à la brique, on le reset pour forcer son recalcul au début
            // du pas de temps suivant
            if( !m_pTuyau[0]->GetBriqueParente()->IsTuyauAval(m_pNextTuyau) )
            {
                m_pNextTuyau = NULL;
            }
        }
        else
            m_pNextTuyau = NULL;
    }

    // Remise à jour de la distance parcourue
    if (m_pTuyau[1] == NULL || m_pVoie[1] == NULL)			// Instant d'entrée sur le réseau
        m_dbDstParcourue = m_pPos[0];
    else if (m_pTuyau[0] == NULL)		// Instant de sortie du réseau
        m_dbDstParcourue += m_pVoie[1]->GetLength() - m_pPos[1];
    else
        m_dbDstParcourue += m_pVit[0] * m_pReseau->GetTimeStep();

    // MAJ des map des véhicules parcourant les tuyaux ou les mouvements autorisés en affectation calculée
    // rmq. amélioration possible : utiliser la liste des voies empruntées pour mieux gérer le cas des tuyaux "sautés" au cours d'un pas de temps ?
    if( m_pTuyau[1] || m_pTuyau[0] )
    {
        if( m_pReseau->IsCptItineraire() )
        {
            if( m_pTuyau[1] != m_pTuyau[0]/*&& (m_pPos[0]>0 || m_pPos[1] > 0)*/ )	// le véhicule a changé de tronçon au cours du pas de temps			
            {					
                TuyauMicro* pTPrev = (TuyauMicro*) m_pTuyau[1];
                // Inscription de l'instant de sortie du tronçon précédent
                if( pTPrev && !pTPrev->GetBriqueParente()  && pTPrev->GetType() == Tuyau::TT_MICRO )
                {
                    double dbDstParcourueEx = GetDstParcourueEx();
                    double dbExitTime = dbInst;
                    if (pTPrev->m_mapVeh.find(m_nID) != pTPrev->m_mapVeh.end())
                    {
                        if (dbDstParcourueEx > 0)
                        {
                            // rmq. : en effet, il arrive qu'un véhicule puisse passer de l'extremité aval d'un tronçon à l'extremité amont du tronçon suivant
                            // et donc changer de tronçon avec une distance parcourue nulle.
                            dbExitTime -= m_pReseau->GetTimeStep() * m_pPos[0] / dbDstParcourueEx;
                        }
						if (!m_pTuyau[0])	// Si le véhicule est sorti du réseau, il faut prendre en compte l'instant exact de sortie
							dbExitTime = this->m_dbInstDestinationReached;

                        pTPrev->m_mapVeh.find(m_nID)->second.second.second = dbExitTime;
                    }
                    else
                    {
                        // On n'a pas noté le temps d'entrée du véhicule sur le tronçon précédent : cas de l'entrée du véhicule sur le réseau.
                        if (dbDstParcourueEx > 0) // rmq. : pour être sûr de ne pas avoir de division par 0 ici non plus (même si c'est peut être impossible dans ce cas ?)
                        {
                            // on retranche la fraction du bout de pas de temps depuis lequel le véhicule est créé qui correspond au déplacement sur le trnoçon suivant jusqu'à la position courante
                            dbExitTime -= (dbInst - this->m_dbHeureEntree) * m_pPos[0] / dbDstParcourueEx;

                            // pour éviter les problèmes de temps de parcours très légèrement négatifs (-1E-15) qu'on peut avoir par ce calcul à cause de problèmes de précision numérique :
                            dbExitTime = std::max<double>(dbExitTime, this->m_dbHeureEntree);
                        }
                        pTPrev->m_mapVeh.insert(make_pair(m_nID, make_pair(this->GetType(), make_pair(this->m_dbHeureEntree, dbExitTime))));
                    }

                    pTPrev->m_mapNbVehSorti.find( GetType() )->second++;

                    if (m_pReseau->GetTravelTimesOutputManager())
                    {
                        const std::pair<double, double> & inputOutputTimes = pTPrev->m_mapVeh.at(m_nID).second;
                        m_pReseau->GetTravelTimesOutputManager()->AddMeasuredTravelTime(pTPrev, inputOutputTimes.second - inputOutputTimes.first);
                    }
                }

#ifdef USE_SYMUCORE
                if (m_pReseau->IsSymuMasterMode())
                {
                    // on note les temps de sortie de zone pour SymuMaster
                    if (pTPrev)
                    {
                        SymuViaFleetParameters * pSymuViaParams = dynamic_cast<SymuViaFleetParameters*>(GetFleetParameters());
                        if (pSymuViaParams && pTPrev == pSymuViaParams->GetLastLinkInOriginZone())
                        {
                            ZoneDeTerminaison * pZone = dynamic_cast<ZoneDeTerminaison*>(GetOrigine());
                            if (pZone)
                            {
                                double dbInstantSortie = dbInst;
                                double dbDstParcourueEx = GetDstParcourueEx();
                                if (dbDstParcourueEx > 0)
                                {
                                    // rmq. : en effet, il arrive qu'un véhicule puisse passer de l'extremité aval d'un tronçon à l'extremité amont du tronçon suivant
                                    // et donc changer de tronçon avec une distance parcourue nulle.
                                    dbInstantSortie -= m_pReseau->GetTimeStep() * m_pPos[0] / dbDstParcourueEx;
                                }
                                double dbDistanceParcourue = m_dbDstParcourue - m_pPos[0];
                                std::pair<TypeVehicule*, std::pair<std::pair<double, double>, double> > zoneOutputTiming = std::make_pair(GetType(), std::make_pair(std::make_pair(dbInstantSortie, dbInstantSortie - this->m_dbHeureEntree), dbDistanceParcourue));
                                pZone->m_AreaHelper.GetMapDepartingVehicles()[pTPrev->GetCnxAssAv()][GetID()] = zoneOutputTiming;
                                if (pSymuViaParams->GetPlaqueOrigin())
                                {
                                    pSymuViaParams->GetPlaqueOrigin()->m_AreaHelper.GetMapDepartingVehicles()[pTPrev->GetCnxAssAv()][GetID()] = zoneOutputTiming;
                                }
                            }
                            else
                            {
                                assert(false); // a priori impossible d'avoir pSymuViaParams->GetLastLinkInOriginZone() non nul sans zone !
                            }
                        }
                    }
                }
#endif // USE_SYMUCORE

                TuyauMicro* pTNext = (TuyauMicro*) m_pTuyau[0];
                if( pTNext  && !pTNext->GetBriqueParente() && pTNext->GetType() == Tuyau::TT_MICRO )
                {
                    // Inscription de l'instant d'entrée sur le tronçon suivant
                    if (pTPrev)
                    {
                        double dbExitTime = dbInst;
                        double dbDstParcourueEx = GetDstParcourueEx();
                        if (dbDstParcourueEx > 0)
                        {
                            // rmq. : en effet, il arrive qu'un véhicule puisse passer de l'extremité aval d'un tronçon à l'extremité amont du tronçon suivant
                            // et donc changer de tronçon avec une distance parcourue nulle.
                            dbExitTime -= m_pReseau->GetTimeStep() * m_pPos[0] / dbDstParcourueEx;
                        }
                        pTNext->m_mapVeh.insert(make_pair(m_nID, make_pair(this->GetType(), make_pair(dbExitTime, 0))));
                    }
                    else
                        pTNext->m_mapVeh.insert( make_pair( m_nID, make_pair( this->GetType(), make_pair( this->m_dbHeureEntree, 0) ) ) );

                    pTNext->m_mapNbVehEntre.find( GetType() )->second++;
                }	

                // Incrémentation du compteur de véhicules pour le mouvement de la connexion franchie
                if( pTNext && !pTNext->GetBriqueParente()  )
                {
                    vector<Tuyau*> dqT;
                    dqT.push_back( pTNext );
                    // On doit récupérer le tronçon amont de la connexion franchie d'où vient le véhicule (pas forcément m_pTuyau[1] ...)
                    Tuyau * pTuyAm = NULL;
                    if(pTPrev && pTPrev->GetCnxAssAv() == pTNext->GetCnxAssAm())
                    {
                        pTuyAm = pTPrev;
                    }
                    else
                    {
                        // recherche du tronçon précédent dans l'itinéraire du véhicule
                        int iTuy = GetItineraireIndex(isSameTuyau, pTNext);
                        if(iTuy > 0)
                        {
                            pTuyAm = GetItineraire()->at(iTuy-1);
                        }
                    }
                    if(pTuyAm)
                    {
                        SymuViaTripNode * pSymuTripNode = dynamic_cast<SymuViaTripNode*>(this->GetDestination());
                        if(pSymuTripNode)
                        {
                            pTNext->GetCnxAssAm()->IncNbVehAff( GetType(), pTuyAm, pSymuTripNode, dqT);
                        }

                        // uniquement dans le cas des briques, mesure des temps de parcours des mouvements internes :
                        if(pTNext->GetBriqueAmont() && pTuyAm->GetType() == Tuyau::TT_MICRO)
                        {
                            // On prend l'instant d'entrée dans la map du tronçon amont si elle y est
                            std::map<int, std::pair< TypeVehicule*, std::pair<double, double> > > &  myMap = ((TuyauMicro*)pTuyAm)->m_mapVeh;
                            std::map<int, std::pair< TypeVehicule*, std::pair<double, double> > >::const_iterator iter = myMap.find(m_nID);
                            if(iter != myMap.end())
                            {
                                double dbInstantEntree = iter->second.second.second;
                                if(dbInstantEntree > 0)
                                {
                                    double dbInstantSortie = dbInst;
                                    double dbDstParcourueEx = GetDstParcourueEx();
                                    if (dbDstParcourueEx > 0)
                                    {
                                        // rmq. : en effet, il arrive qu'un véhicule puisse passer de l'extremité aval d'un tronçon à l'extremité amont du tronçon suivant
                                        // et donc changer de tronçon avec une distance parcourue nulle.
                                        dbInstantSortie -= m_pReseau->GetTimeStep() * m_pPos[0] / dbDstParcourueEx;
                                    }
                                    std::pair<Tuyau*, Tuyau*> myMove = std::make_pair(pTuyAm, pTNext);
                                    pTNext->GetBriqueAmont()->m_mapVeh[myMove][m_nID] = std::make_pair(m_pTypeVehicule, std::make_pair(dbInstantSortie, dbInstantSortie - dbInstantEntree));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Autres mises à jour spécifiques au type de flotte 
    GetFleet()->FinCalculTrafic(this);

    // Détection de la fin du dépassement des véhicules et réaffectation des leaders
    if(m_VehiculeDepasse)
    {
        double dbDistance;
        // on regarde si on a dépassé le véhicule
        bool bFinDepassement = false;
        if(!ComputeDistance(m_VehiculeDepasse->GetLink(0), m_VehiculeDepasse->GetPos(0), GetLink(0), GetPos(0), dbDistance))
        {
            // Dépassement avorté (le véhicule qu'on souhaite dépasser n'est plus sur l'itinéraire du véhicule dépassant : rien a faire de spécial
            // si ce n'est remettre à NULL m_VehiculeDepasse :
            m_VehiculeDepasse.reset();
        }
        else
        {
            // on regarde si véhicule dépassé a un leader.
            // Si oui, on regarde si le véhicule dépassant a dépassé ce véhicule.
            //     Si oui, on force le rabattement en positionnant le véhicule rabbatu sur le leader du véhicule dépassé.
            // Si non, on regarde si le véhicule a dépassé le véhicule dépassé de plus de la distance de la sécurité.
            //     Si oui, on rabat.
            //     Si non, on continue le dépassement

            boost::shared_ptr<Vehicule> pLeaderLeader = m_VehiculeDepasse->GetLeader();
            bool bSortieDepassement = false;
            if(pLeaderLeader && GetPos(0) >= pLeaderLeader->GetPos(0))
            {
                bSortieDepassement = true;

                // on force la position du véhicule dépassant pour le ramener au niveau du leader du véhicule dépassé
                m_pPos[0] = pLeaderLeader->GetPos(0);
                m_pVit[0] = (m_pPos[0] - m_pPos[1])/m_pReseau->GetTimeStep();
                m_pAcc[0] = (m_pVit[0] - m_pVit[1])/m_pReseau->GetTimeStep();
            }
            else
            {
                // Pas de leader du leader, on ne s'occupe que de respecter la distance de sécurité
                if(dbDistance >= GetCarFollowing()->GetMaxInfluenceRange())
                {
                    bSortieDepassement =true;
                }
            }

            if(bSortieDepassement)
            {
                // Sortie du mode dépassement
                m_VehiculeDepasse.reset();
            }
        }
    }

    // Gestion des phases de décélération
    if(!m_bDeceleration)
    {
        // Si on sort d'une phase de décélération, on RAZ la décélération tirée pour en tirer une nouvelle à la prochaine phase de décélération
        m_dbDeceleration = -1;
    }

    // Gestion de la liste des tuyaux déjà parcourus
    if(m_pVoie[1])
    {
        AddTuyauParcouru((Tuyau*)m_pVoie[1]->GetParent(), dbInst);
    }
    for(size_t iLane = 0; iLane < m_LstUsedLanes.size(); iLane++)
    {
        AddTuyauParcouru((Tuyau*)m_LstUsedLanes[iLane]->GetParent(), dbInst);
    }
    if(m_pVoie[0])
    {
        AddTuyauParcouru((Tuyau*)m_pVoie[0]->GetParent(), dbInst);
    }

    // Terminaison du leg et passage au suivant le cas échéant (après la mise à jour des tuyaux parcourus
    // sinon les fonctions de traitement des itinéraires ne fonctionne pas si on a sauté un tuyau, par exemple)
    if (m_bEndCurrentLeg)
    {
		//this->m_pReseau->log() << std::endl << "Before OnCurrentLegFinished - id " << GetID() << std::endl;
        GetFleet()->OnCurrentLegFinished(shared_from_this(), m_pDestinationEnterLane, m_dbInstDestinationReached, dbInst, m_dbDestinationTimeStep);
    }    
}

void Vehicule::SetNextTuyau(Tuyau* pT)
{
    m_pNextTuyau = pT;
}

void Vehicule::AddTuyauParcouru(Tuyau * pLink, double dbInstant)
{
    if(m_TuyauxParcourus.empty() || pLink != m_TuyauxParcourus.back())
    {
		if (pLink != m_pNextTuyauMeso)
		{
            boost::lock_guard<boost::mutex> guard(m_Mutex);

			m_TuyauxParcourus.push_back(pLink);

            // Cas particulier du mode DIR pour lequel on n'a pas d'itinéraire : on le construit donc au fil de l'eau pour se ramener au cas classique
            // pour gestion des différentes choses (voir tous les appels à GetItineraireIndex : feux tricolores, traversées, ...)
            if (m_pReseau->IsCptDirectionnel() && this->GetFleet() == m_pReseau->GetSymuViaFleet() && !pLink->GetBriqueParente())
            {
                std::vector<Tuyau*> currentPath = this->GetTrip()->GetCurrentLeg()->GetPath();
                currentPath.push_back(pLink);
                this->GetTrip()->GetCurrentLeg()->SetPath(currentPath);
                this->GetTrip()->InvalidateFullPath();
            }

			GetFleet()->SetLinkUsed(dbInstant, this, pLink);
		}
    }
}

//================================================================
double Vehicule::CalculPosition
//----------------------------------------------------------------
// Fonction  :  Calcule la position au bout de tr durant le pas de
//              temps en exprimant le résultat dans le repère défini
//              par pVoie
// Remarque  :  Si le véhicule ne passe pas le tuyau de pVoie, retourne
//              9999.
// Version du:  07/03/17
// Historique:  07/03/17 (O.Tonck - Ipsis)
//              Exploration du réseau pour trouver la bonne distance
//              même si le véhicule ne passe pas par pVoie
//              23/09/08 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    double dbtr, 
    Voie* pVoie,
    double dbLimitPosition
)
{
    double dbPosOrg; 

    dbPosOrg = m_pPos[1] + m_pVit[0]*dbtr;  // Position au bout de tr sur m_pVoie[1]

    if(pVoie->GetParent() == m_pVoie[1]->GetParent())
        return dbPosOrg;

    std::vector<Tuyau*> Iti = m_pReseau->ComputeItineraireComplet(*GetItineraire());

    int iTuyIdx = GetItineraireIndex(Iti, isSameTuyau, m_pVoie[1]->GetParent());
    if(iTuyIdx != -1)
    {
        dbPosOrg = - (m_pVoie[1]->GetLength() - dbPosOrg);
        for( int j=iTuyIdx+1; j< (int)Iti.size(); j++)
        {
            if( Iti[j] == pVoie->GetParent() )
            {
                return dbPosOrg;
            }
            else
            {
                dbPosOrg -= Iti[j]->GetLength();
            }
        }
    }

    // Ca priori plus correct mais pose des problèmes de gidlicks plus fréquents sur les giratoires... donc on laisse de côté pour le moment.
    /*
    // Si le véhicule ne passe pas par la voie demandée, on explore le réseau
    // en amont de pVoie pour trouver le véhicule afin d'avoir une distance valide et mieux traiter le cas
    // des giratoires par exemple :
    std::map<Tuyau*, double> mapPositions;
    mapPositions[(Tuyau*)pVoie->GetParent()] = 0;
    std::set<double> positions;
    Tuyau * pPrevLink;
    bool bContinue;
    do
    {
        // Remplacement des tuyaux par leur prédécesseurs :
        std::map<Tuyau*, double> newPreviousLinks;
        for (std::map<Tuyau*, double>::const_iterator iter = mapPositions.begin(); iter != mapPositions.end(); ++iter)
        {
            if (iter->first == m_pVoie[1]->GetParent())
            {
                positions.insert(iter->second);
            }

            if (iter->second >= dbLimitPosition)
            {
                for (size_t iPrevLink = 0; iPrevLink < iter->first->getConnectionAmont()->m_LstTuyAm.size(); iPrevLink++)
                {
                    pPrevLink = iter->first->getConnectionAmont()->m_LstTuyAm[iPrevLink];
                    std::map<Tuyau*, double>::iterator iterPrevLink = newPreviousLinks.find(pPrevLink);
                    double dbNewPos = iter->second - pPrevLink->GetLength();

                    if (iterPrevLink == newPreviousLinks.end())
                    {
                        newPreviousLinks[pPrevLink] = dbNewPos;
                    }
                    else
                    {
                        if (iterPrevLink->second < dbNewPos)
                        {
                            iterPrevLink->second = dbNewPos;
                        }
                    }

                    if (pPrevLink == m_pVoie[1]->GetParent())
                    {
                        positions.insert(dbNewPos);
                    }
                }
            }
        }

        // On fusionne les nouveaux tronçons avec ceux déjà traités :
        bContinue = false;
        for (std::map<Tuyau*, double>::const_iterator iter = newPreviousLinks.begin(); iter != newPreviousLinks.end(); ++iter)
        {
            std::map<Tuyau*, double>::iterator iterPreviousDist = mapPositions.find(iter->first);
            if (iterPreviousDist == mapPositions.end())
            {
                mapPositions[iter->first] = iter->second;
                bContinue = true;
            }
            else
            {
                if (iterPreviousDist->second < iter->second)
                {
                    iterPreviousDist->second = iter->second;
                    bContinue = true;
                }
            }
        }
    } while (bContinue);

    if (!positions.empty())
    {
        double dbClosestPosition = *positions.rbegin() + m_pPos[1] + m_pVit[0] * dbtr;
        if (dbClosestPosition >= dbLimitPosition)
        {
            return dbClosestPosition;
        }
    }

    
    

    // Si pas trouvé, le véhicule est considéré comme trop loin en amont plutôt que trop loin en aval.
    return -9999;*/

    return 9999;
}

void Vehicule::CalculVoiesPossibles
(
    double dbInstant
)
{    

    Connexion   *pCnx = NULL;    
    Giratoire   *pGir = NULL;

    if( !m_pTuyau[1] || !m_pVoie[1] )
        return;

    m_bVoiesOK.clear();

    // Cas particulier du bus qui voit un arrêt en aval sur le tronçon courant.
    // On impose de passer par la voie sur laquelle se trouve l'arrêt
    TripNode * pNextTripNode;
    double dbDistanceArret = GetNextTripNodeDistance(1, &pNextTripNode);
    bool bHasTripNodeBeforeEndLink = false;

    if (dbDistanceArret <= m_pVoie[1]->GetLength() - m_pPos[1] && dbDistanceArret <= m_pTuyau[1]->GetDstChgtVoie(m_pReseau->GetDstChgtVoie())
        && pNextTripNode && (!pNextTripNode->GetInputPosition().IsLink() || pNextTripNode->GetInputPosition().GetLink() == m_pTuyau[1])) // Pour gérer le cas du véhicule qui est en fin de tronçon précédent au tronçon de destination : la distance est nulle mais
             // On n'est pas encore sur le bon tronçon...
    {
        bHasTripNodeBeforeEndLink = true;
        if(GetTrip()->GetCurrentDestination()->hasNumVoie(GetTrip()))
        {
            int numVoie = GetTrip()->GetCurrentDestination()->getNumVoie(GetTrip(), GetType());
            for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
            {
                m_bVoiesOK.push_back(numVoie == i);
            }
            return;
        }
    }

    // Pas de restriction sur les voies dans le cas d'un comportement de type directionnel
    // Maintenant si (pour CSTB) : on fait comme pour les autres modes
    /*
    if( m_pReseau->IsCptDirectionnel() )
    {        
        for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
            // Cas de la voie impossible car voie réservée à un ou plusieurs autres types de véhicule
            if(m_pTuyau[1]->IsVoieInterdite(this->GetType(), i, dbInstant))
                m_bVoiesOK.push_back(false);
            else if( !((VoieMicro*)m_pTuyau[1]->GetLstLanes()[i])->IsChgtVoieObligatoire(GetType()) )  
                m_bVoiesOK.push_back(true);
            else
                m_bVoiesOK.push_back(false);  // La voie se réduit

            return;
    }*/

    //if( m_pReseau->IsCptItineraire() || m_pReseau->IsCptDestination() )
    {
        // Le véhicule se trouve dans la zone de 'mandatory changing' et il n'y a pas de TripNode destination sur la fin du troncon
        if( (m_pPos[1] > m_pVoie[1]->GetLength() - m_pTuyau[1]->GetDstChgtVoie(m_pReseau->GetDstChgtVoie())) &&
            !bHasTripNodeBeforeEndLink)
        {
            // Cas d'une brique de connexion
            if( m_pTuyau[1]->GetBriqueAval() )
            {
                if( m_pTuyau[1]->GetBriqueAval()->GetType() == 'C' || ( m_pTuyau[1]->GetBriqueAval()->GetType() == 'G' && m_pTuyau[1]->getNbVoiesDis()>1 ))
                {                               					
                    pCnx = m_pTuyau[1]->GetBriqueAval();
                }
            } 
            else if( m_pTuyau[1]->GetBriqueParente() )
            {
                if( m_pTuyau[1]->GetBriqueParente()->GetType() == 'G' && m_pTuyau[1]->getNbVoiesDis()>1 )
                {
                    pCnx = m_pTuyau[1]->GetBriqueParente();
                }
            }
            else
            {
                // Cas d'un élément simple de connexion
                if(m_pTuyau[1]->get_Type_aval())
                {
                    pCnx = m_pTuyau[1]->getConnectionAval();                                           
                }    
            }			

            if( pCnx ) // La matrice des mouvements permet de lister les voies accessibles pour le mouvement voulu
            {      
                // Cas particulier d'une sortie
                if( pCnx->GetNbElAval() == 0)	
                {
                    for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
                    {
                        if(m_pTuyau[1]->IsVoieInterdite(this->GetType(), i, dbInstant))
                            m_bVoiesOK.push_back(false);
                        else
                            m_bVoiesOK.push_back(true); 
                    }

                    return;
                }

                // Cas particulier de l'entrée sur un giratoire (pas gérés par mouvements autorisés : il faut regarder les coefficients d'insertion)
                if( m_pTuyau[1]->GetBriqueAval() && m_pTuyau[1]->GetBriqueAval()->GetType() == 'G' ) 
                {
                    Giratoire * pGir = (Giratoire*) pCnx;
                    std::map<int, boost::shared_ptr<MouvementsSortie> > mapCoeffs;
                    bool bVoieOk;
                    for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
                    {
                        bVoieOk = false;
                        if(m_pNextTuyau && i<pGir->GetNbVoie())
                        {
                            pGir->GetCoeffsMouvementsInsertion(this, (VoieMicro*)m_pTuyau[1]->GetLstLanes()[i], m_pNextTuyau, mapCoeffs);
                            bVoieOk = mapCoeffs[i]->m_dbTaux > 0;
                        }

                        m_bVoiesOK.push_back(bVoieOk); 
                    }

                    return;
                }

                // Cas particulier du parcours sur les tronçons internes au giratoire
                // On permet le rabattement pour sortie du giratoire si le tuyau de sortie a moins de voies que la voie du véhicule
                // sur le tronçon interne au giratoire
                if( m_pNextTuyau && m_pTuyau[1]->GetBriqueParente() && m_pTuyau[1]->GetBriqueParente()->GetType() == 'G' ) 
                {
                    for(size_t iTuyAv = 0; iTuyAv < m_pTuyau[1]->getConnectionAval()->m_LstTuyAv.size(); iTuyAv++)
                    {
                        if(m_pNextTuyau == m_pTuyau[1]->getConnectionAval()->m_LstTuyAv[iTuyAv]
                        && m_pNextTuyau->getNbVoiesDis() <= m_pVoie[1]->GetNum())
                        {
                            for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
                            {
                                if( i < m_pVoie[1]->GetNum() )
                                    m_bVoiesOK.push_back(true);
                                else
                                    m_bVoiesOK.push_back(false); 
                            }

                            return;
                        }
                    }
                }

                for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
                {
                    // Remarque : précédemment, les tests de IsVoieInterdite et IsChgtVoieObligatoire n'étaient pas
                    // faits ici (normalement les mouvements autorisés sont sensés être définis "intelligemment" en
                    // prenant en compte ces contraintes, mais lorsque ce n'est pas le cas, ces tests permettent 
                    // d'éviter des problèmes de blocage).
                    if( pCnx->IsMouvementAutorise(m_pTuyau[1]->GetLstLanes()[i], m_pNextTuyau, GetType(), &m_SousType ) )
                    {
                        if(m_pTuyau[1]->IsVoieInterdite(this->GetType(), i, dbInstant))
                            m_bVoiesOK.push_back(false);
                        else if( ((VoieMicro*)m_pTuyau[1]->GetLstLanes()[i])->IsChgtVoieObligatoire(GetType()) )  
                            m_bVoiesOK.push_back(false);
                        else
                            m_bVoiesOK.push_back(true);
                    }
                    else
                        m_bVoiesOK.push_back(false); 
                }
            }
            else // Toutes les voies sont possibles sauf celles qui se réduisent ou celles qui sont réservées
            {
                for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
                    // Cas de la voie impossible car voie réservée à un ou plusieurs autres types de véhicule
                    if(m_pTuyau[1]->IsVoieInterdite(this->GetType(), i, dbInstant))
                        m_bVoiesOK.push_back(false);
                    else if( !((VoieMicro*)m_pTuyau[1]->GetLstLanes()[i])->IsChgtVoieObligatoire(GetType()) )  
                        m_bVoiesOK.push_back(true);
                    else
                        m_bVoiesOK.push_back(false);  // La voie se réduit

                    return;
            }

        }
        else // Toutes les voies sont accessibles sauf celles qui se réduisent ou celles qui sont réservées
        {
            for(int i=0; i<m_pTuyau[1]->getNbVoiesDis(); i++)
                // Cas de la voie impossible car voie réservée à un ou plusieurs autres types de véhicule
                if(m_pTuyau[1]->IsVoieInterdite(this->GetType(), i, dbInstant))
                    m_bVoiesOK.push_back(false);
                else if( !((VoieMicro*)m_pTuyau[1]->GetLstLanes()[i])->IsChgtVoieObligatoire(GetType()) )  
                    m_bVoiesOK.push_back(true);
                else
                    m_bVoiesOK.push_back(false);  // La voie se réduit

                return;
        }
    }
}

void Vehicule::CalculVoieDesiree
(
    double dbInstant
)
{
    int nV, nVOK;
    
    // dans le cas des véhicules qui s'arrêtent à un TripNode, la voie préférée est calculée par ailleurs pour l'insertion dans la circulation.
    // Si une voie désirée est déjà présente ici, on ne fait pas de calcul supplémentaire.
    if(m_nVoieDesiree != -1)
    {
        return;
    }

    if( !m_pVoie[1] )
    {
        m_nVoieDesiree = -1;
        return;
    }

    nV = m_pVoie[1]->GetNum();  

    if( !m_bVoiesOK[nV] )   // Voie qui ne permet pas d'accéder à la destination souhaitée
    {
        // Recherche d'une voie correcte

        // D'abord les voies adjacentes
        nVOK = nV - 1; 
        if( nVOK >= 0)
        {
            if( m_bVoiesOK[nVOK] )
            {
                m_nVoieDesiree = nVOK;
                return;
            }
        }

        nVOK = nV + 1; 
        if( nVOK < (int)m_bVoiesOK.size() )
        {
            if( m_bVoiesOK[nVOK] )
            {
                m_nVoieDesiree = nVOK;
                return;
            }
        }

        // Puis les autres
        for(int nVOK = nV - 2; nVOK >= 0; nVOK--)
        {
            if( m_bVoiesOK[nVOK] )
            {
                // en cas de voie reservée détectée, on saute quand même la ou les voies reservees adjacentes
                TuyauMicro * pTuy = (TuyauMicro*)m_pVoie[1]->GetParent();
                for(int i = nV-1; i >= nVOK; i--)
                {
                    if(!pTuy->IsVoieInterdite(GetType(), i, dbInstant))
                    {
                        m_nVoieDesiree = i;		// Un changement d'une seule voie par pas de temps
                        return;
                    }
                }

                // Imossible a priori : m_bVoiesOK[nVOK] ne peut être true que si ce n'est pas une voie interdite
                assert(false); 

                //m_nVoieDesiree = nVOK;
                m_nVoieDesiree = nV-1;		// Un changement d'une seule voie par pas de temps
                return;
            }
        }

        for(int nVOK = nV + 2; nVOK < (int)m_bVoiesOK.size(); nVOK++)
        {
            if( m_bVoiesOK[nVOK] )
            {
                // en cas de voie reservée détectée, on saute quand même la ou les voies reservees adjacentes
                TuyauMicro * pTuy = (TuyauMicro*)m_pVoie[1]->GetParent();
                for(int i = nV+1; i <= nVOK; i++)
                {
                    if(!pTuy->IsVoieInterdite(GetType(), i, dbInstant))
                    {
                        m_nVoieDesiree = i;		// Un changement d'une seule voie par pas de temps
                        return;
                    }
                }

                // Imossible a priori : m_bVoiesOK[nVOK] ne peut être true que si ce n'est pas une voie interdite
                assert(false); 

                //m_nVoieDesiree = nVOK;
                m_nVoieDesiree = nV+1;      // Un changement d'une seule voie par pas de temps
                return;
            }
        }
    }
}

//================================================================
double Vehicule::GetTempsDistance
//----------------------------------------------------------------
// Fonction  :  Calcule le temps mis par le véhicule pour parcourir
//              la distance dbDst avec les caractéristiques du 
//              véhicule à la fin du pas de temps (ou au début
//              en fonction des paramètres)
// Remarque  :  Calcul discrétisé
// Version du:  
// Historique:  
//================================================================
(
    double	dbInst,			 // instant de simulation
    double	dbDst,           // distance à parcourir
    double	dbPasTemps,      // pas de temps de simulation
    bool    bDebutPasTemps   // Utilise les données du véhicule au début du pas de temps
)
{
    double dbT;
    double dbDsto;
    int no;
    double dbVitMax, dbAcc, dbVit, dbDstTmp;

    int tIdx = bDebutPasTemps?1:0;

    // Cas du véhicule guidé avec arrêt                      
    double dbDstArret;
    TripNode * pNextTripNode;

    dbDstArret = GetNextTripNodeDistance(0, &pNextTripNode);

    if (pNextTripNode && pNextTripNode->IsStopNeeded() && pNextTripNode->GetInputPosition().HasPosition() && dbDstArret<dbDst) // l'arrêt est avant la position cible
    {
        return CalculTempsAvecArret(dbInst, dbPasTemps, tIdx, GetLink(tIdx), GetPos(tIdx) + dbDst, pNextTripNode);
    }

    // Régime fluide aval
    if( m_bRegimeFluide)
    {
        if( m_pAcc[tIdx] > 0)  // le véhicule est en train d'accélérer
        {
            // Calcul du nombre de pas de temps nécessaire pour atteindre la vitesse libre
            dbAcc = GetAccMax(GetVit(tIdx));
            dbVitMax = std::min<double>( GetDiagFonda()->GetVitesseLibre(), m_pTuyau[tIdx]->GetVitRegByTypeVeh(GetType(), dbInst, m_pPos[tIdx], m_pVoie[tIdx]->GetNum()));

            // Nombre de pas de temps nécessaire pour atteindre la vitesse libre
            no = (int)ceill(  ( dbVitMax - GetVit(tIdx) ) / (dbAcc*dbPasTemps));

            // Calcul de la distance parcourue avant d'atteindre la vitesse libre
            dbDsto = no*GetVit(tIdx)*dbPasTemps + no*(no+1)/2*dbAcc*dbPasTemps*dbPasTemps;

            if( dbDsto < dbDst  )    // la vitesse libre est atteinte avant la distance parcourue désirée
                dbT = dbPasTemps*( no + floor( dbDst / (dbVitMax * dbPasTemps)));
            else // Le véhicule est en train d'accélérer quand il atteind la distance à parcourir
            {
                dbT = 0;                                
                dbVit = std::min<double>( dbVitMax, GetVit(tIdx) + GetAccMax(GetVit(tIdx))*dbPasTemps);
                dbDstTmp = dbVit*dbPasTemps; 
                while(dbDstTmp <= dbDst)
                {
                    dbVit = std::min<double>( dbVitMax, dbVit + GetAccMax(dbVit)*dbPasTemps);
                    dbDstTmp += dbVit*dbPasTemps;
                    dbT += dbPasTemps;
                } 
                dbT += (dbDstTmp - dbDst) / dbVit;
            }     
        }
        else
        {
            if( GetVit(tIdx) > 0)
                dbT = dbDst / GetVit(tIdx);
            else
                dbT = DBL_MAX;
        }        
    }
    else    // Régime congestionné
    {
        if(GetVit(tIdx)>0)
            dbT = dbDst / GetVit(tIdx);
        else
            dbT = DBL_MAX;
    }
    return dbT;
}

///<summary>	
/// Copie d'un véhicule
///</summary>	
///<param name="pVehDst">Véhicule destination
///</param>
///<returns>void</returns>
void Vehicule::CopyTo( boost::shared_ptr<Vehicule> pVehDst )
{
    if(!pVehDst)
        return;

    pVehDst->m_pReseau = m_pReseau;

    pVehDst->m_nID = m_nID;
    pVehDst->m_ExternalID = m_ExternalID;
    pVehDst->m_strNextRouteID = m_strNextRouteID;

    pVehDst->SetVitRegTuyAct( m_dbVitRegTuyAct );

    pVehDst->SetTypeVeh( m_pTypeVehicule );	

    // Variables d'état

    // Allocation
    pVehDst->m_pPos  = new double[m_nNbPasTempsHist+1];
    pVehDst->m_pVoie = new VoieMicro*[m_nNbPasTempsHist+1];
    pVehDst->m_pTuyau = new Tuyau*[m_nNbPasTempsHist+1];
    pVehDst->m_pVit  = new double[m_nNbPasTempsHist+1];
    pVehDst->m_pAcc  = new double[m_nNbPasTempsHist+1];

    for(int i=0; i<m_nNbPasTempsHist; i++)
    {
        pVehDst->m_pPos[i] = m_pPos[i];
        pVehDst->m_pVoie[i] = m_pVoie[i];
        pVehDst->m_pTuyau[i] = m_pTuyau[i];
        pVehDst->m_pVit[i] = m_pVit[i];
        pVehDst->m_pAcc[i] = m_pAcc[i];
    }

    pVehDst->m_dbVitMax =  m_dbVitMax;
    pVehDst->m_nNbPasTempsHist = m_nNbPasTempsHist;      

    pVehDst->m_pDF->SetProperties( m_pDF->GetW(), m_pDF->GetKMax(), m_pDF->GetVitesseLibre() );

    pVehDst->m_bAccMaxCalc = m_bAccMaxCalc;

    pVehDst->m_dbResTirFoll = m_dbResTirFoll;        
    pVehDst->m_dbResTirFollInt = m_dbResTirFollInt;                                                                                   

    pVehDst->m_bOutside = m_bOutside;  

    pVehDst->m_bArretAuFeu = m_bArretAuFeu;

    pVehDst->m_dbInstantCreation = m_dbInstantCreation;

    // Origine Destination
    pVehDst->m_dbHeureEntree = m_dbHeureEntree;    

    if(pVehDst->GetTrip())
    {
        delete pVehDst->GetTrip();
    }
    if(GetTrip())
    {
        Trip * newTrip = new Trip();
        GetTrip()->CopyTo(newTrip);
        pVehDst->SetTrip(newTrip);
    }

    pVehDst->m_pLastTripNode = m_pLastTripNode;
    pVehDst->m_pFleet = m_pFleet;

    if(pVehDst->GetFleetParameters())
    {
        delete pVehDst->GetFleetParameters();
    }
    if(GetFleetParameters())
    {
        AbstractFleetParameters * newFleetParams = GetFleetParameters()->Clone();
        pVehDst->m_pFleetParameters = newFleetParams;
    }
    pVehDst->m_pScheduleElement = m_pScheduleElement;

    pVehDst->m_dbTpsArretRestant = m_dbTpsArretRestant;
    pVehDst->m_dbInstInsertion = m_dbInstInsertion;
    pVehDst->m_nVoieInsertion = m_nVoieInsertion;

    pVehDst->m_pNextVoie = m_pNextVoie;
    pVehDst->SetNextTuyau( m_pNextTuyau );

    pVehDst->m_bAgressif = m_bAgressif;	
    pVehDst->m_bJorgeAgressif = m_bJorgeAgressif;	
    pVehDst->m_bJorgeShy = m_bJorgeShy;	
    

    pVehDst->m_dbDstParcourue = m_dbDstParcourue;

    pVehDst->m_pPrevVoie = m_pPrevVoie;
    pVehDst->m_bDejaCalcule = m_bDejaCalcule;
    pVehDst->m_bDriven = m_bDriven;
    pVehDst->m_bForcedDriven = m_bForcedDriven;

    pVehDst->m_bChgtVoie = m_bChgtVoie;
    pVehDst->m_bVoiesOK = m_bVoiesOK;
    pVehDst->m_nVoieDesiree = m_nVoieDesiree;

    pVehDst->m_TuyauxParcourus = m_TuyauxParcourus;

    pVehDst->m_bRegimeFluide = m_bRegimeFluide;
    pVehDst->m_bRegimeFluideLeader = m_bRegimeFluideLeader;
    pVehDst->m_dbCumCO2 = m_dbCumCO2;
    pVehDst->m_dbCumNOx = m_dbCumNOx;
    pVehDst->m_dbCumPM = m_dbCumPM;
    pVehDst->m_pVoieOld = m_pVoieOld;
    pVehDst->m_MouvementsInsertion = m_MouvementsInsertion;

    pVehDst->m_NextVoieTirage = m_NextVoieTirage;

    pVehDst->m_dbDeceleration = m_dbDeceleration;
    pVehDst->m_bDeceleration = m_bDeceleration;

    if(pVehDst->m_pCarFollowing)
    {
        delete pVehDst->m_pCarFollowing;
    }
    pVehDst->m_pCarFollowing = m_pReseau->GetCarFollowingFactory()->copyCarFollowing(pVehDst.get(), GetCarFollowing());

    pVehDst->m_dbCreationPosition = m_dbCreationPosition;
    pVehDst->m_dbGenerationPosition = m_dbGenerationPosition;

    pVehDst->m_pCurrentTuyauMeso = m_pCurrentTuyauMeso;
	pVehDst->m_pNextTuyauMeso = m_pNextTuyauMeso;
    pVehDst->m_timeCode = m_timeCode;

    pVehDst->m_lstReservoirs = m_lstReservoirs;
}

void  Vehicule::SetTuyau(Tuyau* pT, double dbInst)     
{
    m_pTuyau[0] = (TuyauMicro *) pT;
}
void Vehicule::SetTuyauAnt(TuyauMicro* pT)
{
    m_pTuyau[1] = pT;
}

///<summary>	
/// Pour gestion des portions de vitesse réglementaire
///</summary>	
///<param name="dbInst">instant courant
///</param>
///<returns>void</returns>
void Vehicule::UpdateVitesseReg(double dbInst) 
{
    Tuyau * pTuyau = m_pTuyau[0];
    VoieMicro * pVoie = m_pVoie[0];
    if(pTuyau && pVoie)
    {
        m_dbVitRegTuyAct = pTuyau->GetVitRegByTypeVeh(m_pTypeVehicule, dbInst, m_pPos[0], pVoie->GetNum());
    }
}

double Vehicule::Drive(TuyauMicro *pT, int nVoie, double dbPos, bool bForce)
{
    int result = 1;

    m_bDriven = true;
    m_bForcedDriven = bForce;
    bool bKeepComputedValue = false;

    // Préparation au calcul si pas déjà fait
    if(!m_pReseau->m_bDriven)
    {
        m_pReseau->InitVehicules(m_pReseau->m_dbInstSimu + m_pReseau->GetTimeStep());
        m_pReseau->m_bDriven = true;
    }   

    // changement de voie si besoin
    if (m_pVoie[1]->GetNum() != nVoie
        && pT == m_pVoie[1]->GetParent()) // Ajouté pour rejeu simulation avec SymuGame : plantage si on ne fait pas un changement de voie entre deux voeis d'un même tronçon...
    {
        ChangementVoieExt( m_pReseau->m_dbInstSimu + m_pReseau->GetTimeStep(), (VoieMicro*)pT->GetLstLanes()[nVoie], m_pReseau->GetGestionsCapteur()); 
        SetChtgVoie(true);
    }
    ReinitVoieDesiree();

    // Calcul de la position théorique calculée pour le véhicule.
    CalculTraficEx(m_pReseau->m_dbInstSimu + m_pReseau->GetTimeStep());

    // Comparaison de la position calculée avec la position demandée par pilotage pour :
    // - avertir si la position demandée dépasse la position calculée
    // - si la position demandée dépasse la position calculée, utiliser la position calculée à la place de la position demandée en mode non forcé
    if(IsCloserThan(m_pTuyau[0], m_pPos[0], pT, dbPos))
    {
        // Cas où la position calculée est inférieure à la position demandée
        result = IsRegimeFluide() ? -6 : -7; // avertissement
        if(!bForce)
        {
            bKeepComputedValue = true;
        }
    }

	double dbDist = -1;
    if(!bKeepComputedValue)
    {
        m_pTuyau[0] = pT;
        m_pVoie[0] = (VoieMicro*)pT->GetLstLanes()[nVoie];
        m_pPos[0] = dbPos;
        
        if(ComputeDistance(m_pTuyau[1], m_pPos[1], m_pTuyau[0], m_pPos[0], dbDist))
        {
            m_pVit[0] = dbDist / m_pReseau->GetTimeStep();
        }
        else
        {
            // Si le calcul de la distance est impossible, on utilise l'ancienne formule...
            //m_pVit[0] = (m_pPos[0] - m_pPos[1])/m_pReseau->GetTimeStep();

            // Ou pas car faux... du coup on ne modifie pas la vitesse dans ce cas et on garde la vitesse calculée : OK pour SymuGame pour la reproduction d'une simulation
        }
        m_pAcc[0] = (m_pVit[0] - m_pVit[1])/m_pReseau->GetTimeStep();
    }

    this->m_bDejaCalcule = true;

    m_bDriveSuccess = result == 1;

	ComputeDistance(m_pTuyau[1], m_pPos[1], m_pTuyau[0], m_pPos[0], dbDist);

    return dbDist;
}

int Vehicule::Delete()
{
    int result = 1;

    m_bDriven = true;
    m_bForcedDriven = true;

    // Préparation au calcul si pas déjà fait
    if(!m_pReseau->m_bDriven)
    {
        m_pReseau->InitVehicules(m_pReseau->m_dbInstSimu + m_pReseau->GetTimeStep());
        m_pReseau->m_bDriven = true;
    }   

    // Calcul de la position théorique calculée pour le véhicule.
    CalculTraficEx(m_pReseau->m_dbInstSimu + m_pReseau->GetTimeStep());

	m_pPos[0]       = UNDEF_POSITION;
	m_pVoie[0]      = NULL;
	m_pTuyau[0]     = NULL;
	m_pVit[0]       = m_dbVitMax;
	m_pAcc[0]       = 0;

	SetInstantSortie(m_pReseau->m_dbInstSimu + m_pReseau->GetTimeStep() - 0.00001 );

	m_bDejaCalcule = true;
	m_bDriven = true;

    m_bDriveSuccess = result == 1;

    return result;
}

// Alternative route function
int Vehicule::AlterRoute(const std::vector<Tuyau*> & newIti)
{
    // Check
    // OTK - TODO - ca ne va pas pour le SG : on doit pouvoir rerouter un véhicule vers un parking qui n'est pas la destination finale du véhicule.
    // voir s'il ne faut pas faire plus propre et faire une réaffectation de la destination finale dans ce cas ?
    //if( GetDestination()->GetInputID() != newIti.back()->GetCnxAssAv()->GetID() )
    //    return -6;	// The destination is wrong

    if (!GetTrip()->ChangeRemainingPath(newIti, this))
        return -7;

    return 0;
}

// Comparaison de deux positions au sein de l'itinéraire du véhicule
bool    Vehicule::IsCloserThan(Tuyau * pT1, double dbPos1, Tuyau * pT2, double dbPos2)
{
    // cas NULL : cas de la sortie du réseau
    if(pT1 == NULL || pT2 == NULL)
    {
        // pT1 est strictement avant pT2 s'il est non nul (encore dans le réseau)
        if(pT1 != NULL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // on commence par regarder la position du tuyau dans l'itinéraire auquel on intégre 
    // les tuyaux internes des briques.
    std::vector<Tuyau*> lstTuyaux = m_pReseau->ComputeItineraireComplet(*GetItineraire());


    size_t idxT1, idxT2;
    bool bT1InItineraire = false;
    bool bT2InItineraire = false;
    int iTuy1 = GetItineraireIndex(lstTuyaux, isSameTuyau, pT1);
    int iTuy2 = GetItineraireIndex(lstTuyaux, isSameTuyau, pT2);
    if(iTuy1 != -1)
    {
        idxT1 = (size_t)iTuy1;
        bT1InItineraire = true;
    }
    if(iTuy2 != -1)
    {
        idxT2 = (size_t)iTuy2;
        bT2InItineraire = true;
    }

    // Si au moins un des tronçons n'est pas dans l'itinéraire, en renvoie true
    // afin de passer dans le code permettant le renvoi du warning et de garder 
    // la position calculée si on n'est pas en more forcé.
    if(!bT1InItineraire || !bT2InItineraire)
        return true;


    // Si le tronçon T1 est avant le T2, c'est simple.
    if(idxT1<idxT2) 
    {
        return true;
    }
    else
    {   // si le tronçon T1 est le même que T2, il faut regarder les positions
        if((idxT1 == idxT2)
            && (dbPos1 < dbPos2))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

// Calcul de la distance entre deux points de deux troncons
bool Vehicule::ComputeDistance(Tuyau * pT1, double dbPos1, Tuyau * pT2, double dbPos2, double & dbDistance)
{
    dbDistance = 0;

    // Vérifications
    if(pT1 == NULL && pT2 == NULL)
    {
        return false;
    }

    // on commence par constituer l'itinéraire intégrant les tuyaux internes des briques.
    std::vector<Tuyau*> lstTuyaux = m_pReseau->ComputeItineraireComplet(*GetItineraire());

    if(lstTuyaux.size() < 1)
    {
        return false;
    }

    // traitement des entrées/sorties
    if(pT1 == NULL)
    {
        pT1 = lstTuyaux.front();
        dbPos1 = 0;
    }
    if(pT2 == NULL)
    {
        pT2 = lstTuyaux.back();
        dbPos2 = pT2->GetLength();
    }

    // On vérifie aussi que les tuyaux appartiennent à l'itinéraire
    bool bFound1 = false;
    bool bFound2 = false;
    size_t idxT1, idxT2;
    int iTuy1 = GetItineraireIndex(lstTuyaux, isSameTuyau, pT1);
    int iTuy2 = GetItineraireIndex(lstTuyaux, isSameTuyau, pT2);
    if(iTuy1 != -1)
    {
        idxT1 = (size_t)iTuy1;
        bFound1 = true;
    }
    if(iTuy2 != -1)
    {
        idxT2 = (size_t)iTuy2;
        bFound2 = true;
    }


    if(!bFound1 || !bFound2)
    {
        return false;
    }

    // parcours de l'itinéraire entre le point 1 et deux et calcul de la distance
    if(pT1 == pT2) 
    {
        dbDistance = dbPos2 - dbPos1;
    }
    else
    {
        bool bBetweenT1AndT2 = false;
        for(size_t i = idxT1 ; i <= idxT2; i++)
        {
            if(i == idxT1)
            {
                bBetweenT1AndT2 = true;
                dbDistance += pT1->GetLength() - dbPos1;
            }
            else if(i == idxT2)
            {
                bBetweenT1AndT2 = false;
                dbDistance += dbPos2;
            }
            else if(bBetweenT1AndT2)
            {
                dbDistance += lstTuyaux[i]->GetLength();
            }
        }
    }


    return true;
}

std::set<AbstractSensor*> Vehicule::GetPotentiallyImpactedSensors(SensorsManager * pSensorsManager)
{
	std::set<AbstractSensor*> potentiallyImpactedSensors;
	Tuyau * pLink = m_pTuyau[0];
	if (pLink)
	{
		potentiallyImpactedSensors.insert(pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].begin(), pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].end());
	}
	for (size_t iUsedLane = 0; iUsedLane < m_LstUsedLanes.size(); iUsedLane++)
	{
		pLink = (Tuyau*)m_LstUsedLanes[iUsedLane]->GetParent();
		potentiallyImpactedSensors.insert(pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].begin(), pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].end());
	}
	pLink = m_pTuyau[1];
	if (pLink)
	{
		potentiallyImpactedSensors.insert(pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].begin(), pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].end());
	}
    if (!m_pTuyau[0] && GetTrip()->GetFinalDestination())
	{
		pLink = GetTrip()->GetFinalDestination()->GetInputPosition().GetLink();
		if (pLink)
			potentiallyImpactedSensors.insert(pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].begin(), pLink->getLstRelatedSensorsBySensorsManagers()[pSensorsManager].end());		
	}

    return potentiallyImpactedSensors;
}

// ETS Pour l'implémentation mésoscopique
void Vehicule::SetTimeCode(Tuyau * pTuyau, ETimeCode eTimeCodeType,double dInstant)
{
    m_timeCode[pTuyau][eTimeCodeType] = dInstant;
    if(pTuyau )
    {
        m_pReseau->log() << "Vehicle " << this->GetID() << " type " << this->GetType()->GetLabel()
            << " time code " << eTimeCodeType << " on link " << pTuyau->GetLabel()
            << " at time "<< dInstant << std::endl;
    }

}

double Vehicule::GetTimeCode(Tuyau * pTuyau, ETimeCode eTimeCodeType) const
{
	std::map< Tuyau*, std::map<ETimeCode, double> >::const_iterator iter = m_timeCode.find(pTuyau);
	if (iter != m_timeCode.end())
    {
		std::map<ETimeCode, double>::const_iterator iter2 = iter->second.find(eTimeCodeType);
		if (iter2 != iter->second.end())
		{
			return iter->second.at(eTimeCodeType);
		}
    }
    return DBL_MAX;
}

void Vehicule::SetTuyauMeso(  Tuyau * pTuyau, double dInstant)
{ 

    m_pCurrentTuyauMeso = pTuyau;
    m_pNextTuyau = NULL;
	if (pTuyau == NULL)
	{
		m_pNextTuyauMeso = NULL;
	}

}

Tuyau* Vehicule::GetCurrentTuyauMeso() const
{

    if( m_pCurrentTuyauMeso )
    {
        return m_pCurrentTuyauMeso;
    }
    else
    {
        return m_pTuyau[0];
    }
}

// end ETS add

boost::shared_ptr<Vehicule> Vehicule::GetLeader()
{
    boost::shared_ptr<Vehicule> pLeader;
    if( GetCarFollowing()->GetCurrentContext() && !GetCarFollowing()->GetCurrentContext()->GetLeaders().empty() )
    {
        pLeader = GetCarFollowing()->GetCurrentContext()->GetLeaders().front();
    }
    return pLeader;
}

// Calcul du temps nécessaire pour atteindre la position indiquée en prenant en compte les arrêts sur le chemin
double Vehicule::CalculTempsAvecArret(double dbInstant, double dbPasTemps, int tIdx, Tuyau * pTuyau, double dbPos, TripNode * pArret)
{
    double dbVitMax = std::min<double>( GetDiagFonda()->GetVitesseLibre(), pTuyau->GetVitRegByTypeVeh(GetType(), dbInstant, GetPos(tIdx), GetVoie(tIdx)->GetNum()));
    double dbVit = GetVit(tIdx);
    double dbX = GetPos(tIdx);
    bool bFin = false;
    double dbT = 0;
    double dbDec = GetType()->GetDeceleration();
    double dbDst;

    // Calcul du nombre de pas de temps de décélération
    int no = 0;
    if(dbDec != 0)
    {
        no = (int)ceill( dbVit / (dbDec*dbPasTemps));
    }

    // Calcul de la distance à partir de laquelle le VGP doit décelérer
    double dbXo = pArret->GetInputPosition().GetPosition() - (no * GetVit(tIdx)*dbPasTemps - (no/2 * (no-1)*dbDec*dbPasTemps*dbPasTemps));                            

    while(!bFin)
    {
        // Le VGP roule à vitesse max ou à vitesse constante                            
        if( dbVit >= dbVitMax || fabs(GetAcc(tIdx)) < 0.001)
        {
            if(dbVit != 0) 
            {
                dbT += dbPasTemps * (std::max<double>(0,ceill((dbXo-dbX)/(dbVit*dbPasTemps)))+no);
            }
            bFin = true;
        }
        else if (GetAcc(tIdx)<0.001) // Le VGP est en train de décélérer
        {          
            if(dbDec != 0)
            {
                dbT = dbPasTemps * floor(dbVit / (dbDec*dbPasTemps));
            }
            else
            {
                dbT = 0;
            }
            bFin = true;
        }
        else // Le VGP accélère
        {                                  
            // Calcul de la vitesse au prochain pas de temps en prenant en compte l'accélération
            double dbVitNextStep = std::min<double>( dbVitMax, GetVit(tIdx) + GetAccMax(GetVit(tIdx))*dbPasTemps);
            // Calcul du nombre de pas de temps nécessaire pour la décélération si la vitesse calculée est atteinte
            // Anomalie n°109 : division par 0 ici ! correction : on fait comme pour le n0 en début de fonction
            int n0f = 0;
            if(dbDec != 0)
            {
                n0f = (int)ceill( dbVitNextStep / (dbDec*dbPasTemps));
            }
            // Calcul de la distance parcourue durant la phase de décélération
            double dbdn0f = n0f * dbVitNextStep *dbPasTemps - (n0f/2 * (n0f-1)*dbDec*dbPasTemps*dbPasTemps);                                   

            dbX = GetPos(tIdx);

            while(dbX < pArret->GetInputPosition().GetPosition() - dbdn0f)
            {
                dbVit = dbVitNextStep;
                dbX = dbX + dbVitNextStep*dbPasTemps;
                dbVitNextStep = std::min<double>( dbVitMax, dbVitNextStep + GetAccMax(dbVitNextStep)*dbPasTemps);
                // Anomalie n°109 : division par 0 ici ! correction : on fait comme pour le n0 en début de fonction
                n0f = 0;
                if(dbDec != 0)
                {
                    n0f = (int)ceill( dbVitNextStep / (dbDec*dbPasTemps));
                }
                dbdn0f = n0f * dbVitNextStep *dbPasTemps - (n0f/2 * (n0f-1)*dbDec*dbPasTemps*dbPasTemps);
                dbT += dbPasTemps;                                      
            }
            // Anomalie n°109 : si tIdx = 1, ca provoquait une boucle infinie
            if(tIdx == 1)
            {
                SetAccAnt(0);
            }
            else
            {
                SetAcc(0);
            }
            // Anomalie n°109 : division par 0 ici ! correction : on fait comme pour le n0 en début de fonction
            no = 0;
            if(dbDec != 0)
            {
                no = (int)ceill( dbVit / (dbDec*dbPasTemps));
            }
            dbXo = pArret->GetInputPosition().GetPosition() - (no * dbVit*dbPasTemps - (no/2 * (no-1)*dbDec*dbPasTemps*dbPasTemps));                                                                         
        }
    }                            

    // Ajout du temps d'arrêt restant
    if(IsAtTripNode())
    {
        dbT += m_dbTpsArretRestant;
    }
    else
    {
        dbT += GetFleet()->GetStopDuration(shared_from_this(), pArret, false);
    }


    // Distance qui reste à parcourir de l'arrêt à la position cible
    if( pTuyau == pArret->GetInputPosition().GetLink() )
        dbDst = dbPos - pArret->GetInputPosition().GetPosition();
    else
        dbDst = pArret->GetInputPosition().GetLink()->GetLength() - pArret->GetInputPosition().GetPosition() + dbPos;

    // Nombre de pas de temps nécessaire pour atteindre la vitesse libre
    double dbAcc = GetAccMax(0);
    no = (int)ceill(  ( dbVitMax ) / (dbAcc*dbPasTemps));
    double dbXno = no*(no+1)/2*dbAcc*dbPasTemps*dbPasTemps;

    if( dbXno <= dbDst)
    {
        // Le bus atteind sa vitesse libre avant d'entrer dans la zone
        dbT += dbPasTemps*( no + floor((dbDst-dbXno) / (dbVitMax * dbPasTemps)));
    }
    else
    {
        // Le bus est en train d'accélérer quand il rentre dans la zone
        //dbT += dbPasTemps * floor( sqrt( dbAcc*dbAcc*dbPasTemps*dbPasTemps/4 + 2*dbAcc*dbDst)/(dbAcc*dbPasTemps) - 0.5);

        dbVit = std::min<double>( dbVitMax, dbAcc*dbPasTemps);
        double dbDstTmp = dbVit*dbPasTemps;
        while(dbDstTmp <= dbDst)
        {
            dbVit = std::min<double>( dbVitMax, dbVit + GetAccMax(dbVit)*dbPasTemps);
            dbDstTmp += dbVit*dbPasTemps;
            dbT += dbPasTemps;
        } 
        dbT += (dbDstTmp - dbDst) / dbVit;
    }

    return dbT;
}

// Teste si le sous type de véhicule respecte la brique passée en paramètre (Effectue le tirage si nécessaire)
bool SousTypeVehicule::checkRespect(RegulationBrique * pBrique)
{
    bool bResult;
    map<RegulationBrique*, bool>::iterator iter = m_mapRespectConsigne.find(pBrique);
    if(iter == m_mapRespectConsigne.end())
    {
        double dbRand = pBrique->GetNetwork()->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
        bResult = dbRand <= pBrique->getTauxRespect();
        m_mapRespectConsigne[pBrique] = bResult;
    }
    else
    {
        bResult = iter->second;
    }

    return bResult;
}

std::set<TankSensor*> & Vehicule::getReservoirs()
{
    return m_lstReservoirs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void PlageAcceleration::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PlageAcceleration::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PlageAcceleration::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dbVitSup);
    ar & BOOST_SERIALIZATION_NVP(dbAcc);
}

template void SrcAcoustique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SrcAcoustique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SrcAcoustique::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dbPosition);
    ar & BOOST_SERIALIZATION_NVP(strIDDataBase);
}

template void TypeVehicule::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TypeVehicule::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TypeVehicule::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strLibelle);
    ar & BOOST_SERIALIZATION_NVP(m_strGMLLibelle);

    ar & BOOST_SERIALIZATION_NVP(m_bVxDistr);
    ar & BOOST_SERIALIZATION_NVP(m_dbVx);
    ar & BOOST_SERIALIZATION_NVP(m_dbVxDisp);
    ar & BOOST_SERIALIZATION_NVP(m_dbVxMin);
    ar & BOOST_SERIALIZATION_NVP(m_dbVxMax);

    ar & BOOST_SERIALIZATION_NVP(m_bWDistr);
    ar & BOOST_SERIALIZATION_NVP(m_dbW);
    ar & BOOST_SERIALIZATION_NVP(m_dbWDisp);
    ar & BOOST_SERIALIZATION_NVP(m_dbWMin);
    ar & BOOST_SERIALIZATION_NVP(m_dbWMax);

    ar & BOOST_SERIALIZATION_NVP(m_bInvKxDistr);
    ar & BOOST_SERIALIZATION_NVP(m_dbKx);
    ar & BOOST_SERIALIZATION_NVP(m_dbInvKxDisp);
    ar & BOOST_SERIALIZATION_NVP(m_dbKxMin);
    ar & BOOST_SERIALIZATION_NVP(m_dbKxMax);

    ar & BOOST_SERIALIZATION_NVP(m_dbEspacementArret);
    ar & BOOST_SERIALIZATION_NVP(m_dbDeceleration);
    ar & BOOST_SERIALIZATION_NVP(m_dbDecelerationEcartType);

    ar & BOOST_SERIALIZATION_NVP(m_dbValAgr);
    ar & BOOST_SERIALIZATION_NVP(m_dbTauChgtVoie);
    ar & BOOST_SERIALIZATION_NVP(m_dbPiRabattement);
    ar & BOOST_SERIALIZATION_NVP(m_LstPlagesAcceleration);
    ar & BOOST_SERIALIZATION_NVP(m_LstSrcAcoustiques);

    ar & BOOST_SERIALIZATION_NVP(m_dbVitLaterale);

    ar & BOOST_SERIALIZATION_NVP(m_dbTauDepassement);

    ar & BOOST_SERIALIZATION_NVP(m_ScriptedLawsParams);
    ar & BOOST_SERIALIZATION_NVP(m_ScriptedLawsCoeffs);

    ar & BOOST_SERIALIZATION_NVP(m_dbAgressiveProportion);
    ar & BOOST_SERIALIZATION_NVP(m_dbShyProportion);
    ar & BOOST_SERIALIZATION_NVP(m_dbAgressiveEtaT);
    ar & BOOST_SERIALIZATION_NVP(m_dbShyEtaT);
    ar & BOOST_SERIALIZATION_NVP(m_epsylon0);
    ar & BOOST_SERIALIZATION_NVP(m_epsylon1);
}

template void SousTypeVehicule::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SousTypeVehicule::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SousTypeVehicule::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_mapRespectConsigne);
}


template void Vehicule::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Vehicule::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Vehicule::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_dbVitMax);
    ar & BOOST_SERIALIZATION_NVP(m_nNbPasTempsHist);

    int nbItems = m_nNbPasTempsHist+1;
    SerialiseTab<Archive, double>(ar, "m_pPos", m_pPos, nbItems);
    SerialiseTab<Archive, double>(ar, "m_pVit", m_pVit, nbItems);
    SerialiseTab<Archive, double>(ar, "m_pAcc", m_pAcc, nbItems);

    ar & BOOST_SERIALIZATION_NVP(m_dbVitRegTuyAct);

    ar & BOOST_SERIALIZATION_NVP(m_bDriven);
    ar & BOOST_SERIALIZATION_NVP(m_bForcedDriven);

    ar & BOOST_SERIALIZATION_NVP(m_bAccMaxCalc);
    ar & BOOST_SERIALIZATION_NVP(m_dbResTirFoll);
    ar & BOOST_SERIALIZATION_NVP(m_dbResTirFollInt);
    ar & BOOST_SERIALIZATION_NVP(m_bOutside);
    ar & BOOST_SERIALIZATION_NVP(m_bArretAuFeu);

    ar & BOOST_SERIALIZATION_NVP(m_bChgtVoie);

    ar & BOOST_SERIALIZATION_NVP(m_bVoiesOK);

    ar & BOOST_SERIALIZATION_NVP(m_dbHeureEntree);
    ar & BOOST_SERIALIZATION_NVP(m_nVoieDesiree);

    ar & BOOST_SERIALIZATION_NVP(m_bRegimeFluide);
    ar & BOOST_SERIALIZATION_NVP(m_bRegimeFluideLeader);
    ar & BOOST_SERIALIZATION_NVP(m_bAgressif);
    ar & BOOST_SERIALIZATION_NVP(m_bJorgeShy);
    ar & BOOST_SERIALIZATION_NVP(m_bJorgeAgressif);

    ar & BOOST_SERIALIZATION_NVP(m_bAttenteInsertion);

    ar & BOOST_SERIALIZATION_NVP(m_dbCumCO2);
    ar & BOOST_SERIALIZATION_NVP(m_dbCumNOx);
    ar & BOOST_SERIALIZATION_NVP(m_dbCumPM);

    ar & BOOST_SERIALIZATION_NVP(m_dbDstParcourue);
    ar & BOOST_SERIALIZATION_NVP(m_pDF);

    ar & BOOST_SERIALIZATION_NVP(m_nGhostRemain);
    ar & BOOST_SERIALIZATION_NVP(m_dbTpsArretRestant);
    ar & BOOST_SERIALIZATION_NVP(m_nVoieInsertion);

    ar & BOOST_SERIALIZATION_NVP(m_bTypeChgtVoie);
    ar & BOOST_SERIALIZATION_NVP(m_TypeChgtVoie);
    ar & BOOST_SERIALIZATION_NVP(m_bVoieCible);
    ar & BOOST_SERIALIZATION_NVP(m_nVoieCible);
    ar & BOOST_SERIALIZATION_NVP(m_bPi);
    ar & BOOST_SERIALIZATION_NVP(m_dbPi);
    ar & BOOST_SERIALIZATION_NVP(m_bPhi);
    ar & BOOST_SERIALIZATION_NVP(m_dbPhi);
    ar & BOOST_SERIALIZATION_NVP(m_bRand);
    ar & BOOST_SERIALIZATION_NVP(m_dbRand);

    ar & BOOST_SERIALIZATION_NVP(m_MouvementsInsertion);

    ar & BOOST_SERIALIZATION_NVP(m_SousType);

    ar & BOOST_SERIALIZATION_NVP(m_dbDeceleration);
    ar & BOOST_SERIALIZATION_NVP(m_bDeceleration);

    ar & BOOST_SERIALIZATION_NVP(m_dbCreationPosition);
    ar & BOOST_SERIALIZATION_NVP(m_dbGenerationPosition);

    ar & BOOST_SERIALIZATION_NVP(m_ExternalID);
    ar & BOOST_SERIALIZATION_NVP(m_bDejaCalcule);

    ar & BOOST_SERIALIZATION_NVP(m_pTrip);

    // Don't save all attribute if we are in light save mode
    if (!m_pReseau->m_bLightSavingMode) {

        ar & BOOST_SERIALIZATION_NVP(m_nID); 
        ar & BOOST_SERIALIZATION_NVP(m_pTypeVehicule);
        SerialiseTab<Archive, VoieMicro*>(ar, "m_pVoie", m_pVoie, nbItems);
        SerialiseTab<Archive, Tuyau*>(ar, "m_pTuyau", m_pTuyau, nbItems);

        ar & BOOST_SERIALIZATION_NVP(m_pNextVoie);
        ar & BOOST_SERIALIZATION_NVP(m_pNextTuyau);

        ar & BOOST_SERIALIZATION_NVP(m_pPrevVoie);
        int nbOct = Loi_emission::Nb_Octaves+1;
        SerialiseTab<Archive, double>(ar, "m_pWEmission", m_pWEmission, nbOct);

        ar & BOOST_SERIALIZATION_NVP(m_dbInstantCreation);
        ar & BOOST_SERIALIZATION_NVP(m_dbInstantEntree);
        ar & BOOST_SERIALIZATION_NVP(m_dbInstInsertion);
        ar & BOOST_SERIALIZATION_NVP(m_dbInstantSortie);

        ar & BOOST_SERIALIZATION_NVP(m_pGhostFollower);
        ar & BOOST_SERIALIZATION_NVP(m_pGhostLeader);
        ar & BOOST_SERIALIZATION_NVP(m_pGhostVoie);

        ar & BOOST_SERIALIZATION_NVP(m_pVoieOld);

        ar & BOOST_SERIALIZATION_NVP(m_TuyauxParcourus);

        //ar & BOOST_SERIALIZATION_NVP(m_pTrip);
        ar & BOOST_SERIALIZATION_NVP(m_pLastTripNode);
        ar & BOOST_SERIALIZATION_NVP(m_pFleet);
        ar & BOOST_SERIALIZATION_NVP(m_pFleetParameters);
        ar & BOOST_SERIALIZATION_NVP(m_pScheduleElement);

        ar & BOOST_SERIALIZATION_NVP(m_pCarFollowing);

        ar & BOOST_SERIALIZATION_NVP(m_LstUsedLanes);
        ar & BOOST_SERIALIZATION_NVP(m_LstVoiesSvt);

        ar & BOOST_SERIALIZATION_NVP(m_VehiculeDepasse);

        ar & BOOST_SERIALIZATION_NVP(m_NextVoieTirage);

        ar & BOOST_SERIALIZATION_NVP(m_pCurrentTuyauMeso);
        ar & BOOST_SERIALIZATION_NVP(m_timeCode);
        ar & BOOST_SERIALIZATION_NVP(m_pNextTuyauMeso);

        ar & BOOST_SERIALIZATION_NVP(m_lstReservoirs);
        //For now, we don't save connected vehicle in light saving mode
#ifdef USE_SYMUCOM
        ar & BOOST_SERIALIZATION_NVP(m_pConnectedVehicle);
#endif //USE_SYMUCOM

    }
    else {

        std::vector<int> listVoieNum;
        std::vector<std::string> listTuyauName;
        std::string strTypeVehicule;
        int iNextVoie;
        int iPrevVoie;
        int iVoieOld;
        std::string strNextVoieTuyau;
        std::string strPrevVoieTuyau;
        std::string strVoieOldTuyau;
        std::vector<std::string> listTuyauxParcourusName;
        std::string strNextTuyau;
        std::string strTripID;
        std::string strLastTripNodeID;
        int iFleetIndex = -1;

        if (Archive::is_saving::value)
        {
            for (int i = 0; i < nbItems; i++) {
                if (m_pVoie[i])
                    listVoieNum.push_back(m_pVoie[i]->GetNum());
                else
                    listVoieNum.push_back(-1);
                if (m_pTuyau[i])
                    listTuyauName.push_back(m_pTuyau[i]->GetLabel());
                else
                    listTuyauName.push_back("");
            }
            strTypeVehicule = m_pTypeVehicule->GetLabel(); 

            if (m_pNextVoie){
                iNextVoie = m_pNextVoie->GetNum();
                strNextVoieTuyau = m_pNextVoie->GetUnifiedID().m_sTuyauID;
            }
            else{
                iNextVoie = -1;
            }

            if (m_pPrevVoie){
                iPrevVoie = m_pPrevVoie->GetNum();
                strPrevVoieTuyau = m_pPrevVoie->GetUnifiedID().m_sTuyauID;
            }
            else{
                iPrevVoie = -1;
            }

            if (m_pVoieOld){
                iVoieOld = m_pVoieOld->GetNum();
                strVoieOldTuyau = m_pVoieOld->GetUnifiedID().m_sTuyauID;
            }
            else{
                iVoieOld = -1;
            }

            for (int i = 0; i < m_TuyauxParcourus.size(); i++) {
                listTuyauxParcourusName.push_back(m_TuyauxParcourus[i]->GetLabel());
            }

            if (m_pNextTuyau)
                strNextTuyau = m_pNextTuyau->GetLabel();
            else
                strNextTuyau = "";

            if (m_pLastTripNode){
                strLastTripNodeID = m_pLastTripNode->GetID();
            }
            else{
                strLastTripNodeID = "";
            }

            if (dynamic_cast<SymuViaFleet*>(m_pFleet)){
                iFleetIndex = 0;
            }
            else if (dynamic_cast<PublicTransportFleet*>(m_pFleet)){
                iFleetIndex = 1;
            }
            else if (dynamic_cast<DeliveryFleet*>(m_pFleet)){
                iFleetIndex = 2;
            }
        }
        ar & BOOST_SERIALIZATION_NVP(listVoieNum);
        ar & BOOST_SERIALIZATION_NVP(listTuyauName);

        ar & BOOST_SERIALIZATION_NVP(strTypeVehicule);

        ar & BOOST_SERIALIZATION_NVP(iNextVoie);
        ar & BOOST_SERIALIZATION_NVP(strNextVoieTuyau);
        ar & BOOST_SERIALIZATION_NVP(iPrevVoie);
        ar & BOOST_SERIALIZATION_NVP(strPrevVoieTuyau);
        ar & BOOST_SERIALIZATION_NVP(iVoieOld);
        ar & BOOST_SERIALIZATION_NVP(strVoieOldTuyau);

        ar & BOOST_SERIALIZATION_NVP(listTuyauxParcourusName);

        ar & BOOST_SERIALIZATION_NVP(strNextTuyau);

        ar & BOOST_SERIALIZATION_NVP(m_pTrip);
        ar & BOOST_SERIALIZATION_NVP(strLastTripNodeID);

        ar & BOOST_SERIALIZATION_NVP(iFleetIndex);

        if (Archive::is_loading::value)
        {
            m_nID = m_pReseau->IncLastIdVeh();
            m_pTypeVehicule = m_pReseau->GetVehicleTypeFromID(strTypeVehicule);

            //Reloading simulation from intant 0
            m_dbInstantCreation = -1.0;
            m_dbInstantEntree = 1.0;
            m_dbInstInsertion = 1.0;
            m_dbInstantSortie = -1.0;

            if (strNextTuyau != ""){
                m_pNextTuyau = m_pReseau->m_mapTuyaux[strNextTuyau];
            }
            else{
                m_pNextTuyau = NULL;
            }

            m_pTuyau = new Tuyau*[listTuyauName.size()];
            m_pVoie = new VoieMicro*[listVoieNum.size()];
            for (int i = 0; i < listTuyauName.size(); i++){
                if (listTuyauName[i] == ""){
                    m_pTuyau[i] = NULL; 
                }
                else{
                    m_pTuyau[i] = m_pReseau->m_mapTuyaux[listTuyauName[i]];
                }
                if (listVoieNum[i] == -1 || listTuyauName[i] == ""){
                    m_pVoie[i] = NULL;
                }
                else{
                    m_pVoie[i] = (VoieMicro*)(m_pTuyau[i]->GetVoie(listVoieNum[i]));
                }
            }

            for (int i = 0; i < listTuyauxParcourusName.size(); i++){
                if (listTuyauxParcourusName[i] == ""){
                    m_TuyauxParcourus.push_back(NULL);
                }
                else{
                    m_TuyauxParcourus.push_back(m_pReseau->m_mapTuyaux[listTuyauxParcourusName[i]]);
                }
            }

            if (iNextVoie == -1){
                m_pNextVoie = NULL;
            }
            else{
                m_pNextVoie = (VoieMicro*)(m_pReseau->m_mapTuyaux[strNextVoieTuyau]->GetVoie(iNextVoie));
            }

            if (iPrevVoie == -1){
                m_pPrevVoie = NULL;
            }
            else{
                m_pPrevVoie = (VoieMicro*)(m_pReseau->m_mapTuyaux[strPrevVoieTuyau]->GetVoie(iPrevVoie));
            }

            if (iVoieOld == -1){
                m_pVoieOld = NULL;
            }
            else{
                m_pVoieOld = m_pReseau->m_mapTuyaux[strVoieOldTuyau]->GetVoie(iVoieOld);
            }

            if (iFleetIndex == -1){
                m_pFleet = NULL;
            }
            else{
                m_pFleet = m_pReseau->m_LstFleets[iFleetIndex];
            }

            // Only for vehicle associated to a fleet
            if (strLastTripNodeID == "" || !m_pTrip){
                m_pLastTripNode = NULL;
            }
            else{
                m_pLastTripNode = m_pFleet->GetTripNode(strLastTripNodeID);
            }

            m_pCarFollowing = m_pReseau->GetCarFollowingFactory()->buildCarFollowing(this);

            // Détermination du la distance jusqu'à laquelle construire le contexte de calul de la loi de poursuite du véhicule
            double contextRange = ComputeContextRange(m_pReseau->GetTimeStep());

            // Construction du nouveau contexte pour le pas de temps à calculer
            m_pCarFollowing->BuildContext(0.0, contextRange);


        } // end is_loading
    } // end if(!isLightSavingMode)
}

