#pragma once

#include <boost/program_options.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <optional>

using namespace std::literals;

struct Args {
    size_t tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points;
    std::string state_file;     //путь к файлу, в который приложение должно сохранять своё состояние в процессе работы
    size_t save_state_period;   //игровое-время-в-миллисекундах, период автоматического сохранения состояния сервера
};

namespace options{

    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]);

}   //namespace options