#include "program_options.h"

namespace options{

    std::optional<Args> ParseCommandLine(int argc, const char *const *argv) {
        namespace po = boost::program_options;

        po::options_description desc{"Allowed options"s};

        Args args;
        desc.add_options()
                // Добавляем опцию --help и её короткую версию -h
                ("help,h", "produce help message")
                ("tick-period,t", po::value<size_t>(&args.tick_period)->default_value(0)->value_name("milliseconds"), "set tick period")
                ("config-file,c", po::value(&args.config_file)->value_name("file"), "set config file path")
                ("www-root,w", po::value(&args.www_root)->value_name("dir"), "set static files root")
                ("randomize-spawn-points", po::value<bool>(&args.randomize_spawn_points)->default_value(true), "spawn dogs at random positions ")
                ("state-file,f", po::value(&args.state_file)->value_name("file"), "set game state file")
                ("save-state-period,p", po::value<size_t>(&args.save_state_period)->default_value(0)->value_name("milliseconds"), "set save spate period");
        // variables_map хранит значения опций после разбора
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.contains("help"s)) {
            // Если был указан параметр --help, то выводим справку и возвращаем nullopt
            std::cout << desc;
            return std::nullopt;
        }

        // Проверяем наличие опций
        if (!vm.contains("config-file"s)) {
            throw std::runtime_error("Config file have not been specified"s);
        }
        if (!vm.contains("www-root"s)) {
            throw std::runtime_error("Static file directory path is not been specified"s);
        }

        return args;
    }

}