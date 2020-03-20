#pragma once
#ifndef SYMUCOMSERIALIZATIONH
#define SYMUCOMSERIALIZATIONH

#include <Communication/CommunicationRunner.h>
#include <Communication/SimulationConfiguration.h>
#include <Communication/Graph.h>
#include <Communication/NoiseSimulation.h>
#include <Communication/Message.h>
#include <Communication/MessageBox.h>
#include <Communication/Entity.h>
#include <Utilitaires/Point.h>

#include <boost/date_time/posix_time/time_serialize.hpp>

#include <boost/serialization/assume_abstract.hpp>

BOOST_SERIALIZATION_ASSUME_ABSTRACT(SymuCom::Entity) // la classe est pure virtual

namespace boost {
    namespace serialization {

        template <class Archive>
        void serialize(Archive& ar, SymuCom::MessageBox& object, unsigned int version)
        {
            std::vector<SymuCom::Message*> lstTravellingMessages;
            if (Archive::is_saving::value)
            {
                lstTravellingMessages = object.GetTravellingMessages();
            }
            ar & boost::serialization::make_nvp("m_lstTravellingMessages", lstTravellingMessages);
            if (Archive::is_loading::value)
            {
                object.SetTravellingMessages(lstTravellingMessages);
            }

            std::vector<SymuCom::Message*> lstTreatedMessages;
            if (Archive::is_saving::value)
            {
                lstTreatedMessages = object.GetTreatedMessages();
            }
            ar & boost::serialization::make_nvp("m_lstTreatedMessages", lstTreatedMessages);
            if (Archive::is_loading::value)
            {
                object.SetTreatedMessages(lstTreatedMessages);
            }
        }

        template <class Archive>
        void serialize(Archive& ar, SymuCom::Entity& object, unsigned int version)
        {
            unsigned int ui;
            if (Archive::is_saving::value)
            {
                ui = object.GetEntityID();
            }
            ar & boost::serialization::make_nvp("m_uiID", ui);
            if (Archive::is_loading::value)
            {
                object.SetEntityID(ui);
            }

            std::string strValue;
            if (Archive::is_saving::value)
            {
                strValue = object.GetEntityLabel();
            }
            ar & boost::serialization::make_nvp("m_strLabel", strValue);
            if (Archive::is_loading::value)
            {
                object.SetStrLabel(strValue);
            }

            SymuCom::Point coords;
            if (Archive::is_saving::value)
            {
                coords = object.GetCoordinates();
            }
            ar & boost::serialization::make_nvp("m_ptCoordinates", coords);
            if (Archive::is_loading::value)
            {
                object.SetCoordinates(coords);
            }

            SymuCom::MessageBox * pBox = NULL;
            if (Archive::is_saving::value)
            {
                pBox = object.GetMessageBox();
            }
            ar & boost::serialization::make_nvp("m_pMessageBox", pBox);
            if (Archive::is_loading::value)
            {
                object.SetMessageBox(pBox);
            }

            SymuCom::CommunicationRunner * pComRunner = NULL;
            if (Archive::is_saving::value)
            {
                pComRunner = object.GetCommunicationRunner();
            }
            ar & boost::serialization::make_nvp("m_pCommunicationRunner", pComRunner);
            if (Archive::is_loading::value)
            {
                object.SetCommunicationRunner(pComRunner);
            }

            bool bValue = false;
            if (Archive::is_saving::value)
            {
                bValue = object.GetCanSend();
            }
            ar & boost::serialization::make_nvp("m_bCanSend", bValue);
            if (Archive::is_loading::value)
            {
                object.SetCanSend(bValue);
            }

            bValue = false;
            if (Archive::is_saving::value)
            {
                bValue = object.GetCanReceive();
            }
            ar & boost::serialization::make_nvp("m_bCanReceive", bValue);
            if (Archive::is_loading::value)
            {
                object.SetCanReceive(bValue);
            }

            double dbValue;
            if (Archive::is_saving::value)
            {
                dbValue = object.GetAlphaDown();
            }
            ar & boost::serialization::make_nvp("m_dbAlphaDown", dbValue);
            if (Archive::is_loading::value)
            {
                object.SetAlphaDown(dbValue);
            }

            if (Archive::is_saving::value)
            {
                dbValue = object.GetBetaDown();
            }
            ar & boost::serialization::make_nvp("m_dbBetaDown", dbValue);
            if (Archive::is_loading::value)
            {
                object.SetBetaDown(dbValue);
            }

            boost::posix_time::ptime tmpTime;
            if (Archive::is_saving::value)
            {
                tmpTime = object.GetLastBreakDown();
            }
            ar & boost::serialization::make_nvp("m_dtLastBreakDown", tmpTime);
            if (Archive::is_loading::value)
            {
                object.SetLastBreakDown(tmpTime);
            }

            bValue = false;
            if (Archive::is_saving::value)
            {
                bValue = object.isDown();
            }
            ar & boost::serialization::make_nvp("m_isDown", bValue);
            if (Archive::is_loading::value)
            {
                object.SetIsDown(bValue);
            }

            if (Archive::is_saving::value)
            {
                dbValue = object.GetRange();
            }
            ar & boost::serialization::make_nvp("m_dbRange", dbValue);
            if (Archive::is_loading::value)
            {
                object.SetRange(dbValue);
            }

            int i;
            if (Archive::is_saving::value)
            {
                i = object.GetTimeOut();
            }
            ar & boost::serialization::make_nvp("m_dbTimeOut", i);
            if (Archive::is_loading::value)
            {
                object.SetTimeOut(i);
            }

            std::set<int> setInt;
            if (Archive::is_saving::value)
            {
                setInt = object.GetLstWaitingAckMessageIDs();
            }
            ar & boost::serialization::make_nvp("m_lstWaitingAckMessageID", setInt);
            if (Archive::is_loading::value)
            {
                object.SetLstWaitingAckMessageIDs(setInt);
            }
        }


