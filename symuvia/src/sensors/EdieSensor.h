#pragma once
#ifndef EdieSensorH
#define EdieSensorH

#include "LongitudinalSensor.h"

#include <vector>
#include <set>
#include <map>

class MFDSensor;
class TypeVehicule;

/*===========================================================================================*/
/* Structure EdieSensorData                                                                 */
/*===========================================================================================*/
class EdieSensorData
{
public:
        std::vector<std::map<TypeVehicule*,double> > dbTotalTravelledDistance;		// Distance totale parcourue pour chaque voie et pour chaque type de véhicule
        std::vector<std::map<TypeVehicule*, double> > dbTotalTravelledTime;			// Temps total passé pour chaque voie et pour chaque type de véhicule
		std::map<TypeVehicule*, int> nNumberOfRemovalVehicles;					// Nombre de véhicules supprimé pour chaque type de véhicule 

        double GetTotalTravelledDistance() const;
        double GetTotalTravelledTime() const;
        double GetTravelledTimeForLane(size_t k) const;
        double GetTravelledDistanceForLane(size_t k) const;

		int	GetTotalNumberOfRemovalVehicles() const;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

class EdieSensorSnapshot : public AbstractSensorSnapshot
{
public:
	EdieSensorData workingData;
	EdieSensorData previousPeriodData;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Sérialisation
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};


class EdieSensor : public LongitudinalSensor
{
public:
    enum ETuyauType
    {
        TT_Link = 1, // toncon classique
        TT_Aval = 2  // tronçon interne à la brique aval d'un tronçon classique, constituant une partie de mouvement dont le tronçon classique est le tronçon amont
    };

public:
    EdieSensor();
    EdieSensor(const std::string & strNom, Tuyau * pTuyau, double startPos, double endPos, int tuyauType, MFDSensor * pParent);
    virtual ~EdieSensor();

    virtual void CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh);

    virtual void AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink = DBL_MAX);

    virtual void WriteDef(DocTrafic* pDocTrafic);
    virtual void Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput);

    virtual void PrepareNextPeriod();

	MFDSensor * GetParentMFD() const;

    EdieSensorData & GetData();
	const EdieSensorData & GetPreviousData() const;
    int GetTuyauType();

    virtual AbstractSensorSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup);

protected:

    int     eTuyauType;         // indique s'il s'agit d'un bout de capteur MFD sur un tronçon interne ou un troncon intern aval
    MFDSensor *m_pParentMFD;    // Capteur MFD parent éventuel

	// Informations dynamiques du capteur agrégées sur une période finie (non glissante)
	EdieSensorSnapshot data;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // EdieSensorH