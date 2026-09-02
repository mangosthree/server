#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# World of Warcraft, and all World of Warcraft or Warcraft art, images,
# and lore are copyrighted by Blizzard Entertainment, Inc.

"""Generate the two custody-pinned Cata build-15595 Warden profiles."""

from __future__ import annotations

import argparse
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Profile:
    architecture: str
    architecture_enum: str
    abi_enum: str
    provenance: str
    provenance_enum: str
    mode: str
    mode_enum: str
    assurance: str
    assurance_enum: str
    symbol: str
    include_guard: str
    container_length: int
    container_sha256: str
    key_sha256: str
    rekey_seed: str
    rekey_response: str
    client_to_server: str
    server_to_client: str
    mpq_enabled: bool
    provenance_note: str


PROFILES = {
    "x86": Profile(
        architecture="x86",
        architecture_enum="X86",
        abi_enum="Cata15595X86",
        provenance="build-matched-public",
        provenance_enum="BuildMatchedPublic",
        mode="full",
        mode_enum="Full",
        assurance="static-verified",
        assurance_enum="StaticVerified",
        symbol="WardenModuleWin15595X86",
        include_guard="MANGOS_WARDEN_MODULE_WIN_15595_X86_DATA_H",
        container_length=18_439,
        container_sha256=(
            "7ad7870d064c5a2bc8e55b00c23239b6e964c622c298beb99187fc6f163df4cd"
        ),
        key_sha256=(
            "b6b3b5c18100b49149e286b449ddc6b6415051fd8593dcee0e94c177caa23c87"
        ),
        rekey_seed="49f95776e6ddf99d9de91d75cc93e955",
        rekey_response="71be54fdf23061892d6eea2fb79119b9f7e05084",
        client_to_server="8ab07213fcff7bacb77b4804d239445c",
        server_to_client="6aea6e524748f22d122b27d96622d765",
        mpq_enabled=True,
        provenance_note=(
            "Takenbacon/molten-cata-archive commit "
            "81441e843862a9d26b18dec7cf85cb8da1e88f07"
        ),
    ),
    "x64": Profile(
        architecture="x64",
        architecture_enum="X64",
        abi_enum="Cata15595X64",
        provenance="signed-cross-build",
        provenance_enum="SignedCrossBuild",
        mode="full",
        mode_enum="Full",
        assurance="exact-client-lab-validated",
        assurance_enum="ExactClientLabValidated",
        symbol="WardenModuleWin15595X64",
        include_guard="MANGOS_WARDEN_MODULE_WIN_15595_X64_DATA_H",
        container_length=24_405,
        container_sha256=(
            "3ead4470f0f4b6d4e5f620153f138993ce76821ad55c08866b600f54a8462248"
        ),
        key_sha256=(
            "d752fab4ed8d1c18680e6e907f5feaeeca4b4492ebbce8e1ff414fb0498a64f3"
        ),
        rekey_seed="8db6e0c5865a1fdb810f26db773f681f",
        rekey_response="57790e891c05e7ceb34e6754daf39e8197ff5cec",
        client_to_server="558017aaed7fffab273cb00abf517795",
        server_to_client="1b12c1eab47a79a32b3f8f7b3c985912",
        mpq_enabled=True,
        provenance_note=(
            "Warpten/TrinityCore commit "
            "31e402dc95fc91168399dd08f00f6bed9281aff7; lab candidate only"
        ),
    ),
}


LICENSE = """/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */
"""


def bytes_initializer(data: bytes, indent: str = "        ") -> str:
    rows = []
    for offset in range(0, len(data), 16):
        values = ", ".join(f"0x{value:02X}" for value in data[offset : offset + 16])
        rows.append(f"{indent}{values}")
    return ",\n".join(rows)


def fixed_initializer(hex_value: str) -> str:
    return bytes_initializer(bytes.fromhex(hex_value), "            ")


def generate_header(profile: Profile) -> str:
    return f"""{LICENSE}
#ifndef {profile.include_guard}
#define {profile.include_guard}

#include "WardenModuleCatalog.h"

namespace warden
{{
/** Returns the immutable custody-pinned {profile.architecture} build-15595 profile. */
ModuleProfile const& Get{profile.symbol}Profile();
}}

#endif
"""


