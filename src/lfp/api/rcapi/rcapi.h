#pragma once

#include "lfp/api/base/Global.h"

#include "ll/api/event/ListenerBase.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/world/level/storage/DBStorage.h"
#include "mc/world/level/storage/LevelStorage.h"
#include "nlohmann/json_fwd.hpp"

#include "RemoteCallAPI.h"

#include "lfp/api/base/Global.h"
#include "lfp/api/events/base/FakePlayerEventBase.h"
#include "lfp/manager/FakePlayer.h"

// example getVersionString = ImportFakePlayerAPI
#define LEVIFAKEPLAYER_NAMESPACE "LeviFakePlayerAPI"
#define ImportFakePlayerAPI(name)                                                                  \
    RemoteCall::importAs<decltype(lfp::api::rcapi::name)>(LEVIFAKEPLAYER_NAMESPACE, #name)

[[maybe_unused]] constexpr int REMOTE_CALL_API_VERSION = 1;
#define INFO_OBJECT_SUPPORTED false

namespace lfp::api {


enum class EventType {
    create,
    remove,
    login,
    logout,
    update,
};

struct FakePlayerInfo {
#if INFO_OBJECT_SUPPORTED
    using SerializedType = RemoteCall::ValueType;
#else
    using SerializedType = std::string;
#endif
    std::string      name;
    std::string      xuid;
    std::string      uuid;
    std::string      skinId{};
    bool             online    = false;
    bool             autoLogin = false;
    SimulatedPlayer* player    = nullptr;

    LFPNDAPI std::string toJson() const;
    SerializedType       toValue() const;
    // static ll::Expected<FakePlayerInfo> fromValue(RemoteCall::ValueType const& value);
};

using FakePlayerInfoRaw = FakePlayerInfo::SerializedType;
using RemoteCallCallbackFn =
    void(std::string const& name, FakePlayerInfoRaw const& info, SimulatedPlayer* sp);


LFPNDAPI FakePlayerInfo getInfo(std::string const& name);
LFPNDAPI std::vector<FakePlayerInfo> getAllInfo();
LFPAPI SimulatedPlayer* createAt(std::string const& name, int x, int y, int z, int dimId);

//////////////////// RemoteCall API ////////////////////
namespace rcapi {

LFPNDAPI int getApiVersion();
LFPNDAPI std::string getVersion();
LFPNDAPI std::vector<SimulatedPlayer*> getOnlineList();
LFPNDAPI FakePlayerInfoRaw             getInfo(std::string const& name);
LFPNDAPI std::vector<FakePlayerInfoRaw> getAllInfo();
LFPNDAPI SimulatedPlayer*               getRuntimePlayer(std::string const& name);
LFPNDAPI std::vector<std::string> list();

LFPAPI bool             create(std::string const& name);
LFPAPI bool             createWithData(std::string const& name, CompoundTag* data);
LFPAPI SimulatedPlayer* createAt(std::string const& name, std::pair<BlockPos, int> pos);
LFPAPI SimulatedPlayer* login(std::string const& name);
LFPAPI bool             logout(std::string const& name);
LFPAPI bool             setAutoLogin(std::string const& name, bool autoLogin);
LFPAPI bool             remove(std::string const& name);
LFPAPI bool             importClientFakePlayer(std::string const& name);
LFPNDAPI ll::event::ListenerId subscribeEvent(
    std::string const& eventName,
    std::string const& callbackNamespace,
    std::string const& callbackName
);
LFPAPI bool unsubscribeEvent(ll::event::ListenerId id);

} // namespace rcapi


LFPNDAPI ll::event::ListenerId subscribeEventImpl(
    EventType                                                eventType,
    std::function<void(event::FakePlayerEventBase const&)>&& callback
);

#ifdef LFP_EXPORT

[[nodiscard]] bool ExportRemoteCallApis();
void               RemoveRemoteCallApis();

#endif

}; // namespace lfp::api