#pragma once
#ifndef frontiereH
#define frontiereH

namespace boost {
    namespace serialization {
        class access;
    }
}

/*===========================================================================================*/
/* Classe de modélisation d'une frontière                                                    */
/*===========================================================================================*/

class Frontiere
{
public:
        // Constructeur
        Frontiere    (){};
        Frontiere    (int nNbPadTempsHist, double dbVitInit);

        // Destructeur
        ~Frontiere   ();

private:

        double *m_pN;               // pointeur sur le tableau de stockage de N
        double *m_pVit;             // pointeur sur le tableau de stockage de la vitesse
        double *m_pAcc;             // pointeur sur le tableau de stockage de l'accélération

        int     m_nNbPasTempsHist;  // Nombre de pas de temps historisé pour N

public:
        void    InitSimu(double dbVitInit);
        double  GetN    (int nDiffTemps);
        double  GetVit  (int nDiffTemps);
        double  GetAcc  (int nDiffTemps);

        double  GetLastN            (){return m_pN[m_nNbPasTempsHist-1];};
        double  GetBefLastN         (){return m_pN[m_nNbPasTempsHist-2];};
        double  GetLastVit          (){return m_pVit[m_nNbPasTempsHist-1];};
        double  GetBefLastVit       (){return m_pVit[m_nNbPasTempsHist-2];};

        int     GetNbPasTempsHist(){return m_nNbPasTempsHist;};

        void    DecalVarTrafic      ();

        void    SetN    (double dbN)    {m_pN[0]    = dbN;};
        void    SetVit  (double dbVit)  {m_pVit[0]  = dbVit;};
        void    SetAcc  (double dbAcc)  {m_pAcc[0]  = dbAcc;};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
    template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
#endif
 