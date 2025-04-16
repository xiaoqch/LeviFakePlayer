import {
  type FakePlayerInfo,
  type SimulatedPlayer,
  LeviFakePlayerAPI as LFPAPI,
} from "plugins/lfp-ts-example/api/lfp-api.js";

ll.registerPlugin(
  "lfp-ts-example",
  "Example Typescript Plugin for LeviFakePlayer API",
  [0, 0, 1],
  { license: "CC0-1.0" }
);

class LeviFakePlayer {
  private _player?: SimulatedPlayer;
  constructor(private info: FakePlayerInfo) {}
  get name() {
    return this.info.name;
  }
  get xuid() {
    return this.info.xuid;
  }
  get uuid() {
    return this.info.uuid;
  }
  get online() {
    return this.info.online;
  }
  get autoLogin() {
    return this.info.autoLogin;
  }
  set autoLogin(val: boolean) {
    if (LFPAPI.setAutoLogin(this.name, val)) this.info.autoLogin = true;
    if (this.autoLogin) this.login();
  }
  get player() {
    this._player ??= mc.getPlayer(this.uuid);
    return this._player;
  }
  set player(val: SimulatedPlayer | undefined) {
    this._player = val;
  }
  private _update(info: FakePlayerInfo, player?: SimulatedPlayer) {
    if (info.uuid && info.uuid != this.uuid)
      throw new Error("info uuid not match");
    this.info.autoLogin = info.autoLogin ?? this.info.autoLogin;
    this.info.online = info.online ?? this.info.online;
    this.info.skinId = info.skinId ?? this.info.skinId;
    if (this.info.online) {
      this.player = player ?? this.player;
    } else {
      this.player = undefined;
    }
  }
  update(info?: FakePlayerInfo, player?: SimulatedPlayer) {
    info ??= LFPAPI.getInfo(this.name);
    if (!info)
      throw new Error(`Failed to get fake player info for "${this.name}"`);
    return this._update(info, player);
  }
  login() {
    return LFPAPI.login(this.name);
  }
  logout() {
    return LFPAPI.logout(this.name);
  }
}

const LeviFakePlayerManager = new (class {
  private fpList: { [name: string]: LeviFakePlayer };
  constructor() {
    this.fpList = Object.fromEntries(
      Object.values(LFPAPI.getAllInfo()).map((info) => [
        info.name,
        new LeviFakePlayer(info),
      ])
    );
    LFPAPI.subscribeEvent("login", (name, info, player) => {
      const fp = (this.fpList[name] ??= new LeviFakePlayer(info));
      fp.update(info, player);
    });
    LFPAPI.subscribeEvent("logout", (name, info) => {
      const fp = (this.fpList[name] ??= new LeviFakePlayer(info));
      fp.update(info);
    });
    LFPAPI.subscribeEvent("create", (name, info) => {
      const fp = (this.fpList[name] ??= new LeviFakePlayer(info));
      fp.update(info);
    });
    LFPAPI.subscribeEvent("remove", (name, info) => {
      delete this.fpList[name];
    });
    LFPAPI.subscribeEvent("update", (name, info) => {
      const fp = (this.fpList[name] ??= new LeviFakePlayer(info));
      fp.update(info);
    });
  }
  create(name: string): boolean;
  create(name: string, pos: IntPos | FloatPos): boolean;
  create(name: string, data: NbtCompound): boolean;
  create(name: string, posOrData?: any) {
    const info = LFPAPI.create(name, posOrData);
    // if (info)
    //     this.fpList[info.name] = new LeviFakePlayer(info)
    return info !== undefined;
  }
  remove(...args: Parameters<typeof LFPAPI.remove>) {
    if (!LFPAPI.remove(...args)) return false;
    // delete this.fpList[args[0]]
    return true;
  }
  login(...args: Parameters<typeof LFPAPI.login>) {
    return LFPAPI.login(...args);
  }
  logout(...args: Parameters<typeof LFPAPI.logout>) {
    return LFPAPI.logout(...args);
  }
})();

// ...
