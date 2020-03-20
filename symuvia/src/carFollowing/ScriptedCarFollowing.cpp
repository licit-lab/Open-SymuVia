#include "stdafx.h"
#include "ScriptedCarFollowing.h"

#include "../regulation/PythonUtils.h"
#include "../reseau.h"
#include "../vehicule.h"
#include "../voie.h"
#include "../tuyau.h"
#include "../convergent.h"
#include "../SystemUtil.h"
#include "CarFollowingContext.h"
#include "../Logger.h"

#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>

using namespace boost::python;

ScriptedCarFollowing::ScriptedCarFollowing()
{
}

ScriptedCarFollowing::~ScriptedCarFollowing()
{
}

std::string ScriptedCarFollowing::getModuleName()
{
    if(m_ShortModuleName.empty())
    {
        m_ShortModuleName = SystemUtil::GetFileName(m_ModuleName);
    }
    return m_ShortModuleName;
}

void ScriptedCarFollowing::Copy(ScriptedCarFollowing * pOriginal)
{
    m_ID = pOriginal->GetID();
    m_StateVariable = pOriginal->GetStateVariable();

    m_InternalComputeFuncName = pOriginal->GetInternalComputeFuncName();
    m_VehicleLengthFuncName = pOriginal->GetVehicleLengthFuncName();
    m_MaxInfluenceDistanceFuncName = pOriginal->GetMaxInfluenceDistanceFuncName();
    m_ApproxSpeedFuncName = pOriginal->GetApproxSpeedFuncName();
    m_AggressivityFuncName = pOriginal->GetAggressivityFuncName();
    m_TestLaneChangeFuncName = pOriginal->GetTestLaneChangeFuncName();
    m_ApplyLaneChangeFuncName = pOriginal->GetApplyLaneChangeFuncName();
    m_TestConvergentInsertionFuncName = pOriginal->GetTestConvergentInsertionFuncName();
    m_LaneSpeedFuncName = pOriginal->GetLaneSpeedFuncName();

    m_ModuleName = pOriginal->GetModuleName();

    m_Parameters = pOriginal->GetParameters();
}

bool ScriptedCarFollowing::IsPositionComputed()
{
    return m_StateVariable == STATE_VARIABLE_POSITION;
}

bool ScriptedCarFollowing::IsSpeedComputed()
{
    return m_StateVariable == STATE_VARIABLE_SPEED;
}

bool ScriptedCarFollowing::IsAccelerationComputed()
{
    return m_StateVariable == STATE_VARIABLE_ACCELERATION;
}

double ScriptedCarFollowing::GetVehicleLength()
{
    if(m_VehicleLengthFuncName.empty())
    {
        return AbstractCarFollowing::GetVehicleLength();
    }
    else
    {
        return ExecutePythonFunc<double>(m_VehicleLengthFuncName, m_pContext);
    }
}

double ScriptedCarFollowing::GetMaxInfluenceRange()
{
    return ExecutePythonFunc<double>(m_MaxInfluenceDistanceFuncName, m_pContext);
}

void ScriptedCarFollowing::InternalCompute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext)
{
    dict locals;
    std::vector<std::string> additionalParams;
    additionalParams.push_back("time_tep");
    additionalParams.push_back("instant");
    locals[additionalParams[0]] = dbTimeStep;
    locals[additionalParams[1]] = dbInstant;
    ExecutePythonFunc(m_InternalComputeFuncName, pContext, locals, additionalParams);
}


double ScriptedCarFollowing::CalculVitesseApprochee
(
    boost::shared_ptr<Vehicule> pVehicle,
    boost::shared_ptr<Vehicule> pVehLeader,
    double dbDistanceBetweenVehicles
)
{    
    dict locals;
    std::vector<std::string> additionalParams;
    additionalParams.push_back("vehicle");
    additionalParams.push_back("leader");
    additionalParams.push_back("distance_between_vehicles");
    locals[additionalParams[0]] = ptr(pVehicle.get());
    locals[additionalParams[1]] = ptr(pVehicle.get());
    locals[additionalParams[2]] = dbDistanceBetweenVehicles;
    return ExecutePythonFunc<double>(m_ApproxSpeedFuncName, m_pContext, locals, additionalParams);
}

void ScriptedCarFollowing::CalculAgressivite(double dbInstant)
{
    if(m_AggressivityFuncName.empty())
    {
        return AbstractCarFollowing::CalculAgressivite(dbInstant);
    }
    else
    {
        dict locals;
        std::vector<std::string> additionalParams(1, "instant");
        locals[additionalParams[0]] = dbInstant;
        ExecutePythonFunc(m_AggressivityFuncName, m_pContext, locals, additionalParams);
    }
}

