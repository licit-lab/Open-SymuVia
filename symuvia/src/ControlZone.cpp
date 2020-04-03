#include "stdafx.h"
#include "ControlZone.h"

#include "reseau.h"
#include "RandManager.h"
#include "vehicule.h"
#include "usage/Trip.h"
#include "SymuCoreManager.h"
#include "usage/TripNode.h"
#include "Connection.h"
#include "tuyau.h"
#include "TronconOrigine.h"

ControlZone::ControlZone()
{
}

ControlZone::ControlZone(Reseau * pNetwork, ControlZoneManagement *pManager)
{
	m_pNetwork = pNetwork;
	m_pManager = pManager;
}

ControlZone::~ControlZone()
{
}

void ControlZone::Init(int nID, double dbAcceptanceRate, double dbDistanceLimit, std::vector<Tuyau*> Links)
{
	m_nID = nID;
	m_dbAcceptanceRate = dbAcceptanceRate;
	m_dbDistanceLimit = dbDistanceLimit;
	m_Links = Links;
}

void ControlZone::Activation()
{
	// Nothing to do if acceptance rate is zero
	if(m_dbAcceptanceRate<= DBL_EPSILON)
		return;

	// Reroute vehicles already on the network

	// Search affected vehicles
	std::vector<boost::shared_ptr<Vehicule>> m_LstVehicles;
	
	for (size_t iVeh = 0; iVeh < m_pNetwork->m_LstVehicles.size(); iVeh++)
	{
		boost::shared_ptr<Vehicule> pVeh = m_pNetwork->m_LstVehicles[iVeh];
		CheckAndRerouteVehicle(pVeh);
	}
	
}

bool ControlZone::CheckAndRerouteVehicle(boost::shared_ptr<Vehicule> pVeh)
{
	// Nothing to do if acceptance rate is zero
	if(m_dbAcceptanceRate<= DBL_EPSILON)
		return false;

	// Check vehicle type
	std::string sVL = "VL";
	if (pVeh->GetType()->GetLabel() != sVL)
		return false;

	// Check vehicle remaining path (if it has to go through the control zone)
	std::vector<Tuyau*> remainingPath = pVeh->GetTrip()->GetRemainingPath();
	bool bFound = false;
	double dbDst;
	for (size_t i = 0; i < this->m_Links.size(); i++)
	{
		if (std::find(remainingPath.begin(), remainingPath.end(), m_Links[i]) != remainingPath.end())
		{
			// Check distance limit 
			pVeh->ComputeDistance(pVeh->GetLink(1), pVeh->GetPos(1), m_Links[i], 0, dbDst);
			if (dbDst > this->m_dbDistanceLimit)
				bFound = true;
			break;
		}
	}
	if (!bFound)
		return false;

	// Is candidate ?
	double dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
	if (dbRand > m_dbAcceptanceRate)
		return false;

	// Calculate new path
	std::vector<PathResult> newremainingpaths;
	std::map<std::vector<Tuyau*>, double> MapFilteredItis;
	
	TronconOrigine	*pTO = new TronconOrigine(remainingPath.front(), NULL);

	m_pNetwork->GetSymuScript()->ComputePaths(pTO,
		pVeh->GetDestination(), pVeh->GetType(),
		m_pNetwork->GetInstSim()*m_pNetwork->GetTimeStep(),
		1, 0, remainingPath, m_Links, newremainingpaths, MapFilteredItis, false);

	delete(pTO);

	if (newremainingpaths.size() == 0)		// No other paths
		return false;

	// Apply new path
	const std::vector<Tuyau*> newremainingpath = newremainingpaths.front().links;
	if (!pVeh->GetTrip()->ChangeRemainingPath(newremainingpath, pVeh.get()))
		return false;

	std::cout << "Rerouted vehicle id: " << pVeh->GetID() << std::endl;
	m_pManager->Write(pVeh->GetID(), m_nID, 1);

	m_routedvehs.insert(std::make_pair(pVeh->GetID(), std::vector<Tuyau*>()));
	for (size_t i = 0; i < pVeh->GetTrip()->GetFullPath()->size(); i++)
		m_routedvehs[pVeh->GetID()].push_back((pVeh->GetTrip()->GetFullPath()->at(i)));

	return true;
}

