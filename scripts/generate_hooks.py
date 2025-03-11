import itertools
from pathlib import Path
from collections import defaultdict
import re

headers_dir = "..\\.xmake\\packages\\l\\levilamina\\main\\0c60ce77dbe34c6a92c2718bf0205f88\\include\\"

include_path_list = [
    # "mc\\world\\actor\\player\\Player.h",
    # "mc\\server\\ServerPlayer.h",
    # "mc\\server\\SimulatedPlayer.h",
    # "mc\\network\\LoopbackPacketSender.h",
    # "mc\\network\\ServerNetworkHandler.h",
    "mc\\server\\commands\\CommandRegistry.h",
    # "mc\\network\\PacketObserver.h",
    # "mc\\gametest\\MinecraftGameTestHelper.h",
]

disable_list = {
    "&ServerPlayer::$ctor",
    "&SimulatedPlayer::$ctor",
    "HOOK_Player_setSpawnPoint1",
    "HOOK_ServerNetworkHandler_$ctor1",
}
ignore_list = {
    "HOOK_LoopbackPacketSender_$sendToClients1",
    "HOOK_SimulatedPlayer_$aiStep1",  # 3980
    "HOOK_SimulatedPlayer_preAiStep1",  # 3944
    "HOOK_SimulatedPlayer__updateRidingComponents1",  # 3944
    "HOOK_ServerPlayer_$getEditorPlayer1",  # 655821
    "HOOK_ServerPlayer_$isActorRelevant1",  # 238055
    "HOOK_ServerPlayer_addActorToReplicationList1",  # 219451
    "HOOK_ServerPlayer_tryGetFromEntity1",  # 17814
    "HOOK_ServerPlayer_$getPlayerEventCoordinator1",  # 9144
    "HOOK_ServerPlayer_$checkMovementStats1",  # 6621
    "HOOK_ServerPlayer_$isLoading1",  # 5942
    "HOOK_ServerPlayer_preReplicationTick1",  # 5938
    "HOOK_ServerPlayer_postReplicationTick1",  # 5938
    "HOOK_ServerPlayer__updateNearbyActors1",  # 5938
    "HOOK_ServerPlayer_$normalTick1",  # 5938
    "HOOK_ServerPlayer_$isPlayerInitialized1",  # 5938
    "HOOK_ServerPlayer_$moveView1",  # 5923
    "HOOK_ServerPlayer__checkForLoadedTickingAreas1",  # 5923
    "HOOK_ServerPlayer_checkCheating1",  # 2677
    "HOOK_ServerPlayer_$_updateChunkPublisherView1",  # 1958
    "HOOK_ServerPlayer_$aiStep1",  # 1958
    "HOOK_ServerPlayer_$isValidTarget1",  # 848
    # "HOOK_ServerPlayer_$sendNetworkPacket1",# 528
    "HOOK_ServerPlayer_$getCurrentStructureFeature1",  # 211
    "HOOK_Player_$canInteractWithOtherEntitiesInGame1",  # 5704
    "HOOK_Player_$isSleeping1",  # 2173
    "HOOK_Player_$hasDiedBefore1",  # 1911
    "HOOK_Player_$getLastDeathDimension1",  # 953
    "HOOK_Player_$isFireImmune1",  # 828
    "HOOK_Player_$tickWorld1",  # 482
    "HOOK_Player_tickArmor1",  # 482
    "HOOK_Player_$getLastDeathPos1",  # 479
    "HOOK_Player_$normalTick1",  # 474
    "HOOK_Player_$aiStep1",  # 474
    "HOOK_Player_updateTrackedBosses1",  # 474
    "HOOK_Player__updateInteraction1",  # 474
    "HOOK_Player__handleCarriedItemInteractText1",  # 474
    "HOOK_Player_getCurrentActiveShield1",  # 474
    "HOOK_Player_updateInventoryTransactions1",  # 474
    "HOOK_Player_$moveView1",  # 428
    "HOOK_Player_$isBlocking1",  # 390
    "HOOK_Player_getPickupArea1",  # 383
    "HOOK_Player_$isInvulnerableTo1",  # 342
    "HOOK_Player_$getItemUseIntervalProgress1",  # 309
    "HOOK_Player_$getItemUseStartupProgress1",  # 309
    "HOOK_Player_$_hurt1",  # 256
    "HOOK_ServerNetworkHandler__buildSubChunkPacketData1",  # 2988
    "HOOK_ServerNetworkHandler_$_getServerPlayer1",  # 2617
    "HOOK_ServerNetworkHandler_$onTick1",  # 1255
    "HOOK_ServerNetworkHandler_$allowOutgoingPacket1",  # 1117
    "HOOK_ServerNetworkHandler_$allowIncomingPacketId1",  # 534
    "HOOK_ServerNetworkHandler_handle2",  # 269
    "HOOK_LoopbackPacketSender_$sendToClient1",  # 697
}


