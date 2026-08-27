/**
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

#include "TestHarness.h"

#include "WardenCryptoContext.h"
#include "WardenProtocol.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
uint8 Nibble(char value)
{
    if (value >= '0' && value <= '9')
        return uint8(value - '0');
    return uint8(std::toupper(static_cast<unsigned char>(value)) - 'A' + 10);
}

template <std::size_t Size>
std::array<uint8, Size> ArrayFromHex(char const* text)
{
    std::array<uint8, Size> result{};
    for (std::size_t index = 0; index < result.size(); ++index)
    {
        result[index] = uint8((Nibble(text[index * 2]) << 4) |
            Nibble(text[index * 2 + 1]));
    }
    return result;
}

warden::SessionKey LeadingZeroSessionKey()
{
    warden::SessionKey key{};
    for (uint8 index = 0; index < 39; ++index)
        key[index] = uint8(index + 1);
    return key;
}

struct ArchitectureVector
{
    char const* seed;
    char const* clientToServer;
    char const* digest;
    char const* serverToClient;
};

void CheckArchitectureVector(warden::WardenArchitecture architecture,
    ArchitectureVector const& vector)
{
    warden::Key16 const seed = ArrayFromHex<16>(vector.seed);
    std::optional<warden::ArchitectureProof> const proof =
        warden::DeriveArchitectureProof(architecture, seed);
    REQUIRE(proof.has_value());
    CHECK_HEX(proof->clientToServer.data(), proof->clientToServer.size(),
        vector.clientToServer);
    CHECK_HEX(proof->digest.data(), proof->digest.size(), vector.digest);
    CHECK_HEX(proof->serverToClient.data(), proof->serverToClient.size(),
        vector.serverToClient);
}
}

static_assert(sizeof(warden::ModuleId) == 32,
    "Cata Warden module identities are SHA-256 digests");
static_assert(!std::is_copy_constructible_v<warden::AdmissionData>);
static_assert(!std::is_copy_assignable_v<warden::AdmissionData>);

TEST(WardenCrypto_x86_architecture_transform_matches_client_oracle)
{
    ArchitectureVector const vectors[] =
    {
        {"00000000000000000000000000000000",
         "35f0f05add8e0b51908cf8a48aa3917d",
         "74b6e298ff16601f2d096b26779016386c9779ae",
         "849706abcc69294923c7bc0b1e86fb55"},
        {"000102030405060708090a0b0c0d0e0f",
         "374bd89775562bb29b0419c41c1767f8",
         "aef2003ce3e7968e485e5dff7c362b3a5372772e",
         "955479f70a2d5c4210d59274489dab43"},
        {"00112233445566778899aabbccddeeff",
         "66f10cb6e0aa013a1c379ee9426365cf",
         "5cccc0537b3621dbc5827afb17e13295c24920ba",
         "b8e8c9483550b551b47389c9966fd25a"},
        {"ffeeddccbbaa99887766554433221100",
         "8f4cf398fb2ecc99faa6c71f6f493d43",
         "40c6a33a8fb984d8ac251a01b8e8e857ad5f2142",
         "5edd58c8f69751d66671f0b9c2e09e80"}
    };

    for (ArchitectureVector const& vector : vectors)
        CheckArchitectureVector(warden::WardenArchitecture::X86, vector);
}

TEST(WardenCrypto_x64_architecture_transform_matches_client_oracle)
{
    ArchitectureVector const vectors[] =
    {
        {"00000000000000000000000000000000",
         "efbeaddebebafeca223f310500000000",
         "30891e88668839884e549993278ad9ef39984057",
         "000000007c75fd95447e620a00000000"},
        {"000102030405060708090a0b0c0d0e0f",
         "efbfafddc2bf04d22a483b109ce985a1",
         "d17833ca05ec90c62b88cb3c8465d38917831cd7",
         "00010203807a039d4c876c15ec1c5383"},
        {"00112233445566778899aabbccddeeff",
         "efaf8fed02106542aad8dbc05c83e4b9",
         "eaf36192bfd7848d48f66396babaf6b38df04672",
         "00112233c0ca630dcc170dc6aceb84b8"},
        {"ffeeddccbbaa99887766554433221100",
         "105070127965985399a58649978ce332",
         "c580ed09a513ad1ce863ac32ed2e496749d26e3a",
         "ffeeddcc3720971ebbe4b74eabb3cc52"}
    };

    for (ArchitectureVector const& vector : vectors)
        CheckArchitectureVector(warden::WardenArchitecture::X64, vector);
}

TEST(WardenCrypto_unclassified_architecture_has_no_proof)
{
    warden::Key16 const seed{};
    CHECK(!warden::DeriveArchitectureProof(
        warden::WardenArchitecture::Unclassified, seed).has_value());
    CHECK(!warden::DeriveArchitectureProof(
        static_cast<warden::WardenArchitecture>(0xFF), seed).has_value());
}

TEST(WardenCrypto_fixed_40_byte_initial_derivation_matches_client_streams)
{
    warden::WardenCryptoContext crypto;
    CHECK(crypto.Initialize(LeadingZeroSessionKey()));
    CHECK(crypto.IsInitialized());

    warden::Bytes outbound(37, 0);
    CHECK(crypto.TransformServerToClient(outbound));
    CHECK_HEX(outbound.data(), outbound.size(),
        "e7dd4693b8e1ac2c3db375c20c9f123fd03eb96465b54215a87cc50f44dcff17f7014a4bd2");

    warden::Bytes inbound(1, 0);
    CHECK(crypto.TransformClientToServer(inbound));
    CHECK_HEX(inbound.data(), inbound.size(), "88");
}

TEST(WardenCrypto_split_transforms_preserve_stream_continuity)
{
    warden::WardenCryptoContext whole;
    warden::WardenCryptoContext split;
    CHECK(whole.Initialize(LeadingZeroSessionKey()));
    CHECK(split.Initialize(LeadingZeroSessionKey()));

    warden::Bytes wholeBytes(64);
    for (std::size_t index = 0; index < wholeBytes.size(); ++index)
        wholeBytes[index] = uint8(index);
    warden::Bytes splitBytes = wholeBytes;

    CHECK(whole.TransformServerToClient(wholeBytes));
    warden::Bytes first(splitBytes.begin(), splitBytes.begin() + 7);
    warden::Bytes second(splitBytes.begin() + 7, splitBytes.end());
    CHECK(split.TransformServerToClient(first));
    CHECK(split.TransformServerToClient(second));
    std::copy(first.begin(), first.end(), splitBytes.begin());
    std::copy(second.begin(), second.end(), splitBytes.begin() + first.size());
    CHECK(std::equal(wholeBytes.begin(), wholeBytes.end(), splitBytes.begin()));
}

TEST(WardenCrypto_transaction_clone_rolls_back_discarded_inbound_stream)
{
    warden::WardenCryptoContext original;
    warden::WardenCryptoContext reference;
    CHECK(original.Initialize(LeadingZeroSessionKey()));
    CHECK(reference.Initialize(LeadingZeroSessionKey()));

    warden::WardenCryptoContext rejected = original.CloneForTransaction();
    warden::Bytes malformed = {0x01, 0x02, 0x03, 0x04};
    CHECK(rejected.TransformClientToServer(malformed));

    warden::Bytes actual = {0x10, 0x20, 0x30};
    warden::Bytes expected = actual;
    CHECK(original.TransformClientToServer(actual));
    CHECK(reference.TransformClientToServer(expected));
    CHECK(std::equal(actual.begin(), actual.end(), expected.begin()));
}

TEST(WardenCrypto_transaction_clone_commits_stream_by_move)
{
    warden::WardenCryptoContext original;
    warden::WardenCryptoContext reference;
    CHECK(original.Initialize(LeadingZeroSessionKey()));
    CHECK(reference.Initialize(LeadingZeroSessionKey()));

    warden::WardenCryptoContext accepted = original.CloneForTransaction();
    warden::Bytes first = {0x01, 0x02, 0x03, 0x04};
    warden::Bytes expectedFirst = first;
    CHECK(accepted.TransformServerToClient(first));
    CHECK(reference.TransformServerToClient(expectedFirst));
    CHECK(std::equal(first.begin(), first.end(), expectedFirst.begin()));
    original = std::move(accepted);

    warden::Bytes second = {0x10, 0x20, 0x30};
    warden::Bytes expectedSecond = second;
    CHECK(original.TransformServerToClient(second));
    CHECK(reference.TransformServerToClient(expectedSecond));
    CHECK(std::equal(second.begin(), second.end(), expectedSecond.begin()));
    CHECK(!accepted.IsInitialized());
}

TEST(WardenCrypto_directional_install_replaces_both_streams_atomically)
{
    warden::WardenCryptoContext crypto;
    CHECK(crypto.Initialize(LeadingZeroSessionKey()));

    warden::Key16 const clientToServer =
        ArrayFromHex<16>("374bd89775562bb29b0419c41c1767f8");
    warden::Key16 const serverToClient =
        ArrayFromHex<16>("955479f70a2d5c4210d59274489dab43");
    CHECK(crypto.InstallDirectionalKeys(clientToServer, serverToClient));

    warden::Bytes inbound(16, 0);
    warden::Bytes outbound(16, 0);
    CHECK(crypto.TransformClientToServer(inbound));
    CHECK(crypto.TransformServerToClient(outbound));
    CHECK_HEX(inbound.data(), inbound.size(),
        "e6679f117e275c0505cb4f8f014410b5");
    CHECK_HEX(outbound.data(), outbound.size(),
        "882c053bf3ef43a96c8c3810416aab0b");
}

TEST(WardenCrypto_module_rekey_exposes_client_to_server_check_xor_key)
{
    warden::WardenCryptoContext crypto;
    CHECK(crypto.Initialize(LeadingZeroSessionKey()));

    warden::Key16 const clientToServer =
        ArrayFromHex<16>("558017aaed7fffab273cb00abf517795");
    warden::Key16 const serverToClient =
        ArrayFromHex<16>("1b12c1eab47a79a32b3f8f7b3c985912");
    std::optional<warden::WardenCheckXorKey> const checkXorKey =
        crypto.InstallModuleDirectionalKeys(clientToServer, serverToClient);

    REQUIRE(checkXorKey.has_value());
    CHECK_EQ(checkXorKey->Value(), uint8(0x55));
}

TEST(WardenCrypto_rejects_stream_use_before_initialization)
{
    warden::WardenCryptoContext crypto;
    warden::Bytes bytes = {1, 2, 3};
    warden::Key16 key{};

    CHECK(!crypto.IsInitialized());
    CHECK(!crypto.TransformClientToServer(bytes));
    CHECK(!crypto.TransformServerToClient(bytes));
    CHECK(!crypto.InstallDirectionalKeys(key, key));
    CHECK_HEX(bytes.data(), bytes.size(), "010203");
}

TEST(WardenProtocol_admission_status_is_explicit)
{
    warden::AdmissionData admission;
    CHECK(!admission.IsAvailable());

    admission.status = warden::AdmissionStatus::MissingExactLocale;
    CHECK(!admission.IsAvailable());
    admission.status = warden::AdmissionStatus::UnsupportedExactLocale;
    CHECK(!admission.IsAvailable());
    admission.status = warden::AdmissionStatus::SessionKeyUnavailable;
    CHECK(!admission.IsAvailable());
    admission.status = warden::AdmissionStatus::Available;
    CHECK(admission.IsAvailable());
}

TEST(WardenProtocol_admission_move_transfers_and_cleanses_sole_key_owner)
{
    warden::AdmissionData source;
    source.build = 15595;
    source.clientOs = "Win";
    source.clientLocale = "enUS";
    source.status = warden::AdmissionStatus::Available;
    std::fill(source.sessionKey.begin(), source.sessionKey.end(), uint8(0xA5));

    warden::AdmissionData destination(std::move(source));
    CHECK_EQ(destination.build, uint32(15595));
    CHECK_STR(destination.clientOs, "Win");
    CHECK_STR(destination.clientLocale, "enUS");
    CHECK(destination.IsAvailable());
    CHECK(std::all_of(destination.sessionKey.begin(), destination.sessionKey.end(),
        [](uint8 value) { return value == 0xA5; }));

    CHECK_EQ(source.build, uint32(0));
    CHECK(source.clientOs.empty());
    CHECK(source.clientLocale.empty());
    CHECK(!source.IsAvailable());
    CHECK(std::all_of(source.sessionKey.begin(), source.sessionKey.end(),
        [](uint8 value) { return value == 0; }));

    destination.Clear();
    CHECK_EQ(destination.build, uint32(0));
    CHECK(destination.clientOs.empty());
    CHECK(destination.clientLocale.empty());
    CHECK(!destination.IsAvailable());
    CHECK(std::all_of(destination.sessionKey.begin(),
        destination.sessionKey.end(), [](uint8 value) { return value == 0; }));
}