def generate_source(profile: Profile, data: bytes, key: bytes) -> str:
    code_prefix = f"Cata15595{profile.architecture.upper()}"
    mpq_code = f"warden::{code_prefix}MpqCode" if profile.mpq_enabled else "0"
    return f"""{LICENSE}
/*
 * Generated by extra/warden/generate_cata_warden_module_resource.py.
 * Build: 15595; architecture: {profile.architecture}; provenance:
 * {profile.provenance}; mode: {profile.mode}; assurance: {profile.assurance}.
 * Container SHA-256: {profile.container_sha256.upper()}.
 * Evidence source: {profile.provenance_note}.
 * Regenerate with the custody-pinned container/key and the matching explicit
 * --architecture/--provenance/--mode/--assurance/--symbol arguments.
 */

#include "{profile.symbol}Data.h"

#include <cstddef>

namespace
{{
uint8 const EncryptedContainer[{profile.container_length}] =
{{
{bytes_initializer(data)}
}};

warden::ModuleProfile BuildProfile()
{{
    warden::ModuleProfile profile;
    profile.key = {{15595, warden::WardenArchitecture::{profile.architecture_enum}}};
    profile.abi = warden::ModuleAbi::{profile.abi_enum};
    profile.provenance = warden::ModuleProvenance::{profile.provenance_enum};
    profile.operatingMode = warden::ModuleOperatingMode::{profile.mode_enum};
    profile.assurance = warden::ModuleAssurance::{profile.assurance_enum};
    profile.checkCodes = {{
        warden::{code_prefix}TimingCode,
        warden::{code_prefix}LuaCode,
        {mpq_code},
        warden::{code_prefix}MemoryCode}};
    profile.rekey.seed = {{{{
{fixed_initializer(profile.rekey_seed)}
    }}}};
    profile.rekey.expectedResponse = {{{{
{fixed_initializer(profile.rekey_response)}
    }}}};
    profile.rekey.clientToServer = {{{{
{fixed_initializer(profile.client_to_server)}
    }}}};
    profile.rekey.serverToClient = {{{{
{fixed_initializer(profile.server_to_client)}
    }}}};
    profile.moduleId = {{{{
{fixed_initializer(profile.container_sha256)}
    }}}};
    profile.moduleKey = {{{{
{bytes_initializer(key, "            ")}
    }}}};
    profile.declaredSize = {profile.container_length};
    profile.container.assign(EncryptedContainer,
        EncryptedContainer + sizeof(EncryptedContainer));
    return profile;
}}
}}

namespace warden
{{
ModuleProfile const& Get{profile.symbol}Profile()
{{
    static ModuleProfile const profile = BuildProfile();
    return profile;
}}
}}
"""


def write_if_changed(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.write_text(content, encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--architecture", choices=sorted(PROFILES), required=True)
    parser.add_argument("--provenance", required=True)
    parser.add_argument("--mode", required=True)
    parser.add_argument("--assurance", required=True)
    parser.add_argument("--build", type=int, required=True)
    parser.add_argument("--container", required=True, type=Path)
    parser.add_argument("--key", required=True, type=Path)
    parser.add_argument("--header", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--symbol", required=True)
    args = parser.parse_args()

    profile = PROFILES[args.architecture]
    supplied_metadata = (
        args.build,
        args.provenance,
        args.mode,
        args.assurance,
        args.symbol,
    )
    expected_metadata = (
        15595,
        profile.provenance,
        profile.mode,
        profile.assurance,
        profile.symbol,
    )
    if supplied_metadata != expected_metadata:
        raise SystemExit("profile metadata does not match the custody-pinned target")
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.symbol):
        raise SystemExit("symbol is not a valid C++ identifier")

    data = args.container.read_bytes()
    key = args.key.read_bytes()
    if len(data) != profile.container_length:
        raise SystemExit(
            f"container length {len(data)} != {profile.container_length}"
        )
    if hashlib.sha256(data).hexdigest() != profile.container_sha256:
        raise SystemExit("container SHA-256 does not match the custody manifest")
    if len(key) != 16:
        raise SystemExit(f"module key length {len(key)} != 16")
    if hashlib.sha256(key).hexdigest() != profile.key_sha256:
        raise SystemExit("module key SHA-256 does not match the custody manifest")

    write_if_changed(args.header, generate_header(profile))
    write_if_changed(args.source, generate_source(profile, data, key))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
