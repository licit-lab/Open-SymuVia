#pragma once
#ifndef repartiteurH
#define repartiteurH

#include "ConnectionPonctuel.h"

class Tuyau;

// La classe répartiteur est une classe fille de connection. Le répartiteur gère
// les échanges de flux entre les différents tronçons situés amont et aval.

class Repartiteur :  public ConnectionPonctuel
{
private:

        // Variables caractéristiques du répartiteur

	    char m_cType;       // Type de répartiteur (M: macro (flux) ; H: microscopique)

        // Paramètres géométriques du répartiteur        
        double rayon; // rayon du répartiteur        

        // Variables de simulation
        std::vector<std::vector<double> > debit_entree; // Tableau des débits pour chaque entrée du répartiteur
        std::vector<std::vector<double> > debit_sortie; // Tableau des débits pour chaque sortie du répartiteur
        std::vector<std::vector<bool> > sortie_j_fluide; // Tableau de booléans indiquant si les sorties sont dans un état fluide (true) ou congestionné (false)
        std::vector<std::vector<bool> > entree_i_fluide; // Tableau de booléans indiquant si les entrées sont dans un état fluide (true) ou congestionné (false)

        int     m_NbEltAmont; // taille des double buffers
        int     m_NbEltAval; // taille des double buffers

        std::vector<std::vector<double> > Vmax; // Tableau des vitesses maximales autorisées au niveau de chacune des sorties du répartiteur

public:

    // Constructeurs, destructeurs et assimilés
    virtual ~Repartiteur(void); // Destructeur
    Repartiteur(void) ; // Constructeur par défaut
    Repartiteur(std::string, char _Type, Reseau *pR) ; 
	Repartiteur(std::string ,char, double, double, double, double, double,Reseau *pR); // Constructeur normal (Nom, Type, Abscisse, Ordonnee, Rayon)

    void Constructeur_complet(int,int, bool bInit = true); // constructeur complet du répartiteur (Nb elt amont, Nb elt aval)
                                            // son rôle est d'initialiser les tableaux dynamiques

    void    FinInit(Logger *pChargement);

    // création automatique des mouvements autorisés pour un répartiteur généré automatiquement pour l'insertion
    void CreateMouvementsAutorises(int nbVoiesInsertionDroite, int nbVoiesInsertiongauche, const std::string & strTAvPrincipal);

    // Fonctions relatives à la simulation des répartiteurs
    void    InitSimuTrafic(void);        // Initialise la simualtion des répartiteurs
    void    CalculEcoulementConnections(double);    // Simulation à chaque pas de temps (instant de la simulation , pas de temps, debit cumule ?)
    void    FinSimuTrafic();						// Termine la simulation des répartiteurs

    void    CalculVitMaxSortie(double  dbTime);               // Calcul des vitesses maximales au niveau des sorties

    // Fonctions liées à la gestion des autobus
    bool FeuOk(Tuyau*, int);   // retourne true si le feu est vert pour passer d'un troncon a un autre (pointeur troncon amont, pointeur troncon aval)

    // Fonctions de renvoi de l'état du répartiteur
    double Debit_entree(std::string, int nVoie); // renvoi du débit entrant provenant un troncon donné (nom troncon)
    double Debit_sortie(std::string, int nVoie); // renvoi du débit sortant vers un troncon donné (nom troncon)
    bool get_entree_fluide(std::string, int nVoie); // renvoi de l'état de l'entrée pour un troncon donné (nom troncon)
    bool get_sortie_fluide(std::string, int nVoie); // renvoi de l'état de la sortie pour un troncon donné (nom troncon)
    double getVitesseMaxEntree(std::string, int nVoie); // renvoi la vitesse maximum autorisée à la sortie avec le troncçon donné (nom troncon)

    // Fonctions utilitaires   
    void calculRayon(void); // Calcul le rayon du répartiteur     

    // Fonctions de renvoi des paramètres du répartiteur
    int  getNbAmont(void); // renvoi le nombre de tronçons en amont

    // Fonctions de modifications des paramètres du répartieur
    void SetType(char); // Affecte le type du répartiteur (type du répariteur P ponctuel, C complexe)
    void setNbAmont(int); // (Nombre de tronçons en amont)
    void setNbAval(int); // (Nombre de tronçons en aval)

    bool    IsFlux()    {return m_cType=='M';};
    bool    IsMicro()   {return m_cType=='H';};
        
    double GetVitesseMaxSortie(int n, int nVoie, double  dbTime);        // Retourne la vitesse maximale de sortie

    // Fonctions utilitaires
    void    AllocTabDblElt(double**   pTab, int nElt);
    void    DesallocTabDblElt(double**   pTab, int nElt);

    void    MiseAJourSAS(double dbInstant);

    void    IncNbVehRep(TypeVehicule* pTypeVeh, int i, int nVoieAmont, int j, int nVoieAval, double time);
    int     GetNbVehRep(TypeVehicule* pTypeVeh, int i, int nVoieAmont, int j, int nVoieAval, int nVar);

    //! Calcule le cout d'un mouvement autorisé
    virtual double ComputeCost(TypeVehicule* pTypeVeh, Tuyau* pTuyauAmont, Tuyau * pTuyauAval);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif
