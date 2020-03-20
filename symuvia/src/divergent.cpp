#include "stdafx.h"
#include "divergent.h"

#include "tuyau.h"
#include "BriqueDeConnexion.h"
#include "vehicule.h"
#include "voie.h"

//================================================================
    Divergent::Divergent
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{   
    m_pTuyauAmont = NULL;
}

//================================================================
    Divergent::Divergent
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	std::string strID,
    Reseau      *pReseau
): ConnectionPonctuel(strID, pReseau)
{       
    m_pTuyauAmont = NULL;
}    


//================================================================
    Divergent::~Divergent
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
}

//================================================================
    void Divergent::Init
//----------------------------------------------------------------
// Fonction  : Initialisation du divergent
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    Tuyau*      pTAm,               // Tuyau amont 
    Tuyau*      pTAvP,              // Tuyau aval principal
    Tuyau*      pTAvS               // Tuyau aval secondaire

)
{
    m_pTuyauAmont = pTAm;
//    m_pTuyauAvalPrincipal = pTAvP;
//    m_pTuyauAvalSecondaire = pTAvS;

    AddEltAmont(pTAm);
    AddEltAval(pTAvP);
    AddEltAval(pTAvS);
}

//================================================================
    void Divergent::Init
//----------------------------------------------------------------
// Fonction  : Initialisation du divergent
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
)
{
    m_pTuyauAmont = m_LstTuyAm.front();
    //m_pTuyauAvalPrincipal = m_LstTuyAv.front();
    //m_pTuyauAvalSecondaire = m_LstTuyAv[1];

}

//================================================================
    void Divergent::CorrigeAcceleration
//----------------------------------------------------------------
// Fonction  : Correction de l'accélération d'un véhicule
//             sortant d'un giratoire 
//             (amélioration du modèle acoustique)
// Version du: 23/05/2008
// Historique: 23/05/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	double dbInst
)
{
    boost::shared_ptr<Vehicule> pV;    

	if( m_LstTuyAv.size() == 0)
		return;

    // Divergent d'un giratoire ?
    if(m_LstTuyAv.front()->GetBriqueAmont())
    {
        if( m_LstTuyAv.front()->GetBriqueAmont()->GetType() == 'G' )
        {            

            for(int i=0; i < (int)m_LstInsVeh.size(); i++)
            {
                pV = m_LstInsVeh[i];

                if(pV && pV->GetLink(0))       
                {
                    if( pV->GetLink(0)->GetBriqueParente() == NULL ) // Le véhicule sort du giratoire
                        if( pV->IsRegimeFluide() )      // Correction uniquement en régime fluide
                            if( pV->GetVit(0) < pV->GetVitMax() && pV->GetVit(0) < pV->GetLink(0)->GetVitRegByTypeVeh(pV->GetType(), dbInst, pV->GetPos(0), pV->GetVoie(0)->GetNum()) )   // Correction si vitesse max non atteinte
                                pV->SetAcc( pV->GetAccMax( std::min<double>(pV->GetVitMax(), pV->GetLink(0)->GetVitRegByTypeVeh(pV->GetType(), dbInst, pV->GetPos(0), pV->GetVoie(0)->GetNum())) ) );  // Application de la correction
                }
            }
        }
    }    
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Divergent::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Divergent::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Divergent::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ConnectionPonctuel);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyauAmont);
}