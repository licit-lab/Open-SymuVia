#include "SimulationConfiguration.h"
#include "Launcher/ColouredConsoleLogger.h"

#include "IO/XMLLoader.h"
#include "IO/XMLStringUtils.h"
#include "Utils/SystemUtils.h"
#include "Simulation/Config/SimulatorConfigurationFactory.h"
#include "Simulation/Assignment/AssignmentModel.h"
#include "Simulation/Assignment/AssignmentModelFactory.h"

#include <Demand/MacroType.h>
#include <Demand/Populations.h>
#include <Demand/Population.h>
#include <Demand/SubPopulation.h>
#include <Demand/Motive.h>
#include <Demand/Path.h>
#include <Demand/VehicleType.h>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>

#include <fstream>


using namespace SymuMaster;

XERCES_CPP_NAMESPACE_USE

SimulationConfiguration::SimulationConfiguration()
{
    // default values...
    m_tdPredictionPeriod = boost::posix_time::minutes(20);
    m_tdAssignmentPeriod = boost::posix_time::minutes(15);
    m_tdTravelTimesUpdatePeriod = boost::posix_time::minutes(1);

    m_bDoWorkAtParentODLevel = false;

    m_pPopulations = NULL;
    m_pAssignmentModel = NULL;

    m_pPopulations = new SymuCore::Populations();

    m_bDoExtraIterationToProduceOutputs = true;
}

SimulationConfiguration::~SimulationConfiguration()
{
    if (m_pAssignmentModel)
    {
        delete m_pAssignmentModel;
    }

    for (size_t iConfig = 0; iConfig < m_lstSimulatorConfigs.size(); iConfig++)
    {
        delete m_lstSimulatorConfigs[iConfig];
    }

    // populations does not have owership of the macrotypes (graph has but we don't have it here)
    for (size_t iPop = 0; iPop < m_pPopulations->getListPopulations().size(); iPop++)
    {
        SymuCore::MacroType * pMacroType = m_pPopulations->getListPopulations()[iPop]->GetMacroType();
        if (pMacroType)
        {
            delete pMacroType;
        }
    }

    delete m_pPopulations;

    for (size_t iMotive = 0; iMotive < m_listMotives.size(); iMotive++)
    {
        delete m_listMotives[iMotive];
    }
}

bool SimulationConfiguration::LoadFromXML(const std::string & xmlConfigFile)
{
    BOOST_LOG_TRIVIAL(info) << "Using configuration file '" + xmlConfigFile << "'";

    XMLLoader configLoader;
    if (!configLoader.Load(xmlConfigFile, boost::filesystem::path(SystemUtils::GetCurrentModuleFolder()).operator/=(SymuMasterConstants::s_SYMUMASTER_CONFIG_XSD_NAME).string()))
    {
        BOOST_LOG_TRIVIAL(error) << "Failed loading configuration file '" << xmlConfigFile << "'";
        return false;
    }
    else
    {
        m_strLoadedConfigurationFilePath = xmlConfigFile;

        XERCES_CPP_NAMESPACE::DOMDocument * pConfigDoc = configLoader.getDocument();

        DOMNodeList * pRootChildren = pConfigDoc->getChildNodes();
        for (XMLSize_t iChild = 0; iChild < pRootChildren->getLength(); iChild++)
        {
            DOMNode * pRootNode = pRootChildren->item(iChild);

            if (US(pRootNode->getNodeName()) == "ROOT_SYMUMASTER")
            {
                // Checking configuration file version number
                std::string strVersion;
                XMLStringUtils::GetAttributeValue(pRootNode, "version", strVersion);
                if (strVersion != SymuMasterConstants::s_SYMUMASTER_CONFIG_XSD_VERSION)
                {
                    BOOST_LOG_TRIVIAL(error) << "Wrong configuration file version. Expecting " << SymuMasterConstants::s_SYMUMASTER_CONFIG_XSD_VERSION << ", got " << strVersion;
                    return false;
                }

                DOMNodeList * pRootChildren = pRootNode->getChildNodes();
                for (XMLSize_t iChild = 0; iChild < pRootChildren->getLength(); iChild++)
                {
                    DOMNode * pChildNode = pRootChildren->item(iChild);

                    const std::string & nodeName = US(pChildNode->getNodeName());
                    if (nodeName == "SIMULATION")
                    {
                        if (!LoadSimulationXMLNode(pChildNode))
                        {
                            return false;
                        }
                        break;
                    }
                }
                break;
            }
        }

        BOOST_LOG_TRIVIAL(info) << "Configuration file '" + xmlConfigFile + "' loaded successfuly";
        return true;
    }
}

