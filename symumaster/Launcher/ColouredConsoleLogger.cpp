#include <iostream>
#include "ColouredConsoleLogger.h"

#include <boost/log/trivial.hpp>


#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#include <fstream>
using namespace SymuMaster;

#ifdef WIN32
WORD get_colour(boost::log::trivial::severity_level level)
{
    switch (level)
    {
    case boost::log::trivial::trace: return 0x08;
    case boost::log::trivial::debug: return 0x02;
    case boost::log::trivial::info: return 0x0F;
    case boost::log::trivial::warning: return 0x0E;
    case boost::log::trivial::error: return 0x0C;
    case boost::log::trivial::fatal: return 0x0C;
    default: return 0x0F;
    }
}
#else
std::string get_colour(boost::log::trivial::severity_level level)
{
    switch (level)
    {
    case boost::log::trivial::trace: return "\033[37m";
    case boost::log::trivial::debug: return "\033[32m";
    case boost::log::trivial::info: return "\033[0m";
    case boost::log::trivial::warning: return "\033[33m";
    case boost::log::trivial::error: return "\033[31m";
    case boost::log::trivial::fatal: return "\033[31m";
    default: return "\033[0m";
    }
}
#endif // WIN32

void ColouredConsoleLogger::consume(boost::log::record_view const& rec, string_type const& formatted_string)
{
    auto level = rec.attribute_values()["Severity"].extract<boost::log::trivial::severity_level>();

#ifdef WIN32
    auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hstdout, &csbi);

    SetConsoleTextAttribute(hstdout, get_colour(level.get()));
#else
    std::cout << get_colour(level.get());
#endif // WIN32
	std::cout << formatted_string << std::endl;
    
#ifdef WIN32
    SetConsoleTextAttribute(hstdout, csbi.wAttributes);
#else
    std::cout << "\033[0m";
#endif // WIN32
}
