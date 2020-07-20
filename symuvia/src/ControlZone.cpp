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

	m_dbCurrentAcceptanceRate=0;
	m_dbAcceptanceRateToApply=0;

}

ControlZone::~ControlZone()
{
}

void ControlZone::Init(int nID, double dbAcceptanceRate, double dbDistanceLimit, std::vector<Tuyau*> Links)
{
	m_nID = nID;
	m_dbAcceptanceRateToApply = dbAcceptanceRate;
	m_dbDistanceLimit = dbDistanceLimit;
	m_Links = Links;

	m_nNbVehsPassingBeforeReroutingProcess=0;			// Number of vehicles passing through before the rerouting process
	m_nNbVehsPassingAfterReroutingProcess=0;			// Number of vehicles passing through after the rerouting process
	m_nNbVehsNotPassingBofReroutingProcess=0;			// Number of vehicles not passing through because of rerouting process
	m_nNbVehsPassingBofReroutingProcess=0;				// Number of vehicles passing through because of rerouting process
}

bool ControlZone::IsSelectedBannedZone(boost::shared_ptr<Vehicule> pVeh)
{
	if (m_dbCurrentAcceptanceRate <= DBL_EPSILON)
		return false; // nothing to do

	// Check vehicle type
	std::string sVL = "VL";
	if (pVeh->GetType()->GetLabel() != sVL)
		return false; // nothing to do

	// Random
	double dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
	if (dbRand <= m_dbCurrentAcceptanceRate)
	{
		return true;
	}

	return false;
}

bool ControlZone::IsVehiclePassesThrough(boost::shared_ptr<Vehicule> pVeh)
{
	std::vector<Tuyau*> remainingPath = pVeh->GetTrip()->GetRemainingPath();
	
	for (size_t i = 0; i < this->m_Links.size(); i++)
	{
		if (std::find(remainingPath.begin(), remainingPath.end(), m_Links[i]) != remainingPath.end())
		{
			return true;
		}
	}
	return false;
}

bool ControlZone::IsPathPassesThrough(std::vector<Tuyau*> remainingPath)
{
	for (size_t i = 0; i < this->m_Links.size(); i++)
	{
		if (std::find(remainingPath.begin(), remainingPath.end(), m_Links[i]) != remainingPath.end())
		{
			return true;
		}
	}
	return false;
}

bool ControlZone::IsVehicleTooClose(std::vector<Tuyau*> remainingPath)
{
	// Check distance (if it has to go through the control zone)
	double dbDst;
	for (size_t i = 0; i < m_Links.size(); i++)
	{
		if (std::find(remainingPath.begin(), remainingPath.end(), m_Links[i]) != remainingPath.end())
		{
			// Check distance limit 
			Point pt(m_Links[i]->GetExtAmont()->dbX,m_Links[i]->GetExtAmont()->dbY,0.0);
			dbDst=remainingPath.front()->GetExtAmont()->DistanceTo(pt);
			if (dbDst < m_dbDistanceLimit)
			{
				return true;
			}
			
		}
	}
	return false;
}

void ControlZone::Update()
{
	m_dbCurrentAcceptanceRate=m_dbAcceptanceRateToApply;

	m_nNbVehsPassingBeforeReroutingProcess=0;			// Number of vehicles passing through before the rerouting process
	m_nNbVehsPassingAfterReroutingProcess=0;			// Number of vehicles passing through after the rerouting process
	m_nNbVehsNotPassingBofReroutingProcess=0;			// Number of vehicles not passing through because of rerouting process
	m_nNbVehsPassingBofReroutingProcess=0;				// Number of vehicles passing through because of rerouting process
}

ControlZoneManagement::ControlZoneManagement()
{
}

