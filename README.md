# LeviFakePlayer

A Fake Player mod for LeviLamina

This mod modify generate fake player based on BDS builtin SimulatedPlayer with some modification

This mod aims to let BDS builtin SimulatedPlayer work like a normal player

## Install

`lip install ...`

## Usage

You can use this nod by command or exported api

### Command

The full command name of this mod is `levifakeplayer`, and default alias is `lfp`

- `/lfp help` - Show help for this command

- ` /lfp list [onlineOnly: false]` - List fake players, the online

- `/lfp create <name: string> [pos: x y z] [dim: Dimension]` - Create a fake player at world spawn point by default, unless pass pos param

- `/lfp login <name: string>` - Login a fake player by name

- `/lfp logout <name: string>` - Logout a fake player by name

- `/lfp remove <name: string>` - Remove a fake player by name

- `/lfp swap <name: string>` - Swap bag data with fake player by name

- `/lfp autologin <name: string>` - Switch fake player's auto login config by name

### LeviFakePlayer API

#### RemoteCallAPI

NameSpace: `LeviFakePlayerAPI`, exported apis:

> `SimulatedPlayer` is alias of `Player`, `ListenerId` is alias of `number`, and `FakePlayerInfoType` now is json `string` of `FakePlayerInfo`, and may be changed to `FakePlayerInfo` object in future

##### Structures

- `FakePlayerInfo`: `{name: string, xuid: string, uuid: UuidString, online: boolean, autoLogin: boolean}`

##### Apis

- `getApiVersion`: `() => number`
- `getVersion`: `() => string`
- `getOnlineList`: `() => SimulatedPlayer[]`
- `getInfo`: `(name: string) => FakePlayerInfoType`
- `getAllInfo`: `() => FakePlayerInfoType[]`
- `list`: `() => string[]`
- `login`: `(name: string) => SimulatedPlayer`
- `logout`: `(name: string) => boolean`
- `create`: `(name: string) => boolean`
- `createWithData`: `(name: string, data: NbtCompound) => boolean`
- `createAt`: `(name: string, position: IntPos) => SimulatedPlayer`
- `remove`: `(name: string) => boolean`
- `setAutoLogin`: `(name: string, autoLogin: boolean) => boolean`
- `importClientFakePlayer`: `(name: string) => boolean`
- `subscribeEvent`: `(eventName: EventName, callbackNamespace: string, callbackName: string) => ListenerId`
- `unsubscribeEvent`: `(listenerId: ListenerId) => boolean`

##### Event and callback params

- `create`: `[name: string, info: FakePlayerInfoType]`
- `remove`: `[name: string, info: FakePlayerInfoType]`
- `login`: `[name: string, info: FakePlayerInfoType, player: SimulatedPlayer]`
- `logout`: `[name: string, info: FakePlayerInfoType]`
- `update`: `[name: string, info: FakePlayerInfoType]`

> You can directly use api wrapper provided by this project

#### Native Api

>

## Build from source

1. Clone project: `git clone ...`

2. Run `xmake repo -u` in the root of the repository.

3. Run `xmake f -m release --test=n` to config release mode

4. Run `xmake` to build the mod.

Now the build is complete at `bin/`.

## Contributing

### Test

1. `xmake f -m release --test=y`

2. `xmake -y`

## License

LGPL-3.0-or-later
