#pragma once
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>

extern bool g_networkTestFailed;
extern std::string g_url;

class $modify(HookedMenuLayer, MenuLayer) {
    struct Fields {
        geode::EventListener<geode::utils::web::WebTask> m_listener;
    };

    bool init();
};
