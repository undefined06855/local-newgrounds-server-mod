#include "MenuLayer.hpp"

bool g_networkTestFailed = false;
std::string g_url = "";

bool HookedMenuLayer::init() {
    if (!MenuLayer::init()) return false;

    auto fields = m_fields.self();

    g_url = geode::Mod::get()->getSettingValue<std::string>("server-url");

    // network test
    auto req = geode::utils::web::WebRequest();
    fields->m_listener.bind([this](geode::utils::web::WebTask::Event* event) {
        if (event->getValue()) {
            auto res = event->getValue()->json();
            if (res.isErr()) {
                geode::log::error("Invalid response from server!");
                auto pop = FLAlertLayer::create(
                    "Local Newgrounds Server",
                    "Server did not return expected response when testing connection!",
                    "ok"
                );
                pop->m_scene = this;
                pop->show();
                g_networkTestFailed = true;
                return;
            }

            auto data = res.unwrap();
            
            auto version = data.get<std::string>("version").unwrapOr("v0.0.0");
            auto serverVersion = geode::VersionInfo::parse(version).unwrapOrDefault();
            auto targetVersion = geode::VersionInfo::parse("v1.0.0").unwrapOrDefault();
            
            if (serverVersion < targetVersion) {
                geode::log::error("Out of date version from server!");
                auto pop = FLAlertLayer::create(
                    "Local Newgrounds Server",
                    fmt::format(
                        "Server version <cj>{}</c> is <cr>lower</c> than "
                        "expected version <cj>{}</c>! This may lead to "
                        "<co>unexpected</c> behaviour!",
                        serverVersion, targetVersion
                    ).c_str(),
                    "ok"
                );
                pop->m_scene = this;
                pop->show();
                return;
            }

            geode::log::info("Server test successful!");
        }
    });
    fields->m_listener.setFilter(req.get(g_url));

    return true;
}
