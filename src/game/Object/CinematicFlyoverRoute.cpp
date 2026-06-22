/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "CinematicFlyoverRoute.h"
#include "SharedDefines.h"

// GENERATED FILE - do not hand-edit. Regenerate with:
//     python .ai_tools/bake_cinematic_routes_three.py --out <this file>
//
// World-space camera routes for the twelve Cataclysm starting-race intro
// cinematics plus the Death Knight class intro. Derived offline from Cataclysm
// 4.3.4a client data: CinematicCamera.dbc provides a world origin and facing
// per cinematic; the referenced camera-only M2 model (MD20 v272) provides the
// flyby as keyframed position/target tracks relative to the camera base vector.
// world = origin + RotZ(facing) * (base + key value).
// Orientation per keyframe faces the interpolated target track position.
// Routes are keyed by the CinematicSequences.dbc id actually sent in
// SMSG_TRIGGER_CINEMATIC (ChrRaces.dbc::CinematicSequence for races,
// ChrClasses.dbc::CinematicSequence = 165 for Death Knight); cameraId keeps
// the CinematicCamera.dbc row as an audit field.
// Only derived coordinates are stored here; no client assets are shipped.
// Each classic race route's final keyframe lands near that race's player spawn
// point (verified against playercreateinfo). The Goblin, Worgen and Death
// Knight intros are not spawn-aligned by design and are validated visually.

// Human (race 1, sequence 81, cinematic 142, map 0)
static const CinematicFlyoverKeyframe s_humanKeyframes[] = {
    { 0, -8922.41f, 543.34f, 144.58f, 0.7977f },
    { 2233, -8941.29f, 529.33f, 125.54f, 0.7750f },
    { 4969, -8965.94f, 507.07f, 111.10f, 0.8157f },
    { 8444, -8990.02f, 488.07f, 104.71f, 0.7667f },
    { 11708, -9018.12f, 468.53f, 104.25f, 0.7247f },
    { 14160, -9052.47f, 442.88f, 105.50f, 0.7221f },
    { 16067, -9095.03f, 408.58f, 106.42f, 0.6885f },
    { 18867, -9158.32f, 347.20f, 99.85f, 0.7048f },
    { 22100, -9197.33f, 235.04f, 76.24f, 1.4226f },
    { 27000, -9152.41f, 77.17f, 82.30f, 2.0292f },
    { 28300, -9130.91f, 43.19f, 97.58f, 2.1656f },
    { 30967, -9060.81f, -8.17f, 135.19f, 2.1207f },
    { 34333, -9014.38f, -50.38f, 116.33f, 3.7178f },
    { 35733, -8998.97f, -66.41f, 103.57f, 4.8658f },
    { 45233, -8917.21f, -127.87f, 83.43f, 5.1133f },
    { 47667, -8917.21f, -127.87f, 83.43f, 5.1133f },
};

