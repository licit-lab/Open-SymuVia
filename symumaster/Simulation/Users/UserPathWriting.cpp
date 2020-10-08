#include "UserPathWriting.h"

#include "Simulation/Config/SimulationConfiguration.h"
#include "TrackedUser.h"
#include "Graph/Model/Pattern.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Node.h"
#include "UserEngine.h"
#include "PPath.h"

#include <Demand/Trip.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>

using namespace SymuMaster;

UserPathWriting::UserPathWriting()
{

}

UserPathWriting::UserPathWriting(const SimulationConfiguration & config, UserEngine* pUserEngine)
{
    m_strOutpuDirectoryPath = config.GetStrOutpuDirectoryPath();
    m_strOutputPrefix = config.GetStrOutputPrefix();

    //create the file and empty it if already exist
    std::string fileName = m_strOutputPrefix + "_PPaths.csv";
    boost::filesystem::ofstream PPathOutputOFS;
    PPathOutputOFS.open(boost::filesystem::path(m_strOutpuDirectoryPath) / fileName, boost::filesystem::ofstream::trunc);
    PPathOutputOFS.close();

    pUserEngine->setUserPathWriter(this);
}

UserPathWriting::~UserPathWriting()
{
}

void UserPathWriting::CompletedPPath(const boost::posix_time::ptime & nextPPathDepartureTime, SymuCore::Trip* pTrip, const PPath& pPPath)
{
    std::vector<PPath>& listPPath = m_lstAllPPath[pTrip];
    find(listPPath.begin(), listPPath.end(), pPPath)->SetArrivalTime(nextPPathDepartureTime);
}

void UserPathWriting::SetPPaths(SymuCore::Trip* pTrip, const std::vector<PPath> &newListPPaths)
{
    m_lstAllPPath[pTrip] = newListPPaths;
}

