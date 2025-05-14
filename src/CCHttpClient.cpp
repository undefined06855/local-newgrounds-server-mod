#include "CCHttpClient.hpp"
#include "CCHttpRequest.hpp"
#include "MenuLayer.hpp"

void HookedCCHttpClient::send(cocos2d::extension::CCHttpRequest *request) {
    auto url = std::string(request->getUrl());

    // network test failed
    if (g_networkTestFailed) {
        CCHttpClient::send(request);
        return;
    }

    // we've just handled this
    if (url.ends_with("?done=true")) {
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
        // robtop (library or sfx)
        isSong = url.find("music") != std::string::npos;
        auto start = url.find_last_of("/") + 1;
        auto end = url.find_first_of(".ogg", start);
        id = url.substr(start, end - start);
    } else {
        // newgrounds
        isSong = true;
        auto start = url.find_last_of("/") + 1;
        auto end = url.find_first_of("_", start);
        id = url.substr(start, end - start);
    }

    // send req to server to poll if it has it
    auto req = geode::utils::web::WebRequest();
    req.timeout(std::chrono::seconds(2));

    auto type = isSong ? "song" : "sfx";
    auto pollUrl = fmt::format("{}/poll/{}/{}", g_url, type, id);
    auto downloadUrl = fmt::format("{}/download/{}/{}", g_url, type, id);

    geode::log::debug("Testing {} {}", isSong, id);

    // and send the request
    auto requestFields = reinterpret_cast<FieldsCCHttpRequest *>(request)->m_fields.self();
    requestFields->m_listener.getFilter().cancel();
    requestFields->m_listener.setFilter(req.get(pollUrl));
    requestFields->m_listener.bind([this, request, downloadUrl](geode::utils::web::WebTask::Event *event) {
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

            request->setUrl(fmt::format("{}?done=true", request->getUrl()).c_str());
            CCHttpClient::send(request);
            request->release();
        }
    });
}