bool SimulationConfiguration::LoadSimulationXMLNode(DOMNode * pSimulationNode)
{
    DOMNodeList * pSimulationChildren = pSimulationNode->getChildNodes();
    for (XMLSize_t iChild = 0; iChild < pSimulationChildren->getLength(); iChild++)
    {
        DOMNode * pChildNode = pSimulationChildren->item(iChild);

        const std::string & nodeName = US(pChildNode->getNodeName());
        if (nodeName == "RESTITUTION")
        {
            if (!LoadRestitutionXMLNode(pChildNode))
            {
                return false;
            }
        }
        else if (nodeName == "DEMAND")
        {
            if (!LoadDemandXMLNode(pChildNode))
            {
                return false;
            }
        }
        else if (nodeName == "ASSIGNMENT")
        {
            if (!LoadAssignmentXMLNode(pChildNode))
            {
                return false;
            }
        }
        else if (nodeName == "SIMULATORS")
        {
            if (!LoadSimulatorsXMLNode(pChildNode))
            {
                return false;
            }
        }
    }

    return true;
}

bool SimulationConfiguration::LoadRestitutionXMLNode(DOMNode * pRestitutionNode)
{
    std::string strOutpuDirectoryPath;
    XMLStringUtils::GetAttributeValue(pRestitutionNode, "directory_out", strOutpuDirectoryPath);

    std::string strOutputPrefix;
    XMLStringUtils::GetAttributeValue(pRestitutionNode, "prefix_out", strOutputPrefix);

    XMLStringUtils::GetAttributeValue(pRestitutionNode, "do_extra_iteration_to_produce_outputs", m_bDoExtraIterationToProduceOutputs);

    XMLStringUtils::GetAttributeValue(pRestitutionNode, "write_ppath", m_bWritePpathAtEachIteration);

    XMLStringUtils::GetAttributeValue(pRestitutionNode, "write_costs", m_bWriteCosts);
    XMLStringUtils::GetAttributeValue(pRestitutionNode, "write_costs_for_every_iteration", m_bWriteCostsForEveryIteration);
    

    XMLStringUtils::GetAttributeValue(pRestitutionNode, "write_costs", m_bWriteCosts);

    return ConfigureOutputDirectory(strOutpuDirectoryPath, strOutputPrefix);
}

bool SimulationConfiguration::LoadDemandXMLNode(DOMNode * pDemandNode)
{
    XMLStringUtils::GetAttributeValue(pDemandNode, "csv_file_path", m_strDemandFilePath);

    XMLStringUtils::GetAttributeValue(pDemandNode, "ignore_subODs", m_bDoWorkAtParentODLevel);

    DOMNodeList * pDemandChildren = pDemandNode->getChildNodes();
    for (XMLSize_t iChild = 0; iChild < pDemandChildren->getLength(); iChild++)
    {
        DOMNode * pChildNode = pDemandChildren->item(iChild);

        const std::string & nodeName = US(pChildNode->getNodeName());
        if (nodeName == "MOTIVES")
        {
            DOMNodeList * pMotivesChildren = pChildNode->getChildNodes();
            for (XMLSize_t iMotive = 0; iMotive < pMotivesChildren->getLength(); iMotive++)
            {
                DOMNode * pMotiveNode = pMotivesChildren->item(iMotive);

                const std::string & nodeName = US(pMotiveNode->getNodeName());
                if (nodeName == "MOTIVE")
                {
                    std::string motiveName;
                    XMLStringUtils::GetAttributeValue(pMotiveNode, "id", motiveName);

                    SymuCore::Motive* pMotive = new SymuCore::Motive(motiveName);
                    m_listMotives.push_back(pMotive);
                }
            }
        }
        else if (nodeName == "POPULATIONS")
        {
            if (!LoadPopulationsXMLNode(pChildNode))
            {
                return false;
            }
        }
    }
    return true;
}