bool UserPathWriting::WriteUserPaths(int iPeriod, int iIteration, int iInstanceOffset, int iInstance, const boost::posix_time::ptime & startTimeSimulation, const boost::posix_time::ptime & startTimePeriod, const boost::posix_time::time_duration & timeAssignmentPeriodDuration, const boost::posix_time::time_duration & timePredictionPeriodDuration, bool isFinalFile)
{
    boost::posix_time::ptime endTimeAssignmentPeriod = startTimePeriod + timeAssignmentPeriodDuration;
    boost::posix_time::ptime endTimePredictionPeriod = startTimePeriod + timePredictionPeriodDuration;
    // rmk : we don't use boost's time_duration here to avoid overflow with big values
    double dbSumAssignmentTravelTime = 0.0;
    double dbSumPredictionTravelTime = 0;

    //prepare outputfiles
    boost::filesystem::ofstream PPathOutputOFS;

    if (isFinalFile)
    {
        std::string fileName = m_strOutputPrefix + "_final_PPaths.csv";
        PPathOutputOFS.open(boost::filesystem::path(m_strOutpuDirectoryPath) / fileName, boost::filesystem::ofstream::trunc);
    }else{
        std::string fileName = m_strOutputPrefix + "_PPaths.csv";
        PPathOutputOFS.open(boost::filesystem::path(m_strOutpuDirectoryPath) / fileName, boost::filesystem::ofstream::app);
    }

    if(PPathOutputOFS.fail())
    {
        BOOST_LOG_TRIVIAL(error) << "Unable to open the file '" << m_strOutpuDirectoryPath << "\\" << m_strOutputPrefix << "_final_PPaths.csv' !";
        return false;
    }

    //result is Period;Iteration;IDUser;DepartureTime;ArrivalTime;PPathServiceType;ArrivalTime;.....;PPathServiceType;ArrivalTime;
    const SymuCore::Trip* pTrip;
    std::vector<PPath> lstPPath;
    boost::posix_time::ptime dtPPathStartTime;
    boost::posix_time::ptime dtPPathPredictionPeriodEndTime;
    boost::posix_time::ptime dtPPathAssignmentPeriodEndTime;
    for (std::map<SymuCore::Trip *, std::vector<PPath>, TripCompare >::const_iterator itLstTrips = m_lstAllPPath.begin(); itLstTrips != m_lstAllPPath.end(); itLstTrips++)
    {
        pTrip = itLstTrips->first;
        lstPPath = itLstTrips->second;
        const boost::posix_time::ptime dtTripArrivalTime = lstPPath[lstPPath.size()-1].GetArrivalTime();

        // if not final file, we have to skip the trips that are not active during at least a part of the prediction period
        if (!isFinalFile && (pTrip->GetDepartureTime() > endTimePredictionPeriod || (!dtTripArrivalTime.is_not_a_date_time() && dtTripArrivalTime < startTimePeriod)))
            continue;

        dtPPathStartTime = pTrip->GetDepartureTime();
        if (!isFinalFile) {
            // for the results per period, we ignore time spent of the trip before the start of the current period
            dtPPathStartTime = (dtPPathStartTime > startTimePeriod) ? dtPPathStartTime : startTimePeriod;
        }

        // and we ignore the time spent after the end of the period
        if(dtTripArrivalTime.is_not_a_date_time() || dtTripArrivalTime > endTimeAssignmentPeriod){
            dtPPathAssignmentPeriodEndTime = endTimeAssignmentPeriod;
        }else{
            dtPPathAssignmentPeriodEndTime = dtTripArrivalTime;
        }
        if(dtTripArrivalTime.is_not_a_date_time() || dtTripArrivalTime > endTimePredictionPeriod){
            dtPPathPredictionPeriodEndTime = endTimePredictionPeriod;
        }else{
            dtPPathPredictionPeriodEndTime = dtTripArrivalTime;
        }

        dbSumAssignmentTravelTime += (double)(dtPPathAssignmentPeriodEndTime - dtPPathStartTime).total_microseconds()/1000000.0;
        dbSumPredictionTravelTime += (double)(dtPPathPredictionPeriodEndTime - dtPPathStartTime).total_microseconds()/1000000.0;

        std::string allPPaths;
        std::string strTripArrivalTime;
        if(dtTripArrivalTime.is_not_a_date_time())
        {
            strTripArrivalTime = "Undefined;";
        }else{
            strTripArrivalTime = to_simple_string(dtTripArrivalTime.time_of_day()) + ";";
        }
        boost::posix_time::ptime lastPPathArrival = pTrip->GetDepartureTime();

        for(std::vector<PPath>::iterator itPPath = lstPPath.begin(); itPPath != lstPPath.end(); itPPath++)
        {
            PPath currentPPath = (*itPPath);
            SymuCore::Pattern* pPattern = pTrip->GetPath(iInstance).GetlistPattern()[currentPPath.GetFirstPatternIndex()];
            switch (pPattern->getPatternType())
            {
            case SymuCore::PT_Walk:
                allPPaths += "Walk;";
                break;
            case SymuCore::PT_Cycling:
                allPPaths += "Cycling;";
                break;
            case SymuCore::PT_Driving:
                allPPaths += "Driving;";
                break;
            case SymuCore::PT_PublicTransport:
                allPPaths += "PublicTransport;";
                break;
            case SymuCore::PT_Taxi:
                allPPaths += "Taxi;";
                break;
			case SymuCore::PT_Undefined:
				// Arrive pour les itinéraires de zone à zone, pour lesquels on a ici un NullPattern uniquement dans le PPATH.
				// On se base du coup sur le type de service pour préciser le type de transport associé malgré tout
				switch (currentPPath.GetServiceType())
				{
				case SymuCore::ST_Undefined:
					allPPaths += "Undefined;";
					break;
				case SymuCore::ST_All:
					allPPaths += "All;";
					break;
				case SymuCore::ST_Driving:
					allPPaths += "Driving;";
					break;
				case SymuCore::ST_PublicTransport:
					allPPaths += "PublicTransport;";
					break;
				case SymuCore::ST_Taxi:
					allPPaths += "Taxi;";
					break;
				default:
					BOOST_LOG_TRIVIAL(error) << "Service type " << currentPPath.GetServiceType() << " not handled here for trip " << pTrip->GetID();
					return false;
				}
				break;
            }

            size_t firstIndex = itPPath->GetFirstPatternIndex();
            size_t lastIndex = itPPath->GetLastPatternIndex();
            allPPaths += pTrip->GetPath(iInstance).GetlistPattern()[firstIndex]->getLink()->getUpstreamNode()->getStrName();
            for(size_t i = firstIndex; i <= lastIndex; i++)
            {
                assert(i < pTrip->GetPath(iInstance).GetlistPattern().size());

                allPPaths += "\\" + pTrip->GetPath(iInstance).GetlistPattern()[i]->toString();
                allPPaths += "\\" + pTrip->GetPath(iInstance).GetlistPattern()[i]->getLink()->getDownstreamNode()->getStrName();
            }
            allPPaths += ";";

            if (currentPPath.GetArrivalTime().is_not_a_date_time()){
                allPPaths += "Undefined;";
            }else{
                allPPaths += to_simple_string(currentPPath.GetArrivalTime() - lastPPathArrival) + ";";
            }
            lastPPathArrival = currentPPath.GetArrivalTime();
        }

        PPathOutputOFS  << iPeriod << ";"
                        << iIteration << ";"
                        << (iInstanceOffset+iInstance) << ";"
                        << pTrip->GetID() << ";"
                        << to_simple_string(pTrip->GetDepartureTime().time_of_day()) << ";"
                        << strTripArrivalTime
                        << allPPaths << std::endl;
    }

    PPathOutputOFS << "Total travel time on the assignment period is " << dbSumAssignmentTravelTime << "s;" << std::endl;
    PPathOutputOFS << "Total travel time on the prediction period is " << dbSumPredictionTravelTime << "s;" << std::endl;

    PPathOutputOFS << std::endl;
    PPathOutputOFS.close();

    return true;
}

void UserPathWriting::AssignmentPeriodStart()
{
    // dont take the snapshot of the beginning of the prediction period every iteration...
    if (m_lstUserPathWritingSnapshots.empty())
    {
        m_lstUserPathWritingSnapshots.push_back(m_lstAllPPath);
    }
}

void UserPathWriting::AssignmentPeriodEnded()
{
    m_lstUserPathWritingSnapshots.push_back(m_lstAllPPath);
}

void UserPathWriting::AssignmentPeriodRollback()
{
    // If we have a newer snapshot for the end of the assignment period, discard it.
    if (m_lstUserPathWritingSnapshots.size() == 2)
    {
        m_lstUserPathWritingSnapshots.pop_back();
    }
    // Restore the snapshot of the beginning of the prediction period
    m_lstAllPPath = m_lstUserPathWritingSnapshots.front();
}

void UserPathWriting::AssignmentPeriodCommit()
{
    // if we have a snapshot for the end of the assignment period, restore it. If not, there is no rolling horizon so nothing to be done
    if (m_lstUserPathWritingSnapshots.size() == 2)
    {
        m_lstAllPPath = m_lstUserPathWritingSnapshots.back();
    }

    // no need to keep any snapshot.
    m_lstUserPathWritingSnapshots.clear();
}


