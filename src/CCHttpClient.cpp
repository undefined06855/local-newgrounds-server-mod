#include "CCHttpClient.hpp"
#include "CCHttpRequest.hpp"
#include "MenuLayer.hpp"

// async web + newgrounds proxy compatibility
void HookedCCHttpClient::onModify(auto& self) {
    auto res = self.setHookPriorityPre("cocos2d::extension::CCHttpClient::send", geode::Priority::VeryEarly);
    if (res.isErr()) {
        geode::log::warn("failed to set hook prio");
    }
}

void HookedCCHttpClient::send(cocos2d::extension::CCHttpRequest* request) {
    auto url = std::string(request->getUrl());
    auto requestFields = reinterpret_cast<FieldsCCHttpRequest *>(request)->m_fields.self();

    // network test failed
    if (g_networkTestFailed) {
        CCHttpClient::send(request);
        return;
    }

    // we've just handled this
    if (requestFields->m_handled) {
        CCHttpClient::send(request);
        return;
    }

    // see if this is a url we should handle
    static const std::array<const char*, 3> songUrlPrefixes = {
        "https://audio.",
        "http://audio.",
        "https://geomet",
    };

    bool found = false;
    for (auto prefix : songUrlPrefixes) {
        if (url.starts_with(prefix)) {
            found = true;
            break;
        }
    }

    // not a url we need to handle
    if (!found) {
        CCHttpClient::send(request);
        return;
    }

    request->retain();

    std::string id;
    bool isSong;
    if (url.starts_with("https://geom")) {
        isSong = url.find("sfx") == std::string::npos;
        if (isSong) {
            if (url.find("music") != std::string::npos) {
                // https://geometrydashfiles.b-cdn.net/music/10010848.ogg (library)
                auto start = url.find("music/") + 6;
                auto end = url.find(".ogg", start);
                id = url.substr(start, end - start);
            } else {
                // https://geometrydashcontent.b-cdn.net/songs/933704.mp3 (ng rob)
                auto start = url.find("songs/") + 6;
                auto end = url.find(".mp3", start);
                id = url.substr(start, end - start);
            }
        } else {
            // https://geometrydashfiles.b-cdn.net/sfx/s10010848.ogg (sfx)
            auto start = url.find("sfx/s") + 5;
            auto end = url.find(".ogg", start);
            id = url.substr(start, end - start);
        }
    } else {
        // newgrounds
        // https://audio.ngfiles.com/1416000/1416510_ircurs.mp3?f1744069343 (ng direct)
        isSong = true;
        auto start = url.find("0/") + 2;
        auto end = url.find("_", start);
        id = url.substr(start, end - start);
    }

    // send req to server to poll if it has it
    auto req = geode::utils::web::WebRequest();
    req.timeout(std::chrono::seconds(2));

    auto type = isSong ? "song" : "sfx";
    auto pollUrl = fmt::format("{}/poll/{}/{}", g_url, type, id);
    auto downloadUrl = fmt::format("{}/download/{}/{}", g_url, type, id);

    geode::log::debug("Testing {} {}", type, id);

    // and send the request
    requestFields->m_listener.getFilter().cancel();
    requestFields->m_listener.setFilter(req.get(pollUrl));
    requestFields->m_listener.bind([this, request, requestFields, downloadUrl](geode::utils::web::WebTask::Event *event) {
        if (event->isCancelled()) {
            request->setShouldCancel(true);
            request->release();
            return;
        }

        if (event->getValue()) {
            auto value = event->getValue()->json().unwrapOrDefault();

            // invalid (somehow)
            // this will retry next frame since itll call the hook again
            if (value.get<bool>("invalid").unwrapOr(true)) {
                CCHttpClient::send(request);
                request->release();
                return;
            }

            // yes it exists on server! change url to download url before sending off
            if (value.get<bool>("exists").unwrapOr(false)) {
                geode::log::debug("Exists on server, downloading using that...");
                request->setUrl(downloadUrl.c_str());
            }

            requestFields->m_handled = true;
            CCHttpClient::send(request);
            request->release();
        }
    });

    IconType::
}
