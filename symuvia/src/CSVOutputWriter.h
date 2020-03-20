#pragma once
#ifndef csvoutputwriterH
#define csvoutputwriterH

#include "TimeUtil.h"
#pragma warning(disable: 4005)
#include "ogr_spatialref.h"
#pragma warning(default: 4005)
#include <fstream>
#include <deque>
#include <vector>

class Tuyau;
class PonctualSensor;
class Vehicule;
class SimulationSnapshot;

class CSVOutputWriter
{
public:
    CSVOutputWriter();
    CSVOutputWriter(const std::string& path, const std::string& suffix, const SDateTime& dateDebutSimu, OGRCoordinateTransformation *pCoordTransf,
                    bool bTrajectoriesOutput, bool bSensorsOutput);
    virtual ~CSVOutputWriter();

    // passage en mode d'écriture dans fichiers temporaires (affectation dynamique)
    void doUseTempFiles(SimulationSnapshot * pSimulationSnapshot, size_t snapshotIdx);
    // validation de la dernièer période d'affectation et recopie des fichiers temporaires dans les fichiers finaux
    void converge(SimulationSnapshot * pSimulationSnapshot);

    // écriture du fichier des tronçons
    void writeLinksFile(const std::deque<Tuyau*> &lstTuyaux);
    // écriture du fichier des capteurs
    void writeSensorsFile(const std::vector<PonctualSensor*> &lstCapteurs);
    // écriture d'une trajectoire
    void writeTrajectory(double dbInstant, Vehicule * pVehicule);
    // écriture des données d'un capteur pour une période
    void writeSensorData(double dbInstant, const std::string& sID, int nNbVehFranchissant, double dbVitGlobal, const std::string& sVit);
	
private:
    std::string   m_Path;
    std::string   m_Suffix; // Pour gestion des réplications

    SDateTime     m_StartTime;

    // flux vers le fichier des trajectoires
    std::ofstream m_TrajsOFS;

    // flux vers le fichier des changements de tronçon
    std::ofstream m_LinkChangesOFS;
    
    // flux vers le fichier des sorties capteur
    std::ofstream m_SensorsOFS;

    SimulationSnapshot * m_pCurrentSimulationSnapshot;

	// Transformation des coordonnées
	OGRCoordinateTransformation *m_pCoordTransf;

    bool m_bTrajectoriesOutput;
    bool m_bSensorsOutput;
};

#endif // csvoutputwriterH
