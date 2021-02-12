#pragma once

#ifndef _SYMUMASTER_SYMUVIACONFIG_
#define _SYMUMASTER_SYMUVIACONFIG_

#include "SymuMasterExports.h"

#include "Simulation/Config/SimulatorConfiguration.h"

#include <string>

#pragma warning( push )  
#pragma warning( disable : 4251 )  

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF SymuViaConfig : public SimulatorConfiguration {

    public:
        SymuViaConfig();

        virtual ~SymuViaConfig();

        virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pSimulationConfigNode);

        void SetXMLFilePath(const std::string & strXMLFilePath);
        const std::string & GetXMLFilePath() const;

        void SetUseSpatialMethodForTravelTimesMeasurement(bool bValue);
        bool UseSpatialMethodForTravelTimesMeasurement() const;

		void SetUseClassicSpatialMethodForTravelTimesMeasurement(bool bValue);
		bool UseClassicSpatialMethodForTravelTimesMeasurement() const;

		void SetUseSustainable(bool bValue);
		bool UseSustainable() const;

        void SetEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes(bool bValue);
        bool EstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes() const;

        void SetUseTravelTimesAsMarginalsInAreas(bool bValue);
        bool UseTravelTimesAsMarginalsInAreas() const;

        void SetMarginalsDeltaN(int iValue);
        int GetMarginalsDeltaN() const;

        void SetUseMarginalsMeanMethod(bool bValue);
        bool UseMarginalsMeanMethod() const;

        void SetUseMarginalsMedianMethod(bool bValue);
        bool UseMarginalsMedianMethod() const;

        void SetHistoryVehiclesNb(int iValue);
        int GetHistoryVehiclesNb() const;

        void SetHistoryPeriodsNb(int iValue);
        int GetHistoryPeriodsNb() const;

        void SetMinimumSpeedForTravelTimes(double dValue);
        double GetMinimumSpeedForTravelTimes() const;

        void SetMaximumMarginalsValue(double dValue);
        double GetMaximumMarginalsValue() const;
        
        void SetConcentrationRatioForFreeFlowCriterion(double dValue);
        double GetConcentrationRatioForFreeFlowCriterion() const;

        void SetUseLastBusIfAnyForTravelTimes(bool bValue);
        bool UseLastBusIfAnyForTravelTimes() const;

        void SetUseEmptyTravelTimesForBusLines(bool bValue);
        bool UseEmptyTravelTimesForBusLines() const;

        void SetMeanBusExitTime(double dValue);
        double GetMeanBusExitTime() const;

        void SetNbStopsToConnectOutsideOfSubAreas(int iValue);
        int GetNbStopsToConnectOutsideOfSubAreas() const;

        void SetMinLengthForMarginals(double dValue);
        double GetMinLengthForMarginals() const;

		void SetNbClassRobustTTMethod(int nValue);
		int GetNbClassRobustTTMethod() const;

		void SetMaxNbPointsPerClassRobustTTMethod(int nValue);
		int GetMaxNbPointsPerClassRobustTTMethod() const;

		void SetMinNbPointsStatisticalFilterRobustTTMethod(int nValue);
		int GetMinNbPointsStatisticalFilterRobustTTMethod() const;

		void SetRobustTravelTimesFilePath(const std::string & strRobustTravelTimesFilePath);
		const std::string & GetRobustTravelTimesFilePath() const;

		void SetRobustTravelSpeedsFilePath(const std::string & strRobustTravelSpeedsFilePath);
		const std::string & GetRobustTravelSpeedsFilePath() const;

		void SetRobustPointsBackup(const bool & bRobustPointsBackup);
		const bool & GetRobustPointsBackup() const;

		void SetPollutantEmissionsComputation(const bool & PollutantEmissionsComputation);
		const bool & GetPollutantEmissionsComputation() const;

    protected:

        /**
         * SymuVia XML input scenario file
         */
        std::string m_strXMLFilePath;

        /**
         * true for "spatial" method, false for "output vehicles" method for travel time measurements on links
         */
        bool m_bUseSpatialMethodForTravelTimesMeasurement;

		/**
		* true for "classic method" method, false for "robust spatial" method for travel time measurements on links
		*/
		bool m_bUseClassicSpatialMethodForTravelTimesMeasurement;

		/**
		* true for sustainable system optimum simulation, false otherwise
		*/
		bool m_bUseSustainable;

        /**
         * true to consider mean wait time on traffic lights on upstream links for empty travel times
         */
        bool m_bEstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes;

        /**
         * true to disable marginals computations in area or subarea patterns and use 0 for itt, so marginal = tt.
         */
        bool m_bUseTravelTimeAsMarginalsInAreas;

        /**
         * DeltaN to use for the marginals
         */
        int m_nMarginalsDeltaN;

        /**
         * Use Means method for temporal marginals computation
         */
        bool m_bUseMarginalsMeanMethod;

        /**
        * Use Medians method for temporal marginals computation
        */
        bool m_bUseMarginalsMedianMethod;

        /**
         * Number of vehicles to use for marginals and travel times when using "output vehicles" method
         */
        int m_nHistoryVehiclesNb;

        /**
         * Maximum number of past travel times update periods to search for the m_nHistoryVehicles needed vehicles for marginals and travel times when using "output vehicles" method
         */
        int m_nHistoryPeriodsNb;

        /**
         * Minimum speed (m/s) used for travel time computations. The travel time can't be greater than the length divided by this minimal speed,
         * whatever the method used ("spatial" or "output vehicles").
         */
        double m_dbMinimumSpeedForTravelTimes;

        /**
         * Maximum marginals value for possible patterns and penalties
         */
        double m_dbMaximumMarginalValue;

        /**
         * Percentage of the maximum concentration of the links used to distinguish free flow from fully congested when no output vehicule is seen for travel times calculation.
         */
        double m_dbConcentrationRatioForFreeFlowCriterion;

        /** 
         * If true, try to use the last bus's travel time for PublicTransport Patterns, if there is one. If false,
         * use travel times based on the travel times of the corresponding private cars driving patterns.
         */
        bool m_bUseLastBusIfAnyForTravelTimes;

        /**
         * If true, the travel time on PublicTransport patterns is always the empty travel time of the corresponding private cars driving patterns.
         */
        bool m_bUseEmptyTravelTimesForBusLines;

        /**
         * Mean time spend by a passenger to leave a bus/tram
         */
        double m_dbMeanBusExitTime;

        /**
        * Number fo public transport stops outside of a subarea but inside the parent area to connect to the subarea.
        */
        int m_nbStopsToConnectOutsideOfSubAreas;

        /**
        * If defined, minimum length for a link or a move inside a connection to compute marginals. Taking 0 for marginals otherwise.
        */
        double m_dbMinLengthForMarginals;

		/**
		* Number of class used by the robust travel times calculation method.
		*/
		int m_nNbClassRobustTTMethod;

		/**
		* Maximum number of points per class used by the robust travel times calculation method.
		*/
		int m_nMaxNbPointsPerClassRobustTTMethod;

		/**
		* Minimum number of points per class for applying statiscal filter used by the robust travel times method.
		*/
		int m_nMinNbPointsStatisticalFilterRobustTTMethod;

		/**
		* If robust travel times method is used, indicates whether data of robust travel times is pre-computed
		*/
		bool m_bUsePreComputedRobustTravelTimes;

		/**
		* Robust travel times file
		*/
		std::string m_strRobustTravelTimesFilePath;

		/**
		* Robust travel speeds file
		*/
		std::string m_strRobustTravelSpeedsFilePath;

		/**
		* Indicates whether the computed points used to build the robust functions are saved
		*/
		bool m_bRobustPointsBackup;

		/**
		* Computation of pollutant emissions
		*/
		bool m_bPollutantEmissionsComputation;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_SYMUVIACONFIG_
