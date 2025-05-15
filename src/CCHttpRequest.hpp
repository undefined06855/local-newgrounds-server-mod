#pragma once
#include <alphalaneous.alphas_geode_utils/include/NodeModding.h>
#include <Geode/utils/web.hpp>

class $objectModify(FieldsCCHttpRequest, cocos2d::extension::CCHttpRequest) {
    struct Fields {
        geode::EventListener<geode::utils::web::WebTask> m_listener;
        bool m_handled;
        Fields();
    };

    void modify();
};