bool SimulationConfiguration::LoadPopulationsXMLNode(XERCES_CPP_NAMESPACE::DOMNode * pPopulationsNode)
{
    DOMNodeList * pPopulationsChildren = pPopulationsNode->getChildNodes();
    for (XMLSize_t iChild = 0; iChild < pPopulationsChildren->getLength(); iChild++)
    {
        DOMNode * pPopulationNode = pPopulationsChildren->item(iChild);

        const std::string & nodeName = US(pPopulationNode->getNodeName());
        if (nodeName == "POPULATION")
        {
            std::string populationName;
            XMLStringUtils::GetAttributeValue(pPopulationNode, "id", populationName);

            SymuCore::Population * pPopulation = new SymuCore::Population(populationName);

            double populationInitialWalkSpeed;
            XMLStringUtils::GetAttributeValue(pPopulationNode, "initial_walk_speed", populationInitialWalkSpeed);
            pPopulation->SetInitialWalkSpeed(populationInitialWalkSpeed);

            double populationIntermediateWalkSpeed;
            XMLStringUtils::GetAttributeValue(pPopulationNode, "intermediate_walk_speed", populationIntermediateWalkSpeed);
            pPopulation->SetIntermediateWalkSpeed(populationIntermediateWalkSpeed);

            double populationInitialWalkRadius;
            XMLStringUtils::GetAttributeValue(pPopulationNode, "initial_walk_radius", populationInitialWalkRadius);
            pPopulation->SetInitialWalkRadius(populationInitialWalkRadius);

            double populationIntermediateWalkRadius;
            XMLStringUtils::GetAttributeValue(pPopulationNode, "intermediate_walk_radius", populationIntermediateWalkRadius);
            pPopulation->SetIntermediateWalkRadius(populationIntermediateWalkRadius);

            bool bDisableOptimization = false;
            XMLStringUtils::GetAttributeValue(pPopulationNode, "disable_optimization", bDisableOptimization);
            pPopulation->SetDisableOptimization(bDisableOptimization);
            

            m_pPopulations->getListPopulations().push_back(pPopulation);

            // getting the population's available service types
            DOMNodeList * pPopulationChildren = pPopulationNode->getChildNodes();
            for (XMLSize_t iPopulationChild = 0; iPopulationChild < pPopulationChildren->getLength(); iPopulationChild++)
            {
                DOMNode * pPopulationChild = pPopulationChildren->item(iPopulationChild);

                const std::string & nodeName = US(pPopulationChild->getNodeName());
                if (nodeName == "MACRO_TYPE")
                {
                    SymuCore::MacroType * pMacroType = new SymuCore::MacroType();
                    DOMNodeList * pMacroTypeChildren = pPopulationChild->getChildNodes();
                    for (XMLSize_t iVehicleType = 0; iVehicleType < pMacroTypeChildren->getLength(); iVehicleType++)
                    {
                        DOMNode * pVehicleType = pMacroTypeChildren->item(iVehicleType);

                        const std::string & nodeName = US(pVehicleType->getNodeName());
                        if (nodeName == "VEHICLE_TYPE")
                        {
                            std::string vehicleTypeName;
                            XMLStringUtils::GetAttributeValue(pVehicleType, "name", vehicleTypeName);

                            pMacroType->addVehicleType(new SymuCore::VehicleType(vehicleTypeName));
                        }
                    }
                    pPopulation->SetMacroType(pMacroType);
                }
                else if (nodeName == "AVAILABLE_SERVICES")
                {
                    DOMNodeList * pAvailableServicesChildren = pPopulationChild->getChildNodes();
                    for (XMLSize_t iAvailableService = 0; iAvailableService < pAvailableServicesChildren->getLength(); iAvailableService++)
                    {
                        DOMNode * pAvailableService = pAvailableServicesChildren->item(iAvailableService);

                        const std::string & nodeName = US(pAvailableService->getNodeName());
                        if (nodeName == "AVAILABLE_SERVICE")
                        {
                            std::string serviceName;
                            XMLStringUtils::GetAttributeValue(pAvailableService, "type", serviceName);

                            pPopulation->addServiceType(SymuMasterConstants::GetServiceTypeFromXMLTypeName(serviceName));
                        }
                    }
                }else if (nodeName == "SUB_POPULATIONS")
                {
                    DOMNodeList * pSubPopulationsChildren = pPopulationChild->getChildNodes();
                    for (XMLSize_t iSubPopulation = 0; iSubPopulation < pSubPopulationsChildren->getLength(); iSubPopulation++)
                    {
                        DOMNode * pSubPopulationNode = pSubPopulationsChildren->item(iSubPopulation);

                        const std::string & nodeName = US(pSubPopulationNode->getNodeName());
                        if (nodeName == "SUB_POPULATION")
                        {
                            std::string subPopulationName;
                            XMLStringUtils::GetAttributeValue(pSubPopulationNode, "id", subPopulationName);

                            SymuCore::SubPopulation* pSubPopulation = new SymuCore::SubPopulation(subPopulationName);

                            double subPopulationInitialWalkSpeed;
                            if (XMLStringUtils::GetAttributeValue(pSubPopulationNode, "initial_walk_speed", subPopulationInitialWalkSpeed))
                            {
                                pSubPopulation->SetInitialWalkSpeed(subPopulationInitialWalkSpeed);
                            }

                            double subPopulationInitialWalkRadius;
                            if (XMLStringUtils::GetAttributeValue(pSubPopulationNode, "initial_walk_radius", subPopulationInitialWalkRadius))
                            {
                                pSubPopulation->SetInitialWalkRadius(subPopulationInitialWalkRadius);
                            }

                            std::string subPopulationMotiveName;
                            XMLStringUtils::GetAttributeValue(pSubPopulationNode, "motive", subPopulationMotiveName);
                            SymuCore::Motive* pMotive = GetMotivesByName(subPopulationMotiveName);
                            pSubPopulation->SetMotive(pMotive);

                            pSubPopulation->SetPopulation(pPopulation);
                            pPopulation->GetListSubPopulations().push_back(pSubPopulation);
                        }
                    }
                }
            }

            // A population should have a macro type if it has access to Driving service
            if (!pPopulation->GetMacroType() && pPopulation->hasAccess(SymuCore::ST_Driving))
            {
                BOOST_LOG_TRIVIAL(error) << "The population '" << pPopulation->GetStrName() << "' has access to driving service, therefore should be associated to a Macro-Type of vehicles";
                return false;
            }
        }
    }

    return true;
}