ControlZoneManagement::ControlZoneManagement(Reseau * pNetwork)
{
	m_pNetwork = pNetwork;
	m_nCount = 330;

	m_bChangedSinceLastUpdate = false;

	m_dbComplianceRate = 1;

	if (!m_output.is_open())
	{
		m_output.open((pNetwork->GetPrefixOutputFiles() + "_ctrlzonedata_" + ".csv").c_str(), std::ios::out);
		m_output.setf(std::ios::fixed);

		m_output << "time" << CSV_SEPARATOR << "veh" << CSV_SEPARATOR << "already rerouted" << CSV_SEPARATOR << "zones selected at random" << CSV_SEPARATOR << "zones not selected at random" << CSV_SEPARATOR << "zones selected but not banned" << CSV_SEPARATOR << "path" << CSV_SEPARATOR << CSV_NEW_LINE;
	}

	if (!m_outputProbabilities.is_open())
	{
		m_outputProbabilities.open((pNetwork->GetPrefixOutputFiles() + "_ctrlzonedata_probabilities" + ".csv").c_str(), std::ios::out);
		m_outputProbabilities.setf(std::ios::fixed);

		m_outputProbabilities << "TimeEnd" << CSV_SEPARATOR << "Zone" << CSV_SEPARATOR << "InitialProba" << CSV_SEPARATOR << "NbVehsPassingBeforeReroutingProcess" << CSV_SEPARATOR << "NbVehsPassingAfterReroutingProcess" << CSV_SEPARATOR << "NbVehsNotPassingBofReroutingProcess" << CSV_SEPARATOR << "NbVehsPassingBofReroutingProcess" << CSV_NEW_LINE;
	}

}

ControlZoneManagement::~ControlZoneManagement()
{
	std::vector< ControlZone*>::iterator itCZ;

	if (m_output.is_open())
	{
		m_output.close();
	}

	if (m_outputProbabilities.is_open())
	{
		m_outputProbabilities.close();
	}

	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		delete (*itCZ);
}

bool ControlZoneManagement::Update()
{
	std::vector< ControlZone*>::iterator itCZ;

	if (m_ControlZones.size() == 0)
		return false;

	// Save last period
	if(m_outputProbabilities)
	{
		for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		{			
			m_outputProbabilities << m_pNetwork->GetInstSim() << CSV_SEPARATOR << (*itCZ)->GetID() << CSV_SEPARATOR << (*itCZ)->GetCurrentAcceptanceRate() << CSV_SEPARATOR; 
			m_outputProbabilities << (*itCZ)->GetNbVehsPassingAfterReroutingProcess() << CSV_SEPARATOR << (*itCZ)->GetNbVehsPassingAfterReroutingProcess() << CSV_SEPARATOR;
			m_outputProbabilities << (*itCZ)->GetNbVehsNotPassingBofReroutingProcess() << CSV_SEPARATOR << (*itCZ)->GetNbVehsPassingBofReroutingProcess() << CSV_SEPARATOR; 
			m_outputProbabilities << std::endl;
		}
	}

	if (!m_bChangedSinceLastUpdate)
		return false;

	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		(*itCZ)->Update();

	for (size_t iVeh = 0; iVeh < m_pNetwork->m_LstVehicles.size(); iVeh++)
	{
		boost::shared_ptr<Vehicule> pVeh = m_pNetwork->m_LstVehicles[iVeh];
		CheckAndRerouteVehicle(pVeh);
	}

	m_bChangedSinceLastUpdate = false;

	return true;
}

int ControlZoneManagement::AddControlZone(double dbAcceptanceRate, double dbDistanceLimit, double dbComplianceRate, std::vector<Tuyau*> Links)
{
	ControlZone* pControlZone = new ControlZone(m_pNetwork, this);
	pControlZone->Init(++m_nCount,dbAcceptanceRate, dbDistanceLimit, Links);

	m_ControlZones.push_back(pControlZone);

	m_bChangedSinceLastUpdate = true;

	m_dbComplianceRate = dbComplianceRate;

	return m_nCount;
}


