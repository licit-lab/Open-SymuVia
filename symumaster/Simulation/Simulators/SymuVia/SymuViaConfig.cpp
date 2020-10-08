#include "SymuViaConfig.h"

#include "IO/XMLStringUtils.h"

#include <boost/log/trivial.hpp>

using namespace SymuMaster;

SymuViaConfig::SymuViaConfig()
{
    m_eSimulatorName = ST_SymuVia;

    m_bUseSpatialMethodForTravelTimesMeasurement = true;
	m_bUseClassicSpatialMethodForTravelTimesMeasurement = true;
    m_bEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes = true;
    m_bUseTravelTimeAsMarginalsInAreas = false;
    m_nMarginalsDeltaN = 1;
    m_nHistoryVehiclesNb = 5;
    m_nHistoryPeriodsNb = 1;
    m_dbMinimumSpeedForTravelTimes = 0.1;
    m_dbConcentrationRatioForFreeFlowCriterion = 0.5;
    m_dbMaximumMarginalValue = 1E10;

	m_nNbClassRobustTTMethod = 20;
	m_nMaxNbPointsPerClassRobustTTMethod = 40;
	m_nMinNbPointsStatisticalFilterRobustTTMethod = 10;

	m_bUsePreComputedRobustTravelTimes = false;

    m_bUseLastBusIfAnyForTravelTimes = true;
    m_bUseEmptyTravelTimesForBusLines = false;
    m_dbMeanBusExitTime = 4.0;
    m_nbStopsToConnectOutsideOfSubAreas = 1;
    m_dbMinLengthForMarginals = -1;

}

SymuViaConfig::~SymuViaConfig()
{
}

bool SymuViaConfig::LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pSimulationConfigNode)
{
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "xml_file_path", m_strXMLFilePath);

    std::string strTmp;
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "travel_times_mode", strTmp);
	m_bUseSpatialMethodForTravelTimesMeasurement = (strTmp == "classic_spatial");
	m_bUseSpatialMethodForTravelTimesMeasurement = (strTmp == "robust_spatial");
	m_bUseClassicSpatialMethodForTravelTimesMeasurement = (strTmp == "classic_spatial");

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "estimate_wait_time_on_traffic_lights", m_bEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "use_travel_times_as_marginals_on_areas", m_bUseTravelTimeAsMarginalsInAreas);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "marginals_deltaN", m_nMarginalsDeltaN);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "use_means_method_for_temporal_marginals", m_bUseMarginalsMeanMethod);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "use_medians_method_for_temporal_marginals", m_bUseMarginalsMedianMethod);
    if (m_bUseMarginalsMeanMethod && m_bUseMarginalsMedianMethod)
    {
        BOOST_LOG_TRIVIAL(error) << "Means and Medians methods can't be both enabled";
        return false;
    }
	XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "class_number_for_robust_travel_times_method", m_nNbClassRobustTTMethod);
	XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "max_points_number_per_class_for_robust_travel_times_method", m_nMaxNbPointsPerClassRobustTTMethod);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "min_points_number_for_statistical_filter_for_robust_travel_times_method", m_nMinNbPointsStatisticalFilterRobustTTMethod);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "history_travel_times_period_nb", m_nHistoryPeriodsNb);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "min_speed_for_travel_times", m_dbMinimumSpeedForTravelTimes);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "max_marginals_value", m_dbMaximumMarginalValue);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "concentration_ratio_for_free_flow_criterion", m_dbConcentrationRatioForFreeFlowCriterion);
	XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "robust_travel_times_file_path", m_strRobustTravelTimesFilePath);
	XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "robust_travel_speeds_file_path", m_strRobustTravelSpeedsFilePath);
	if (m_strRobustTravelTimesFilePath.size() > 0)
		m_bUsePreComputedRobustTravelTimes = 1;
	XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "calculation_of_pollutant_emissions", m_bPollutantEmissionsComputation);

    // for public transport :
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "use_bus_observations_for_travel_times", m_bUseLastBusIfAnyForTravelTimes);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "use_empty_travel_times_for_bus_travel_times", m_bUseEmptyTravelTimesForBusLines);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "mean_bus_exit_time", m_dbMeanBusExitTime);
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "nb_stops_to_connect_outside_of_subareas", m_nbStopsToConnectOutsideOfSubAreas);

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "min_length_for_marginals", m_dbMinLengthForMarginals);

    return true;
}