bool SimulationConfiguration::LoadAssignmentXMLNode(DOMNode * pAssignmentNode)
{
    // getting common parameters :
    int iTmp;
    XMLStringUtils::GetAttributeValue(pAssignmentNode, "prediction_period", iTmp);
    m_tdPredictionPeriod = boost::posix_time::minutes(iTmp);
    XMLStringUtils::GetAttributeValue(pAssignmentNode, "assignment_period", iTmp);
    m_tdAssignmentPeriod = boost::posix_time::minutes(iTmp);
    XMLStringUtils::GetAttributeValue(pAssignmentNode, "traveltimes_update_period", iTmp);
    m_tdTravelTimesUpdatePeriod = boost::posix_time::minutes(iTmp);
    XMLStringUtils::GetAttributeValue(pAssignmentNode, "day_to_day_assignment", m_bDayToDayAssignement);
    XMLStringUtils::GetAttributeValue(pAssignmentNode, "use_costs_for_temporal_shortest_path", m_bUseCostsForTemporalShortestPath);


    // getting assignment model :
    DOMNodeList * pAssignmentModelNodes = pAssignmentNode->getChildNodes();
    for (XMLSize_t iChild = 0; iChild < pAssignmentModelNodes->getLength(); iChild++)
    {
        DOMNode * pAssignmentModelNode = pAssignmentModelNodes->item(iChild);

        if (pAssignmentModelNode->getNodeType() == DOMNode::ELEMENT_NODE)
        {
            const std::string & assignmentNodeName = US(pAssignmentModelNode->getNodeName());

            m_pAssignmentModel = AssignmentModelFactory::CreateAssignmentModel(assignmentNodeName);
            if (!m_pAssignmentModel->LoadFromXML(pAssignmentModelNode, m_strPredefinedPathsFilePath))
            {
                return false;
            }
            break;
        }
    }

    return true;
}

