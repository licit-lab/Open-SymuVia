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

	double	m_dbCurrentAcceptanceRate;		// Current acceptance rate of vehicles to reroute
	double	m_dbAcceptanceRateToApply;		// Acceptance rate of vehicles to reroute to apply

	double	m_dbDistanceLimit;		// Distance limit with control zone:for a candidate to reroute, 
									// if its distance to the zone is lesser than this value, the 
									// re-routing is not apply

	int		m_nNbVehsPassingBeforeReroutingProcess;			// Number of vehicles passing through before the rerouting process
	int		m_nNbVehsPassingAfterReroutingProcess;			// Number of vehicles passing through after the rerouting process
	int		m_nNbVehsNotPassingBofReroutingProcess;			// Number of vehicles not passing through because of rerouting process
	int		m_nNbVehsPassingBofReroutingProcess;				// Number of vehicles passing through because of rerouting process

	std::map<int, std::vector<Tuyau*>>		m_routedvehs;	// List of rerouted vehicles with their initial path

public:
	void		Init(int nID, double dbAcceptanceRate, double dbDistanceLimit, std::vector<Tuyau*>	Links);

	bool 		IsSelectedBannedZone(boost::shared_ptr<Vehicule> pVeh);
	bool 		IsVehiclePassesThrough(boost::shared_ptr<Vehicule> pVeh);
	bool		IsVehicleTooClose(std::vector<Tuyau*> remainingPath);

	bool 		IsPathPassesThrough(std::vector<Tuyau*> remainingPath);

	int			GetID() { return m_nID; };

	double		GetCurrentAcceptanceRate(){return m_dbCurrentAcceptanceRate;};
	void		SetCurrentAcceptanceRate(double dbAcceptanceRate) { m_dbCurrentAcceptanceRate = dbAcceptanceRate; };

	void		SetAcceptanceRateToApply(double dbAcceptanceRate) { m_dbAcceptanceRateToApply = dbAcceptanceRate; };

	void		Update();

	void		IncNbVehsPassingBeforeReroutingProcess(){m_nNbVehsPassingBeforeReroutingProcess++;}
	void		IncNbVehsPassingAfterReroutingProcess(){m_nNbVehsPassingAfterReroutingProcess++;}
	void		IncNbVehsNotPassingBofReroutingProcess(){m_nNbVehsNotPassingBofReroutingProcess++;}
	void		IncNbVehsPassingBofReroutingProcess(){m_nNbVehsPassingBofReroutingProcess++;}

	int			GetNbVehsPassingBeforeReroutingProcess(){return m_nNbVehsPassingBeforeReroutingProcess;};
	int			GetNbVehsPassingAfterReroutingProcess(){return m_nNbVehsPassingAfterReroutingProcess;}
	int			GetNbVehsNotPassingBofReroutingProcess(){return m_nNbVehsNotPassingBofReroutingProcess;}
	int			GetNbVehsPassingBofReroutingProcess(){return m_nNbVehsPassingBofReroutingProcess;}

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
	std::ofstream m_outputProbabilities;				// csv output 

	bool			m_bChangedSinceLastUpdate;

	double			m_dbComplianceRate;		// Rate of users' compliance (equal at 1 if all users follows the guideline) 

	std::vector<int> m_CompliantVehicle;	// List of compliant vehicle
	std::vector<int> m_NoCompliantVehicle;	// List of NO compliant vehicle

public:
	bool	Update();
	int		AddControlZone(double dbAcceptanceRate, double dbDistanceLimit, double dbComplianceRate, std::vector<Tuyau*> Links);
	
	bool	CheckAndRerouteVehicle(boost::shared_ptr<Vehicule> pVeh);
	void	ModifyControlZone(int id, double dbAcceptanceRate);

	void	Write(int idVeh, bool bActivation);
};

