#pragma once

#ifndef _SYMUMASTER_ASSIGNMENTMODEL_
#define _SYMUMASTER_ASSIGNMENTMODEL_

#include "SymuMasterExports.h"

#include <xercesc/util/XercesVersion.hpp>
#include <Utils/SymuCoreConstants.h>

#include <Demand/SubPopulation.h>
#include <Demand/Destination.h>
#include <Demand/Origin.h>

#include <vector>
#include <map>

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace SymuCore {
    class Trip;
    class Origin;
    class Destination;
    class Path;
    class Population;
    class SubPopulation;
    class ShortestPathsComputer;
}

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuMaster {

class TraficSimulators;
class SimulationConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF AssignmentModel {

    public:

        struct SortSubPopulation{
            bool operator()(const SymuCore::SubPopulation* p1, const SymuCore::SubPopulation* p2 ) const {return p1->GetStrName() < p2->GetStrName();}
        };

        struct SortOrigin{
            bool operator()(const SymuCore::Origin* p1, const SymuCore::Origin* p2 ) const {return p1->getStrNodeName() < p2->getStrNodeName();}
        };

        struct SortDestination{
            bool operator()(const SymuCore::Destination* p1, const SymuCore::Destination* p2 ) const {return p1->getStrNodeName() < p2->getStrNodeName();}
        };


        AssignmentModel();
        AssignmentModel(SimulationConfiguration* pSimulationConfiguration, const boost::posix_time::ptime& dtStartSimulationTime, const boost::posix_time::ptime& dtEndSimulationTime);

        virtual ~AssignmentModel();

        virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pAssignmentConfigNode, std::string& predefinedPathsFilePath) = 0;

        virtual bool Initialize(const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, std::vector<SymuCore::Population*> &listPopulations, std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, bool bDayToDayAffectation, bool bUseCostsForTemporalShortestPath) = 0;

        virtual bool NeedsParallelSimulationInstances() { return false; }
        virtual int  GetMaxSimulationInstancesNumber() { return 1; }
        virtual bool GetNeededSimulationInstancesNumber(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int iCurrentIterationIndex, int iPeriod, int & nbSimulationInstances) { nbSimulationInstances = 1; return true; }
        virtual bool AssignPathsToTrips(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int& iCurrentIterationIndex, int iPeriod, int iSimulationInstance, int iSimulationInstanceForBatch, bool bAssignAllToFinalSolution) = 0;

        virtual bool IsRollbackNeeded(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startTime, const boost::posix_time::time_duration &assignmentPeriod, int iCurrentIterationIndex, int nbParallelSimulationMaxNumber, bool& bOk) = 0;

        virtual bool onSimulationInstanceDone(const std::vector<SymuCore::Trip *> &listTrip, const boost::posix_time::ptime & endPeriodTime, int iSimulationInstance, int iSimulationInstanceForBatch, int iCurrentIterationIndex) { return true; }

        void SetStartSimulationTime(const boost::posix_time::ptime &startSimulationTime);
        void SetEndSimulationTime(const boost::posix_time::ptime &dtEndSimulationTime);

        std::map<SymuCore::SubPopulation *, std::map<SymuCore::Origin *, std::map<SymuCore::Destination *, std::map<SymuCore::Path, std::vector<double> >, SortDestination >, SortOrigin >, SortSubPopulation >& GetListPredefinedPaths();

        const SimulationConfiguration *GetSimulationConfiguration() const;
        void SetSimulationConfiguration(const SimulationConfiguration *pSimulationConfiguration);


    protected:

        boost::posix_time::ptime            m_dtStartSimulationTime;

        boost::posix_time::ptime            m_dtEndSimulationTime;

        const SimulationConfiguration*      m_SimulationConfiguration;

        std::map<SymuCore::SubPopulation *, std::map<SymuCore::Origin *, std::map<SymuCore::Destination *, std::map<SymuCore::Path, std::vector<double> >, SortDestination >, SortOrigin >, SortSubPopulation > m_mapPredefinedPaths; //list of paths that a user have to use for an specific OD and subpopulation

    };

}

#pragma warning(pop)

#endif // _SYMUMASTER_ASSIGNMENTMODEL_
