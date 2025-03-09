# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

- Added manager and storage supported for LeviFakePlayer
- Added Command `levifakeplayer`(default alias: `lfp`)
  - added: help subcommand
  - added: list subcommand
  - added: create subcommand
  - added: login subcommand
  - added: logout subcommand
  - added: remove subcommand
  - added: swap subcommand
  - added: autologin subcommand
- Added a experiment command `ticking` for debug(disabled by default)
- Added RemoteCall API supported for LegacyRemoteCall Plugin
  - added: api getApiVersion
  - added: api getVersion
  - added: api getOnlineList
  - added: api getInfo
  - added: api getAllInfo
  - added: api list
  - added: api create
  - added: api createAt
  - added: api createWithData
  - added: api login
  - added: api logout
  - added: api setAutoLogin
  - added: api remove
  - added: api importClientFakePlayer
  - added: api subscribeEvent
  - added: api unsubscribeEvent
- Added Registered Event for LeviLamina Event System
  - added: FakePlayerCreatedEvent
  - added: FakePlayerJoinEvent
  - added: FakePlayerUpdateEvent
  - added: FakePlayerLeaveEvent
  - added: FakePlayerRemoveEvent
- Added feature fix for SimulatedPlayer created by LeviFakePlayer, and optional by other plugin, exclude gametest framework(no test)
  - fix abnormal chunk load range
  - fix respawn(auto respawn)
  - fix change dimension chunk load
  - fix change dimension from the end(can not pass credit)
- Added ts/js api wrapper and ts plugin example file
  > see [lse-api\ts-src\api\lfp-api.ts](lse-api\ts-src\api\lfp-api.ts) and [lse-api\ts-src\lfp-api-example.ts](lse-api\ts-src\lfp-api-example.ts) for details

- Added some unstable native api export

**NOTICE: All Native export are unstable and may be change any time**
