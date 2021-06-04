#pragma once

#ifndef _SYMUMASTER_SIMULATIONDESCRIPTOR_
#define _SYMUMASTER_SIMULATIONDESCRIPTOR_

#include "SymuMasterExports.h"

#include "SimulatorsInterface.h"
#include "Utils/SymuCoreConstants.h"

#include <Demand/Populations.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <map>

namespace SymuCore {
    class MultiLayersGraph;
    class SubPopulation;
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF SimulationDescriptor {

    public:
        SimulationDescriptor();

        virtual ~SimulationDescriptor();

        void SetStartTime(boost::posix_time::ptime startTime);
        boost::posix_time::ptime GetStartTime();

        void SetEndTime(boost::posix_time::ptime endTime);
        boost::posix_time::ptime GetEndTime();

        void SetTimeStepDuration(boost::posix_time::time_duration timeStep);
        boost::posix_time::time_duration GetTimeStepDuration();

        SymuCore::MultiLayersGraph * GetGraph();

        SymuCore::Populations& GetPopulations();
        const SymuCore::Populations& GetPopulations() const;
        void SetPopulations(const SymuCore::Populations &Populations);

        const std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation *> > &GetSubPopulationsByServiceType() const;
        std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation *> > &GetSubPopulationsByServiceType();
        void SetSubPopulationsByServiceType(const std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation *> > &mapSubPopulationsByServiceType);

        std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & GetSimulatorsInterfaces();
        const std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & GetSimulatorsInterfaces() const;

    protected:

        std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation*> > m_mapSubPopulationsByServiceType;

        boost::posix_time::ptime            m_StartTime;

        boost::posix_time::ptime            m_EndTime;

        boost::posix_time::time_duration    m_TimeStepDuration;

        SymuCore::MultiLayersGraph*         m_pGraph;

        SymuCore::Populations               m_Populations;

        // the first key is the downstream simulator, the second is the upstream simulator
        std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > m_mapSimulatorsInterfaces;

    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_SIMULATIONDESCRIPTOR_
