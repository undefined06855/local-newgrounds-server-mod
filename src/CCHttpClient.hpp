#pragma once
#include <Geode/modify/CCHttpClient.hpp>

class $modify(HookedCCHttpClient, cocos2d::extension::CCHttpClient) {
    static void onModify(auto& self);
    void send(cocos2d::extension::CCHttpRequest* request);
};