        template <class Archive>
        void serialize(Archive& ar, SymuCom::Point& object, unsigned int version)
        {
            double x, y, z;
            if (Archive::is_saving::value)
            {
                x = object.getX();
            }
            ar & boost::serialization::make_nvp("m_X", x);
            if (Archive::is_loading::value)
            {
                object.setX(x);
            }
            if (Archive::is_saving::value)
            {
                y = object.getY();
            }
            ar & boost::serialization::make_nvp("m_Y", y);
            if (Archive::is_loading::value)
            {
                object.setY(y);
            }
            if (Archive::is_saving::value)
            {
                z = object.getZ();
            }
            ar & boost::serialization::make_nvp("m_Z", z);
            if (Archive::is_loading::value)
            {
                object.setZ(z);
            }
        }

        template <class Archive>
        void serialize(Archive& ar, SymuCom::Message& object, unsigned int version)
        {
            int i;
            if (Archive::is_saving::value)
            {
                i = object.GetMessageID();
            }
            ar & boost::serialization::make_nvp("m_iMessageID", i);
            if (Archive::is_loading::value)
            {
                object.SetMessageID(i);
            }

            unsigned int ui;
            if (Archive::is_saving::value)
            {
                ui = object.GetOriginID();
            }
            ar & boost::serialization::make_nvp("m_uiOriginID", ui);
            if (Archive::is_loading::value)
            {
                object.SetOrigineID(ui);
            }

            if (Archive::is_saving::value)
            {
                ui = object.GetDestinationID();
            }
            ar & boost::serialization::make_nvp("m_uiDestinationID", ui);
            if (Archive::is_loading::value)
            {
                object.SetDestinationID(ui);
            }

            boost::posix_time::ptime tmpTime;
            if (Archive::is_saving::value)
            {
                tmpTime = object.GetTimeSend();
            }
            ar & boost::serialization::make_nvp("m_dtTimeSend", tmpTime);
            if (Archive::is_loading::value)
            {
                object.SetTimeSend(tmpTime);
            }

            if (Archive::is_saving::value)
            {
                tmpTime = object.GetTimeReceive();
            }
            ar & boost::serialization::make_nvp("m_dtTimeReceive", tmpTime);
            if (Archive::is_loading::value)
            {
                object.SetTimeReceive(tmpTime);
            }

            std::string strData;
            if (Archive::is_saving::value)
            {
                strData = object.GetData();
            }
            ar & boost::serialization::make_nvp("m_strData", strData);
            if (Archive::is_loading::value)
            {
                object.SetData(strData);
            }

            SymuCom::Message::EMessageState eState;
            if (Archive::is_saving::value)
            {
                eState = object.GetState();
            }
            ar & boost::serialization::make_nvp("m_eState", eState);
            if (Archive::is_loading::value)
            {
                object.SetState(eState);
            }

            SymuCom::Message::EMessageType eType;
            if (Archive::is_saving::value)
            {
                eType = object.GetType();
            }
            ar & boost::serialization::make_nvp("m_eType", eType);
            if (Archive::is_loading::value)
            {
                object.SetType(eType);
            }
        }

