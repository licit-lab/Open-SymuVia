#pragma once

#ifndef _SYMUMASTER_COLOUREDCONSOLELOGGER_
#define _SYMUMASTER_COLOUREDCONSOLELOGGER_

#include "SymuMasterExports.h"

#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>

namespace SymuMaster {


    class ColouredConsoleLogger : public boost::log::sinks::text_ostream_backend
    {
    public:
        static void consume(boost::log::record_view const& rec, string_type const& formatted_string);
    };

    typedef boost::log::sinks::synchronous_sink<SymuMaster::ColouredConsoleLogger> coloured_console_sink_t;
}

#endif // _SYMUMASTER_COLOUREDCONSOLELOGGER_
