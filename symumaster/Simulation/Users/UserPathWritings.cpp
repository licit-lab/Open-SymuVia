#include "UserPathWritings.h"

#include "UserEngines.h"
#include "UserPathWriting.h"
#include "PPath.h"

using namespace SymuMaster;

UserPathWritings::UserPathWritings()
{

}

UserPathWritings::UserPathWritings(const SimulationConfiguration & config, const UserEngines &userEngines)
{
    for (size_t iUserEngine = 0; iUserEngine < userEngines.GetUserEngines().size(); iUserEngine++)
    {
        m_lstUserPathWritings.push_back(new UserPathWriting(config, userEngines.GetUserEngines().at(iUserEngine)));
    }
}

UserPathWritings::~UserPathWritings()
{
    for (size_t iPathWriting = 0; iPathWriting < m_lstUserPathWritings.size(); iPathWriting++)
    {
        delete m_lstUserPathWritings[iPathWriting];
    }
}

bool UserPathWritings::WriteUserPaths(int iPeriod, int iIteration, int iInstanceOffset, const boost::posix_time::ptime &startTimeSimulation, const boost::posix_time::ptime &startTimePeriod, const boost::posix_time::time_duration & timeAssignmentPeriodDuration, const boost::posix_time::time_duration & timePredictionPeriodDuration, bool isFinalFile)
{
    if (!isFinalFile)
    {
        bool bOk = true;
        for (size_t i = 0; i < m_lstUserPathWritings.size() && bOk; i++)
        {
            bOk = m_lstUserPathWritings[i]->WriteUserPaths(iPeriod, iIteration, iInstanceOffset, (int)i, startTimeSimulation, startTimePeriod, timeAssignmentPeriodDuration, timePredictionPeriodDuration, isFinalFile);
        }
        return bOk;
    }
    else
    {
        // for final paths, we are in the case of the N simulations have been resynced so juste write one final result
        return m_lstUserPathWritings.front()->WriteUserPaths(iPeriod, iIteration, iInstanceOffset, 0, startTimeSimulation, startTimePeriod, timeAssignmentPeriodDuration, timePredictionPeriodDuration, isFinalFile);
    }
}


void UserPathWritings::AssignmentPeriodStart(int nbMaxInstancesNb)
{
    for (size_t i = 0; i < m_lstUserPathWritings.size() && i < (size_t)nbMaxInstancesNb; i++)
    {
        m_lstUserPathWritings[i]->AssignmentPeriodStart();
    }
}

void UserPathWritings::AssignmentPeriodRollback(int nbMaxInstancesNb)
{
    for (size_t i = 0; i < m_lstUserPathWritings.size() && i < (size_t)nbMaxInstancesNb; i++)
    {
        m_lstUserPathWritings[i]->AssignmentPeriodRollback();
    }
}

void UserPathWritings::AssignmentPeriodCommit()
{
    for (size_t i = 0; i < m_lstUserPathWritings.size(); i++)
    {
        m_lstUserPathWritings[i]->AssignmentPeriodCommit();
    }
}