void SymuViaConfig::SetXMLFilePath(const std::string & strXMLFilePath)
{
    m_strXMLFilePath = strXMLFilePath;
}

const std::string & SymuViaConfig::GetXMLFilePath() const
{
    return m_strXMLFilePath;
}

void SymuViaConfig::SetUseSpatialMethodForTravelTimesMeasurement(bool bValue)
{
    m_bUseSpatialMethodForTravelTimesMeasurement = bValue;
}

bool SymuViaConfig::UseSpatialMethodForTravelTimesMeasurement() const
{
    return m_bUseSpatialMethodForTravelTimesMeasurement;
}

void SymuViaConfig::SetUseClassicSpatialMethodForTravelTimesMeasurement(bool bValue)
{
	m_bUseClassicSpatialMethodForTravelTimesMeasurement = bValue;
}

bool SymuViaConfig::UseClassicSpatialMethodForTravelTimesMeasurement() const
{
	return m_bUseClassicSpatialMethodForTravelTimesMeasurement;
}

void SymuViaConfig::SetUseSustainable(bool bValue)
{
	m_bUseSustainable = bValue;
}

bool SymuViaConfig::UseSustainable() const
{
	return m_bUseSustainable;
}

void SymuViaConfig::SetEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes(bool bValue)
{
    m_bEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes = bValue;
}

bool SymuViaConfig::EstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes() const
{
    return m_bEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes;
}

void SymuViaConfig::SetUseTravelTimesAsMarginalsInAreas(bool bValue)
{
    m_bUseTravelTimeAsMarginalsInAreas = bValue;
}

bool SymuViaConfig::UseTravelTimesAsMarginalsInAreas() const
{
    return m_bUseTravelTimeAsMarginalsInAreas;
}

void SymuViaConfig::SetMarginalsDeltaN(int iValue)
{
    m_nMarginalsDeltaN = iValue;
}

int SymuViaConfig::GetMarginalsDeltaN() const
{
    return m_nMarginalsDeltaN;
}

void SymuViaConfig::SetUseMarginalsMeanMethod(bool bValue)
{
    m_bUseMarginalsMeanMethod = bValue;
}

bool SymuViaConfig::UseMarginalsMeanMethod() const
{
    return m_bUseMarginalsMeanMethod;
}

void SymuViaConfig::SetUseMarginalsMedianMethod(bool bValue)
{
    m_bUseMarginalsMedianMethod = bValue;
}

bool SymuViaConfig::UseMarginalsMedianMethod() const
{
    return m_bUseMarginalsMedianMethod;
}

void SymuViaConfig::SetHistoryVehiclesNb(int iValue)
{
    m_nHistoryVehiclesNb = iValue;
}

int SymuViaConfig::GetHistoryVehiclesNb() const
{
    return m_nHistoryVehiclesNb;
}

void SymuViaConfig::SetHistoryPeriodsNb(int iValue)
{
    m_nHistoryPeriodsNb = iValue;
}

int SymuViaConfig::GetHistoryPeriodsNb() const
{
    return m_nHistoryPeriodsNb;
}

void SymuViaConfig::SetMinimumSpeedForTravelTimes(double dValue)
{
    m_dbMinimumSpeedForTravelTimes = dValue;
}

double SymuViaConfig::GetMinimumSpeedForTravelTimes() const
{
    return m_dbMinimumSpeedForTravelTimes;
}

void SymuViaConfig::SetMaximumMarginalsValue(double dValue)
{
    m_dbMaximumMarginalValue = dValue;
}

