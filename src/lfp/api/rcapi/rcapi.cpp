#include "rcapi.h"
#include "mc/server/SimulatedPlayer.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/utils/ErrorUtils.h"

#include "lfp/LeviFakePlayer.h"
#include "lfp/Version.h"
#include "lfp/api/events/FakePlayerCreatedEvent.h"
#include "lfp/api/events/FakePlayerJoinEvent.h"
#include "lfp/api/events/FakePlayerLeaveEvent.h"
#include "lfp/api/events/FakePlayerRemoveEvent.h"
#include "lfp/api/events/FakePlayerUpdateEvent.h"
#include "lfp/api/events/base/FakePlayerEventBase.h"
#include "lfp/manager/FakePlayer.h"
#include "lfp/manager/FakePlayerManager.h"
#include "lfp/utils/DebugUtils.h"
#include "nlohmann/json.hpp"


#define FAKE_PLAYER_API_DEBUG
#if defined(FAKE_PLAYER_API_DEBUG) && !defined(LFP_DEBUG)

#endif // FAKE_PLAYER_API_DEBUG

#define ExportRemoteCallApi(name)                                                                  \
    RemoteCall::exportAs<decltype(lfp::api::rcapi::name)>(                                         \
        LEVIFAKEPLAYER_NAMESPACE,                                                                  \
        #name,                                                                                     \
        lfp::api::rcapi::name                                                                      \
    )

