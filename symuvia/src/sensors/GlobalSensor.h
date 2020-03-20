#pragma once
#ifndef GlobalSensorH
#define GlobalSensorH

#include "AbstractSensor.h"
#include "tools.h"

#include <map>


class Vehicule;

struct VehData
{
    VehData();

    //double dbInstCreation;		// instant de création du véhicule (si -1, antérieure à la période courante)
	double dbInstSuppres;		// instant de sortie du réseau du véhicule (si -1, postérieur à la période courante)
	//double dbDstParcourue;		// Distance parcourue au cours de la période
	double dbDstCumParcPrev;	// Distance cumulée parcourue depuis sa création et jusqu'à la période précédente

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

/*===========================================================================================*/
/* Structure GlobalSensorData                                                                 */
/*===========================================================================================*/
struct GlobalSensorData
{
    std::map<boost::shared_ptr<Vehicule>, VehData, LessVehiculePtr<Vehicule> > mapVehs; // maps des véhicules yant circulé sur le réseau durant la période d'agrégation

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};


struct GlobalSensorSnapshot : public AbstractSensorSnapshot
{
	std::map<int, VehData> mapVehs;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


class GlobalSensor: public AbstractSensor
{
public:
    GlobalSensor();
	virtual ~GlobalSensor();

    virtual std::string GetSensorPeriodXMLNodeName() const;

    virtual void CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh);

    virtual void AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink = DBL_MAX);

    virtual void WriteDef(DocTrafic* pDocTrafic);
    virtual void Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput);

    virtual void PrepareNextPeriod();

    virtual AbstractSensorSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup);

private:
    void ProcessVehicle(boost::shared_ptr<Vehicule> pV);

protected:
	// Informations dynamiques du capteur agrégées sur une période finie (non glissante)
	GlobalSensorData    data;		

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};


#endif // GlobalSensorH