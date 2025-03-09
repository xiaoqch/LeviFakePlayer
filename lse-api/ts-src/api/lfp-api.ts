// A way to reference lse lib, this line will be remove in target and declare file by tsc
import type {} from "./lib";
// cSpell: words: dimid
export const LEVIFAKEPLAYER_API_NAMESPACE = "LeviFakePlayerAPI";
const SCRIPT_API_VERSION = 1;

const INFO_OBJECT_SUPPORTED = false;
type INFO_OBJECT_SUPPORTED = typeof INFO_OBJECT_SUPPORTED;

type FakePlayerInfoType = INFO_OBJECT_SUPPORTED extends true
  ? FakePlayerInfo
  : string;

export type ListenerId = number;
export type UuidString = string;
export type SimulatedPlayer = Player;
export type RemoteCallCallbackFn = (
  name: string,
  info: FakePlayerInfoType,
  player?: SimulatedPlayer
) => void;

export enum AllRawApi {
  getApiVersion = "getApiVersion",
  getVersion = "getVersion",
  getOnlineList = "getOnlineList",
  getInfo = "getInfo",
  getAllInfo = "getAllInfo",
  list = "list",
  login = "login",
  logout = "logout",
  create = "create",
  createWithData = "createWithData",
  createAt = "createAt",
  setAutoLogin = "setAutoLogin",
  remove = "remove",
  importClientFakePlayer = "importClientFakePlayer",
  subscribeEvent = "subscribeEvent",
  unsubscribeEvent = "unsubscribeEvent",
}

export interface LeviFakePlayerEventMap {
  create: [name: string, info: FakePlayerInfo];
  remove: [name: string, info: FakePlayerInfo];
  login: [name: string, info: FakePlayerInfo, player: SimulatedPlayer];
  logout: [name: string, info: FakePlayerInfo];
  update: [name: string, info: FakePlayerInfo];
}

type EventName = keyof LeviFakePlayerEventMap;

// name: [ret,[...args]]
interface LeviFakePlayerRawApiMap {
  [AllRawApi.getApiVersion]: [apiVersion: number, []];
  [AllRawApi.getVersion]: [version: string, []];
  [AllRawApi.getOnlineList]: [playerList: SimulatedPlayer[], []];
  [AllRawApi.getInfo]: [info: FakePlayerInfoType, [name: string]];
  [AllRawApi.getAllInfo]: [infoList: FakePlayerInfoType[], []];
  [AllRawApi.list]: [nameList: string[], []];
  [AllRawApi.login]: [result: SimulatedPlayer, [name: string]];
  [AllRawApi.logout]: [result: boolean, [name: string]];
  [AllRawApi.create]: [result: boolean, [name: string]];
  [AllRawApi.createWithData]: [
    result: boolean,
    [name: string, data: NbtCompound]
  ];
  [AllRawApi.createAt]: [
    result: SimulatedPlayer,
    [name: string, position: IntPos]
  ];
  [AllRawApi.remove]: [result: boolean, [name: string]];
  [AllRawApi.setAutoLogin]: [
    result: boolean,
    [name: string, autoLogin: boolean]
  ];
  [AllRawApi.importClientFakePlayer]: [result: boolean, [name: string]];
  [AllRawApi.subscribeEvent]: [
    listenerId: ListenerId,
    [eventName: EventName, callbackNamespace: string, callbackName: string]
  ];
  [AllRawApi.unsubscribeEvent]: [result: boolean, [listenerId: ListenerId]];
}

if (false) {
  // Make sure all value of AllRawApis in LeviFakePlayerRawApiMap
  ({}) as keyof LeviFakePlayerRawApiMap satisfies AllRawApi;
  ({}) as AllRawApi satisfies keyof LeviFakePlayerRawApiMap;
}
export interface FakePlayerInfo {
  name: string;
  xuid: string;
  uuid: UuidString;
  skinId: ``;
  online: boolean;
  autoLogin: boolean;
  player: INFO_OBJECT_SUPPORTED extends true ? SimulatedPlayer : undefined;
}

type LeviFakePlayerRawAPIInterface = {
  [name in keyof LeviFakePlayerRawApiMap]?: (
    ...args: LeviFakePlayerRawApiMap[name][1]
  ) => LeviFakePlayerRawApiMap[name][0];
};

type RawApiFunc<T extends AllRawApi> = LeviFakePlayerRawAPIInterface[T];
type EventCallbackFunc<T extends keyof LeviFakePlayerEventMap> = (
  ...args: LeviFakePlayerEventMap[T]
) => void;

const CachedRawApi: LeviFakePlayerRawAPIInterface = {};
const EmptyApiFn = (...args: any[]) => {};

export function importRawApiWithCache<T extends keyof LeviFakePlayerRawApiMap>(
  name: T
): RawApiFunc<T> {
  return (CachedRawApi[name] ??= ll.hasExported(
    LEVIFAKEPLAYER_API_NAMESPACE,
    name
  )
    ? ll.imports(LEVIFAKEPLAYER_API_NAMESPACE, name) ?? EmptyApiFn
    : EmptyApiFn);
}

// API Compatibility Check
const getApiVersion = importRawApiWithCache(AllRawApi.getApiVersion);
if (!getApiVersion || getApiVersion === EmptyApiFn) {
  logger.error(`Can not import LeviFakePlayer API`);
} else if (getApiVersion() != SCRIPT_API_VERSION) {
  logger.warn(
    `The api version between this plugin<${SCRIPT_API_VERSION}> and LeviFakePlayer<${getApiVersion()}> not match`
  );
}

/**
 * Raw RemoteCall Api
 */
