#include "stdafx.h"
#include "DiagFonda.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <float.h>
#include <math.h>

//================================================================
        CDiagrammeFondamental::CDiagrammeFondamental
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 01/12/2004
// Historique: 01/12/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	void
)
{    
    m_strContexte="";
    m_pFicLog = NULL;
}

		
//================================================================
        CDiagrammeFondamental::CDiagrammeFondamental
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 01/12/2004
// Historique: 01/12/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(    
    double      dbW,
    double      dbKMax,    
    double      dbVitMax,            
	std::ofstream    *pFicLog
)
{    
    m_dbW               = dbW;    
    m_dbKMax            = dbKMax;
    m_dbVitesseLibre    = dbVitMax;

    // Evaluation des variables calculées
    m_dbDebitMax = dbW * dbKMax / ( (dbW / dbVitMax) - 1);
    m_dbKCritique       = dbW * dbKMax / (dbW - dbVitMax) ;    
        
    m_pFicLog           = pFicLog;
}

//================================================================
		void CDiagrammeFondamental::SetProperties
//----------------------------------------------------------------
// Fonction  : Initialisation des propriétés
// Version du: 01/12/2004
// Historique: 01/12/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(    
    double      dbW,
    double      dbKMax,    
    double      dbVitMax	
)
{
    m_dbW               = dbW;    
    m_dbKMax            = dbKMax;
    m_dbVitesseLibre    = dbVitMax;

    // Evaluation des variables calculées
    m_dbDebitMax = dbW * dbKMax / ( (dbW / dbVitMax) - 1);
    m_dbKCritique       = dbW * dbKMax / (dbW - dbVitMax) ;    
        
}

//================================================================
    void CDiagrammeFondamental::SetContexte
//----------------------------------------------------------------
// Fonction  : Met à jour le contexte
// Version du: 11/01/2005
// Historique: 11/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	std::string strContexte
)
{
    m_strContexte = strContexte;
}

//================================================================
    void CDiagrammeFondamental::TraceErreur
//----------------------------------------------------------------
// Fonction  : Trace les erreurs dans le fichier log
// Version du: 11/01/2005
// Historique: 11/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    const char strMethode[64],
    const char strComment[1024]
)
{
    if( !m_pFicLog )
        return;	

	*m_pFicLog << "*** Error Fundamental Diagram object ***" << std::endl;
	*m_pFicLog << " Context : " << m_strContexte.c_str() << std::endl;
    *m_pFicLog << " Method  : " << strMethode    << std::endl;
    *m_pFicLog << " Comment. : " << strComment    << std::endl;
    *m_pFicLog << "******************************************" << std::endl;
}

//================================================================
    int CDiagrammeFondamental::CalculDebitEqDiagrOrig
//----------------------------------------------------------------
// Fonction  : Calcul du débit connaissant la concentration pour
//             un diagramme original
// Version du: 01/12/2004
// Historique: 01/12/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double &dbConcentration,
    double &dbDebit,
    int    nCalcul      /*= CALCUL_DEBIT*/
)
{
    dbDebit = VAL_NONDEF;    

    if( nCalcul == CALCUL_DEMANDE &&  dbConcentration > m_dbKCritique )
    {
        dbDebit = m_dbDebitMax;
        return ERR_SUCCES;
    }

    if( nCalcul == CALCUL_OFFRE &&  dbConcentration < m_dbKCritique )
    {
        dbDebit = m_dbDebitMax;
        return ERR_SUCCES;
    }

    // Conc <= Kc
    if( dbConcentration <= m_dbKCritique )
    {
        // Debit = Vc * Conc
        dbDebit = m_dbVitesseLibre * dbConcentration;

        if( fabs(dbDebit) < DBL_EPSILON)  
			dbDebit = 0;
        return ERR_SUCCES;
    }
    // Conc > Kc
    else
    {
        if(  fabs(m_dbKMax-m_dbKCritique) <= DBL_EPSILON )
        {
            TraceErreur("CalculDebitEqDiagrOrig", "division by zero during flow calculation");
            return ERR_DIVZERO;
        }

        // Debit = p * (conc - Kmax)
        dbDebit = m_dbW * (dbConcentration - m_dbKMax);

        if( fabs(dbDebit) < DBL_EPSILON)
			dbDebit = 0;

        return ERR_SUCCES;
    }
    
}