def gen_hook_cpp(include_path):
    file_path = Path(headers_dir) / include_path

    with open(file_path) as header:
        className = Path(file_path).stem
        content = header.read()
        hooks = [
            "#include <debug/generated/base.h>",
            f"#ifdef HOOK_{className}",
            f"#include \"{include_path.replace("\\","/")}\"",
        ]
        name_count = defaultdict(int)
        for declare in re.finditer(r"MCAPI (.*?;)", content, re.MULTILINE + re.DOTALL):
            # print(declare.group(1))
            api = re.sub(r" +", " ", declare.group(1).replace("\n", " "))
            print(api)
            if api.startswith(f"{className}("):
                continue
            if not (
                m := re.match(
                    r"^(?P<static>static )?(?P<rtn>.+[^,]) (?P<name>\$?[\w_]+)\((?P<params>.*) *\)( const)?;",
                    api,
                )
            ):
                continue
            groupdict=m.groupdict()
            print(groupdict)

            def iter_params(params: str):
                last_index = 0
                level = 0
                split_pos = 0
                params = params.strip()
                for m in re.finditer(r"[<>, ]", params):
                    m: re.Match
                    span = m.group(0)
                    if span == "<":
                        level += 1
                    elif span == ">":
                        level -= 1
                    elif span == " ":
                        split_pos = m.end()
                    else:
                        if level > 0:
                            continue
                        type_name = params[last_index : split_pos - 1]
                        name = params[split_pos : m.start()]
                        print(name, split_pos, m.pos, m, m.endpos, m.start())
                        if name == "const" or not (
                            name[-1].isalnum() or name[-1] == "_"
                        ):
                            type_name = f"{type_name} {name}"
                            name = None
                        yield {"name": name, "type": type_name}
                        last_index = m.end()
                        if params[last_index] == " ":
                            last_index += 1
                if params[last_index:] != "":
                    type_name = params[last_index : split_pos - 1]
                    name = params[split_pos:]
                    if name == "const" or not (name[-1].isalnum() or name[-1] == "_"):
                        type_name = f"{type_name} {name}"
                        name = None
                    yield {"name": name, "type": type_name}

            # params = map(
            #     lambda m: m.groupdict(),
            #     re.finditer(
            #         r"(?P<type>.+?( const)?&?)( (?P<name>[\w_]*))?((, )| ?$)",
            #         m["params"],
            #     ),
            # )
            params = iter_params(groupdict["params"])
            params = list(params)
            print(params)
            hook_type = (
                "LL_AUTO_TYPE_STATIC_HOOK"
                if groupdict["static"]
                else "LL_AUTO_TYPE_INSTANCE_HOOK"
            )
            hook_id = f"HOOK_{className}_{groupdict["name"]}"
            name_count[hook_id] += 1
            if name_count[hook_id] > 0:
                hook_id = f"{hook_id}{name_count[hook_id]}"
            hook_addr = f"&{className}::{groupdict["name"]}"
            hook_priority = "ll::memory::HookPriority::Normal"
            hook_rtn = groupdict["rtn"]
            alias_list = []
            if hook_rtn.find(",") >= 0:
                alias_list.append(f"using {hook_id}_rtn = {hook_rtn};")
                hook_rtn = f"{hook_id}_rtn"
            for index, param in enumerate(params):
                if param["name"] == None or len(param["name"]) == 0:
                    param["name"] = f"arg_{index}"

            def build_marco_param(index: int, param: dict[str, str]):
                type_name = param["type"]
                if type_name.find(",") >= 0:
                    alias_list.append(f"using {hook_id}_arg_{index} = {type_name};")
                    type_name = f"{hook_id}_arg_{index}"
                return f"{type_name} {param["name"]}"

            args_list = list(itertools.starmap(build_marco_param, enumerate(params)))
            origin_param_list = map(
                lambda x: f"std::forward<{x["type"]}>({x["name"]})", params
            )
            should_log = hook_addr not in ignore_list and hook_id not in ignore_list
            call_template = className if should_log else f"{className},false"
            call_name = "onHookCall"
            call_params = f'"{hook_id}"'
            if (
                groupdict["name"] == "$handle"
                and params[0]["type"] == "::NetworkIdentifier const&"
            ):
                call_name = "onHandlePacket"
                call_params = "source, packet"
                # call_template = f"{className},{params[1]["type"]}" if should_log else f"{className},false"

            hook = f"{"\n".join(alias_list)}\n{hook_type}(\n{hook_id},\n{hook_priority},\n{className},\n{hook_addr},\n{hook_rtn},\n{",\n".join(args_list)}\n){"{"}\n\
            {call_name}<{call_template}>({call_params});\n\
            return origin(\n{",\n".join(origin_param_list)}\n);\n\
            {"};"}\n"
            hook = re.sub(f"\n+", "\n", hook)
            if hook_addr in disable_list or hook_id in disable_list:
                hook = "#if false\n\n" + hook + "\n#endif"
            print(hook)
            hooks.append(hook)
        hooks.append(f"#endif // HOOK_{className}")
        hooks.append("")
        # if(m["name"]=="setHudVisibilityState" or m["name"] == "$ctor"):
        #     break
        with open(f"src/debug/generated/hook_{className}.cpp", "w") as cpp:
            cpp.write("\n\n".join(hooks))


def gen_cpp(include_path_list):
    for include_path in include_path_list:
        gen_hook_cpp(include_path)


def analyzeLog(log_path):
    with open(log_path, "r", encoding="utf-16le") as f:
        log = f.read()
    count: defaultdict[str, defaultdict[str, int]] = defaultdict(
        lambda: defaultdict(int)
    )

    for hook in re.finditer(r"HOOK_(.+?)_(.+?) called", log):
        count[hook.groups()[0]][hook.groups()[1]] += 1
    for class_name, record in count.items():
        print(class_name, record)
        print(class_name + ": \n")

        print(
            "\n".join(
                map(
                    lambda x: f'"HOOK_{class_name}_{x[0]}",# {x[1]}',
                    sorted(record.items(), key=lambda x: x[1], reverse=True),
                )
            )
        )


gen_cpp(include_path_list)
