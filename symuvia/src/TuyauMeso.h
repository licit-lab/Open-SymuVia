#pragma once

#include "tuyau.h"
#include "vehicule.h"
class Vehicule;
class ConnectionPonctuel;
class Reseau;

//! Définie un lien/tuyau/troncon pour un calcul mésoscopique
class CTuyauMeso :
	public TuyauMicro
{
public:

    CTuyauMeso();

	//! Constructeur
	CTuyauMeso( Reseau* pReseau, std::string _Nom,char _Type_amont ,char _Type_aval,ConnectionPonctuel *_p_aval,ConnectionPonctuel *_p_amont,
                char typevoie, std::string nom_rev, std::vector<double> larg_voie, double _Abscisse_amont,double _Ordonnee_amont,
                double _Abscisse_aval,double _Ordonnee_aval, double _Hauteur_amont, double _Hauteur_aval, int _Nb_voies,
                double pastemps,
                int nNbCellAcoustique, double dbVitReg, double dbVitCat, double dbCellAcousLength, const std::string & strRoadLabel,
                ETuyauType resolution);

	//! Destructeur
	virtual ~CTuyauMeso(void);

    // ** Méthodes hérités
    //! Retourne le type du troncon
    virtual ETuyauType GetType() const;

	// Fonctions utilitaires      

    void ComputeMeanW();
    void ComputeMeanK();
    double GetMeanK() const{return m_dCurrentMeanK; }
    double GetMeanW() const{return m_dCurrentMeanW;} 

    void AddCurrentVehicule( Vehicule * pVehicule);
    void RemoveCurrentVehicule(Vehicule * pVehicule);

    //! Retourne les couples vehicule, instant arrivant sur le troncon
    std::list< std::pair< double, Vehicule*> > GetArrivalTimes() const;

    //! Obtient le dernier temps d'arrivé
    double GetMaxArrivalTime() const;

  
    //! Obtient la prochaine arrivée d'un troncon
    std::pair<double,Vehicule* >  GetMinArrival() const;

     //! Obtient le prochain couple Vehicule, instant arrivant sur le troncon
    std::pair<double,Vehicule* >  GetNextArrival() const {return m_nextArrival; }

    //! Ajoute un couple vehicule, instant arrivant sur le troncon.
    void AddArrival(double dArrivalTime, Vehicule *pVehicule, bool bEnsureIsLastArrival);

    //! Vide les arrivés sur le troncon pTuyau de la connexion
    void ClearArrivals( );
   
    //! Défini le prochain couple instant à "passer" le connexion vers le tuyau 
    void SetNextSupplyTime(double dSupplyTime ){ m_dSupplyTime = dSupplyTime;}

    //! Obtient le prochain couple tuyau instant à "passer" le connexion vers le tuyau pTuyau
    double GetNextSupplyTime() const {return m_dSupplyTime; }

    //! Obtient le vehicule sur le troncon à l'indice iRank
    Vehicule* GetCurrentVehiculeAt(int iRank) const;

    //! Obtient le temps de passage aval pour le rang iRank
    double GetDownstreamPassingTime(int iRank) const;

    //! Obtient le temps de passage amont pour le rang iRank
    double GetUpstreamPassingTimeAt(int iRank) const;
    
    void RemoveFirstDownStreamPassingTime(  );

    void RemoveFirstUpstreamPassingTime(  );

    //! Ajoute un temps de passage sur un troncon amont
    void AddUpstreamPassingTime(  double dInstant  );

    //! Ajoute un temps de passage sur un troncon aval
    void AddDownstreamPassingTime(  double dInstant  );

    //! Obtient les temps de passages des vehicules entrant sur le pTuyau de la connexion
    std::deque<double> GetUpstreamPassingTimes( ) const;

    //! Remplace la liste des arrivées de vehicules sur un troncon de la connexion
    void ReplaceArrivalTimes(std::list<std::pair<double, Vehicule *>  >newList );

    //! Obtient le prochain vehicule entrant sur le noeud
    Vehicule * GetNextIncommingVehicle() const;

    //! Defini le prohain couple Vehicule, instant arrivant sur un troncon de la connexion
    void SetNextArrival( std::pair<double, Vehicule*> arrival);

    virtual void CalculTempsDeParcours( double dbInst, TypeVehicule *pTypeVeh, char cTypeEvent, double dbPeriode, char cTypeCalcul );
 
    bool IsCongested( ) const;

    void ToMicro();

    void ToMeso();

    // Sorties dans le fichier résultat
    void SortieTrafic(DocTrafic *pXMLDocTrafic);

    // Accesseurs pour la sauvegarde/restitution en cas d'affectation dynamique...
    std::deque<Vehicule *> & getCurrentVehicules() {return m_currentVehicules;}
    double getCurrentMeanW() {return m_dCurrentMeanW;}
    void setCurrentMeanW(double dCurrentMeanW) {m_dCurrentMeanW = dCurrentMeanW;}
    double getCurrentMeanK() {return m_dCurrentMeanK;}
    void setCurrentMeanK(double dCurrentMeanK) {m_dCurrentMeanK = dCurrentMeanK;}
    std::list< std::pair<double, Vehicule*> > & getArrivals() {return m_arrivals;}
    void setUpstreamPassingTimes(const std::deque<double > & upstreamPassingTimes) {m_upstreamPassingTimes = upstreamPassingTimes;}
    std::deque<double > & getDownstreamPassingTimes() {return m_downstreamPassingTimes;}

private:
	
    //! Liste ordonnée des vehicules présents sur le troncon
	std::deque<Vehicule *> m_currentVehicules;

	//! Moyenne courante des w des vehicules du troncon
	double m_dCurrentMeanW;

	//! Moyenne courante des k des vehicules du troncons
	double m_dCurrentMeanK;

    //! Temps d'arrivé des véhicules sur le troncon et le véhicule concerné
    std::list< std::pair<double, Vehicule*> >  m_arrivals;

    //! Prochain temps d'arrivé des véhicules sur le troncon et le véhicule concerné
    std::pair<double, Vehicule*> m_nextArrival;

    //! Temps de passages depuis la connexion amont 
    std::deque<double >  m_upstreamPassingTimes;
    //! Temps de passage depuis la connexion avale
    std::deque<double> m_downstreamPassingTimes;

    //! Temps de fourniture suivant au troncon
    double m_dSupplyTime;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);

};


