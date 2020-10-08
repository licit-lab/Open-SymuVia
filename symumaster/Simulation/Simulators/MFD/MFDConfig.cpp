#include "MFDConfig.h"

#include "IO/XMLStringUtils.h"

#include <boost/log/trivial.hpp>

using namespace SymuMaster;

MFDConfig::MFDConfig()
{
    m_eSimulatorName = ST_MFD;
}

MFDConfig::~MFDConfig()
{
}

bool MFDConfig::LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pSimulationConfigNode)
{
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "matlab_workspace_dir", m_strMatlabWorkspacePath);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "demand_periods_nb", m_nDemandPeriodsNbPerPredictionPeriod);

    return true;
}

void MFDConfig::SetMatlabWorkspacePath(const std::string & strMatlabWorkspacePath)
{
    m_strMatlabWorkspacePath = strMatlabWorkspacePath;
}

const std::string & MFDConfig::GetMatlabWorkspacePath() const
{
    return m_strMatlabWorkspacePath;
}

void MFDConfig::SetDemandPeriodsNbPerPredictionPeriod(int nDemandPeriodsNbPerPredictionPeriod)
{
    m_nDemandPeriodsNbPerPredictionPeriod = nDemandPeriodsNbPerPredictionPeriod;
}

int MFDConfig::GetDemandPeriodsNbPerPredictionPeriod() const
{
    return m_nDemandPeriodsNbPerPredictionPeriod;
}


