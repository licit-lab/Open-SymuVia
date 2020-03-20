#pragma once
#ifndef NewellContextH
#define NewellContextH

#include "CarFollowingContext.h"

class NewellContext : public CarFollowingContext
{
public:
    NewellContext();
    NewellContext(Reseau * pNetwork, Vehicule * pVehicle, double dbInstant, bool bIsPostProcessing);
    virtual ~NewellContext();

    virtual void Build(double dbRange, CarFollowingContext * pPreviousContext);

    virtual void SetContext(const std::vector<boost::shared_ptr<Vehicule> > & leaders,
                            const std::vector<double> & leaderDistances,
                            const std::vector<VoieMicro*> & lstLanes,
                            double dbStartPosition,
                            CarFollowingContext * pOriginalContext = NULL,
                            bool bRebuild = false);

    double GetDeltaN() const;
    void SetDeltaN(double dbDeltaN);

    bool IsContraction() const;
    void SetContraction(bool bContraction);

    bool IsJorge() const;
    void SetJorge(bool bJorge);

    double GetJorgeStartTime() const;
    void SetJorgeEndTime(double dbEndTime);

    double GetJorgeEndTime() const;
    void SetJorgeStartTime(double dbEndTime);

    virtual void CopyTo(CarFollowingContext * pDestinationContext);

private:
    void ComputeDeltaN(NewellContext * pPreviousContext);

private:
    // Valeur du deltaN
    double m_dbDeltaN;

    // Indique si le véhicule est en train de se contracter (comportement agressif)
    bool m_bContraction;

    // indique si le véhicule utilise l'extenion Jorge
    bool m_bJorge;
    double m_dbJorgeStartTime;
    double m_dbJorgeEndTime;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // NewellContextH