        template <class Archive>
        void serialize(Archive& ar, SymuCom::SimulationConfiguration& object, unsigned int version)
        {
            std::string strValue;
            if (Archive::is_saving::value)
            {
                strValue = object.GetLoadedConfigurationFilePath();
            }
            ar & boost::serialization::make_nvp("m_strLoadedConfigurationFilePath", strValue);
            if (Archive::is_loading::value)
            {
                object.SetLoadedConfigurationFilePath(strValue);
            }

            if (Archive::is_saving::value)
            {
                strValue = object.GetStrOutputDirectoryPath();
            }
            ar & boost::serialization::make_nvp("m_strOutputDirectoryPath", strValue);
            if (Archive::is_loading::value)
            {
                object.SetStrOutputDirectoryPath(strValue);
            }

            if (Archive::is_saving::value)
            {
                strValue = object.GetStrOutputPrefix();
            }
            ar & boost::serialization::make_nvp("m_strOutputPrefix", strValue);
            if (Archive::is_loading::value)
            {
                object.SetStrOutputPrefix(strValue);
            }
        }

        template <class Archive>
        void serialize(Archive& ar, SymuCom::NoiseSimulation& object, unsigned int version)
        {
            double dbTmp;
            if (Archive::is_saving::value)
            {
                dbTmp = object.GetAlphaLatency();
            }
            ar & boost::serialization::make_nvp("m_dbAlphaLatency", dbTmp);
            if (Archive::is_loading::value)
            {
                object.SetAlphaLatency(dbTmp);
            }

            int i;
            if (Archive::is_saving::value)
            {
                i = object.GetGraphMeanLatency();
            }
            ar & boost::serialization::make_nvp("m_iGraphMeanLatency", i);
            if (Archive::is_loading::value)
            {
                object.SetGraphMeanLatency(i);
            }

            if (Archive::is_saving::value)
            {
                dbTmp = object.GetAlphaLatency();
            }
            ar & boost::serialization::make_nvp("m_dbAlphaLatency", dbTmp);
            if (Archive::is_loading::value)
            {
                object.SetAlphaLatency(dbTmp);
            }

            if (Archive::is_saving::value)
            {
                dbTmp = object.GetAlphaNoise();
            }
            ar & boost::serialization::make_nvp("m_dbAlphaNoise", dbTmp);
            if (Archive::is_loading::value)
            {
                object.SetAlphaNoise(dbTmp);
            }

            if (Archive::is_saving::value)
            {
                dbTmp = object.GetAlphaLost();
            }
            ar & boost::serialization::make_nvp("m_dbAlphaLost", dbTmp);
            if (Archive::is_loading::value)
            {
                object.SetAlphaLost(dbTmp);
            }

            if (Archive::is_saving::value)
            {
                dbTmp = object.GetAlphaDown();
            }
            ar & boost::serialization::make_nvp("m_dbAlphaDown", dbTmp);
            if (Archive::is_loading::value)
            {
                object.SetAlphaDown(dbTmp);
            }

            if (Archive::is_saving::value)
            {
                dbTmp = object.GetBetaDown();
            }
            ar & boost::serialization::make_nvp("m_dbBetaDown", dbTmp);
            if (Archive::is_loading::value)
            {
                object.SetBetaDown(dbTmp);
            }

            if (Archive::is_saving::value)
            {
                i = object.GetStepBeforeDegression();
            }
            ar & boost::serialization::make_nvp("m_iStepBeforeDegression", i);
            if (Archive::is_loading::value)
            {
                object.SetStepBeforeDegression(i);
            }

            // gestion de la seed pour reprise du snapshot au même endroit de la séquence
            int randCount;
            if (Archive::is_saving::value)
            {
                randCount = object.GetRandCount();
            }
            ar & BOOST_SERIALIZATION_NVP(randCount);
            if (Archive::is_loading::value)
            {
                object.Restore(randCount);
            }
        }