bool SimulationConfiguration::LoadSimulatorsXMLNode(DOMNode * pSimulatorsNode)
{
    DOMNodeList * pSimulatorNodes = pSimulatorsNode->getChildNodes();
    for (XMLSize_t iChild = 0; iChild < pSimulatorNodes->getLength(); iChild++)
    {
        DOMNode * pSimulatorNode = pSimulatorNodes->item(iChild);

        if (pSimulatorNode->getNodeType() == DOMNode::ELEMENT_NODE)
        {
            const std::string & simulatorNodeName = US(pSimulatorNode->getNodeName());

            SimulatorConfiguration * pSimulatorConfig = SimulatorConfigurationFactory::CreateSimulatorConfiguration(simulatorNodeName);
            if (!pSimulatorConfig)
            {
                BOOST_LOG_TRIVIAL(error) << "Unknown simulator node name '" << simulatorNodeName << "'";
                if (simulatorNodeName == SymuMasterConstants::s_SIMULATOR_MFD_NODE_NAME)
                {
                    BOOST_LOG_TRIVIAL(error) << "Be sure to use a SymuMaster version compiled with the CMake option USE_MATLAB_MFD enabled";
                }
                return false;
            }
            else
            {
                if (!pSimulatorConfig->LoadFromXML(pSimulatorNode))
                {
                    delete pSimulatorConfig;
                    return false;
                }
                else
                {
                    AddSimulatorConfig(pSimulatorConfig);
                }
            }
        }
    }
    return true;
}

bool SimulationConfiguration::ConfigureOutputDirectory(const std::string & strOutputDir, const std::string & strOutputPrefix)
{
    // File output
    typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > ofstream_sink;
    boost::shared_ptr< ofstream_sink > fileSink = boost::make_shared< ofstream_sink >();
    fileSink->set_formatter(
        boost::log::expressions::stream
        << '['
        << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
        << "] ["
        << boost::log::trivial::severity
        << "] "
        << boost::log::expressions::smessage
        );

    boost::filesystem::path givenPath = boost::filesystem::path(strOutputDir);
    boost::filesystem::path basePath = boost::filesystem::path(m_strLoadedConfigurationFilePath).parent_path();
    boost::filesystem::path outpuDirectoryPath = boost::filesystem::absolute(givenPath, basePath);

    if (!boost::filesystem::exists(outpuDirectoryPath))
    {
        if (!boost::filesystem::create_directories(outpuDirectoryPath))
        {
            BOOST_LOG_TRIVIAL(error) << "The Output directory '" << outpuDirectoryPath << "' is not valid !";
            return false;
        }
    }

	fileSink->locked_backend()->add_stream(boost::make_shared<std::ofstream>(boost::filesystem::path(outpuDirectoryPath / (strOutputPrefix + "_debug.log")).string()));
    fileSink->locked_backend()->auto_flush(true);
    boost::log::core::get()->add_sink(fileSink);

    m_strOutpuDirectoryPath = outpuDirectoryPath.string();
    m_strOutputPrefix = strOutputPrefix;

    return true;
}

bool SimulationConfiguration::GetDayToDayAssignement() const
{
    return m_bDayToDayAssignement;
}

bool SimulationConfiguration::UseCostsForTemporalShortestPath() const
{
    return m_bUseCostsForTemporalShortestPath;
}

bool SimulationConfiguration::bWritePpathAtEachIteration() const
{
    return m_bWritePpathAtEachIteration;
}

bool SimulationConfiguration::bWriteCosts() const
{
    return m_bWriteCosts;
}

bool SimulationConfiguration::bWriteCostsForEveryIteration() const
{
    return m_bWriteCostsForEveryIteration;
}