static const CinematicFlyoverRoute s_humanRoute = {
    81,    // sequenceId (CinematicSequences.dbc ID, Human)
    142,   // cameraId (CinematicCamera.dbc ID)
    RACE_HUMAN,  // raceId
    0,  // classId
    0,     // mapId (Eastern Kingdoms)
    47667, // durationMs
    sizeof(s_humanKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_humanKeyframes
};

// Orc (race 2, sequence 21, cinematic 235, map 1)
static const CinematicFlyoverKeyframe s_orcKeyframes[] = {
    { 0, 1315.22f, -4412.86f, 97.77f, 6.0336f },
    { 3000, 1311.34f, -4411.89f, 67.05f, 6.0359f },
    { 10000, 1220.67f, -4404.04f, 54.95f, 0.2246f },
    { 15467, 1136.99f, -4432.25f, 55.82f, 0.0560f },
    { 23367, 974.36f, -4507.01f, 76.96f, 3.4160f },
    { 33467, 651.43f, -4622.98f, 93.91f, 3.1918f },
    { 40000, 410.20f, -4583.34f, 75.91f, 3.1404f },
    { 43267, 310.01f, -4520.51f, 137.29f, 2.9900f },
    { 46667, 162.39f, -4478.41f, 223.35f, 3.0650f },
    { 51200, -453.87f, -4426.58f, 137.67f, 2.1262f },
    { 54400, -625.99f, -4252.18f, 40.74f, 0.0954f },
    { 56667, -625.99f, -4252.18f, 40.74f, 0.0954f },
};

static const CinematicFlyoverRoute s_orcRoute = {
    21,    // sequenceId (CinematicSequences.dbc ID, Orc)
    235,   // cameraId (CinematicCamera.dbc ID)
    RACE_ORC,  // raceId
    0,  // classId
    1,     // mapId (Kalimdor)
    56667, // durationMs
    sizeof(s_orcKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_orcKeyframes
};

// Dwarf (race 3, sequence 41, cinematic 234, map 0)
static const CinematicFlyoverKeyframe s_dwarfKeyframes[] = {
    { 0, -4990.89f, -868.53f, 533.74f, 5.3654f },
    { 8067, -4998.96f, -862.24f, 518.87f, 5.4329f },
    { 16200, -5023.46f, -832.68f, 506.60f, 2.6482f },
    { 48567, -6161.70f, 128.94f, 521.84f, 1.2977f },
    { 53333, -6243.20f, 240.87f, 499.83f, 0.9558f },
    { 60700, -6235.89f, 328.10f, 384.86f, 0.3485f },
    { 62000, -6235.89f, 328.10f, 384.86f, 0.3485f },
};

static const CinematicFlyoverRoute s_dwarfRoute = {
    41,    // sequenceId (CinematicSequences.dbc ID, Dwarf)
    234,   // cameraId (CinematicCamera.dbc ID)
    RACE_DWARF,  // raceId
    0,  // classId
    0,     // mapId (Eastern Kingdoms)
    62000, // durationMs
    sizeof(s_dwarfKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_dwarfKeyframes
};

// Night Elf (race 4, sequence 61, cinematic 122, map 1)
static const CinematicFlyoverKeyframe s_nightelfKeyframes[] = {
    { 0, 10124.78f, 696.06f, 1376.68f, 0.0174f },
    { 3267, 10171.91f, 695.52f, 1373.12f, 0.0452f },
    { 4867, 10205.37f, 694.47f, 1371.77f, 0.1035f },
    { 7167, 10237.99f, 690.13f, 1370.75f, 0.3312f },
    { 9667, 10283.09f, 695.43f, 1374.60f, 0.4109f },
    { 13800, 10447.70f, 745.10f, 1405.92f, 0.7221f },
    { 15533, 10500.47f, 749.50f, 1410.34f, 1.5860f },
    { 17333, 10536.11f, 769.94f, 1405.73f, 2.0806f },
    { 19433, 10545.04f, 809.58f, 1393.81f, 2.5187f },
    { 22067, 10519.78f, 856.94f, 1376.42f, 3.1335f },
    { 24767, 10470.49f, 864.41f, 1366.97f, 3.3568f },
    { 27200, 10411.79f, 857.43f, 1360.77f, 3.4796f },
    { 35633, 10309.24f, 841.76f, 1333.18f, 5.3017f },
    { 39833, 10307.04f, 834.93f, 1328.80f, 5.6041f },
    { 42000, 10307.04f, 834.93f, 1328.80f, 5.6041f },
};

static const CinematicFlyoverRoute s_nightelfRoute = {
    61,    // sequenceId (CinematicSequences.dbc ID, Night Elf)
    122,   // cameraId (CinematicCamera.dbc ID)
    RACE_NIGHTELF,  // raceId
    0,  // classId
    1,     // mapId (Kalimdor)
    42000, // durationMs
    sizeof(s_nightelfKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_nightelfKeyframes
};

// Undead (race 5, sequence 2, cinematic 2, map 0)
static const CinematicFlyoverKeyframe s_undeadKeyframes[] = {
    { 0, 2162.21f, 1525.12f, 78.98f, 3.1378f },
    { 2973, 2153.60f, 1525.15f, 79.17f, 3.3886f },
    { 11297, 2090.55f, 1520.87f, 73.78f, 2.9674f },
    { 18318, 2040.42f, 1526.02f, 79.27f, 2.7145f },
    { 27785, 1977.07f, 1543.42f, 94.22f, 2.7002f },
    { 32061, 1950.31f, 1553.83f, 99.55f, 2.7786f },
    { 32084, 1950.17f, 1553.89f, 99.58f, 2.7791f },
    { 38899, 1910.07f, 1572.55f, 102.61f, 2.6928f },
    { 38922, 1909.86f, 1572.64f, 102.63f, 2.6924f },
    { 48092, 1786.82f, 1623.08f, 116.47f, 2.7482f },
    { 50928, 1752.71f, 1637.98f, 126.52f, 2.4677f },
    { 54953, 1717.94f, 1654.66f, 137.80f, 2.4429f },
    { 59536, 1690.87f, 1649.10f, 157.71f, 1.5144f },
    { 63753, 1669.19f, 1666.52f, 163.85f, 0.2978f },
    { 67733, 1704.45f, 1701.16f, 156.27f, 5.3823f },
    { 74600, 1698.10f, 1712.86f, 138.75f, 5.0464f },
    { 77333, 1698.10f, 1712.86f, 138.75f, 5.0464f },
};

static const CinematicFlyoverRoute s_undeadRoute = {
    2,     // sequenceId (CinematicSequences.dbc ID, Undead)
    2,     // cameraId (CinematicCamera.dbc ID)
    RACE_UNDEAD,  // raceId
    0,  // classId
    0,     // mapId (Eastern Kingdoms)
    77333, // durationMs
    sizeof(s_undeadKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_undeadKeyframes
};

// Tauren (race 6, sequence 141, cinematic 202, map 1)
static const CinematicFlyoverKeyframe s_taurenKeyframes[] = {
    { 0, -1175.49f, 116.55f, 251.14f, 4.4464f },
    { 4067, -1175.63f, 124.24f, 226.09f, 4.4464f },
    { 7100, -1175.95f, 142.66f, 194.68f, 4.2288f },
    { 10467, -1224.75f, 152.30f, 186.29f, 3.7219f },
    { 12367, -1304.56f, 112.96f, 150.89f, 3.5928f },
    { 13900, -1458.92f, 44.01f, 81.49f, 3.5349f },
    { 15233, -1596.23f, -23.05f, 53.28f, 3.5914f },
    { 19833, -1861.18f, -224.31f, 31.34f, 3.4821f },
    { 29567, -2238.87f, -349.46f, 50.34f, 3.2686f },
    { 36700, -2537.90f, -462.26f, 191.73f, 2.6810f },
    { 41533, -2744.34f, -398.82f, 103.71f, 2.4240f },
    { 42900, -2804.50f, -368.68f, 97.52f, 2.3613f },
    { 46333, -2894.65f, -320.07f, 85.26f, 1.7269f },
    { 48767, -2938.43f, -314.46f, 85.78f, 1.0692f },
    { 53733, -2964.22f, -294.36f, 78.64f, 0.5363f },
    { 67900, -2924.57f, -258.11f, 62.78f, 0.0165f },
    { 69000, -2924.57f, -258.11f, 62.78f, 0.0165f },
};

static const CinematicFlyoverRoute s_taurenRoute = {
    141,   // sequenceId (CinematicSequences.dbc ID, Tauren)
    202,   // cameraId (CinematicCamera.dbc ID)
    RACE_TAUREN,  // raceId
    0,  // classId
    1,     // mapId (Kalimdor)
    69000, // durationMs
    sizeof(s_taurenKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_taurenKeyframes
};

// Gnome (race 7, sequence 101, cinematic 279, map 0)
static const CinematicFlyoverKeyframe s_gnomeKeyframes[] = {
    { 0, -5307.64f, 344.20f, 631.44f, 6.1372f },
    { 7000, -5307.64f, 344.20f, 542.80f, 0.8150f },
    { 18467, -5187.62f, 459.49f, 407.53f, 1.5186f },
    { 27000, -5183.75f, 594.26f, 415.20f, 1.4847f },
    { 28033, -5181.48f, 623.33f, 407.90f, 1.4850f },
    { 28967, -5179.09f, 652.46f, 399.79f, 1.4813f },
    { 29567, -5177.70f, 669.39f, 392.50f, 1.4433f },
    { 30267, -5177.66f, 669.83f, 347.00f, 1.4883f },
    { 31200, -5177.51f, 671.51f, 295.19f, 1.4851f },
    { 32267, -5177.51f, 671.51f, 295.19f, 1.4722f },
    { 33767, -5169.91f, 746.30f, 291.50f, 0.9175f },
    { 34667, -5159.20f, 762.42f, 290.67f, 6.1660f },
    { 35333, -5155.75f, 762.12f, 290.67f, 6.2014f },
    { 36567, -5029.72f, 753.61f, 291.40f, 0.3083f },
    { 37233, -4991.27f, 754.53f, 301.35f, 1.3647f },
    { 38067, -4989.59f, 754.39f, 301.91f, 1.3647f },
    { 38633, -4987.11f, 761.47f, 297.87f, 1.4945f },
    { 41667, -4978.64f, 858.09f, 287.46f, 1.7740f },
    { 45500, -4976.22f, 877.00f, 277.59f, 3.0735f },
    { 48333, -4976.22f, 877.00f, 277.59f, 3.0735f },
};

static const CinematicFlyoverRoute s_gnomeRoute = {
    101,   // sequenceId (CinematicSequences.dbc ID, Gnome)
    279,   // cameraId (CinematicCamera.dbc ID)
    RACE_GNOME,  // raceId
    0,  // classId
    0,     // mapId (Eastern Kingdoms)
    48333, // durationMs
    sizeof(s_gnomeKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_gnomeKeyframes
};

// Troll (race 8, sequence 121, cinematic 251, map 1)
static const CinematicFlyoverKeyframe s_trollKeyframes[] = {
    { 0, -690.27f, -5631.63f, 78.16f, 1.9859f },
    { 1833, -690.27f, -5631.63f, 77.31f, 2.6588f },
    { 4633, -690.27f, -5631.63f, 36.42f, 3.3762f },
    { 5967, -690.27f, -5631.63f, 31.18f, 3.3058f },
    { 9433, -705.63f, -5634.56f, 29.60f, 3.1431f },
    { 14767, -937.61f, -5623.71f, 41.24f, 2.9598f },
    { 17433, -1034.66f, -5619.20f, 18.12f, 2.9026f },
    { 20933, -1106.93f, -5602.74f, 28.44f, 2.3186f },
    { 25400, -1289.64f, -5604.15f, 154.10f, 0.6413f },
    { 30667, -1438.16f, -5392.76f, 75.78f, 6.1541f },
    { 34233, -1294.65f, -5284.24f, 30.17f, 5.7124f },
    { 37733, -1208.32f, -5254.81f, 7.06f, 5.8336f },
    { 42533, -1177.19f, -5260.81f, 3.70f, 5.8063f },
};

static const CinematicFlyoverRoute s_trollRoute = {
    121,   // sequenceId (CinematicSequences.dbc ID, Troll)
    251,   // cameraId (CinematicCamera.dbc ID)
    RACE_TROLL,  // raceId
    0,  // classId
    1,     // mapId (Kalimdor)
    42533, // durationMs
    sizeof(s_trollKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_trollKeyframes
};

// Goblin (race 9, sequence 172, cinematic 288, map 648)
static const CinematicFlyoverKeyframe s_goblinKeyframes[] = {
    { 0, -8104.98f, 1313.31f, 127.17f, 2.0093f },
    { 2067, -8104.98f, 1313.31f, 127.17f, 2.0093f },
    { 8933, -8116.18f, 1337.21f, 69.82f, 1.8979f },
    { 14800, -8100.26f, 1457.78f, 21.72f, 2.1252f },
    { 20967, -8081.27f, 1604.72f, 22.27f, 2.4672f },
    { 26333, -8061.80f, 1747.27f, 61.00f, 2.9481f },
    { 40433, -8409.15f, 1644.79f, 151.56f, 3.7359f },
    { 43000, -8426.93f, 1552.86f, 173.80f, 3.4722f },
    { 45367, -8421.56f, 1449.28f, 171.20f, 3.0509f },
    { 48700, -8423.53f, 1326.32f, 125.28f, 1.6067f },
    { 55633, -8423.98f, 1355.19f, 106.92f, 1.5606f },
    { 56333, -8423.98f, 1355.19f, 106.92f, 1.5606f },
};

static const CinematicFlyoverRoute s_goblinRoute = {
    172,   // sequenceId (CinematicSequences.dbc ID, Goblin)
    288,   // cameraId (CinematicCamera.dbc ID)
    RACE_GOBLIN,  // raceId
    0,  // classId
    648,   // mapId (Lost Isles)
    56333, // durationMs
    sizeof(s_goblinKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_goblinKeyframes
};

// Blood Elf (race 10, sequence 162, cinematic 243, map 530)
static const CinematicFlyoverKeyframe s_bloodelfKeyframes[] = {
    { 0, 9300.95f, -6901.37f, 30.62f, 0.7450f },
    { 3533, 9352.59f, -6862.48f, 26.83f, 0.7558f },
    { 7767, 9419.36f, -6785.47f, 26.83f, 0.1210f },
    { 12633, 9505.82f, -6769.89f, 25.23f, 0.5048f },
    { 14167, 9542.67f, -6754.39f, 20.37f, 0.5886f },
    { 20767, 9699.69f, -6657.06f, 9.48f, 0.5405f },
    { 26733, 9854.86f, -6571.48f, 16.02f, 0.5397f },
    { 29900, 9927.06f, -6530.02f, 20.87f, 0.5721f },
    { 39000, 10065.99f, -6368.02f, 28.49f, 0.5399f },
    { 47167, 10146.48f, -6304.71f, 58.76f, 6.0880f },
    { 58000, 10277.19f, -6295.35f, 39.95f, 5.5820f },
    { 66667, 10346.35f, -6354.05f, 36.26f, 5.5202f },
};

static const CinematicFlyoverRoute s_bloodelfRoute = {
    162,   // sequenceId (CinematicSequences.dbc ID, Blood Elf)
    243,   // cameraId (CinematicCamera.dbc ID)
    RACE_BLOODELF,  // raceId
    0,  // classId
    530,   // mapId (Outland)
    66667, // durationMs
    sizeof(s_bloodelfKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_bloodelfKeyframes
};

// Draenei (race 11, sequence 163, cinematic 244, map 530)
static const CinematicFlyoverKeyframe s_draeneiKeyframes[] = {
    { 0, -4140.26f, -11930.49f, 153.79f, 0.8293f },
    { 12033, -4060.64f, -11951.33f, 6.06f, 0.9719f },
    { 15933, -4081.61f, -12036.68f, 3.76f, 1.1581f },
    { 19667, -4126.72f, -12170.31f, 13.99f, 1.0380f },
    { 23300, -4146.19f, -12239.92f, 46.23f, 4.7033f },
    { 25467, -4154.76f, -12308.64f, 75.90f, 4.6027f },
    { 29933, -4243.37f, -12783.76f, 39.31f, 4.5870f },
    { 32467, -4275.54f, -13107.11f, 98.02f, 4.8353f },
    { 34333, -4270.14f, -13290.12f, 124.69f, 5.0451f },
    { 35900, -4260.24f, -13413.37f, 145.70f, 5.2336f },
    { 38933, -4155.07f, -13557.45f, 150.98f, 4.8857f },
    { 41467, -4076.56f, -13685.00f, 153.24f, 4.8234f },
    { 42767, -4032.31f, -13738.80f, 156.60f, 3.8909f },
    { 45600, -4011.55f, -13848.05f, 163.11f, 2.1167f },
    { 53800, -3958.91f, -13936.08f, 102.92f, 2.1211f },
    { 56000, -3958.91f, -13936.08f, 102.92f, 2.1211f },
};

static const CinematicFlyoverRoute s_draeneiRoute = {
    163,   // sequenceId (CinematicSequences.dbc ID, Draenei)
    244,   // cameraId (CinematicCamera.dbc ID)
    RACE_DRAENEI,  // raceId
    0,  // classId
    530,   // mapId (Outland)
    56000, // durationMs
    sizeof(s_draeneiKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_draeneiKeyframes
};

// Worgen (race 22, sequence 170, cinematic 253, map 654)
static const CinematicFlyoverKeyframe s_worgenKeyframes[] = {
    { 0, -1654.80f, 1624.68f, 44.82f, 3.1069f },
    { 2900, -1636.73f, 1624.93f, 35.74f, 3.1966f },
    { 5867, -1578.35f, 1644.47f, 38.74f, 3.3416f },
    { 12367, -1438.87f, 1682.65f, 68.47f, 3.8446f },
    { 18467, -1277.61f, 1812.99f, 49.88f, 3.8032f },
    { 20967, -1232.77f, 1729.62f, 36.93f, 5.5132f },
    { 24000, -1064.63f, 1600.04f, 45.84f, 5.7852f },
    { 29600, -931.20f, 1559.98f, 63.21f, 6.1713f },
    { 31333, -942.37f, 1515.14f, 56.14f, 0.2959f },
    { 33467, -1027.39f, 1469.65f, 49.69f, 0.2689f },
    { 35000, -1079.25f, 1434.64f, 51.44f, 0.2525f },
    { 38433, -1152.69f, 1413.11f, 51.44f, 0.3131f },
    { 41100, -1259.29f, 1394.01f, 51.44f, 0.3131f },
    { 44500, -1388.36f, 1372.79f, 41.97f, 6.2763f },
    { 48033, -1475.14f, 1357.78f, 40.88f, 0.1113f },
    { 50900, -1539.40f, 1331.06f, 50.98f, 0.2737f },
    { 53533, -1539.07f, 1340.45f, 76.34f, 0.2225f },
    { 59267, -1543.42f, 1370.30f, 73.41f, 6.2451f },
    { 61833, -1543.36f, 1371.93f, 73.41f, 0.1379f },
    { 63767, -1538.18f, 1388.81f, 67.60f, 0.0351f },
    { 65800, -1508.61f, 1393.30f, 49.05f, 0.1544f },
    { 69533, -1457.11f, 1401.60f, 38.37f, 0.3144f },
    { 72000, -1457.11f, 1401.60f, 38.37f, 0.3144f },
};

static const CinematicFlyoverRoute s_worgenRoute = {
    170,   // sequenceId (CinematicSequences.dbc ID, Worgen)
    253,   // cameraId (CinematicCamera.dbc ID)
    RACE_WORGEN,  // raceId
    0,  // classId
    654,   // mapId (Gilneas)
    72000, // durationMs
    sizeof(s_worgenKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_worgenKeyframes
};

// Death Knight (class 6, sequence 165, cinematic 246, map 609)
static const CinematicFlyoverKeyframe s_deathknightKeyframes[] = {
    { 0, 2299.51f, -5343.05f, 97.37f, 5.2908f },
    { 8200, 2282.52f, -5317.85f, 93.90f, 5.3130f },
    { 11567, 2270.67f, -5312.70f, 96.90f, 5.4684f },
    { 15233, 2254.43f, -5310.43f, 106.69f, 5.7015f },
    { 18667, 2244.95f, -5312.51f, 106.69f, 5.9086f },
    { 27600, 2270.50f, -5311.00f, 106.69f, 5.2867f },
    { 28933, 2328.98f, -5681.23f, 387.52f, 0.5876f },
    { 29400, 2330.59f, -5680.14f, 387.52f, 0.5877f },
    { 29967, 2333.44f, -5678.22f, 387.52f, 0.5877f },
    { 33633, 2403.22f, -5631.15f, 387.52f, 0.5792f },
    { 34033, 2427.47f, -5614.79f, 429.37f, 0.5869f },
    { 35233, 2431.08f, -5615.28f, 469.40f, 0.4882f },
    { 36833, 2417.37f, -5626.62f, 482.30f, 0.3439f },
    { 37700, 2297.16f, -5697.56f, 489.09f, 0.5442f },
    { 40600, 2287.31f, -5694.06f, 469.11f, 0.4486f },
    { 48833, 2321.53f, -5686.30f, 434.28f, 0.5659f },
};

static const CinematicFlyoverRoute s_deathknightRoute = {
    165,   // sequenceId (CinematicSequences.dbc ID, Death Knight)
    246,   // cameraId (CinematicCamera.dbc ID)
    0,  // raceId
    CLASS_DEATH_KNIGHT,  // classId
    609,   // mapId (Ebon Hold)
    48833, // durationMs
    sizeof(s_deathknightKeyframes) / sizeof(CinematicFlyoverKeyframe),
    s_deathknightKeyframes
};

/// Returns the baked intro flyover route for a cinematic sequence,
/// or nullptr if none exists.
const CinematicFlyoverRoute* GetCinematicFlyoverRouteForSequence(uint32 sequenceId)
{
    switch (sequenceId)
    {
        case 81:    // Human
        {
            return &s_humanRoute;
        }
        case 21:    // Orc
        {
            return &s_orcRoute;
        }
        case 41:    // Dwarf
        {
            return &s_dwarfRoute;
        }
        case 61:    // Night Elf
        {
            return &s_nightelfRoute;
        }
        case 2:    // Undead
        {
            return &s_undeadRoute;
        }
        case 141:    // Tauren
        {
            return &s_taurenRoute;
        }
        case 101:    // Gnome
        {
            return &s_gnomeRoute;
        }
        case 121:    // Troll
        {
            return &s_trollRoute;
        }
        case 172:    // Goblin
        {
            return &s_goblinRoute;
        }
        case 162:    // Blood Elf
        {
            return &s_bloodelfRoute;
        }
        case 163:    // Draenei
        {
            return &s_draeneiRoute;
        }
        case 170:    // Worgen
        {
            return &s_worgenRoute;
        }
        case 165:    // Death Knight
        {
            return &s_deathknightRoute;
        }
        default:
        {
            return nullptr;
        }
    }
}
