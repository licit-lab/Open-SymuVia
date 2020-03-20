#pragma once

#ifndef SymuCoreDrivingTripsBuilder_H
#define SymuCoreDrivingTripsBuilder_H

class SymuCoreGraphBuilder;
class SymuCoreDrivingGraphBuilder;
class Reseau;
class SymuViaTripNode;
class TypeVehicule;

namespace SymuCore {
    class Populations;
    class SubPopulation;
    class MultiLayersGraph;
    class Trip;
    class VehicleType;
    class Population;

}

namespace boost {
    namespace posix_time {
        class ptime;
    }
}

#include <vector>

class SymuCoreDrivingTripsBuilder
{
public:

    SymuCoreDrivingTripsBuilder();

	//! Constructeur
    SymuCoreDrivingTripsBuilder(Reseau* pReseau);

	//! Destructeur
    virtual ~SymuCoreDrivingTripsBuilder();

    virtual bool FillPopulations(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
        const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime,
        bool bIgnoreSubAreas, std::vector<SymuCore::Trip> & lstTrips);

protected:

    void AddTrip(std::vector<SymuCore::Trip> & symuViaTrips, SymuCore::MultiLayersGraph * pGraph, const boost::posix_time::ptime & simulationStartTime, double dbCreationTime, SymuViaTripNode * pOrigin, SymuViaTripNode * pDestination, TypeVehicule * pTypeVeh, SymuCore::VehicleType * pVehType, SymuCore::SubPopulation * pSubPopulationForTrip,
        bool bIgnoreSubAreas);

protected:

    Reseau*                         m_pNetwork;

};

#endif // SymuCoreDrivingTripsBuilder_H


