#include "UserEngine.h"

#include "Simulation/Simulators/Walk/WalkSimulator.h"
#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "Simulation/Config/SimulationConfiguration.h"
#include "UserEngineSnapshot.h"
#include "UserPathWriting.h"

#include <Graph/Model/Pattern.h>
#include <Graph/Model/PublicTransport/PublicTransportPattern.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Graph.h>
#include <Graph/Model/MultiLayersGraph.h>

#include <Demand/Populations.h>
#include <Demand/Population.h>
#include <Demand/Trip.h>
#include <Demand/Origin.h>
#include <Demand/Destination.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/trivial.hpp>

#include <vector>
#include <fstream>
#include <iostream>

using namespace SymuCore;
using namespace SymuMaster;

UserEngine::UserEngine(int iInstance) :
m_iInstance(iInstance),
m_pWalkSimulator(new WalkSimulator(NULL, iInstance))
{

}

UserEngine::~UserEngine()
{
    if (m_pWalkSimulator)
    {
        delete m_pWalkSimulator;
    }

    for (std::map<int, TrackedUser*>::const_iterator iterUser = m_AllUsers.begin(); iterUser != m_AllUsers.end(); ++iterUser)
    {
        delete iterUser->second;
    }

    for (size_t i = 0; i < m_lstUserEngineSnapshot.size(); i++)
    {
        delete m_lstUserEngineSnapshot[i];
    }
}

void UserEngine::Initialize(const SimulationConfiguration & config, const std::vector<TraficSimulatorInstance*> & lstExternalTraficSimulators)
{
    m_pWalkSimulator->Initialize(config, this);
    m_lstTraficSimulators = lstExternalTraficSimulators;
}

void UserEngine::OnPPathCompleted(int tripID, double dbNextPPathDepartureTimeFromCurrentTimeStep)
{
    std::map<int, TrackedUser*>::const_iterator iterUser = m_AllUsers.find(tripID);

	if (iterUser == m_AllUsers.end())
	{
		std::cout << "On a une erreur  !!! :)   " << std::endl;
		return;
	}
    assert(iterUser != m_AllUsers.end());

    TrackedUser * pUser = iterUser->second;
    boost::posix_time::ptime nextPPathDepartureTime = m_dtCurrentTimeStepStartTime + boost::posix_time::microseconds((int64_t)round(dbNextPPathDepartureTimeFromCurrentTimeStep*1000000.0));
    m_pUserPathWriter->CompletedPPath(nextPPathDepartureTime, pUser->GetTrip(), pUser->GetCurrentPPath());
    bool bEnded = pUser->GoToNextPPath(nextPPathDepartureTime);
    if (!bEnded)
    {
        delete pUser;
        m_AllUsers.erase(iterUser);
    }
    else
    {
        m_OwnedUsers[tripID] = pUser;
    }
}

bool UserEngine::Run(const Populations& populations, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::time_duration & timeStepDuration)
{
    m_dtCurrentTimeStepStartTime = startTimeStepTime;

    // 1 - getting the Trips that must be activated this time step :
    boost::posix_time::ptime endTime = startTimeStepTime + timeStepDuration;

    const std::vector<Population*> & listPopulations = populations.getListPopulations();

    Trip * pTrip;
    Population * pPopulation;
    for (size_t i = 0; i < listPopulations.size(); i++)
    {
        pPopulation = listPopulations[i];

        //The list of trips must be ordered by time
        for (size_t iTrip = m_NextTripsToActivate[pPopulation]; iTrip < pPopulation->GetListTrips().size(); iTrip++)
        {
            pTrip = pPopulation->GetListTrips()[iTrip];

            if (pTrip->GetDepartureTime() >= endTime)
            {
                break;
            }

            if (pTrip->GetDepartureTime() >= startTimeStepTime)
            {
                m_NextTripsToActivate[pPopulation] = iTrip+1;
                TrackedUser * pTrackedUser = CreateTrackedUserFromTrip(pTrip);
                if (pTrackedUser)
                {
                    m_OwnedUsers[pTrip->GetID()] = pTrackedUser;
                    m_AllUsers[pTrip->GetID()] = pTrackedUser;
                    m_pUserPathWriter->SetPPaths(pTrip, pTrackedUser->GetListPPaths());
                }
            }
        }
    }

    // 2 - activating owned users
    // rmq. : dont work directly on m_OwnedUsers since users can be sent back to it in the ActivatePPath ! (Walking simulation case)
    std::vector<TrackedUser*> usersToActivate;
    for (std::map<int, TrackedUser*>::const_iterator iterOwnedUser = m_OwnedUsers.begin(); iterOwnedUser != m_OwnedUsers.end();)
    {
        if (iterOwnedUser->second->GetCurrentPPathDepartureTime() < endTime)
        {
            usersToActivate.push_back(iterOwnedUser->second);
            iterOwnedUser = m_OwnedUsers.erase(iterOwnedUser);
        }
        else
        {
            ++iterOwnedUser;
        }
    }
    for (size_t iUser = 0; iUser < usersToActivate.size(); iUser++)
    {
        TrackedUser * pUser = usersToActivate[iUser];
        const PPath & currentPPath = pUser->GetCurrentPPath();
        if (!currentPPath.GetSimulator()->ActivatePPath(currentPPath, startTimeStepTime, pUser->GetCurrentPPathDepartureTime()))
        {
            BOOST_LOG_TRIVIAL(error) << "Activation of trip " << pUser->GetTrip()->GetID() << " failed !";
            return false;
        }
    }
    return true;
}

