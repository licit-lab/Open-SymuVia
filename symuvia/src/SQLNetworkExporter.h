#pragma once
#ifndef SQLNetworkExporterH
#define SQLNetworkExporterH

#include <string>
#include <vector>
#include <map>

class Reseau;
class Connexion;
struct Point;

class SQLNetworkExporter {

public:
    SQLNetworkExporter();
    virtual ~SQLNetworkExporter();

    // écriture d'un script SQL de création du réseau dans une base tempus
    bool writeSQL(Reseau * pNetwork);

    // Accesseurs
    const std::map<std::string, std::pair<int,int> > & getTPTypes();

private:

    // Construction de la chaîne WKT de la polyligne passée en paramètres
    std::string GetWKTFromPoints(const std::vector<Point> & lstPoints);

    // Construction de la chaîne WKT du point correspondant à la connexion passée en paramètres
    std::string GetWKTFromConnection(Connexion * pConnexion, Reseau * pNetwork);

private:
    // association entre les noms des types de véhicules de type TEC et les identifiants en BDD et BTFS associés
    std::map<std::string, std::pair<int,int> > m_LstTPTypes;

};

#endif // SQLNetworkExporterH