//================================================================
    int CDiagrammeFondamental::CalculConcEqDiagrOrig
//----------------------------------------------------------------
// Fonction  : Calcul de la concentration connaissant le débit pour
//             un diagramme original
// Version du: 01/12/2004
// Historique: 01/12/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double &dbConcentration,
    double &dbDebit,
    bool   bFluide              // Etat
)
{

    dbConcentration = VAL_NONDEF;

    if(bFluide)
    {
        // Conc = debit / Vc
        if( m_dbVitesseLibre < DBL_EPSILON)
        {
            TraceErreur("CalculConcEqDiagrOrig", "division by zero during concentration - Vc = 0 computation");
            return ERR_DIVZERO;
        }
        dbConcentration = dbDebit / m_dbVitesseLibre;
    }
    else
    {
        // Conc = Kmax + debit/p
        if( fabs(m_dbW) < DBL_EPSILON)
        {
            TraceErreur("CalculConcEqDiagrOrig", "division by zero during concentration - P = 0 computation");
            return ERR_DIVZERO;
        }
        dbConcentration = m_dbKMax + (dbDebit / m_dbW);
    }
   
    if( dbConcentration < 0 && fabs(dbConcentration)<DBL_EPSILON ) dbConcentration = 0;

    return ERR_SUCCES;
}


//================================================================
    int CDiagrammeFondamental::CalculVitEqDiagrOrig
//----------------------------------------------------------------
// Fonction  : Calcul de la vitesse connaissant la concentration
//             ou le débit pour un diagramme original
// Version du: 07/12/2004
// Historique: 07/12/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double  &dbVitesse,
    double  &dbConcentration,
    double  &dbDebit,
    bool    bFluide /* = true */
)
{
    int nRes;

    dbVitesse = VAL_NONDEF;

    if( dbConcentration == VAL_NONDEF)
        nRes = CalculConcEqDiagrOrig(dbConcentration, dbDebit,bFluide);
    else
        nRes = CalculDebitEqDiagrOrig(dbConcentration, dbDebit);

    if( nRes != ERR_SUCCES )
        return nRes;

    // Debit nul et état congestionné --> vitesse nulle
    if( fabs(dbDebit) < DBL_EPSILON && !bFluide)
    {
        dbVitesse = 0;
        return ERR_SUCCES;
    }

    // Debit nul et concentration nulle --> vitesse nulle
    if( fabs(dbDebit) < DBL_EPSILON && fabs(dbConcentration) < DBL_EPSILON )
    {
        dbVitesse = 0;
        return ERR_SUCCES;
    }


    if( dbConcentration < DBL_EPSILON )
    {
        dbVitesse = m_dbVitesseLibre;
    }
    else
    {
        dbVitesse = dbDebit / dbConcentration;
    }

    if( fabs(dbVitesse)< DBL_EPSILON )
        dbVitesse = 0;

    return ERR_SUCCES;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CDiagrammeFondamental::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CDiagrammeFondamental::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CDiagrammeFondamental::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_dbVitesseLibre);
    ar & BOOST_SERIALIZATION_NVP(m_dbKMax);
    ar & BOOST_SERIALIZATION_NVP(m_dbW);
    ar & BOOST_SERIALIZATION_NVP(m_dbDebitMax);
    ar & BOOST_SERIALIZATION_NVP(m_dbKCritique);
    //ar & BOOST_SERIALIZATION_NVP(m_pFicLog); remarque : m_pFicLog est toujours NULL : on se permet de ne pas le prendre en compte ici !
    ar & BOOST_SERIALIZATION_NVP(m_strContexte);
}