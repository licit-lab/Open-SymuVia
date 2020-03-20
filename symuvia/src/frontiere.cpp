#include "stdafx.h"
#include "frontiere.h"

#include "SerializeUtil.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <float.h>

//================================================================
    Frontiere::Frontiere
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 03/07/2006
// Historique: 03/07/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    int     nNbPasTempsHist,    // Nombre de pas de temps historisé
    double  dbVitInit           // Valeur d'initialisation de la vitesse
)
{
    m_nNbPasTempsHist = nNbPasTempsHist;             

    // Allocation des tableau des variables caractéristiques du trafic
    m_pN   = new double[m_nNbPasTempsHist];
    m_pVit = new double[m_nNbPasTempsHist];
    m_pAcc = new double[m_nNbPasTempsHist];

    InitSimu(dbVitInit);
}

//================================================================
    void Frontiere::InitSimu
//----------------------------------------------------------------
// Fonction  : Initialisation des variables de simulation
// Version du: 13/07/2007
// Historique: 13/07/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double  dbVitInit           // Valeur d'initialisation de la vitesse
)
{
    for( int i=0; i<m_nNbPasTempsHist; i++)
    {
        m_pN[i]     = 0;
        m_pVit[i]   = dbVitInit;
        m_pAcc[i]   = 0;
    }
}

//================================================================
    Frontiere::~Frontiere
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 03/07/2006
// Historique: 03/07/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    if(m_pN)
        delete [] m_pN;

    if(m_pVit)
        delete [] m_pVit;

    if(m_pAcc)
        delete [] m_pAcc;
}

//================================================================
    double Frontiere::GetN
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
    if(!m_pN)
        return DBL_MAX;
        
    if( nDiffTemps > m_nNbPasTempsHist )
        return DBL_MAX;

    return m_pN[nDiffTemps];
}

//================================================================
    double Frontiere::GetVit
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
    double Frontiere::GetAcc
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
    void Frontiere::DecalVarTrafic
//----------------------------------------------------------------
// Fonction  :  Remet à jour les valeurs des variables caractéristiques
//              du trafic de façon à ce que la première valeur 
//              corresponde à l'instant traité et la dernière valeur à
//              l'instant ( Instant - PasTemps* |w|) où w est la pente
//              de remontée de congestion du diagramme fondamental
// Version du:  29/06/2006
// Historique:  29/06/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
)
{
    int i;


    if(!m_pN & !m_pVit & !m_pAcc)
        return;

    for(i=m_nNbPasTempsHist-2;i>=0;i--)
    {
        m_pN[i+1] = m_pN[i];                // Quantité N
        m_pVit[i+1] = m_pVit[i];            // Vitesse
        m_pAcc[i+1] = m_pAcc[i];            // Accélération
    }

    m_pN[0]     = 0;
    m_pVit[0]   = 0;
    m_pAcc[0]   = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Frontiere::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Frontiere::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Frontiere::serialize(Archive & ar, const unsigned int version)
{
	SerialiseTab<Archive, double>(ar, "N", m_pN, m_nNbPasTempsHist);
	SerialiseTab<Archive, double>(ar, "Vit", m_pVit, m_nNbPasTempsHist);
	SerialiseTab<Archive, double>(ar, "Acc", m_pAcc, m_nNbPasTempsHist);
}
