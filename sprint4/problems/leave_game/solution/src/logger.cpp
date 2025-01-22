
#include "logger.h"

namespace json_logger{
    void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm) {
        auto ts = *rec[timestamp];
        json::object jobject;
        jobject.emplace("timestamp", to_iso_extended_string(ts));
        jobject.emplace("data", *rec[additional_data]);
        jobject.emplace("message", *rec[logging::expressions::smessage]);
        std::string str = json::serialize(jobject);
        strm << str;
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