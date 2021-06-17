#include <iostream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

const int SUCCESS = 0;
const int HELP_PRINTED = 1;
const int ERROR_IN_COMMAND_LINE = 2;
const int FAILURE = 3;

using namespace std;

namespace po = boost::program_options;

#define SYMUBRUIT_EXPORT
#define SYMUBRUIT_CDECL 

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(std::string sTmpXmlDataFile, std::string sScenarioID = "", std::string sOutdir = "");
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd);
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymQuit();

int main(int argc, char* argv[])
{
    int nStep = 0;
    bool bLoad, bEnd=0;
    std::string sInputSymuVia;
    std::string sflow;

    int returnCode;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("symuvia-config-file,I", po::value<std::string>()->required(), "SymuVia XML configuration file");

    po::positional_options_description p;
    p.add("symuvia-config-file", -1);

    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        if (vm.count("help") || vm.empty()
            // the input XML file must be the only parameter if present, to avoid handling of priorities of parameters between the config file and the command line parameters
            || !((vm.count("symuvia-config-file") == 1 && vm.size() == 1) || (vm.count("symuvia-config-file") == 0 && vm.count("symuvia") == 1))) {
            std::cout << "usage : SymuViaLauncher symuvia-config-file.xml" << std::endl;
            std::cout << desc << std::endl;
            return HELP_PRINTED;
        }

        po::notify(vm);
    }
    catch (boost::program_options::required_option& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        return ERROR_IN_COMMAND_LINE;
    }
    catch (boost::program_options::error& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        return ERROR_IN_COMMAND_LINE;
    }
    
    if(!SymLoadNetwork(vm.at("symuvia-config-file").as<std::string>()))
    {
        return FAILURE;
    }

    while(!bEnd)
    {
    // Time step computation
        SymRunNextStep(sflow, 1, bEnd);
        nStep = nStep + 1;
        if (bEnd == 0)
            std::cout << nStep << std::endl;
        else
            std::cout << "Microscopic simulation completed" << std::endl;
    }

    // Terminate properly
    SymQuit();

    return SUCCESS;  
}