void ControlZoneManagement::ModifyControlZone(int id, double dbAcceptanceRate)
{
	std::vector< ControlZone*>::iterator itCZ;

	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		if ((*itCZ)->GetID() == id)
		{
			(*itCZ)->SetAcceptanceRateToApply(dbAcceptanceRate);
			m_bChangedSinceLastUpdate = true;

			break;
		}
}

bool ControlZoneManagement::CheckAndRerouteVehicle(boost::shared_ptr<Vehicule> pVeh)
{
	std::vector< ControlZone*>::iterator itCZ;
	std::vector<Tuyau*> BannedLinks;
	
	bool bRerouted = false;

	std::vector<ControlZone*> NoSelectedBannedZone;
	std::vector<ControlZone*> SelectedBannedZones;
	std::vector<ControlZone*> BannedZones;
	std::vector<ControlZone*> NoCandidateBannedZone;
	std::vector<ControlZone*> InitialCrossedPassedZones;
	
	TronconOrigine	*pTO;
	std::vector<Tuyau*> remainingPath;
	std::vector<PathResult> newremainingpaths;
	std::map<std::vector<Tuyau*>, double> MapFilteredItis;

	// Get compliance of vehicle
	/*if( std::find(m_CompliantVehicle.begin(), m_CompliantVehicle.end(), pVeh->GetID()) == m_CompliantVehicle.end() and 
	std::find(m_NoCompliantVehicle.begin(), m_NoCompliantVehicle.end(), pVeh->GetID())== m_NoCompliantVehicle.end() )
	{
		{
			double dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
			if (dbRand > m_dbComplianceRate)
			{
				m_NoCompliantVehicle.push_back(pVeh->GetID());
				std::cout << pVeh->GetID() << ": No Compliant Vehicle " << std::endl;
			}
			else
			{
				m_CompliantVehicle.push_back(pVeh->GetID());
				std::cout << pVeh->GetID() << " Compliant Vehicle " << std::endl; 
			}
		}	
	}*/

	/*bool bCompliantVehicle = (std::find(m_CompliantVehicle.begin(), m_CompliantVehicle.end(), pVeh->GetID()) != m_CompliantVehicle.end());
	std::cout << pVeh->GetID() << " " << bCompliantVehicle << std::endl; 
	
	if(!bCompliantVehicle)
		return false;*/

	//std::cout << "CheckAndRerouteVehicle id: " << pVeh->GetID() << std::endl;

	m_output << m_pNetwork->GetInstSim() << CSV_SEPARATOR << pVeh->GetID() << CSV_SEPARATOR << pVeh->IsAlreadyRerouted() << CSV_SEPARATOR;

	remainingPath = pVeh->GetTrip()->GetRemainingPath();
    pTO = new TronconOrigine(remainingPath.front(), NULL);

	// Trace - Par quel zone passe le véhicule avant le processus de bannissement ?
	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
	{
		if( (*itCZ)->IsPathPassesThrough(remainingPath) ) 
		{
			(*itCZ)->IncNbVehsPassingBeforeReroutingProcess();
			InitialCrossedPassedZones.push_back((*itCZ)); 
		}
	}

	double dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
	std::cout << pVeh->GetID() << " " << m_dbComplianceRate << " " << dbRand << std::endl; 
	if (dbRand > m_dbComplianceRate)
	{
		// Trace - Par quel zone passe le véhicule après le processus de bannissement ?
		for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		{
			if( (*itCZ)->IsPathPassesThrough(remainingPath) ) 
			{
				(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
			}
		}
		return false;
	}

    // The vehicles not already rerouted are not taken into account - they stay on their initial path
    if(pVeh->IsAlreadyRerouted())
    {
        // Calculate shortest path (without banned zones)
		const std::vector<Tuyau*> linksToAvoid;

        m_pNetwork->GetSymuScript()->ComputePaths(pTO,
            pVeh->GetDestination(), pVeh->GetType(),
            m_pNetwork->GetInstSim()*m_pNetwork->GetTimeStep(),
            1, 0, remainingPath, linksToAvoid, newremainingpaths, MapFilteredItis, false);

        if (newremainingpaths.size() == 0)		// No other paths
        {
            delete pTO;
			m_output << std::endl;

			// Trace
			for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
			{
				if( (*itCZ)->IsPathPassesThrough(remainingPath) ) 
					(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
			}
            return false;
        }
	}

	// Build the map of banned zones
	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		if ((*itCZ)->IsSelectedBannedZone(pVeh))
		{
			SelectedBannedZones.push_back((*itCZ));
		}
		else
		{
			NoSelectedBannedZone.push_back((*itCZ));
		}

	for (itCZ = SelectedBannedZones.begin(); itCZ != SelectedBannedZones.end(); itCZ++)
		m_output << (*itCZ)->GetID() << " ";
	
	m_output << CSV_SEPARATOR;

	for (itCZ = NoSelectedBannedZone.begin(); itCZ != NoSelectedBannedZone.end(); itCZ++)
		m_output << (*itCZ)->GetID() << " ";
	
	m_output << CSV_SEPARATOR;

	if(SelectedBannedZones.size()==0)	
    {	
        if(pVeh->IsAlreadyRerouted())
        {
            // Apply new path
            pVeh->GetTrip()->ChangeRemainingPath(newremainingpaths.front().links, pVeh.get());
			pVeh->SetAlreadyRerouted( true );

			std::vector<Tuyau*>::iterator itL;
			m_output << CSV_SEPARATOR;
			for(itL=newremainingpaths.front().links.begin();itL!=newremainingpaths.front().links.end();itL++)
			{
				m_output << (*itL)->GetLabel() << " ";
			}
			m_output << std::endl;

			// Trace
			std::vector<ControlZone*>::iterator itCCZ;
			std::vector<ControlZone*> FinalCrossedPassedZones;
			bool bFound;
			for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
			{
				if( (*itCZ)->IsPathPassesThrough(newremainingpaths.front().links) )
				{
					(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
					FinalCrossedPassedZones.push_back((*itCZ));
				
					if( std::find(InitialCrossedPassedZones.begin(),InitialCrossedPassedZones.end(),(*itCZ)) == InitialCrossedPassedZones.end())
						(*itCZ)->IncNbVehsPassingBofReroutingProcess();		// Not found in initial path (but in final path)
				}
			}

			for (itCCZ = InitialCrossedPassedZones.begin(); itCCZ != InitialCrossedPassedZones.end(); itCCZ++)
			{
				if( std::find(FinalCrossedPassedZones.begin(),FinalCrossedPassedZones.end(),(*itCCZ)) == FinalCrossedPassedZones.end())
					(*itCCZ)->IncNbVehsNotPassingBofReroutingProcess();		// Not found in final path (but in initial path)
			}
            return true;
        }

		m_output << std::endl;

		// Trace
		for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		{
			if( (*itCZ)->IsPathPassesThrough(remainingPath) )
				(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
		}
		return false;
    }

	// Check if the vehicle passes through the banned zones (with the new shortest path)
	bool bPasses = false;
	for (itCZ = SelectedBannedZones.begin(); itCZ != SelectedBannedZones.end(); itCZ++)
	{
        if(pVeh->IsAlreadyRerouted())
        {
            if ((*itCZ)->IsPathPassesThrough(newremainingpaths.front().links))
            {
                bPasses = true;
                break;
            }
        }
        else
        {
            if ((*itCZ)->IsVehiclePassesThrough(pVeh))
            {
                bPasses = true;
                break;
            }
			    
        }
	}

	if(!bPasses) // None banned zone is on the shortest path of the vehicle, it keeps it
	{
        if(pVeh->IsAlreadyRerouted())
        {
            // Apply new path
            std::vector<Tuyau*> newremainingpath = newremainingpaths.front().links;
            pVeh->GetTrip()->ChangeRemainingPath(newremainingpath, pVeh.get());
			pVeh->SetAlreadyRerouted( true );

			std::vector<Tuyau*>::iterator itL;
			m_output << CSV_SEPARATOR;
			for(itL=newremainingpaths.front().links.begin();itL!=newremainingpaths.front().links.end();itL++)
			{
				m_output << (*itL)->GetLabel() << " ";
			}
			m_output << std::endl;

			// Trace
			std::vector<ControlZone*>::iterator itCCZ;
			std::vector<ControlZone*> FinalCrossedPassedZones;
			bool bFound;
			for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
			{
				if( (*itCZ)->IsPathPassesThrough(newremainingpaths.front().links) )
				{
					(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
					FinalCrossedPassedZones.push_back((*itCZ));
				
					if( std::find(InitialCrossedPassedZones.begin(),InitialCrossedPassedZones.end(),(*itCZ)) == InitialCrossedPassedZones.end())
						(*itCZ)->IncNbVehsPassingBofReroutingProcess();		// Not found in initial path (but in final path)
				}
			}

			for (itCCZ = InitialCrossedPassedZones.begin(); itCCZ != InitialCrossedPassedZones.end(); itCCZ++)
			{
				if( std::find(FinalCrossedPassedZones.begin(),FinalCrossedPassedZones.end(),(*itCCZ)) == FinalCrossedPassedZones.end())
					(*itCCZ)->IncNbVehsNotPassingBofReroutingProcess();		// Not found in final path (but in initial path)
			}
			
            return true; 
        }
        else
		{
			m_output << std::endl;

			// Trace
			for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
			{
				if( (*itCZ)->IsPathPassesThrough(remainingPath) )
					(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
			}
            return false;   // nothing to do
		}
	}

	// Check if the vehicle is too close to the banned zones
    std::vector<Tuyau*> rm;
    if(pVeh->IsAlreadyRerouted())
        rm = newremainingpaths.front().links;
    else 
        rm = pVeh->GetTrip()->GetRemainingPath();

	for (itCZ = SelectedBannedZones.begin(); itCZ != SelectedBannedZones.end(); itCZ++)
	{
		if (!(*itCZ)->IsVehicleTooClose(rm))
		{
			bRerouted = true;
			BannedLinks.insert(BannedLinks.end(), (*itCZ)->m_Links.begin(), (*itCZ)->m_Links.end());
			BannedZones.push_back((*itCZ));
		}
		else
		{
			 NoCandidateBannedZone.push_back((*itCZ));
		}
	}

	for(itCZ=NoCandidateBannedZone.begin();itCZ!=NoCandidateBannedZone.end();itCZ++)
		m_output << (*itCZ)->GetID() << " ";
	m_output << CSV_SEPARATOR;
	
	if (!bRerouted)
        if(pVeh->IsAlreadyRerouted())
        {
            // Apply new path
            pVeh->GetTrip()->ChangeRemainingPath(newremainingpaths.front().links, pVeh.get());
			pVeh->SetAlreadyRerouted( true );

			std::vector<Tuyau*>::iterator itL;
			
			for(itL=newremainingpaths.front().links.begin();itL!=newremainingpaths.front().links.end();itL++)
			{
				m_output << (*itL)->GetLabel() << " ";
			}
			m_output << std::endl;

			// Trace
			std::vector<ControlZone*>::iterator itCCZ;
			std::vector<ControlZone*> FinalCrossedPassedZones;
			bool bFound;
			for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
			{
				if( (*itCZ)->IsPathPassesThrough(newremainingpaths.front().links) )
				{
					(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
					FinalCrossedPassedZones.push_back((*itCZ));
				
					if( std::find(InitialCrossedPassedZones.begin(),InitialCrossedPassedZones.end(),(*itCZ)) == InitialCrossedPassedZones.end())
						(*itCZ)->IncNbVehsPassingBofReroutingProcess();		// Not found in initial path (but in final path)
				}
			}

			for (itCCZ = InitialCrossedPassedZones.begin(); itCCZ != InitialCrossedPassedZones.end(); itCCZ++)
			{
				if( std::find(FinalCrossedPassedZones.begin(),FinalCrossedPassedZones.end(),(*itCCZ)) == FinalCrossedPassedZones.end())
					(*itCCZ)->IncNbVehsNotPassingBofReroutingProcess();		// Not found in final path (but in initial path)
			}
            return true; 
        }
        else
		{
			m_output << std::endl;

			// Trace
			for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
			{
				if( (*itCZ)->IsPathPassesThrough(remainingPath) )
					(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
			}
		    return false;
		}

	//std::cout << "Calculate new path with banned zones " << std::endl; 

	// Calculate new path with banned zones
	m_pNetwork->GetSymuScript()->ComputePaths(pTO,
		pVeh->GetDestination(), pVeh->GetType(),
		m_pNetwork->GetInstSim()*m_pNetwork->GetTimeStep(),
		1, 0, remainingPath, BannedLinks, newremainingpaths, MapFilteredItis, false);

	if (newremainingpaths.size() == 0)		// No other paths
	{
		m_output << CSV_NEW_LINE;

		// Trace
		for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		{
			if( (*itCZ)->IsPathPassesThrough(remainingPath) )
				(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
		}

		return false;
	}

	// Compliance computation
	//if (m_dbComplianceRate < 1)
	//{
	//	double dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
	//	if (dbRand > m_dbComplianceRate)
	//	{
	//		m_output << CSV_NEW_LINE;
	//
	//		std::cout << " No compliance for vehicle id: " << pVeh->GetID() << std::endl;
	//
	//		return false;
	//	}
	//}

	// Apply new path
	std::vector<Tuyau*> newremainingpath = newremainingpaths.front().links;
	if (!pVeh->GetTrip()->ChangeRemainingPath(newremainingpath, pVeh.get()))
	{
		m_output << CSV_NEW_LINE;

		// Trace
		for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
		{
			if( (*itCZ)->IsPathPassesThrough(remainingPath) )
				(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
		}
		return false;
	}

	// Change path OK
	pVeh->SetAlreadyRerouted( true );
	std::vector<Tuyau*>::iterator itL;
	
	for(itL=newremainingpaths.front().links.begin();itL!=newremainingpaths.front().links.end();itL++)
	{
		m_output << (*itL)->GetLabel() << " ";
	}
	m_output << std::endl;

	std::cout << "Rerouted vehicle id: " << pVeh->GetID() << std::endl;
	
	delete(pTO);

	// Trace
	std::vector<ControlZone*>::iterator itCCZ;
	std::vector<ControlZone*> FinalCrossedPassedZones;
	bool bFound;
	for (itCZ = m_ControlZones.begin(); itCZ != m_ControlZones.end(); itCZ++)
	{
		if( (*itCZ)->IsPathPassesThrough(newremainingpaths.front().links) )
		{
			(*itCZ)->IncNbVehsPassingAfterReroutingProcess();
			FinalCrossedPassedZones.push_back((*itCZ));
		
			if( std::find(InitialCrossedPassedZones.begin(),InitialCrossedPassedZones.end(),(*itCZ)) == InitialCrossedPassedZones.end())
				(*itCZ)->IncNbVehsPassingBofReroutingProcess();		// Not found in initial path (but in final path)
		}
	}

	for (itCCZ = InitialCrossedPassedZones.begin(); itCCZ != InitialCrossedPassedZones.end(); itCCZ++)
	{
		if( std::find(FinalCrossedPassedZones.begin(),FinalCrossedPassedZones.end(),(*itCCZ)) == FinalCrossedPassedZones.end())
			(*itCCZ)->IncNbVehsNotPassingBofReroutingProcess();		// Not found in final path (but in initial path)
	}

	return true;
}

void ControlZoneManagement::Write(int idVeh, bool bActivation)
{
	if (m_output.is_open())
	{
		m_output << m_pNetwork->GetInstSim() << CSV_SEPARATOR  << idVeh << CSV_SEPARATOR << bActivation << CSV_NEW_LINE;
	}
}
