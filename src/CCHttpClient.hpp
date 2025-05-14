#pragma once
#include <Geode/modify/CCHttpClient.hpp>

class $modify(HookedCCHttpClient, cocos2d::extension::CCHttpClient) {
    void send(cocos2d::extension::CCHttpRequest* request);
};