namespace lfp::api {

std::string InfoToJson(FakePlayerInfo const& info, nlohmann::json& json) {
    json["name"]      = info.name;
    json["xuid"]      = info.xuid;
    json["uuid"]      = info.uuid;
    json["skinId"]    = info.skinId;
    json["online"]    = info.online;
    json["autoLogin"] = info.autoLogin;
    // json["lastUpdateTime"] = info.lastUpdateTime;
    return json.dump();
}


FakePlayerInfo InfoFromFakePlayer(lfp::FakePlayer const& fp) {
    return {
        fp.getRealName(),
        fp.getXuid(),
        fp.getUuid().asString(),
        "",
        fp.isOnline(),
        fp.shouldAutoLogin(),
        fp.getRuntimePlayer(),
    };
}

FakePlayerInfo InfofromEvent(event::FakePlayerEventBase const& ev) {
    return InfoFromFakePlayer(ev.fakePlayer());
}

std::string FakePlayerInfo::toJson() const {
    nlohmann::json json;
    return InfoToJson(*this, json);
}

FakePlayerInfo::SerializedType FakePlayerInfo::toValue() const {
#if INFO_OBJECT_SUPPORTED
    FakePlayerInfo::SerializedType::ObjectType object = {};
    object["name"]                                    = name;
    object["xuid"]                                    = xuid;
    object["uuid"]                                    = uuid;
    object["skinId"]                                  = skinId;
    object["online"]                                  = online;
    object["autoLogin"]                               = autoLogin;
    object["player"]                                  = player;
    return object;
#else
    return toJson();
#endif
}


FakePlayerInfo getInfo(std::string const& name) {
    DEBUGL(__FUNCTION__);
    auto fp = lfp::FakePlayerManager::getManager().tryGetFakePlayer(name);
    if (fp) return InfoFromFakePlayer(*fp);
    return {};
}

std::vector<FakePlayerInfo> getAllInfo() {
    DEBUGL(__FUNCTION__);
    auto view =
        lfp::FakePlayerManager::getManager().iter() | std::views::transform(InfoFromFakePlayer);
    std::vector<FakePlayerInfo> result{};
    for (auto v : view) result.emplace_back(v);
    return result;
}


SimulatedPlayer* createAt(std::string const& name, int x, int y, int z, int dimId) {
    return rcapi::createAt(name, {BlockPos(x, y, z), dimId});
}

namespace rcapi {

int getApiVersion() {
    DEBUGL(__FUNCTION__);
    return REMOTE_CALL_API_VERSION;
};


std::string getVersion() {
    DEBUGL(__FUNCTION__);
    return LFP_FILE_VERSION_STRING;
}

std::vector<SimulatedPlayer*> getOnlineList() {
    DEBUGL(__FUNCTION__);
    std::vector<SimulatedPlayer*> list;
    for (auto& fp : lfp::FakePlayerManager::getManager().getTimeSortedFakePlayerList()) {
        if (fp->isOnline()) list.emplace_back(fp->getRuntimePlayer());
    }
    return list;
}

FakePlayerInfoRaw getInfo(std::string const& name) {
    DEBUGL(__FUNCTION__);
    auto info = api::getInfo(name);
    return info.toValue();
}

std::vector<FakePlayerInfoRaw> getAllInfo() {
    DEBUGL(__FUNCTION__);
    auto view = lfp::FakePlayerManager::getManager().iter()
              | std::views::transform(InfoFromFakePlayer)
              | std::views::transform(&FakePlayerInfo::toValue);
    std::vector<FakePlayerInfoRaw> result{};
    for (auto v : view) result.emplace_back(v);
    return result;
    // return std::vector<FakePlayerInfoRaw>(view.begin(), view.end());
}

SimulatedPlayer* getRuntimePlayer(std::string const& name) {
    DEBUGL(__FUNCTION__);
    auto fp = lfp::FakePlayerManager::getManager().tryGetFakePlayer(name);
    if (!fp) return {};
    return fp->getRuntimePlayer();
};

std::vector<std::string> list() {
    DEBUGL(__FUNCTION__);
    // auto view = lfp::FakePlayerManager::getManager().iter()
    //           | std::views::transform([](auto& fp) { return fp.getRealName(); });
    // return std::vector(view.begin(), view.end());
    auto nameView = FakePlayerManager::getManager().getTimeSortedFakePlayerList()
                  | std::views::transform([](auto fp) { return fp->getRealName(); });
    return {nameView.begin(), nameView.end()};
}

bool create(std::string const& name) {
    DEBUGL(__FUNCTION__);
    return lfp::FakePlayerManager::getManager().create(name);
}
bool createWithData(std::string const& name, CompoundTag* data) {
    DEBUGL(__FUNCTION__);
    return nullptr != lfp::FakePlayerManager::getManager().create(name, false, data->clone());
}

SimulatedPlayer* createAt(std::string const& name, std::pair<BlockPos, int> pos) {
    DEBUGL(__FUNCTION__);
    auto fp = lfp::FakePlayerManager::getManager().create(name, pos.first, pos.second);
    if (fp && fp->login()) {
        return fp->getRuntimePlayer();
    }
    return nullptr;
}

SimulatedPlayer* login(std::string const& name) {
    DEBUGL(__FUNCTION__);
    return lfp::FakePlayerManager::getManager().login(name);
}
bool logout(std::string const& name) {
    DEBUGL(__FUNCTION__);
    return lfp::FakePlayerManager::getManager().logout(name);
}

bool setAutoLogin(std::string const& name, bool autoLogin) {
    DEBUGL(__FUNCTION__);
    auto& manager = lfp::FakePlayerManager::getManager();
    auto  fp      = manager.tryGetFakePlayer(name);
    if (!fp) return false;
    return manager.setAutoLogin(*fp, autoLogin);
};

bool remove(std::string const& name) {
    DEBUGL(__FUNCTION__);
    return lfp::FakePlayerManager::getManager().remove(name);
}

bool importClientFakePlayer(std::string const& name) {
    DEBUGL(__FUNCTION__);
    return lfp::FakePlayerManager::getManager().importData_JavaFakePlayer(name);
}

bool unsubscribeEvent(ll::event::ListenerId id) {
    DEBUGL(__FUNCTION__);
    return ll::event::EventBus::getInstance().removeListener(id);
};

ll::event::ListenerId subscribeEvent(
    std::string const& eventName,
    std::string const& callbackNamespace,
    std::string const& callbackName
) {
    DEBUGL(__FUNCTION__);
    auto eventType = magic_enum::enum_cast<EventType>(eventName);
    if (!eventType) {
        throw std::runtime_error("Unknown EventName");
    }
    auto handleFn = RemoteCall::importAs<RemoteCallCallbackFn>(callbackNamespace, callbackName);
    return subscribeEventImpl(
        *eventType,
        [handleFn = std::move(handleFn)](event::FakePlayerEventBase const& ev) {
            try {
                handleFn(
                    ev.name(),
                    InfoFromFakePlayer(ev.fakePlayer()).toJson(),
                    ev.fakePlayer().getRuntimePlayer()
                );
            } catch (...) {
                auto& logger = lfp::LeviFakePlayer::getLogger();
                logger.error("Error occurred in api event callback");
                ll::error_utils::printCurrentException(logger);
            }
        }
    );
}


} // namespace rcapi

bool ExportRemoteCallApis() {
    bool res = true;
    res      = res && ExportRemoteCallApi(getApiVersion);
    res      = res && ExportRemoteCallApi(getVersion);
    res      = res && ExportRemoteCallApi(getOnlineList);
    res      = res && ExportRemoteCallApi(getInfo);
    res      = res && ExportRemoteCallApi(getAllInfo);
    res      = res && ExportRemoteCallApi(list);
    res      = res && ExportRemoteCallApi(create);
    res      = res && ExportRemoteCallApi(createAt);
    res      = res && ExportRemoteCallApi(createWithData);
    res      = res && ExportRemoteCallApi(login);
    res      = res && ExportRemoteCallApi(logout);
    res      = res && ExportRemoteCallApi(setAutoLogin);
    res      = res && ExportRemoteCallApi(remove);
    res      = res && ExportRemoteCallApi(importClientFakePlayer);
    res      = res && ExportRemoteCallApi(subscribeEvent);
    res      = res && ExportRemoteCallApi(unsubscribeEvent);

    return res;
}

ll::event::ListenerId subscribeEventImpl(
    EventType                                                eventType,
    std::function<void(event::FakePlayerEventBase const&)>&& callback
) {
    DEBUGL(__FUNCTION__);
    auto&                  bus = ll::event::EventBus::getInstance();
    ll::event::ListenerPtr listener;
    switch (eventType) {
    case EventType::create:
        listener = bus.emplaceListener<event::FakePlayerCreatedEvent>(std::move(callback));
        break;
    case EventType::remove:
        listener = bus.emplaceListener<event::FakePlayerRemoveEvent>(std::move(callback));
        break;
    case EventType::login:
        listener = bus.emplaceListener<event::FakePlayerJoinEvent>(std::move(callback));
        break;
    case EventType::logout:
        listener = bus.emplaceListener<event::FakePlayerLeaveEvent>(std::move(callback));
        break;
    case EventType::update:
        listener = bus.emplaceListener<event::FakePlayerUpdateEvent>(std::move(callback));
        break;
    default:
        throw std::runtime_error("Unknown EventType");
    }
    return listener->getId();
};

void RemoveRemoteCallApis() { RemoteCall::removeNameSpace(LEVIFAKEPLAYER_NAMESPACE); }


} // namespace lfp::api
