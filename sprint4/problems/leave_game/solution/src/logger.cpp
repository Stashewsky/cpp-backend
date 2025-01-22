
#include "logger.h"

namespace json_logger{
    using namespace std::literals;
    namespace logging = boost::log;
    namespace keywords = boost::log::keywords;
    namespace sinks = boost::log::sinks;
    namespace json = boost::json;
    namespace sys = boost::system;

    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

    void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm) {

        auto ts = *rec[timestamp];
        strm << to_iso_extended_string(ts) << ": ";

        //data

        // Выводим само сообщение.
        strm << rec[logging::expressions::smessage];
    }

    void SetupLogger() {
        logging::add_common_attributes();
        logging::add_console_log(
                std::clog,
                keywords::format = &MyFormatter,
                keywords::auto_flush = true
        );
    }
}