        template <class Archive>
        void serialize(Archive& ar, SymuCom::Graph& object, unsigned int version)
        {
            SymuCom::NoiseSimulation * pNoise = NULL;
            if (Archive::is_saving::value)
            {
                pNoise = object.GetNoiseSimulation();
            }
            ar & boost::serialization::make_nvp("m_pNoiseSimulation", pNoise);
            if (Archive::is_loading::value)
            {
                object.SetNoiseSimulation(pNoise);
            }

            std::map<unsigned int, SymuCom::Entity *> mapEntity;
            if (Archive::is_saving::value)
            {
                mapEntity = object.GetMapEntity();
            }
            ar & boost::serialization::make_nvp("m_mapEntity", mapEntity);
            if (Archive::is_loading::value)
            {
                object.SetMapEntity(mapEntity);
            }

            SymuCom::CommunicationRunner * pComRunner = NULL;
            if (Archive::is_saving::value)
            {
                pComRunner = object.GetCommunicationRunner();
            }
            ar & boost::serialization::make_nvp("m_pCommunicationRunner", pComRunner);
            if (Archive::is_loading::value)
            {
                object.SetCommunicationRunner(pComRunner);
            }

            unsigned int uiNextEntityID;
            if (Archive::is_saving::value)
            {
                uiNextEntityID = object.GetNextEntityIDCurrent();
            }
            ar & boost::serialization::make_nvp("m_uiNextEntityID", uiNextEntityID);
            if (Archive::is_loading::value)
            {
                object.SetNextEntityID(uiNextEntityID);
            }
        }

        template <class Archive>
        void serialize(Archive& ar, SymuCom::CommunicationRunner& object, unsigned int version)
        {
            std::map<SymuCom::Message*, SymuCom::Entity*, SymuCom::CommunicationRunner::SortMessage> mapMsgs;
            if (Archive::is_saving::value)
            {
                mapMsgs = object.GetMapReceivedMessage();
            }
            ar & boost::serialization::make_nvp("m_mapReceiveMessage", mapMsgs);
            if (Archive::is_loading::value)
            {
                object.SetMapReceivedMessage(mapMsgs);
            }

            SymuCom::Graph * pGraph = NULL;
            if (Archive::is_saving::value)
            {
                pGraph = object.GetGraph();
            }
            ar & boost::serialization::make_nvp("m_pGraph", pGraph);
            if (Archive::is_loading::value)
            {
                object.SetGraph(pGraph);
            }

            SymuCom::SimulationConfiguration * pSimuConf = NULL;
            if (Archive::is_saving::value)
            {
                pSimuConf = object.GetSimulationConfiguration();
            }
            ar & boost::serialization::make_nvp("m_pSimulationConfiguration", pSimuConf);
            if (Archive::is_loading::value)
            {
                object.SetSimulationConfiguration(pSimuConf);
            }

            int i;
            if (Archive::is_saving::value)
            {
                i = object.GetLastMessageID();
            }
            ar & boost::serialization::make_nvp("m_iLastMessageID", i);
            if (Archive::is_loading::value)
            {
                object.SetLastMessageID(i);
            }

            boost::posix_time::ptime tmpTime;
            if (Archive::is_saving::value)
            {
                tmpTime = object.GetRunBeginningTime();
            }
            ar & boost::serialization::make_nvp("m_dtBeginningTime", tmpTime);
            if (Archive::is_loading::value)
            {
                object.SetRunBeginningTime(tmpTime);
            }

            if (Archive::is_saving::value)
            {
                tmpTime = object.GetRunCurrentTime();
            }
            ar & boost::serialization::make_nvp("m_dtCurrentTime", tmpTime);
            if (Archive::is_loading::value)
            {
                object.SetRunCurrentTime(tmpTime);
            }

            if (Archive::is_saving::value)
            {
                tmpTime = object.GetRunEndTime();
            }
            ar & boost::serialization::make_nvp("m_dtRunEndTime", tmpTime);
            if (Archive::is_loading::value)
            {
                object.SetRunEndTime(tmpTime);
            }

            std::string logFile;
            if (Archive::is_saving::value)
            {
                logFile = object.log().GetOutputFile();
            }
            ar & boost::serialization::make_nvp("strLogFile", logFile);
            if (Archive::is_loading::value)
            {
                object.log().SetOutputFile(logFile);
                if (object.GetGraph() && object.GetGraph()->GetNoiseSimulation()) {
                    object.GetGraph()->GetNoiseSimulation()->SetLogger(&object.log());
                }
            }
        }
    }
}


#endif // SYMUCOMSERIALIZATIONH
