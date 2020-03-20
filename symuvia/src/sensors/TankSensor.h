#pragma once
#ifndef TankSensorH
#define TankSensorH

#include "AbstractSensor.h"

#include <vector>
#include <set>
#include <map>


class Tuyau;
class SensorsManager;

class TankSensorData
{
public:
    int idVeh;
    double dbEntryTime;
    double dbExitTime;
    double dbTraveleldDistance;
    bool bCreatedInZone;
    bool bDestroyedInZone;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

class TankSensorSnapshot : public AbstractSensorSnapshot
{
public:
    std::map<int, std::pair<std::pair<double, double>, bool> > vehiclesInsideZone;
    std::vector<TankSensorData> results;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

class TankSensor : public AbstractSensor
{
public:
    TankSensor();
    TankSensor(Reseau * pNetwork,
        SensorsManager * pParentSensorsManager,
        const char* strNom,
        const std::vector<Tuyau*> & Tuyaux);
    virtual ~TankSensor();

    virtual std::string GetSensorPeriodXMLNodeName() const;

    virtual void CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh);

    virtual void AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink = DBL_MAX);

    virtual void WriteDef(DocTrafic* pDocTrafic);
    virtual void Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput);

    virtual void PrepareNextPeriod();

    virtual void VehicleDestroyed(Vehicule * pVeh);


    virtual AbstractSensorSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup);

protected:
    std::set<Tuyau*> m_Tuyaux;

    // Véhicule dans la zone avec l'instant d'entrée et la distance parcourue au moment de l'entrée
    std::map<Vehicule*, std::pair<std::pair<double,double>, bool> > m_vehiclesInsideZone;

    // résultats finaux
    std::vector<TankSensorData> m_results;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // TankSensorH