bool ScriptedCarFollowing::TestLaneChange(double dbInstant, boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehLeaderOrig,
    VoieMicro * pVoieCible, bool bForce, bool bRabattement, bool bInsertion)
{
    if(m_TestLaneChangeFuncName.empty())
    {
        return AbstractCarFollowing::TestLaneChange(dbInstant, pVehFollower, pVehLeader, pVehLeaderOrig, pVoieCible, bForce, bRabattement, bInsertion);
    }
    else
    {
        dict locals;
        std::vector<std::string> additionalParams;
        additionalParams.push_back("instant");
        additionalParams.push_back("follower");
        additionalParams.push_back("leader");
        additionalParams.push_back("original_lane_leader");
        additionalParams.push_back("target_lane");
        additionalParams.push_back("force");
        additionalParams.push_back("overtake");
        additionalParams.push_back("insertion");
        locals[additionalParams[0]] = dbInstant;
        locals[additionalParams[1]] = ptr(pVehFollower.get());
        locals[additionalParams[2]] = ptr(pVehLeader.get());
        locals[additionalParams[3]] = ptr(pVehLeaderOrig.get());
        locals[additionalParams[4]] = ptr(pVoieCible);
        locals[additionalParams[5]] = bForce;
        locals[additionalParams[6]] = bRabattement;
        locals[additionalParams[7]] = bInsertion;
        return ExecutePythonFunc<double>(m_TestLaneChangeFuncName, m_pContext, locals, additionalParams);
    }
}

void ScriptedCarFollowing::ApplyLaneChange(VoieMicro * pVoieOld, VoieMicro * pVoieCible, boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehFollower,
    boost::shared_ptr<Vehicule> pVehLeaderOrig)
{
    if(m_ApplyLaneChangeFuncName.empty())
    {
        AbstractCarFollowing::ApplyLaneChange(pVoieOld, pVoieCible, pVehLeader, pVehFollower, pVehLeaderOrig);
    }
    else
    {
        dict locals;
        std::vector<std::string> additionalParams;
        additionalParams.push_back("original_lane");
        additionalParams.push_back("target_lane");
        additionalParams.push_back("leader");
        additionalParams.push_back("follower");
        additionalParams.push_back("original_lane_leader");
        locals[additionalParams[0]] = ptr(pVoieOld);
        locals[additionalParams[1]] = ptr(pVoieCible);
        locals[additionalParams[2]] = ptr(pVehLeader.get());
        locals[additionalParams[3]] = ptr(pVehFollower.get());
        locals[additionalParams[4]] = ptr(pVehLeaderOrig.get());
        ExecutePythonFunc(m_ApplyLaneChangeFuncName, m_pContext, locals, additionalParams);
    }
}

bool ScriptedCarFollowing::TestConvergentInsertion(PointDeConvergence * pPointDeConvergence, Tuyau *pTAmP, VoieMicro* pVAmP, VoieMicro* pVAmNP, int nVoieGir, double dbInstant,
    int j, double dbDemande1, double dbDemande2, double dbOffre,
    boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehFollower, double & dbTr, double & dbTa)
{
    if(m_TestConvergentInsertionFuncName.empty())
    {
        return AbstractCarFollowing::TestConvergentInsertion(pPointDeConvergence, pTAmP, pVAmP, pVAmNP, nVoieGir, dbInstant, j, dbDemande1, dbDemande2, dbOffre,
            pVehLeader, pVehFollower, dbTr, dbTa);
    }
    else
    {        
        bool returnValue = false;

        m_pNetwork->GetPythonUtils()->enterChildInterpreter();
        try
        {
            object globals = m_pNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");

            // construction de la chaîne des paramètres spécifiques
            std::string paramsStr = "network,car_following_model,parameters,context,prev_context,convergence_point,priority_upstream_link,priority_upstream_lane,non_priority_upstream_lane,roundabout_lane_number,instant,lane_number,freeflow_demand,congested_demand,offer,leader,follower";

            dict locals;
            locals["network"] = ptr(m_pNetwork);
            locals["car_following_model"] = ptr(this);
            locals["parameters"] = m_Parameters;
            locals["context"] = ptr(m_pContext);
            locals["prev_context"] = ptr(m_pPrevContext);
            locals["convergence_point"] = ptr(pPointDeConvergence);
            locals["priority_upstream_link"] = ptr(pTAmP);
            locals["priority_upstream_lane"] = ptr(pVAmP);
            locals["non_priority_upstream_lane"] = ptr(pVAmNP);
            locals["roundabout_lane_number"] = nVoieGir;
            locals["instant"] = dbInstant;
            locals["lane_number"] = j;
            locals["freeflow_demand"] = dbDemande1;
            locals["congested_demand"] = dbDemande2;
            locals["offer"] = dbOffre;
            locals["leader"] = ptr(pVehLeader.get());
            locals["follower"] = ptr(pVehFollower.get());
            
            object result;
            if(!m_ModuleName.empty() && globals[getModuleName()].attr("__dict__").contains(m_TestConvergentInsertionFuncName))
            {
                result = eval((getModuleName() + "." + m_TestConvergentInsertionFuncName + "(" + paramsStr + ")\n").c_str(), globals, locals);
            }
            else
            {
                result = eval((SCRIPTS_CAR_FOLLOWING_MODULE_NAME + "." + m_TestConvergentInsertionFuncName + "(" + paramsStr + ")\n").c_str(), globals, locals);
            }
            returnValue = extract<bool>(result["result"]);
            if(result.contains("ta"))
            {
                dbTa = extract<double>(result["ta"]);
            }
            if(result.contains("tr"))
            {
                dbTr = extract<double>(result["tr"]);
            }
        }
        catch( error_already_set )
        {
            m_pNetwork->log() << Logger::Error << m_pNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
            m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        }
        m_pNetwork->GetPythonUtils()->exitChildInterpreter();
        return returnValue;
    }
}

