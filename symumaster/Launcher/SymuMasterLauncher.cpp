#include "ColouredConsoleLogger.h"

#include <Simulation/SimulationRunner.h>
#include <Simulation/Config/SimulationConfiguration.h>
#include <Simulation/Simulators/SymuVia/SymuViaConfig.h>
#include <Simulation/Assignment/MSADualLoop.h>

#include <boost/log/trivial.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/filesystem.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <fstream>

const int SUCCESS = 0;
const int HELP_PRINTED = 1;
const int ERROR_IN_COMMAND_LINE = 2;
const int FAILURE = 3;


using namespace SymuMaster;

namespace po = boost::program_options;

void initLog()
{
    boost::log::add_common_attributes();

    // Console output
    boost::shared_ptr<coloured_console_sink_t> consoleSink = boost::make_shared<coloured_console_sink_t>();
    consoleSink->set_formatter(
        boost::log::expressions::stream
        << '['
        << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
        << "] ["
        << boost::log::trivial::severity
        << "] "
        << boost::log::expressions::smessage
        );
    boost::log::core::get()->add_sink(consoleSink);

    // File output
    typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > ofstream_sink;
    boost::shared_ptr< ofstream_sink > fileSink = boost::make_shared< ofstream_sink >();
    fileSink->set_formatter(
        boost::log::expressions::stream
        << '['
        << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
        << "] ["
        << boost::log::trivial::severity
        << "] "
        << boost::log::expressions::smessage
        );
    fileSink->locked_backend()->add_stream(boost::make_shared<std::ofstream>("SymuMaster.log"));
    fileSink->locked_backend()->auto_flush(true);
    boost::log::core::get()->add_sink(fileSink);
}

int main(int argc, char* argv[])
{
    int returnCode;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("symumaster-config-file,I", po::value<std::string>()->required(), "SymuMaster XML configuration file");

    po::positional_options_description p;
    p.add("symumaster-config-file", -1);

    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        if (vm.count("help") || vm.empty()
            // the input XML file must be the only parameter if present, to avoid handling of priorities of parameters between the config file and the command line parameters
            || !((vm.count("symumaster-config-file") == 1 && vm.size() == 1) || (vm.count("symumaster-config-file") == 0 && vm.count("symuvia") == 1))) {
            std::cout << "usage : SymuMasterLauncher symumaster-config-file.xml" << std::endl;
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

    // log initialisation
    initLog();

    // Configuration definition
    SimulationConfiguration config;

    // read the configuration from XML input file
    if (!config.LoadFromXML(vm.at("symumaster-config-file").as<std::string>()))
    {
        return FAILURE;
    }

    // Simulation...
    SimulationRunner simulationRunner(config);
    bool bOk = simulationRunner.Run();

    // Termination :
    if (bOk)
    {
        BOOST_LOG_TRIVIAL(info) << "Simulation done.";
        returnCode = SUCCESS;
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "Simulation failed !";
        returnCode = FAILURE;
    }

    return returnCode;
}
