#include "api_request.h"
#include <boost/json.hpp>
#include <algorithm>

namespace http_handler
{
    using namespace std::literals;
    namespace json = boost::json;
    namespace sys = boost::system;

    void ApiRequestHandler::LinkJoinGameWithoutAuthorize()
    {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::JOIN_GAME);
        if(ptr)
        {
            ptr->SetNeedAuthorization(false)
                .SetContentType(Response::ContentType::TEXT_JSON, ErrorMessage::JSON_CONTENT_TYPE)
                .SetAllowedMethods({http::verb::post}, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
                .SetProcessFunction([&](const std::string& target, const std::string& body){
                    return ProcessJoinGame(target, body); } );
        }
    }

    void ApiRequestHandler::LinkPlayersList()
    {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::PLAYERS_LIST);
        if(ptr)
        {
            ptr->SetNeedAuthorization(true)
                .SetAllowedMethods({http::verb::get, http::verb::head}, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                .SetProcessFunction([&](const Token& token, std::string_view body){
                    return ProcessGetPlayersList(token, body);
                });
        }
    }

    void ApiRequestHandler::LinkMaps() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::MAPS);
        if(ptr)
        {
            ptr->SetNeedAuthorization(false)
                .SetAllowedMethods({http::verb::get, http::verb::head},
                                   ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                .SetProcessFunction([&](const std::string& target, const std::string& body){
                    return ProcessGetMaps(target, body);
                } );
        }
    }

    void ApiRequestHandler::LinkMapId() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::MAPSID);
        if(ptr)
        {
            ptr->SetNeedAuthorization(false)
                    .SetAllowedMethods({http::verb::get, http::verb::head},
                                       ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                    .SetProcessFunction([&](const std::string& target, const std::string& body){
                        return ProcessGetMapId(target, body);
                    } );
        }
    }

    void ApiRequestHandler::LinkGameState() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_STATE);
        if(ptr)
        {
            ptr->SetNeedAuthorization(true)
                    .SetAllowedMethods({http::verb::get, http::verb::head}, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                    .SetProcessFunction([&](const Token& token, std::string_view body){
                        return ProcessGetGameState(token, body);
                    });
        }
    }

    void ApiRequestHandler::LinkPlayerAction() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::PLAYER_ACTION);
        if(ptr)
        {
            ptr->SetNeedAuthorization(true)
                    .SetContentType(Response::ContentType::TEXT_JSON, ErrorMessage::JSON_CONTENT_TYPE)
                    .SetAllowedMethods({http::verb::post}, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
                    .SetProcessFunction([&](const Token& token, std::string_view body){
                        return ProcessPlayerAction(token, std::string(body));
                    });
        }
    }

    void ApiRequestHandler::LinkTimeTick() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::TIME_TICK);
        if(ptr)
        {
            ptr->SetNeedAuthorization(false)
                    .SetContentType(Response::ContentType::TEXT_JSON, ErrorMessage::JSON_CONTENT_TYPE)
                    .SetAllowedMethods({http::verb::post}, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
                    .SetProcessFunction([&](const std::string& target, const std::string& body){
                        return ProcessTimeTick(target, body);
                    });
        }
    }

    void ApiRequestHandler::LinkRecords()
    {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::RECORDS);
        if(ptr)
        {
            ptr->SetNeedAuthorization(false)
                    .SetContentType(Response::ContentType::TEXT_JSON, ErrorMessage::JSON_CONTENT_TYPE)
                    .SetAllowedMethods({http::verb::get, http::verb::head}, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                    .SetProcessFunction([&](const std::string& target, const std::string& body){
                        return ProcessRecords(target, body); } );
        }
    }

    StringResponse ApiRequestHandler::ProcessGetMaps(const std::string& target, const std::string& body) const {

        std::string json_response = json_loader::GetGameMaps(application_.GetGameModel());
        return Response::Make(http::status::ok, json_response );
    }

    StringResponse ApiRequestHandler::ProcessGetMapId(const std::string& target, const std::string& body){
        if(target.size() == 13){
            return ProcessGetMaps(target, body);
        }
        auto game = application_.GetGameModel();
        if((target.find("/api/v1/maps/") == std::string::npos) ||
           (game.FindMap(model::Map::Id{std::string(target.substr(13))}) == nullptr)){
            return Response::MakeNotFoundJson();
        }
        auto extra_data = application_.GetExtraData();

        model::Map::Id id{std::string(target.substr(13))};
        std::string json_response = json_loader::GetGameMapFromId(game, extra_data, id);
        return Response::Make(http::status::ok, json_response );
    }

    StringResponse ApiRequestHandler::ProcessJoinGame(const std::string& target, const std::string& body)
    {
        json::error_code ec;
        json::value request_data = json::parse(body, ec);
        if(ec){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_JSON);
        }
        auto const& obj_head = request_data.as_object();
        if(!obj_head.contains("userName") || !obj_head.contains("mapId") ){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_JSON);
        }
        std::string user_name = static_cast<std::string>(obj_head.at("userName").as_string());
        std::string map_id = static_cast<std::string>(obj_head.at("mapId").as_string());

        if(user_name.empty()){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::INVALID_NAME);
        }
        auto map = application_.FindMap(model::Map::Id{map_id});
        if(map == nullptr){
            return Response::MakeNotFoundJson();
        }


        auto [token, player_id] = application_.JoinGame( user_name, map_id);

        json::value response_data{{"authToken", *token}, {"playerId", player_id}};
        std::string response_string = json::serialize(response_data);
        return Response::Make(http::status::ok, response_string );
    }

    StringResponse ApiRequestHandler::ProcessGetPlayersList(const Token& token, std::string_view body)
    {
        auto player = application_.FindPlayerByToken(token);
        if(player == nullptr) {
            return Response::MakeUnauthorizedErrorUnknownToken();
        }

        auto id_and_names = application_.GetPlayersList(token);
        json::object jobject;
        for(const auto& [id, name] : id_and_names){
            json::object jbody {{"name", name}};
            jobject.emplace(std::to_string(id), jbody);
        }

        std::string response_string = json::serialize(jobject);
        return Response::Make( http::status::ok, response_string );
    }

    StringResponse ApiRequestHandler::ProcessGetGameState(const Token& token, std::string_view body){

        auto player = application_.FindPlayerByToken(token);
        if(player == nullptr) {
            return Response::MakeUnauthorizedErrorUnknownToken();
        }

        auto session = player->GetGameSession();
        auto id_and_dog_datas = session->GetIdAndDogDatas();
        json::object jobj_player_id;
        for(const auto& [id, dog_data] : id_and_dog_datas){
            json::object jobj_body;
            jobj_body.emplace("pos", json::array{dog_data.pos.first, dog_data.pos.second});
            jobj_body.emplace("speed", json::array{dog_data.speed.first, dog_data.speed.second});
            jobj_body.emplace("dir", dog_data.dir);

            json::array jlist_bag_item;
            for(const auto& item : dog_data.bag){
                json::object jobj_bag_item;
                jobj_bag_item.emplace("id", item.first);
                jobj_bag_item.emplace("type", item.second);
                jlist_bag_item.push_back(jobj_bag_item);
            }
            jobj_body.emplace("bag", jlist_bag_item);
            jobj_body.emplace("score", dog_data.score);

            jobj_player_id.emplace(std::to_string(id), jobj_body);
        }
        //
        json::object jobj_loot_id;
        for(const auto& [loot_id, loot] : session->GetLoot()){
            json::object jobj_body;
            jobj_body.emplace("type", loot.type);
            jobj_body.emplace("pos", json::array{loot.pos.first, loot.pos.second});

            jobj_loot_id.emplace(std::to_string(loot_id), jobj_body);
        }

        json::object jobj_state;
        jobj_state.emplace("players", jobj_player_id);
        jobj_state.emplace("lostObjects", jobj_loot_id);

        std::string response_string = json::serialize(jobj_state);
        return Response::Make( http::status::ok, response_string );
    }

    StringResponse ApiRequestHandler::ProcessPlayerAction(const Token& token, std::string body){
        auto player = application_.FindPlayerByToken(token);
        if(player == nullptr) {
            return Response::MakeUnauthorizedErrorUnknownToken();
        }

        json::error_code ec;
        json::value request_data = json::parse(body, ec);
        if(ec){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_PLAYER_ACTION);
        }
        auto const& obj_head = request_data.as_object();
        if(!obj_head.contains("move")){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_PLAYER_ACTION);
        }
        std::string move_data = static_cast<std::string>(obj_head.at("move").as_string());

        auto session = player->GetGameSession();
        auto& dog = session->GetDogById(player->GetId());
        dog.SetDirection(move_data);

        return Response::Make( http::status::ok, "{}" );
    }

    StringResponse ApiRequestHandler::ProcessTimeTick(const std::string& target, const std::string& body){

        if(application_.GetTickPeriod() > 0){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::BAD_REQUEST,
                                      ErrorMessage::INVALID_ENDPOINT);
        }

        json::error_code ec;
        json::value request_data = json::parse(body, ec);
        if(ec){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_TIMER_TICK);
        }
        auto const& obj_head = request_data.as_object();
        if(!obj_head.contains("timeDelta")){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_TIMER_TICK);
        }

        if(obj_head.at("timeDelta").kind() != json::kind::int64){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_TIMER_TICK);
        }

        auto time_ms = static_cast<size_t>(obj_head.at("timeDelta").as_int64());
        application_.Tick(time_ms);

        return Response::Make( http::status::ok, "{}" );
    }

    StringResponse ApiRequestHandler::ProcessRecords(const std::string& target, const std::string& body){

        int start = 0;
        int maxItems = Records::RECORDS_MAXITEMS;
        size_t st1 = target.find('?');
        size_t st2 = target.find('&');

        const size_t start_param = 7;
        const size_t max_items_param = 10;
        if( st1 != std::string::npos)
        {
            size_t size = target.size();
            size_t pos_st1 = st1 + start_param;
            size_t count_st1 = st2 - pos_st1;
            std::string start_str = target.substr(pos_st1, count_st1);
            start = std::stoi(start_str);

            size_t pos_st2 = st2 + max_items_param;
            size_t count_st2 = size - pos_st2;
            std::string maxItems_str = target.substr(pos_st2, count_st2);
            maxItems = std::stoi(maxItems_str);
        }

        if(maxItems > Records::RECORDS_MAXITEMS){
            return Response::MakeJSON(http::status::bad_request,
                                      ErrorCode::INVALID_ARGUMENT,
                                      ErrorMessage::PARSE_MAXITEMS);
        }

        std::vector<app::RecordsPlayer> records = application_.ReadRecords(start, maxItems);

        json::array jlist_records;
        for(const auto& record : records){
            json::object jobj_record;
            jobj_record.emplace("name", record.name);
            jobj_record.emplace("score", record.score);
            jobj_record.emplace("playTime", record.play_time_ms * 0.001);
            jlist_records.push_back(jobj_record);
        }

        std::string response_string = json::serialize(jlist_records);
        return Response::Make(http::status::ok, response_string);
    }


}