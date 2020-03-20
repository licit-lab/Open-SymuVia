#pragma once
#ifndef ScriptedCarFollowingH
#define ScriptedCarFollowingH

#include <Python.h>

#include <string>
#include <vector>

#include <boost/python/dict.hpp>

#include "AbstractCarFollowing.h"

class ScriptedCarFollowing : public AbstractCarFollowing
{
public:
    ScriptedCarFollowing();
    virtual ~ScriptedCarFollowing();

    void Copy(ScriptedCarFollowing * pOriginal);

    virtual bool IsPositionComputed();
    virtual bool IsSpeedComputed();
    virtual bool IsAccelerationComputed();

    virtual double GetVehicleLength();

    virtual double GetMaxInfluenceRange();

    // OTK - LOI - Question : voir si on laisse cette méthode, ou bien si on estime la vitesse du leader avec la loi Newell, ce qui est à présent
    // possible puisqu'on définit le diagramme fondamental Newell pour tous les véhicules.
    virtual double CalculVitesseApprochee(boost::shared_ptr<Vehicule> pVehicle, boost::shared_ptr<Vehicule> pVehLeader, double dbDistanceBetweenVehicles);

    virtual void CalculAgressivite(double dbInstant);

    virtual bool TestLaneChange(double dbInstant, boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehLeaderOrig,
        VoieMicro * pVoieCible, bool bForce, bool bRabattement, bool bInsertion);

    virtual void ApplyLaneChange(VoieMicro * pVoieOld, VoieMicro * pVoieCible, boost::shared_ptr<Vehicule> pVehLeader,
        boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeaderOrig);

    virtual bool TestConvergentInsertion(PointDeConvergence * pPointDeConvergence, Tuyau *pTAmP, VoieMicro* pVAmP, VoieMicro* pVAmNP, int nVoieGir, double dbInstant, int j, double dbOffre, double dbDemande1, double dbDemande2,
        boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehFollower, double & dbTr, double & dbTa);

    virtual double ComputeLaneSpeed(VoieMicro * pTargetLane);

    // Accesseurs
    const std::string & GetID();
    void SetID(const std::string & strId);

    void SetStateVariable(EStateVariable stateVariable);
    EStateVariable GetStateVariable();

    void SetInternalComputeFuncName(const std::string & funcName);
    const std::string & GetInternalComputeFuncName();
    void SetVehicleLengthFuncName(const std::string & funcName);
    const std::string & GetVehicleLengthFuncName();
    void SetMaxInfluenceDistanceFuncName(const std::string & funcName);
    const std::string & GetMaxInfluenceDistanceFuncName();
    void SetApproxSpeedFuncName(const std::string & funcName);
    const std::string & GetApproxSpeedFuncName();
    void SetAggressivityFuncName(const std::string & funcName);
    const std::string & GetAggressivityFuncName();
    void SetTestLaneChangeFuncName(const std::string & funcName);
    const std::string & GetTestLaneChangeFuncName();
    void SetApplyLaneChangeFuncName(const std::string & funcName);
    const std::string & GetApplyLaneChangeFuncName();
    void SetTestConvergentInsertionFuncName(const std::string & funcName);
    const std::string & GetTestConvergentInsertionFuncName();
    void SetLaneSpeedFuncName(const std::string & funcName);
    const std::string & GetLaneSpeedFuncName();

    void SetModuleName(const std::string & moduleName, Reseau * pNetwork);
    const std::string & GetModuleName();

    boost::python::dict& GetParameters();

protected:
    virtual void InternalCompute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext);

private:
    template<class T>
    T ExecutePythonFunc(const std::string & functionName, CarFollowingContext * pContext, boost::python::dict locals = boost::python::dict(), const std::vector<std::string> &additionalParams = std::vector<std::string>());

    void ExecutePythonFunc(const std::string & functionName, CarFollowingContext * pContext, boost::python::dict locals, const std::vector<std::string> &additionalParams);

    std::string getModuleName();

private:
    std::string    m_ID;
    EStateVariable m_StateVariable;

    std::string    m_InternalComputeFuncName;
    std::string    m_VehicleLengthFuncName;
    std::string    m_MaxInfluenceDistanceFuncName;
    std::string    m_ApproxSpeedFuncName;
    std::string    m_AggressivityFuncName;
    std::string    m_TestLaneChangeFuncName;
    std::string    m_ApplyLaneChangeFuncName;
    std::string    m_TestConvergentInsertionFuncName;
    std::string    m_LaneSpeedFuncName;

    std::string    m_ModuleName;

    boost::python::dict m_Parameters;

    std::string    m_ShortModuleName;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // ScriptedCarFollowingH
