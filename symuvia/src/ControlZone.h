#pragma once
#ifndef ControlZoneH
#define ControlZoneH

#endif // ControlZoneH

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <fstream>

#define CSV_SEPARATOR ";"
#define CSV_MINOR_SEPARATOR " "
#define CSV_NEW_LINE std::endl

class Reseau;
class Tuyau;
class Vehicule;
class ControlZoneManagement;

class ControlZone
{
public:
	ControlZone();
	ControlZone(Reseau * pNetwork, ControlZoneManagement *pManager);
	virtual ~ControlZone();

private:
	Reseau *m_pNetwork;
	ControlZoneManagement	*m_pManager;

	int		m_nID;					// Unique identifier of the control zone 
	double	m_dbAcceptanceRate;		// Acceptance rate of vehicles to reroute
	double	m_dbDistanceLimit;		// Distance limit with control zone:for a candidate to reroute, 
									// if its distance to the zone is lesser than this value, the 
									// re-routing is not apply

	std::map<int, std::vector<Tuyau*>>		m_routedvehs;	// List of rerouted vehicles with their initial path

public:
	void		Init(int nID, double dbAcceptanceRate, double dbDistanceLimit, std::vector<Tuyau*>	Links);

	bool 		IsSelectedBannedZone(boost::shared_ptr<Vehicule> pVeh);
	bool 		IsVehiclePassesThrough(boost::shared_ptr<Vehicule> pVeh);
	bool		IsVehicleTooClose(boost::shared_ptr<Vehicule> pVeh);

	int			GetID() { return m_nID; }

	void		SetAcceptanceRate(double dbAcceptanceRate) { m_dbAcceptanceRate = dbAcceptanceRate; };

	std::vector<Tuyau*>	GetLinks() { return m_Links; };

	std::vector<Tuyau*>	m_Links;	// List of links compound the zone
};

class ControlZoneManagement
{
public:
	ControlZoneManagement();
	ControlZoneManagement(Reseau * pNetwork);
	virtual ~ControlZoneManagement();

private:
	Reseau			*m_pNetwork;
	int				m_nCount;
	std::vector< ControlZone*>	m_ControlZones;			// List of control zones

	std::ofstream m_output;								// csv output 

	bool			m_bChangedSinceLastUpdate;
		
public:
	bool	Update();
	int		AddControlZone(double dbAcceptanceRate, double dbDistanceLimit, std::vector<Tuyau*> Links);
	
	bool	CheckAndRerouteVehicle(boost::shared_ptr<Vehicule> pVeh);
	void	ModifyControlZone(int id, double dbAcceptanceRate);

	void	Write(int idVeh, bool bActivation);
};

