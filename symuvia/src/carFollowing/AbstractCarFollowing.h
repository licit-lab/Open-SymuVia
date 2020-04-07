#pragma once
#ifndef AbstractCarFollowingH
#define AbstractCarFollowingH

#include <boost/shared_ptr.hpp>

namespace boost {
    namespace serialization {
        class access;
    }
    namespace python {
        class dict;
    }
}

class Reseau;
class CarFollowingContext;
class Vehicule;
class VoieMicro;
class PointDeConvergence;
class Tuyau;

class AbstractCarFollowing
{
public:
    enum EStateVariable {
        STATE_VARIABLE_POSITION = 0,
        STATE_VARIABLE_SPEED,
        STATE_VARIABLE_ACCELERATION
    };

public:
    AbstractCarFollowing();
    virtual ~AbstractCarFollowing();

    // Indique ce qui est calcul� par la loi de poursuite (position, vitesse, acc�l�ration ...)
    virtual bool IsPositionComputed() = 0;
    virtual bool IsSpeedComputed() = 0;
    virtual bool IsAccelerationComputed() = 0;

    virtual void Initialize(Reseau * pNetwork, Vehicule * pVehicle);

    // Renvoie la longueur du v�hicule � prendre en compte pour la loi de poursuite
    virtual double GetVehicleLength();

    // Renvoie la distance d'influence maximum d'un leader
    virtual double GetMaxInfluenceRange() = 0;

    // Estime la vitesse du leader � la fin du pas de temps (n�cessaire pour toutes les lois de poursuite pour le "cassage des boucles" pour la loi de poursuite Newell)
    virtual double CalculVitesseApprochee(boost::shared_ptr<Vehicule> pVehicle, boost::shared_ptr<Vehicule> pVehLeader, double dbDistanceBetweenVehicles) = 0;

    // Construction du contexte utile au calcul de la loi de poursuite
    virtual void BuildContext(double dbInstant, double contextRange);

    // Calcul de la loi de poursuite
    virtual void Compute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext = NULL);

    // Instanciation d'un objet contexte en fonction du type de loi
    virtual CarFollowingContext * CreateContext(double dbInstant, bool bIsPostProcessing = false);

    // Proc�dure d'agressivit�
    virtual void CalculAgressivite(double dbInstant);

    // D�termine si le changement de voie doit avoir lieu
    virtual bool TestLaneChange(double dbInstant, boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehLeaderOrig,
        VoieMicro * pVoieCible, bool bForce, bool bRabattement, bool bInsertion);

    // Effectue le changement de voie pour la loi de poursuite
    virtual void ApplyLaneChange(VoieMicro * pVoieOld, VoieMicro * pVoieCible, boost::shared_ptr<Vehicule> pVehLeader,
        boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeaderOrig);

    // D�termine si l'insertion doit avoir lieu
    virtual bool TestConvergentInsertion(PointDeConvergence * pPointDeConvergence, Tuyau *pTAmP, VoieMicro* pVAmP, VoieMicro* pVAmNP, int nVoieGir, double dbInstant, int j, double dbDemande1, double dbDemande2, double dbOffre, 
        boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehFollower, double & dbTr, double & dbTa);

    // Estime la vitesse d'une voie cible donn�e
    virtual double ComputeLaneSpeed(VoieMicro * pTargetLane) = 0;

    // Accesseurs
    virtual CarFollowingContext* GetCurrentContext();
    virtual void SetCurrentContext(CarFollowingContext *pContext);
    virtual CarFollowingContext* GetPreviousContext();
    virtual double GetComputedTravelledDistance() const;
    virtual double GetComputedTravelSpeed() const;
    virtual double GetComputedTravelAcceleration() const;

    virtual void SetComputedTravelledDistance(double dbTravelledDistance);
    virtual void SetComputedTravelSpeed(double dbTravelSpeed);
    virtual void SetComputedTravelAcceleration(double dbTravelAcceleration);

    virtual Reseau * GetNetwork();
    virtual Vehicule* GetVehicle();

protected:
    // Calcul interne de la loi de poursuite (application de la formule associ�e pour calculer la position, vitesse et/ou acc�l�ration)
    virtual void InternalCompute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext) = 0;

protected:
    // R�seau associ�
    Reseau                     *m_pNetwork;

    // V�hicule associ�
    Vehicule                    *m_pVehicle;

    // Contexte du pas de temps pr�c�dent
    CarFollowingContext        *m_pPrevContext;

    // Context du pas de temps courant
    CarFollowingContext        *m_pContext;

    // Grandeurs calcul�es par la loi de poursuite
    double                      m_dbComputedTravelledDistance;
    double                      m_dbComputedTravelSpeed;
    double                      m_dbComputedTravelAcceleration;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // AbstractCarFollowingH