export const LeviFakePlayerRawAPI = new (class
  implements Required<LeviFakePlayerRawAPIInterface>
{
  constructor() {
    for (const name of Object.values(AllRawApi)) {
      const tmp: RawApiFunc<any> = (...args: any[]) => {
        const tmp = (this[name] = importRawApiWithCache(
          name
        ) as RawApiFunc<any>);
        return tmp(...args);
      };
      this[name] = tmp;
    }
  }
  getApiVersion!: () => number;
  getVersion!: () => string;
  getOnlineList!: () => SimulatedPlayer[];
  getInfo!: (name: string) => FakePlayerInfoType;
  getAllInfo!: () => FakePlayerInfoType[];
  list!: () => string[];
  login!: (name: string) => SimulatedPlayer;
  logout!: (name: string) => boolean;
  create!: (name: string) => boolean;
  createWithData!: (name: string, data: NbtCompound) => boolean;
  createAt!: (name: string, position: IntPos) => SimulatedPlayer;
  remove!: (name: string) => boolean;
  setAutoLogin!: (name: string, autoLogin: boolean) => boolean;
  importClientFakePlayer!: (name: string) => boolean;
  subscribeEvent!: (
    eventName: EventName,
    callbackNamespace: string,
    callbackName: string
  ) => ListenerId;
  unsubscribeEvent!: (listenerId: ListenerId) => boolean;
})();

function parseFakePlayerInfo(info: FakePlayerInfoType): FakePlayerInfo {
  if (typeof info == "string") return JSON.parse(info);
  return info;
}

/**
 * Friendly wrapper for raw api
 */
export namespace LeviFakePlayerAPI {
  export function getVersion(): string {
    return LeviFakePlayerRawAPI.getVersion();
  }
  export function getOnlineList(): SimulatedPlayer[] {
    return LeviFakePlayerRawAPI.getOnlineList();
  }
  export function getInfo(name: string): FakePlayerInfo | undefined {
    const infoRaw = LeviFakePlayerRawAPI.getInfo(name);
    if (undefined == infoRaw) return undefined;
    return parseFakePlayerInfo(infoRaw);
  }
  export function getAllInfo(): { [name: string]: FakePlayerInfo } {
    const infoRawList = LeviFakePlayerRawAPI.getAllInfo();
    if (undefined == infoRawList) return {};
    return Object.fromEntries(
      infoRawList.map(parseFakePlayerInfo).map((it) => [it.name, it])
    );
  }
  export function list(): string[] {
    return LeviFakePlayerRawAPI.list();
  }
  export function login(name: string): SimulatedPlayer {
    return LeviFakePlayerRawAPI.login(name);
  }
  export function logout(name: string): boolean {
    return LeviFakePlayerRawAPI.logout(name);
  }
  /**
   * FakePlayer created with pos will login immediately
   */
  export function create(
    name: string,
    posOrData?: IntPos | NbtCompound
  ): FakePlayerInfo | undefined {
    let result = false;
    if (posOrData == undefined) {
      result = LeviFakePlayerRawAPI.create(name);
    } else if (posOrData instanceof NbtCompound) {
      result = LeviFakePlayerRawAPI.createWithData(name, posOrData);
    } else {
      if (posOrData instanceof FloatPos)
        posOrData = new IntPos(
          posOrData.x,
          posOrData.y,
          posOrData.z,
          posOrData.dimid
        );
      if (posOrData! instanceof IntPos) return;
      result = undefined != LeviFakePlayerRawAPI.createAt(name, posOrData);
    }
    if (!result) return;
    return getInfo(name);
  }
  export function setAutoLogin(name: string, autoLogin: boolean) {
    return LeviFakePlayerRawAPI.setAutoLogin(name, autoLogin);
  }
  export function remove(name: string): boolean {
    return LeviFakePlayerRawAPI.remove(name);
  }
  export function importClientFakePlayer(name: string): boolean {
    return LeviFakePlayerRawAPI.importClientFakePlayer(name);
  }

  const RecordedListeners: {
    [id: ListenerId]: [
      namespace: string,
      name: string,
      handleFn: RemoteCallCallbackFn
    ];
  } = {};
  let listenerNamespace = "";
  let GlobalListenerId = 0;
  export function setNamespace(namespace: string) {
    listenerNamespace = namespace;
  }
  export function subscribeEvent<T extends EventName>(
    eventName: T,
    callback: EventCallbackFunc<T>
  ): ListenerId | undefined {
    const funcName = `${eventName}_${++GlobalListenerId}`;
    const tmpFunc: RemoteCallCallbackFn = (
      name: string,
      infoRaw: FakePlayerInfoType,
      player?: SimulatedPlayer
    ) => {
      const info = parseFakePlayerInfo(infoRaw);
      if (eventName == "login")
        (callback as EventCallbackFunc<"login">)(name, info, player!);
      else
        (callback as EventCallbackFunc<Exclude<EventName, "login">>)(
          name,
          info
        );
    };
    if (ll.exports(tmpFunc, listenerNamespace, funcName)) {
      const id = LeviFakePlayerRawAPI.subscribeEvent(
        eventName,
        listenerNamespace,
        funcName
      );
      // Todo
      if (id > 0) {
        RecordedListeners[id] = [listenerNamespace, funcName, tmpFunc];
        return id;
      }
    }
    return undefined;
  }
  export function unsubscribeEvent(listenerId: number): boolean {
    if (LeviFakePlayerRawAPI.unsubscribeEvent(listenerId)) {
      // delete exports?
      delete RecordedListeners[listenerId];
      return true;
    }
    return false;
  }
}

export default LeviFakePlayerAPI;