double SymuViaConfig::GetMaximumMarginalsValue() const
{
    return m_dbMaximumMarginalValue;
}

void SymuViaConfig::SetConcentrationRatioForFreeFlowCriterion(double dValue)
{
    m_dbConcentrationRatioForFreeFlowCriterion = dValue;
}

double SymuViaConfig::GetConcentrationRatioForFreeFlowCriterion() const
{
    return m_dbConcentrationRatioForFreeFlowCriterion;
}

void SymuViaConfig::SetUseLastBusIfAnyForTravelTimes(bool bValue)
{
    m_bUseLastBusIfAnyForTravelTimes = bValue;
}

bool SymuViaConfig::UseLastBusIfAnyForTravelTimes() const
{
    return m_bUseLastBusIfAnyForTravelTimes;
}

void SymuViaConfig::SetUseEmptyTravelTimesForBusLines(bool bValue)
{
    m_bUseEmptyTravelTimesForBusLines = bValue;
}

bool SymuViaConfig::UseEmptyTravelTimesForBusLines() const
{
    return m_bUseEmptyTravelTimesForBusLines;
}

void SymuViaConfig::SetMeanBusExitTime(double dValue)
{
    m_dbMeanBusExitTime = dValue;
}

double SymuViaConfig::GetMeanBusExitTime() const
{
    return m_dbMeanBusExitTime;
}

void SymuViaConfig::SetNbStopsToConnectOutsideOfSubAreas(int iValue)
{
    m_nbStopsToConnectOutsideOfSubAreas = iValue;
}

int SymuViaConfig::GetNbStopsToConnectOutsideOfSubAreas() const
{
    return m_nbStopsToConnectOutsideOfSubAreas;
}

void SymuViaConfig::SetMinLengthForMarginals(double dValue)
{
    m_dbMinLengthForMarginals = dValue;
}

double SymuViaConfig::GetMinLengthForMarginals() const
{
    return m_dbMinLengthForMarginals;
}

void SymuViaConfig::SetNbClassRobustTTMethod(int nValue)
{
	m_nNbClassRobustTTMethod = nValue;
}

int SymuViaConfig::GetNbClassRobustTTMethod() const
{
	return m_nNbClassRobustTTMethod;
}

void SymuViaConfig::SetMaxNbPointsPerClassRobustTTMethod(int nValue)
{
	m_nMaxNbPointsPerClassRobustTTMethod = nValue;
}

int SymuViaConfig::GetMaxNbPointsPerClassRobustTTMethod() const
{
	return m_nMaxNbPointsPerClassRobustTTMethod;
}

void SymuViaConfig::SetMinNbPointsStatisticalFilterRobustTTMethod(int nValue)
{
	m_nMinNbPointsStatisticalFilterRobustTTMethod = nValue;
}

int SymuViaConfig::GetMinNbPointsStatisticalFilterRobustTTMethod() const
{
	return m_nMinNbPointsStatisticalFilterRobustTTMethod;
}

void SymuViaConfig::SetRobustTravelTimesFilePath(const std::string & strRobustTravelTimesFilePath)
{
	m_strRobustTravelTimesFilePath = strRobustTravelTimesFilePath;
}

void SymuViaConfig::SetRobustTravelSpeedsFilePath(const std::string & strRobustTravelSpeedsFilePath)
{
	m_strRobustTravelSpeedsFilePath = strRobustTravelSpeedsFilePath;
}

const std::string & SymuViaConfig::GetRobustTravelTimesFilePath() const
{
	return m_strRobustTravelTimesFilePath;
}

const std::string & SymuViaConfig::GetRobustTravelSpeedsFilePath() const
{
	return m_strRobustTravelSpeedsFilePath;
}

void SymuViaConfig::SetPollutantEmissionsComputation(const bool & bPollutantEmissionsComputation)
{
	m_bPollutantEmissionsComputation = bPollutantEmissionsComputation;
}

const bool & SymuViaConfig::GetPollutantEmissionsComputation() const
{
	return m_bPollutantEmissionsComputation;
}


