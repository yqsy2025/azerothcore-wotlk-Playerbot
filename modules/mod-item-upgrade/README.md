# mod-item-upgrade

AzerothCore WotLK module companion for the existing Eluna/ALE equipment-upgrade UI.

## What this module fixes

1. Fixes integer division in `increase_ratio` calculations.
2. Keeps the upgrade calculation virtual; `item_template` is never modified.
3. Preserves the existing `OnAfterApplyItemMods` behavior so equipped items re-apply upgraded stats.
4. Preserves AzerothCore/WotLK resilience handling: resilience contributes to the three crit-taken rating channels.
5. Keeps the existing Titan's Grip enchant-skill behavior for spell 97001.

## Important

The supplied Lua script remains the UI/configuration layer. The C++ module does not replace NPC 190012 or your `acore_modules.item_upgrade_setting` configuration.

The resource-transaction fix and real success-rate roll are included in `lua/item_upgrade_patched.lua`; install that Lua file through your existing Eluna/ALE Lua loading mechanism.

## Install

Copy this directory to:

    <AzerothCore>/modules/mod-item-upgrade

Then re-run CMake and build the worldserver. AzerothCore's current module system auto-collects module sources and supports static/dynamic module builds. See the official skeleton module and modules CMake documentation.

## Lua

Replace your current item-upgrade Lua file with `lua/item_upgrade_patched.lua` only after backing up the original.

## Database

Run `data/sql/characters/2026_09_26_0001_mod_item_upgrade.sql` against the character database if `_item_upgrade` does not already exist.

Do NOT recreate an existing `_item_upgrade` table just because the SQL file is included; compare your existing schema first.