void ControlZone::Deactivation()
{
	// Nothing to do if acceptance rate is zero
	if(m_dbAcceptanceRate<= DBL_EPSILON)
		return;

	// Reconsider vehicles that have been rerouted
	std::map<int, std::vector<Tuyau*>>::iterator itRV;
	std::vector<Tuyau*> linkstoavoid;

	for (itRV = m_routedvehs.begin(); itRV != m_routedvehs.end(); itRV++)
	{
		boost::shared_ptr<Vehicule> pVeh = m_pNetwork->GetVehiculeFromID(itRV->first);

		if (!pVeh)				// maybe, vehicle has gone off the network
			continue;

		std::vector<Tuyau*> remainingPath = pVeh->GetTrip()->GetRemainingPath();

		// Calculate path
		std::vector<PathResult> newremainingpaths;
		std::map<std::vector<Tuyau*>, double> MapFilteredItis;

		TronconOrigine	*pTO = new TronconOrigine(remainingPath.front(), NULL);

		m_pNetwork->GetSymuScript()->ComputePaths(pTO,
			pVeh->GetDestination(), pVeh->GetType(),
			m_pNetwork->GetInstSim()*m_pNetwork->GetTimeStep(),
			1, 0, remainingPath, linkstoavoid, newremainingpaths, MapFilteredItis, false);

		delete(pTO);

		if (newremainingpaths.size() == 0)		// No other paths
			continue;

		// Apply new path
		const std::vector<Tuyau*> newremainingpath = newremainingpaths.front().links;
		if (!pVeh->GetTrip()->ChangeRemainingPath(newremainingpath, pVeh.get()))
			continue;

		std::cout << "Retrieving the initial route for the vehicle with id: " << pVeh->GetID() << std::endl;

		m_pManager->Write(pVeh->GetID(), m_nID, 0);
	}

	m_routedvehs.erase(m_routedvehs.begin(), m_routedvehs.end());
}

ControlZoneManagement::ControlZoneManagement()
{
}

ControlZoneManagement::ControlZoneManagement(Reseau * pNetwork)
{
	m_pNetwork = pNetwork;
	m_nCount = 330;

	if (!m_output.is_open())
	{
		m_output.open((pNetwork->GetPrefixOutputFiles() + "_ctrlzonedata_" + ".csv").c_str(), std::ios::out);
		m_output.setf(std::ios::fixed);

		m_output << "time" << CSV_SEPARATOR << "zone" << CSV_SEPARATOR << "veh" << CSV_SEPARATOR << "activation" << CSV_NEW_LINE;
	}
}

ControlZoneManagement::~ControlZoneManagement()
{
	std::vector< ControlZone*>::iterator itCZ;

	if (m_output.is_open())
	{
		m_output.close();
	}

	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		delete (*itCZ);
}

bool ControlZoneManagement::IsActive()
{
	return m_ControlZones.size() > 0;
}

int ControlZoneManagement::AddControlZone(double dbAcceptanceRate, double dbDistanceLimit, std::vector<Tuyau*> Links)
{
	ControlZone* pControlZone = new ControlZone(m_pNetwork, this);
	pControlZone->Init(++m_nCount,dbAcceptanceRate, dbDistanceLimit, Links);

	pControlZone->Activation();

	m_ControlZones.push_back(pControlZone);

	return m_nCount;
}

void ControlZoneManagement::RemoveControlZone(int id)
{
	std::vector< ControlZone*>::iterator itCZ;

	for(itCZ=m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		if ( (*itCZ)->GetID() == id)
		{
			(*itCZ)->Deactivation();
			m_ControlZones.erase(itCZ);
			break;
		}
}

void ControlZoneManagement::ModifyControlZone(int id, double dbAcceptanceRate)
{
	std::vector< ControlZone*>::iterator itCZ;

	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		if ((*itCZ)->GetID() == id)
		{
			(*itCZ)->Deactivation();
			(*itCZ)->SetAcceptanceRate(dbAcceptanceRate);
			(*itCZ)->Activation();
			break;
		}
}

bool ControlZoneManagement::CheckAndRerouteVehicle(boost::shared_ptr<Vehicule> pVeh)
{
	std::vector< ControlZone*>::iterator itCZ;
	bool bRerouted=false;

	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
	{
		bRerouted |= (*itCZ)->CheckAndRerouteVehicle(pVeh);
	}
	return bRerouted;
}

void ControlZoneManagement::Write(int idVeh, int sidZone, bool bActivation)
{
	if (m_output.is_open())
	{
		m_output << m_pNetwork->GetInstSim() << CSV_SEPARATOR << sidZone << CSV_SEPARATOR << idVeh << CSV_SEPARATOR << bActivation << CSV_NEW_LINE;
	}
}
