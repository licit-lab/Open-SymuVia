#pragma once
#ifndef GTFSExporterH
#define GTFSExporterH

#include <map>
#include <string>

class Reseau;

class GTFSExporter {

public:
    GTFSExporter();
    virtual ~GTFSExporter();

    // écriture des fichiers GTFS correspondant au réseau passé en paramètre
    bool write(Reseau * pNetwork, const std::map<std::string, std::pair<int,int> > & lstTPTypes);

private:
    void writeAgency(const std::string & outDir);
    bool writeStops(const std::string & outDir, Reseau * pNetwork);
    void writeCalendar(const std::string & outDir);
    void writeCalendarDates(const std::string & outDir);
    void writeRoutes(const std::string & outDir, Reseau * pNetwork, const std::map<std::string, std::pair<int,int> > & lstTPTypes);
    void writeTrips(const std::string & outDir, Reseau * pNetwork);
    void writeStopTimes(const std::string & outDir, Reseau * pNetwork);
    void writeFrequencies(const std::string & outDir, Reseau * pNetwork);

};

#endif // GTFSExporterH