#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        // TODO: реализуйте самостоятельно
        bool my_turn = my_initiative;
        while(!IsGameEnded()){
            if(my_turn){
                my_turn = MakeMyShot(socket);
            }else{
                my_turn = MakeEnemyShot(socket);
            }
        }

        if(my_turn){
            std::cout << "Congratulations! You win!"sv << std::endl;
        }else{
            std::cout << "You lose! Game over! Good luck for a next try!"sv << std::endl;
        }
        
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию
    bool MakeMyShot(tcp::socket& socket){
        boost::system::error_code err_code;
        std::string user_shot_coordinates_str;
        
        PrintFields();

        std::cout << "Make your shoot!"sv << std::endl;
        std::cin >> user_shot_coordinates_str;
        user_shot_coordinates_str[0] = std::toupper(user_shot_coordinates_str[0]);
        auto parsed_coordinates = ParseMove(user_shot_coordinates_str);
        user_shot_coordinates_str.push_back('\n');

        SendMove(socket, user_shot_coordinates_str);
        SeabattleField::ShotResult shot_result = ReadResult(socket);
        MarkShot(other_field_, parsed_coordinates, shot_result);
        return shot_result != SeabattleField::ShotResult::MISS;
    }

    void SendMove(tcp::socket& socket, const std::string user_shot_coordinates){
        boost::system::error_code err_code;
        socket.write_some(net::buffer(user_shot_coordinates), err_code);

        if(err_code){
            std::cout << "Sending data error!"sv << std::endl;
            std::exit(1);
        }
    }

    SeabattleField::ShotResult ReadResult(tcp::socket& socket){
        boost::system::error_code err_code;
        net::streambuf stream_buffer;
        net::read_until(socket, stream_buffer, '\n', err_code);

        if (err_code) {
            std::cout << "Reading data error!"sv << std::endl;
            std::exit(1);
        }

        std::string shot_result_str{std::istreambuf_iterator<char>(&stream_buffer),
                                std::istreambuf_iterator<char>()};

        if(shot_result_str.length() > 2 && (shot_result_str[0] < 0 || shot_result_str[0] > 2)){
            std::cout << "Shot result error!"sv << std::endl;
            std::exit(1);
        }
        return static_cast<SeabattleField::ShotResult>(std::stoi(shot_result_str));
    }

    bool MakeEnemyShot(tcp::socket& socket){
        PrintFields();
        std::string enemy_shot_coordinates_str = ReadMove(socket);
        std::cout << "Enemy shoot at: "sv << enemy_shot_coordinates_str << std::endl;
        auto parsed_coords = ParseMove(enemy_shot_coordinates_str);
        auto shot_result = my_field_.Shoot(parsed_coords->first, parsed_coords->second);

        SendResult(socket, shot_result);
        MarkShot(my_field_, parsed_coords, shot_result);
        return shot_result != SeabattleField::ShotResult::MISS;
    }
    
    std::string ReadMove(tcp::socket& socket){
        boost::system::error_code err_code;
        net::streambuf stream_buffer;
        std::cout << "Make the enemy shot!"sv << std::endl;
        net::read_until(socket, stream_buffer, '\n', err_code);

        if (err_code) {
            std::cout << "Reading data error!"sv << std::endl;
            std::exit(1);
        }

        std::string shot_coordinates{std::istreambuf_iterator<char>(&stream_buffer),
                                std::istreambuf_iterator<char>()};
        return shot_coordinates.substr(0, 2);
    }

    void SendResult(tcp::socket& socket, SeabattleField::ShotResult& result){
        boost::system::error_code ec;
        std::stringstream shot_result;
        shot_result << static_cast<int>(result) << "\n";

        socket.write_some(net::buffer(shot_result.str()), ec);

        if (ec) {
            std::cout << "Error sending data"sv << std::endl;
            std::exit(1);
        }
    }

    void MarkShot(SeabattleField& field, std::optional<std::pair<int,int>> shot_coords,SeabattleField::ShotResult result){
        switch(result){
            case SeabattleField::ShotResult::HIT:
                field.MarkHit(shot_coords->first, shot_coords->second);
                std::cout << "Hit the ship!"sv << std::endl;
                break;
            case SeabattleField::ShotResult::KILL:
                field.MarkKill(shot_coords->first, shot_coords->second);
                std::cout << "Kill the ship!"sv << std::endl;
                break;
            case SeabattleField::ShotResult::MISS:
                field.MarkMiss(shot_coords->first, shot_coords->second);
                std::cout << "Miss!"sv << std::endl;
                break;
        }
    }
private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    net::io_context io_ctx;
    tcp::acceptor acceptor(io_ctx, tcp::endpoint(tcp::v4(), port));

    std::cout << "Waiting for CLIENT connection..."sv << std::endl;
    boost::system::error_code err_code;
    tcp::socket socket(io_ctx);

    acceptor.accept(socket, err_code);
    if(err_code){
        std::cout << "Can not accept the connection!"sv << std::endl;
        std::exit(1);
    }
    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    boost::asio::io_context io_ctx;
    tcp::socket socket(io_ctx);
    boost::system::error_code err_code;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, err_code), port);
    if(err_code){
        std::cout << "Error! Check IP adress!" << std::endl;
        std::exit(1);
    }

    socket.connect(endpoint, err_code);
    if(err_code){
        std::cout << "Could not connect to the server!"sv << std::endl;
        std::exit(1);
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