TrackedUser * UserEngine::CreateTrackedUserFromTrip(SymuCore::Trip * pTrip) const
{
    std::vector<PPath> subPaths;

    const Path & tripPath = pTrip->GetPath(m_iInstance);

    if (tripPath.GetlistPattern().empty())
    {
        BOOST_LOG_TRIVIAL(error) << "Empty path for trip " << pTrip->GetID() << ", ignoring trip...";
        //return NULL;
    }

    PPath * pPPath = NULL;
    PatternType lastPatternType;
    PublicTransportLine * lastLine = NULL;

    // ignoring first and last pattern, correspoding to virtual links from origin and to destination.
    for (size_t iPattern = 1; iPattern < tripPath.GetlistPattern().size()-1; iPattern++)
    {
        Pattern * pPattern = tripPath.GetlistPattern().at(iPattern);

        // if a PPath is currently in construction : 
        if (pPPath)
        {
            // If the pattern is a virtual interface between networks pattern, skip this and the next pattern (both are virtual)
            if (pPattern->getLink()->getDownstreamNode()->getNodeType() == NT_NetworksInterface)
            {
                // append the closed PPath to the list of PPath
                subPaths.push_back(*pPPath);
                delete pPPath;
                pPPath = NULL;

                // go to the start of the next ppath
                iPattern++;
                continue;
            }
            // if the pattern is compatible with the current PPath :
            // Compatible means :
            // 1 - same ServiceType
            // 2 - no transition between walk and no walk (walk is handled by SymuMaster directly)
            // 3 - no transition bewteen different public transport lines
             else if (pPattern->getLink()->getParent()->GetServiceType() == pPPath->GetServiceType()
                && !((pPattern->getPatternType() == PT_Walk && lastPatternType != PT_Walk) || (pPattern->getPatternType() != PT_Walk && lastPatternType == PT_Walk))
                && !(pPattern->getPatternType() == PT_PublicTransport && lastPatternType == PT_PublicTransport && ((PublicTransportPattern*)pPattern)->getLine() != lastLine))
            {
                // extend the current PPath with the current pattern
                pPPath->SetLastPatternIndex(iPattern);
            }
            else
            {
                // append the closed PPath to the list of PPath
                subPaths.push_back(*pPPath);
                delete pPPath;
                pPPath = NULL;
            }
        }

        // create new ppath if last ppath was closed
        if (pPPath == NULL)
        {
            // get the simulator associated to the PPath :
            TraficSimulatorInstance * pPPathSimulator = NULL;
            if (pPattern->getPatternType() == PT_Walk)
            {
                pPPathSimulator = m_pWalkSimulator;
            }
            else
            {
                Node * pFirstNode = pPattern->getLink()->getUpstreamNode();
                for (size_t iSimuHandler = 0; iSimuHandler < m_lstTraficSimulators.size() && !pPPathSimulator; iSimuHandler++)
                {
                    TraficSimulatorInstance * pSimulator = m_lstTraficSimulators[iSimuHandler];
                    if (pSimulator->GetParentSimulator()->GetSimulationDescriptor().GetGraph()->hasChild(pFirstNode))
                    {
                        pPPathSimulator = pSimulator;
                    }
                }
            }

            if (!pPPathSimulator)
            {
                BOOST_LOG_TRIVIAL(warning) << "No known simulator able to handle trip " << pTrip->GetID();
                return NULL;
            }

            pPPath = new PPath(pTrip, pPattern->getLink()->getParent()->GetServiceType(), iPattern, pPPathSimulator);
        }

        lastPatternType = pPattern->getPatternType();
        lastLine = lastPatternType == PT_PublicTransport ? ((PublicTransportPattern*)pPattern)->getLine() : NULL;
    }

    // adding the last ppath if exists
    if (pPPath)
    {
        subPaths.push_back(*pPPath);
        delete pPPath;
    }

	// internal symuvia demand management
    if (subPaths.empty() && m_lstTraficSimulators.size()==1)
    {
		if( pTrip->GetOrigin()->getStrNodeName() == pTrip->GetDestination()->getStrNodeName())
		pPPath = new PPath(pTrip, ST_Driving, 0, m_lstTraficSimulators[0]);
		subPaths.push_back(*pPPath);
		delete pPPath;
    }

    TrackedUser * pResult = new TrackedUser(pTrip, subPaths);
    return pResult;
}

