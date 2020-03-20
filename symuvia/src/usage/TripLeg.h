#pragma once
#ifndef TripLegH
#define TripLegH

#include <vector>

namespace boost {
    namespace serialization {
        class access;
    }
}

class TripNode;
class Tuyau;
class VoieMicro;

class TripLeg
{
public:
    TripLeg();
    virtual ~TripLeg();

    void CopyTo(TripLeg * pTripLeg);

    void SetDestination(TripNode * pDestination);
    TripNode * GetDestination();

    void SetPath(const std::vector<Tuyau*> & path);
    const std::vector<Tuyau*> & GetPath();

    void SetCurrentLinkInLegIndex(int currentLinkInLegIndex);

    void SetLinkUsed(Tuyau * pLink);

    std::vector<Tuyau*> GetRemainingPath();
    std::vector<Tuyau*> GetUsedPath();

    // Test de l'atteinte de la destination
    virtual bool IsDone(double dbInstant, VoieMicro * pLane, double laneLength, double startPositionOnLane, double endPositionOnLane, bool bExactPosition);

private:
    TripNode           *m_pDestination;
    std::vector<Tuyau*> m_Path;

    int                 m_CurrentLinkInLegIndex;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // TripLegH