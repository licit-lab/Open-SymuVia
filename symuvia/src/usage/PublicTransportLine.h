#pragma once
#ifndef PublicTransportLineH
#define PublicTransportLineH

#include "Trip.h"

#include <map>

class Arret;

class PublicTransportLineSnapshot : public TripSnapshot {

public:
    PublicTransportLineSnapshot();
    virtual ~PublicTransportLineSnapshot();

    std::map<std::pair<Arret*, Arret*>, double> latestDrivingTime;
    std::map<Arret*, double> latestStopDuration;
    std::map<Arret*, double> latestFrequencyAtStop;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class PublicTransportLine : public Trip
{
public:
    PublicTransportLine();
    virtual ~PublicTransportLine();

    void ComputeCosts();

    bool GetLastDrivingTimeBewteenStops(Arret * pUpstreamStop, Arret * pDownstreamStop, double & dbLastDrivingTime);
    double GetLastStopDuration(Arret * pStop);
    double GetLastFrequency(Arret * pStop);

    // méthode pour la sauvegarde et restitution de l'état des lignes de transport public (affectation dynamique convergente).
    virtual TripSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, TripSnapshot * backup);

protected:

    std::map<std::pair<Arret*, Arret*>, double> m_LatestDrivingTime;
    std::map<Arret*, double> m_LatestStopDuration;
    std::map<Arret*, double> m_LatestFrequencyAtStop;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


#endif // PublicTransportLineH