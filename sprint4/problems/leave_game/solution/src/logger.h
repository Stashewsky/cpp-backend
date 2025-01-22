#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include <iostream>
#include <thread>
#include <string_view>
#include <fstream>

#include <boost/json.hpp>

namespace json_loader {
    namespace logging = boost::log;
    namespace json = boost::json;

    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

    void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm);
    void SetupLogger();


}




