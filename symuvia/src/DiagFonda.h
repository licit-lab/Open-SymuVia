#pragma once
#ifndef DiagFondaH
#define DiagFondaH  

namespace boost {
    namespace serialization {
        class access;
    }
}

#include <fstream>   

//------------------------------------------------------------------------------
//  Code des types de calcul
//------------------------------------------------------------------------------
    const int CALCUL_DEBIT        = 1;            // Débit
    const int CALCUL_OFFRE        = 2;            // Offre
    const int CALCUL_DEMANDE      = 3;            // Demande
//------------------------------------------------------------------------------
//  Code de retour des méthodes de calcul
//------------------------------------------------------------------------------
    const int ERR_ERREUR        = 0;            // Echec
    const int ERR_SUCCES        = 1;            // Succès
    const int ERR_NONTRAITE     = 2;            // Cas non traité - pas de résultat
    const int ERR_DIVZERO       = 3;            // Division par zéro
    const int ERR_DONNEES       = 4;            // Incohérence dans les données
//------------------------------------------------------------------------------
    const int VAL_NONDEF        = -99999;       // Valeur non définie

class CDiagrammeFondamental
{

protected:

    // Données du DF
    double      m_dbVitesseLibre;       // vitesse libre   
    double      m_dbKMax;               // Concentration maximum
    double      m_dbW;                  // W

    // Variables calculées
    double      m_dbDebitMax;           // débit maximum    
    double      m_dbKCritique;          // Concentration critique    

	std::ofstream   *m_pFicLog;              // Fichier log
	std::string     m_strContexte;    // Contexte (chaîne de caractère permettant de récupérer
                                        // le contexte lorsque une erreur est tracée dans le fichier log
    
public:
        // Constructeur
        CDiagrammeFondamental       ();
        CDiagrammeFondamental     (	
                                        double      dbW,										
										double		dbKMax,                                                                             
                                        double      dbVitMax,                                                                                                                        
										std::ofstream    *pFicLog
                                    );		

        // Méthodes Get     
        double  GetW()                  {return m_dbW;};
        double  GetVitesseLibre()       {return m_dbVitesseLibre;};       
        double  GetDebitMax()           {return m_dbDebitMax;};
        double  GetKMax()               {return m_dbKMax;};        
        double  GetKCritique()          {return m_dbKCritique;};

		void	SetProperties			(	
                                        double      dbW,										
										double		dbKMax,                                                                             
                                        double      dbVitMax                                                                                                                										
										);

        // Méthodes Set
		void    SetContexte(std::string sContexte);

        // Calcul du débit
        int     CalculDebitEqDiagrOrig(
                                        double &dbConcentration,
                                        double &dbDebit,
                                        int    nCalcul = CALCUL_DEBIT
                                        );            

        // Calcul de la concentration
        int     CalculConcEqDiagrOrig(
                                        double  &dbConcentration,
                                        double  &dbDebit,
                                        bool    bFluide
                                    );

          
        // Calcul de la vitesse
        int     CalculVitEqDiagrOrig(
                                        double  &dbVitesse,
                                        double  &dbConcentration,
                                        double  &dbDebit,
                                        bool    bFluide = true
                                        );                

private:

        // Trace
        void    TraceErreur(const char strMethode[64], const char strComment[1024]);     

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif

