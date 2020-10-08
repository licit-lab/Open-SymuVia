#include "UserEngineSnapshot.h"

using namespace SymuMaster;

UserEngineSnapshot::UserEngineSnapshot()
{
}

UserEngineSnapshot::~UserEngineSnapshot()
{
}

std::vector<int> & UserEngineSnapshot::GetOwnedUsers()
{
    return m_OwnedUsers;
}

std::vector<TrackedUser> & UserEngineSnapshot::GetAllUsers()
{
    return m_AllUsers;
}

std::map<SymuCore::Population*, size_t> & UserEngineSnapshot::GetNextTripsToActivate()
{
    return m_NextTripsToActivate;
}

