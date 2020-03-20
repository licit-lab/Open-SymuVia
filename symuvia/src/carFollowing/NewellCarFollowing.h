#pragma once
#ifndef NewellCarFollowingH
#define NewellCarFollowingH

#include "AbstractCarFollowing.h"

class NewellContext;
class NewellCarFollowing : public AbstractCarFollowing
{
public:
    NewellCarFollowing();
    virtual ~NewellCarFollowing();

    virtual CarFollowingContext * CreateContext(double dbInstant, bool bIsPostProcessing = false);

    virtual bool IsPositionComputed();
    virtual bool IsSpeedComputed();
    virtual bool IsAccelerationComputed();

    virtual double GetMaxInfluenceRange();

    // OTK - LOI - Question : voir si on laisse cette méthode, ou bien si on estime la vitesse du leader avec la loi Newell, ce qui est à présent
    // possible puisqu'on définit le diagramme fondamental Newell pour tous les véhicules.
    virtual double CalculVitesseApprochee(boost::shared_ptr<Vehicule> pVehicle, boost::shared_ptr<Vehicule> pVehLeader, double dbDistanceBetweenVehicles);

    virtual void CalculAgressivite(double dbInstant);

    virtual void ApplyLaneChange(VoieMicro * pVoieOld, VoieMicro * pVoieCible, boost::shared_ptr<Vehicule> pVehLeader,
        boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeaderOrig);

    virtual double ComputeLaneSpeed(VoieMicro * pTargetLane);

protected:
    virtual void InternalCompute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext);

private:
    double ComputeFreeFlowDistance(double dbTimeStep, double dbInstant, CarFollowingContext * pContext);
    double ComputeCongestedDistance(double dbInstant, double dbTimeStep, CarFollowingContext * pContext);

    double ComputeRelaxationEvolution(double dbCurrentValue, CarFollowingContext * pContext, boost::shared_ptr<Vehicule> pVehLeader, double dbLeaderSpeed, double dbTimeStep);

    double ComputeJorgeEta(double dbInstant, NewellContext * pContext);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // NewellCarFollowingH
