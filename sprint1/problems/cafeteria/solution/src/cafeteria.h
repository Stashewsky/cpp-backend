#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <syncstream>
#include <thread>
#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;


using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, int id, Store& store, std::shared_ptr<GasCooker> gas_cooker, HotDogHandler handler)
            : io_{io}
            , id_{id}
            , sausage_(store.GetSausage())
            , bread_(store.GetBread())
            , cooker_(gas_cooker)
            , handler_{std::move(handler)}
            , strand_{net::make_strand(io_)} {
    }

    void Execute() {
        net::dispatch(strand_, [self = shared_from_this()] {
            self->FrySausage();
        });

        net::dispatch(strand_, [self = shared_from_this()] {
            self->BakeBread();
        });
    }
private:
    net::io_context& io_;
    int id_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<GasCooker> cooker_;
    HotDogHandler handler_;
    net::strand<net::io_context::executor_type> strand_;
    Timer sausage_frying_timer_{io_};
    Timer bread_baking_timer_{io_};


    void FrySausage() {
        sausage_->StartFry(*cooker_, [self = shared_from_this()] {
            self->sausage_frying_timer_.expires_after(HotDog::MIN_SAUSAGE_COOK_DURATION);
            self->sausage_frying_timer_.async_wait(
                    net::bind_executor(self->strand_, [self](const sys::error_code& ec){
                        self->StopFrying();
                    }));

        });

    }

    void StopFrying() {
        sausage_->StopFry();

        if (IsReadyToPack()) {
            Pack();
        }
    }

    void BakeBread() {
        bread_->StartBake(*cooker_, [self = shared_from_this()]{
            self->bread_baking_timer_.expires_after(HotDog::MIN_BREAD_COOK_DURATION);
            self->bread_baking_timer_.async_wait(
                    net::bind_executor(self->strand_, [self](const sys::error_code& ec) {
                        self->StopBaking();
                    }));
        });

    }

    void StopBaking() {
        bread_->StopBake();

        if (IsReadyToPack()) {
            Pack();
        }
    }

    [[nodiscard]] bool IsReadyToPack() const {
        return sausage_->IsCooked() && bread_->IsCooked();
    }

    void Pack() {
        HotDog hotdog(id_, sausage_, bread_);
        handler_(std::move(hotdog));
    }
};

class Cafeteria {
public:
    Cafeteria(net::io_context& io) : io_(io) {
    }

    void OrderHotDog(HotDogHandler handler) {
        std::make_shared<Order>(io_, next_order_id_++, store_, gas_cooker_, std::move(handler))->Execute();
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int next_order_id_ = 0;
};