const std::string SimulationConfiguration::GetStrOutputPrefix() const
{
    return m_strOutputPrefix;
}

const std::string SimulationConfiguration::GetStrOutpuDirectoryPath() const
{
    return m_strOutpuDirectoryPath;
}

const std::string & SimulationConfiguration::GetLoadedConfigurationFilePath() const
{
    return m_strLoadedConfigurationFilePath;
}

void SimulationConfiguration::AddSimulatorConfig(SimulatorConfiguration * pSimulatorConfig)
{
    m_lstSimulatorConfigs.push_back(pSimulatorConfig);
}

const std::vector<SimulatorConfiguration*> & SimulationConfiguration::GetLstSimulatorConfigs() const
{
    return m_lstSimulatorConfigs;
}

void SimulationConfiguration::SetAssignmentPeriod(const boost::posix_time::time_duration & assignmentPeriod)
{
    m_tdAssignmentPeriod = assignmentPeriod;
}

boost::posix_time::time_duration SimulationConfiguration::GetAssignmentPeriod() const
{
    return m_tdAssignmentPeriod;
}

void SimulationConfiguration::SetTravelTimesUpdatePeriod(const boost::posix_time::time_duration & travelTimeUpdatePeriod)
{
    m_tdTravelTimesUpdatePeriod = travelTimeUpdatePeriod;
}

boost::posix_time::time_duration SimulationConfiguration::GetTravelTimesUpdatePeriod() const
{
    return m_tdTravelTimesUpdatePeriod;
}

void SimulationConfiguration::SetPredictionPeriod(const boost::posix_time::time_duration & predictionPeriod)
{
    m_tdPredictionPeriod = predictionPeriod;
}

boost::posix_time::time_duration SimulationConfiguration::GetPredictionPeriod() const
{
    return m_tdPredictionPeriod;
}

const std::string & SimulationConfiguration::GetDemandFilePath() const
{
    return m_strDemandFilePath;
}

void SimulationConfiguration::SetDemandFilePath(const std::string &strDemandFilePath)
{
    m_strDemandFilePath = strDemandFilePath;
}

const std::string& SimulationConfiguration::GetPredefinedPathsFilePath() const
{
    return m_strPredefinedPathsFilePath;
}

void SimulationConfiguration::SetPredefinedPathsFilePath(const std::string &strPredefinedPathsFilePath)
{
    m_strPredefinedPathsFilePath = strPredefinedPathsFilePath;
}

bool SimulationConfiguration::DoWorkAtParentODLevel() const
{
    return m_bDoWorkAtParentODLevel;
}

void SimulationConfiguration::DoWorkAtParentODLevel(bool bValue)
{
    m_bDoWorkAtParentODLevel = bValue;
}

bool SimulationConfiguration::DoExtraIterationToProduceOutputs() const
{
    return m_bDoExtraIterationToProduceOutputs;
}

const SymuCore::Populations * SimulationConfiguration::GetPopulations() const
{
    return m_pPopulations;
}

void SimulationConfiguration::SetAssignmentModel(AssignmentModel * pAssignmentModel)
{
    if (m_pAssignmentModel)
    {
        delete m_pAssignmentModel;
    }
    m_pAssignmentModel = pAssignmentModel;
}

AssignmentModel * SimulationConfiguration::GetAssignmentModel() const
{
    return m_pAssignmentModel;
}

std::vector<SymuCore::Motive *> SimulationConfiguration::GetListMotives() const
{
    return m_listMotives;
}

void SimulationConfiguration::SetListMotives(const std::vector<SymuCore::Motive *> &listMotives)
{
    m_listMotives = listMotives;
}

SymuCore::Motive *SimulationConfiguration::GetMotivesByName(std::string strMotiveName)
{
    SymuCore::Motive * pMotive = NULL;
    for(size_t iMotive = 0; iMotive < m_listMotives.size(); iMotive++)
    {
        if(strMotiveName == m_listMotives[iMotive]->GetStrName())
        {
            pMotive = m_listMotives[iMotive];
            break;
        }
    }
    return pMotive;
}