double ScriptedCarFollowing::ComputeLaneSpeed(VoieMicro * pTargetLane)
{
    dict locals;
    std::vector<std::string> additionalParams(1, "target_lane");
    locals[additionalParams[0]] = ptr(pTargetLane);
    return ExecutePythonFunc<double>(m_LaneSpeedFuncName, m_pContext, locals, additionalParams);
}

const std::string & ScriptedCarFollowing::GetID()
{
    return m_ID;
}

void ScriptedCarFollowing::SetID(const std::string & strId)
{
    m_ID = strId;
}

void ScriptedCarFollowing::SetStateVariable(EStateVariable stateVariable)
{
    m_StateVariable = stateVariable;
}

AbstractCarFollowing::EStateVariable ScriptedCarFollowing::GetStateVariable()
{
    return m_StateVariable;
}

void ScriptedCarFollowing::SetInternalComputeFuncName(const std::string & funcName)
{
    m_InternalComputeFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetInternalComputeFuncName()
{
    return m_InternalComputeFuncName;
}

void ScriptedCarFollowing::SetVehicleLengthFuncName(const std::string & funcName)
{
    m_VehicleLengthFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetVehicleLengthFuncName()
{
    return m_VehicleLengthFuncName;
}

void ScriptedCarFollowing::SetMaxInfluenceDistanceFuncName(const std::string & funcName)
{
    m_MaxInfluenceDistanceFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetMaxInfluenceDistanceFuncName()
{
    return m_MaxInfluenceDistanceFuncName;
}

void ScriptedCarFollowing::SetApproxSpeedFuncName(const std::string & funcName)
{
    m_ApproxSpeedFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetApproxSpeedFuncName()
{
    return m_ApproxSpeedFuncName;
}

void ScriptedCarFollowing::SetAggressivityFuncName(const std::string & funcName)
{
    m_AggressivityFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetAggressivityFuncName()
{
    return m_AggressivityFuncName;
}

void ScriptedCarFollowing::SetTestLaneChangeFuncName(const std::string & funcName)
{
    m_TestLaneChangeFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetTestLaneChangeFuncName()
{
    return m_TestLaneChangeFuncName;
}

void ScriptedCarFollowing::SetApplyLaneChangeFuncName(const std::string & funcName)
{
    m_ApplyLaneChangeFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetApplyLaneChangeFuncName()
{
    return m_ApplyLaneChangeFuncName;
}

void ScriptedCarFollowing::SetTestConvergentInsertionFuncName(const std::string & funcName)
{
    m_TestConvergentInsertionFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetTestConvergentInsertionFuncName()
{
    return m_TestConvergentInsertionFuncName;
}

void ScriptedCarFollowing::SetLaneSpeedFuncName(const std::string & funcName)
{
    m_LaneSpeedFuncName = funcName;
}

const std::string & ScriptedCarFollowing::GetLaneSpeedFuncName()
{
    return m_LaneSpeedFuncName;
}

void ScriptedCarFollowing::SetModuleName(const std::string & moduleName, Reseau * pNetwork)
{
    m_ModuleName = moduleName;

    // Chargement du module
    try
    {
        object globals = m_pNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");
        exec("import imp\n", globals);
        m_pNetwork->GetPythonUtils()->importModule(globals, getModuleName(), m_ModuleName);
    }
    catch( error_already_set )
    {
        pNetwork->log() << Logger::Error << m_pNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
        pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
    }
}

const std::string & ScriptedCarFollowing::GetModuleName()
{
    return m_ModuleName;
}

boost::python::dict& ScriptedCarFollowing::GetParameters()
{
    return m_Parameters;
}


template<class T>
T ScriptedCarFollowing::ExecutePythonFunc(const std::string & funcName, CarFollowingContext * pContext, dict locals, const std::vector<std::string> &additionalParams)
{
    m_pNetwork->GetPythonUtils()->enterChildInterpreter();
    T returnValue;
    try
    {
        object globals = m_pNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");

        // construction de la chaîne des paramètres spécifiques
        std::string paramsStr = "network,car_following_model,parameters,context,prev_context";
        for(size_t iParam = 0; iParam < additionalParams.size(); iParam++)
        {
            paramsStr += ",";
            paramsStr += additionalParams[iParam];
        }

        locals["network"] = ptr(m_pNetwork);
        locals["car_following_model"] = ptr(this);
        locals["parameters"] = m_Parameters;
        locals["context"] = ptr(pContext);
        locals["prev_context"] = ptr(m_pPrevContext);

        object result;
        if(!m_ModuleName.empty() && globals[getModuleName()].attr("__dict__").contains(funcName))
        {
            result = eval((getModuleName() + "." + funcName + "(" + paramsStr + ")\n").c_str(), globals, locals);
        }
        else
        {
            result = eval((SCRIPTS_CAR_FOLLOWING_MODULE_NAME + "." + funcName + "(" + paramsStr + ")\n").c_str(), globals, locals);
        }
        returnValue = extract<T>(result);
    }
    catch( error_already_set )
    {
        m_pNetwork->log() << Logger::Error << m_pNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
    }
    m_pNetwork->GetPythonUtils()->exitChildInterpreter();
    return returnValue;
}

void ScriptedCarFollowing::ExecutePythonFunc(const std::string & funcName, CarFollowingContext * pContext, dict locals, const std::vector<std::string> &additionalParams)
{
    m_pNetwork->GetPythonUtils()->enterChildInterpreter();
    try
    {
        object globals = m_pNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");

        // construction de la chaîne des paramètres spécifiques
        std::string paramsStr = "network,car_following_model,parameters,context,prev_context";
        for(size_t iParam = 0; iParam < additionalParams.size(); iParam++)
        {
            paramsStr += ",";
            paramsStr += additionalParams[iParam];
        }

        locals["network"] = ptr(m_pNetwork);
        locals["car_following_model"] = ptr(this);
        locals["parameters"] = m_Parameters;
        locals["context"] = ptr(pContext);
        locals["prev_context"] = ptr(m_pPrevContext);

        if(!m_ModuleName.empty() && globals[getModuleName().c_str()].attr("__dict__").contains(funcName.c_str()))
        {
            exec((getModuleName() + "." + funcName + "(" + paramsStr + ")\n").c_str(), globals, locals);
        }
        else
        {
            exec((SCRIPTS_CAR_FOLLOWING_MODULE_NAME + "." + funcName + "(" + paramsStr + ")\n").c_str(), globals, locals);
        }
    }
    catch( error_already_set )
    {
        m_pNetwork->log() << Logger::Error << m_pNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
    }
    m_pNetwork->GetPythonUtils()->exitChildInterpreter();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ScriptedCarFollowing::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ScriptedCarFollowing::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ScriptedCarFollowing::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractCarFollowing);

    ar & BOOST_SERIALIZATION_NVP(m_ID);
    ar & BOOST_SERIALIZATION_NVP(m_StateVariable);

    ar & BOOST_SERIALIZATION_NVP(m_InternalComputeFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_VehicleLengthFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_MaxInfluenceDistanceFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_ApproxSpeedFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_AggressivityFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_TestLaneChangeFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_ApplyLaneChangeFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_TestConvergentInsertionFuncName);
    ar & BOOST_SERIALIZATION_NVP(m_LaneSpeedFuncName);

    ar & BOOST_SERIALIZATION_NVP(m_ModuleName);

    ar & BOOST_SERIALIZATION_NVP(m_Parameters);
}
