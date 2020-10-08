#include "SymuMaster.h"

#include "Simulation/SimulationRunner.h"
#include "Simulation/Config/SimulationConfiguration.h"

using namespace SymuMaster;

SymuMasterCWrapper m_wrapper;

SymuMasterCWrapper::SymuMasterCWrapper()
{
	m_pConfig = nullptr;
	m_pSimulationRunner = nullptr;
}

SymuMasterCWrapper::~SymuMasterCWrapper()
{
	Clean();
}

void SymuMasterCWrapper::Clean()
{
	if (m_pSimulationRunner)
	{
		delete m_pSimulationRunner;
		m_pSimulationRunner = nullptr;
	}
	if (m_pConfig)
	{
		delete m_pConfig;
		m_pConfig = nullptr;
	}
}

bool SymuMasterCWrapper::Load(const char * configFile)
{
	// delete old simulation (for now, we just allow to work on one simulation at a time)
	m_wrapper.Clean();

	m_pConfig = new SimulationConfiguration();

	// read the configuration from XML input file
	bool bOk = m_pConfig->LoadFromXML(configFile);
	if (bOk)
	{
		m_pSimulationRunner = new SimulationRunner(*m_pConfig);
		bOk = m_pSimulationRunner->Initialize();
	}
	return bOk;
}

bool SymuMasterCWrapper::RunNextAssignmentPeriod(bool * bEnd)
{
	bool bEndTmp;
	bool bOk = m_pSimulationRunner->RunNextAssignmentPeriod(bEndTmp);
	*bEnd = bEndTmp;
	return bOk;
}

bool SymuMasterCWrapper::DropSimulation()
{
	bool bOk = m_pSimulationRunner->Terminate();
	Clean();
	return bOk;
}

bool SymuMasterCWrapper::ForbidSymuviaLinks(char ** linkNames)
{
	char ** pIt = linkNames;
	std::vector<std::string> linkNamesVect;
	while (*pIt != NULL)
	{
		linkNamesVect.push_back(*pIt++);
	}
	return m_pSimulationRunner->ForbidSymuviaLinks(linkNamesVect);
}

bool LoadSimulation(const char * configFilePath)
{
	return m_wrapper.Load(configFilePath);
}

bool RunNextAssignmentPeriod(bool * bEnd)
{
	return m_wrapper.RunNextAssignmentPeriod(bEnd);
}

bool DropSimulation()
{
	return m_wrapper.DropSimulation();
}

bool ForbidSymuviaLinks(char ** linkNames)
{
	return m_wrapper.ForbidSymuviaLinks(linkNames);
}

