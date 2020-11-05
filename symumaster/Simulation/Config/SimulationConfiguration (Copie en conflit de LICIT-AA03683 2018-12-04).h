#pragma once

#ifndef _SYMUMASTER_SIMULATIONCONFIGURATION_
#define _SYMUMASTER_SIMULATIONCONFIGURATION_

#include "SymuMasterExports.h"

#include "SimulatorConfiguration.h"

#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <xercesc/util/XercesVersion.hpp>

#include <vector>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace SymuCore {
    class Populations;
    class Motive;
    class MacroType;
}

#pragma warning( push )  
#pragma warning( disable : 4251 )  

namespace SymuMaster {

    class AssignmentModel;

    class SYMUMASTER_EXPORTS_DLL_DEF SimulationConfiguration {

    public:
        SimulationConfiguration();

        virtual ~SimulationConfiguration();

        bool LoadFromXML(const std::string & xmlConfigFile);

        const std::string & GetLoadedConfigurationFilePath() const;

        void AddSimulatorConfig(SimulatorConfiguration * pSimulatorConfig);

        const std::vector<SimulatorConfiguration*> & GetLstSimulatorConfigs() const;

        void SetAssignmentPeriod(const boost::posix_time::time_duration & assignmentPeriod);
        boost::posix_time::time_duration GetAssignmentPeriod() const;

        void SetTravelTimesUpdatePeriod(const boost::posix_time::time_duration & travelTimeUpdatePeriod);
        boost::posix_time::time_duration GetTravelTimesUpdatePeriod() const;

        void SetPredictionPeriod(const boost::posix_time::time_duration & predictionPeriod);
        boost::posix_time::time_duration GetPredictionPeriod() const;

        const std::string & GetDemandFilePath() const;
        void SetDemandFilePath(const std::string &strDemandFilePath);

        const std::string& GetPredefinedPathsFilePath() const;
        void SetPredefinedPathsFilePath(const std::string &strPredefinedPathsFilePath);

        bool DoWorkAtParentODLevel() const;
        void DoWorkAtParentODLevel(bool bValue);

        const SymuCore::Populations * GetPopulations() const;

        void SetAssignmentModel(AssignmentModel * pAssignmentModel);
        AssignmentModel * GetAssignmentModel() const;

        std::vector<SymuCore::Motive *> GetListMotives() const;
        void SetListMotives(const std::vector<SymuCore::Motive *> &listMotives);
        SymuCore::Motive* GetMotivesByName(std::string strMotiveName);

        const std::string GetStrOutpuDirectoryPath() const;
        const std::string GetStrOutputPrefix() const;

        bool DoExtraIterationToProduceOutputs() const;

        bool bWritePpathAtEachIteration() const;

        bool bWriteCosts() const;
        bool bWriteCostsForEveryIteration() const;

        bool GetDayToDayAssignement() const;

        bool UseCostsForTemporalShortestPath() const;

    private:
        bool LoadSimulationXMLNode(XERCES_CPP_NAMESPACE::DOMNode * pSimulationNode);
        bool LoadRestitutionXMLNode(XERCES_CPP_NAMESPACE::DOMNode *pRestitutionNode);
        bool LoadDemandXMLNode(XERCES_CPP_NAMESPACE::DOMNode * pDemandNode);
        bool LoadPopulationsXMLNode(XERCES_CPP_NAMESPACE::DOMNode * pPopulationsNode);
        bool LoadAssignmentXMLNode(XERCES_CPP_NAMESPACE::DOMNode * pAssignmentNode);
        bool LoadSimulatorsXMLNode(XERCES_CPP_NAMESPACE::DOMNode * pSimulatorsNode);

        bool ConfigureOutputDirectory(const std::string &strOutputDir, const std::string &GetStrOutputPrefix);

    protected:

        std::string m_strLoadedConfigurationFilePath;

        std::vector<SimulatorConfiguration*> m_lstSimulatorConfigs;

        boost::posix_time::time_duration m_tdAssignmentPeriod;

        boost::posix_time::time_duration m_tdTravelTimesUpdatePeriod;

        boost::posix_time::time_duration m_tdPredictionPeriod;

        bool m_bDoWorkAtParentODLevel;

        bool m_bDayToDayAssignement;

        bool m_bUseCostsForTemporalShortestPath;

        std::string m_strDemandFilePath;

        std::string m_strPredefinedPathsFilePath;

        std::string m_strOutpuDirectoryPath;

        std::string m_strOutputPrefix;

        bool m_bDoExtraIterationToProduceOutputs;

        bool m_bWritePpathAtEachIteration;

        bool m_bWriteCosts;
        bool m_bWriteCostsForEveryIteration;

        SymuCore::Populations* m_pPopulations;

        std::vector<SymuCore::Motive*> m_listMotives;

        AssignmentModel * m_pAssignmentModel;
    };

}

#pragma warning( pop ) 

#endif // _SYMUMASTER_SIMULATIONRUNNER_