void UserEngine::AssignmentPeriodStart()
{
    // dont take the snapshot of the beginning of the prediction period every iteration...
    if (m_lstUserEngineSnapshot.empty())
    {
        m_lstUserEngineSnapshot.push_back(TakeSnapshot());
    }
}

void UserEngine::AssignmentPeriodEnded()
{
    m_lstUserEngineSnapshot.push_back(TakeSnapshot());
}

void UserEngine::AssignmentPeriodRollback()
{
    // If we have a newer snapshot for the end of the assignment period, discard it.
    if (m_lstUserEngineSnapshot.size() == 2)
    {
        delete m_lstUserEngineSnapshot.back();
        m_lstUserEngineSnapshot.pop_back();
    }
    // Restore the snapshot of the beginning of the prediction period
    RestoreSnapshot(m_lstUserEngineSnapshot.front());
}

void UserEngine::AssignmentPeriodCommit()
{

    // if we have a snapshot for the end of the assignment period, restore it. If not, there is no rolling horizon so nothing to be done
    if (m_lstUserEngineSnapshot.size() == 2)
    {
        RestoreSnapshot(m_lstUserEngineSnapshot.back());
    }

    // no need to keep any snapshot.
    for (size_t i = 0; i < m_lstUserEngineSnapshot.size(); i++)
    {
        delete m_lstUserEngineSnapshot[i];
    }
    m_lstUserEngineSnapshot.clear();

}

UserEngineSnapshot * UserEngine::TakeSnapshot()
{
    UserEngineSnapshot * pSnapshot = new UserEngineSnapshot();
    std::vector<int> & snapshotOwnedUsers = pSnapshot->GetOwnedUsers();
    snapshotOwnedUsers.reserve(m_OwnedUsers.size());
    for (std::map<int, TrackedUser*>::const_iterator iterUser = m_OwnedUsers.begin(); iterUser != m_OwnedUsers.end(); ++iterUser)
    {
        snapshotOwnedUsers.push_back(iterUser->first);
    }
    std::vector<TrackedUser> & snapshotAllUsers = pSnapshot->GetAllUsers();
    for (std::map<int, TrackedUser*>::const_iterator iterUser = m_AllUsers.begin(); iterUser != m_AllUsers.end(); ++iterUser)
    {
        snapshotAllUsers.push_back(TrackedUser(*iterUser->second));
    }
    pSnapshot->GetNextTripsToActivate() = m_NextTripsToActivate;
    return pSnapshot;
}

void UserEngine::RestoreSnapshot(UserEngineSnapshot* pSnapshot)
{
    for (std::map<int, TrackedUser*>::const_iterator iterUser = m_AllUsers.begin(); iterUser != m_AllUsers.end(); ++iterUser)
    {
        delete iterUser->second;
    }
    m_AllUsers.clear();
    const std::vector<TrackedUser> & snapshotAllUsers = pSnapshot->GetAllUsers();
    for (size_t iUser = 0; iUser < snapshotAllUsers.size(); iUser++)
    {
        const TrackedUser & trackedUser = snapshotAllUsers[iUser];
        m_AllUsers[trackedUser.GetTrip()->GetID()] = new TrackedUser(trackedUser);
    }

    m_OwnedUsers.clear();
    const std::vector<int> & snapshotOwnedUsers = pSnapshot->GetOwnedUsers();
    for (size_t iUser = 0; iUser < snapshotOwnedUsers.size(); iUser++)
    {
        int tripID = snapshotOwnedUsers[iUser];
        m_OwnedUsers[tripID] = m_AllUsers.at(tripID);
    }
    m_NextTripsToActivate = pSnapshot->GetNextTripsToActivate();
}

void UserEngine::setUserPathWriter(UserPathWriting *pUserPathWriter)
{
    m_pUserPathWriter = pUserPathWriter;
}

UserPathWriting * UserEngine::GetUserPathWriter() const
{
    return m_pUserPathWriter;
}


