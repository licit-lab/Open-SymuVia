#pragma once
#ifndef MFDSensorH
#define MFDSensorH

#include "AbstractSensor.h"

#include <vector>

class EdieSensorSnapshot;
class EdieSensor;
class PonctualSensor;
class PonctualSensorSnapshot;
class Tuyau;
class SensorsManager;

class MFDSensorSnapshot : public AbstractSensorSnapshot
{
public:
    std::vector<EdieSensorSnapshot> edieData;
    std::vector<PonctualSensorSnapshot> ponctualData;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

class MFDSensor : public AbstractSensor
{
public:
    MFDSensor();
    MFDSensor(Reseau * pNetwork,
        SensorsManager * pParentSensorsManager,
        double dbPeriodeAgregation,
        const char* strNom,
        const std::vector<Tuyau*> & Tuyaux,
        const std::vector<double> & dbPosDebut,
        const std::vector<double> & dbPosFin,
        const std::vector<int> & eTuyauType,
        bool bIsMFD,
        bool bIsSimpleMFD);
    virtual ~MFDSensor();

    virtual std::string GetSensorPeriodXMLNodeName() const;

    virtual void CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh);

    virtual void AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink = DBL_MAX);

    virtual void WriteDef(DocTrafic* pDocTrafic);
    virtual void Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput);

    virtual void PrepareNextPeriod();

    const std::vector<EdieSensor*> &GetLstSensors();
    bool IsEdie();

    virtual AbstractSensorSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup);

	double GetTotalTravelTime() { return m_dbTotalTravelTime; };
	double GetTotalTravelDistance() { return m_dbTotalTravelDistance; };

protected:
    std::vector<EdieSensor*> lstSensors;
    std::vector<PonctualSensor*> lstPonctualSensors;
    std::vector<PonctualSensor*> lstNonStrictPonctualSensors;
    std::vector<PonctualSensor*> lstStrictPonctualSensors;
    bool m_bEnableStrictSensors;

    // flag indiquant si ce capteur MFD a été défini par l'utilisateur comme un capteur d'Edie 
    bool                     m_bIsEdie;

    // Longueurs géométriques calculées une fois pour toutes
    double                   m_bGeomLength1;
    double                   m_bGeomLength2;
    double                   m_bGeomLength3;

    // true si capteur MFD simple (on veut uniquement le temps passé et distance parcourue pour calcul vitesse moyenne spatialisée pour SymuMaster, sans créer de capteurs ponctuels par exemple)
    bool                     m_bSimpleMFD;

	// Last computed value of total travel time;
	double					m_dbTotalTravelTime;

	// Last computed value of total travel distance
	double					m_dbTotalTravelDistance;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // MFDSensorH