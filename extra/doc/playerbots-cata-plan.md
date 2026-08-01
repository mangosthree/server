# Playerbots → Cata 4.3.4 Correctness Plan (Phase 1 deliverable)

Branch: `spike/playerbots-m1-to-m3` (r-log/server-three). Module: `src/modules/Bots/`.
Reference tree: `/tmp/m0` = mangoszero/server master, sparse `src/modules/Bots`, full history.
Sync baseline: `c32e222 2026-02-24 [PlayerBot] Code Sync between MangosZero and MangosOne` — our module derives from m1, which last synced m0 on that date. 101 m0 commits since.

Branch context (already done on this spike, 9 commits): module import + core wiring, group-invite
accept packet, `Player::Update` AI tick, reagent null-deref crash fix, area-stats hang stub,
`AddPrevQuests` recursion fix, factory level clamp, teleport-ack Cata packet + `.summon` re-add.
`server_install` currently runs a RelWithDebInfo build; build a clean Release before any deploy.

---

## (a) Confirmed Cata blockers — verified with anchors

All six suspected blockers CONFIRMED in-tree (agent-verified by reading code, 2026-07-10).

### A1. No Death Knight support — CONFIRMED
- `playerbot/AiFactory.cpp:22-51` — `createAiObjectContext` has 9 classes, no `CLASS_DEATH_KNIGHT`; DK falls through to bare `AiObjectContext` (`:52`).
- `playerbot/AiFactory.cpp:127-242` (combat) and `:271-337` (non-combat) — no DK case; DK gets generic-only strategies.
- No `strategy/deathknight/` directory (9 class dirs exist).
- `playerbot/RandomPlayerbotFactory.cpp:290` — random gen skips DK via magic `cls != 6`; `availableRaces` has no DK key, and a forced `CreateRandomBot(6)` underflows `size()-1` at `:127`.
- `.bot add` unguarded: `playerbot/PlayerbotMgr.cpp:202-211` (`ProcessBotCommand` add/login) and `OnBotLogin` (`PlayerbotMgr.cpp:110-165`).
- Short-term fix: refusal guard at add + `OnBotLogin` (`bot->getClass()` live there), keep the random skip. Long-term: Phase-2 backlog E1.

### A2. TBC-era race table — CONFIRMED
- `playerbot/RandomPlayerbotFactory.cpp:33-113` — strict TBC subset. Missing vs `mangos3.playercreateinfo` (queried):
  goblin + worgen everywhere applicable; tauren paladin; troll druid; undead hunter; human hunter;
  blood-elf warrior; dwarf shaman; gnome priest; tauren priest; night-elf/orc/dwarf mage; dwarf/troll warlock; undead rogue.
- Side-bug: duplicated `RACE_DRAENEI` push_backs (priest `:65/:70`, mage `:76/:81`, shaman `:94/:100`) double Draenei roll weight.
- Fix: replace the hardcoded ctor table with a startup scan of `playercreateinfo` (via `sObjectMgr.GetPlayerInfo(race, cls)` over the enums) — expansion-proof, kills the dup bug. `RACE_GOBLIN`/`RACE_WORGEN` exist in core.

### A3. Mana division-by-zero / wrong power — CONFIRMED
- `playerbot/strategy/values/StatsValues.cpp:88` — `GetPower(POWER_MANA)/GetMaxPower(POWER_MANA)` unguarded → NaN for hunters (max mana 0 in Cata), warriors, rogues, DKs; NaN→uint8 is UB.
- Unguarded consumers: `ConserveManaStrategy.cpp:16` (evaluates before the `hasMana` check at `:17`; strategy attached to ALL bots via `AiFactory.cpp:125/:339`), `HunterActions.cpp:15`, `ChooseTargetActions.h:36`, `MovementActions.h:65`, `NonCombatActions.h:26`.
- Guarded consumers (via "has mana"): `GenericTriggers.cpp:11/:16/:48/:56`, `ReadyCheckAction.cpp:63` — which has its own copy-paste bug (compares mana vs `mediumHealth`).
- Registration: `ValueContext.h:106` ("mana"), `:111` ("has mana"). Trigger wiring across 12 strategy files (druid/generic/hunter/mage/paladin/priest/shaman/warlock).
- Fix: guard in `ManaValue::Calculate` returning **100** when `GetMaxPower(POWER_MANA)==0` (keeps "low mana" quiet for mana-less classes, matches the no-target default `:86`); reorder `ConserveManaStrategy.cpp:16-17`; fix the `mediumHealth` typo. Power-type-aware display power can come later; 100-default is the safe semantic.

### A4. Hunter AI is mana-based — CONFIRMED
- Viper Sting (removed 4.0.1): `DpsHunterStrategy.cpp:18,:21-25,:78-79`; `HunterActions.cpp:13-16`; `HunterActions.h:48`; `HunterAiObjectContext.cpp:133,:171`; `HunterTriggers.cpp:14`.
- Aspect of the Viper (removed 4.0.1): `HunterActions.h:80-84`; `HunterAiObjectContext.cpp:70,:81` (registered twice!), `:93,:146,:183`; `HunterTriggers.h:26-30`; `HunterBuffStrategies.cpp:30-35` + `.h:18-23` — the whole "bmana" strategy is viper-only.
- Fix (two stages): stage 1 = delete dead wiring (~6 files, hunter dir only) so cast spam stops; stage 2 = focus rewrite (Q8): a `"focus"` value in StatsValues + ValueContext, focus triggers, Cata rotation (Steady/Cobra Shot generators, signature spenders). Note discrepancy to verify at Q8: agent flagged Aspect of the Wild as removed, but its name IS present in 4.3.4 Spell.dbc strings — verify castability, don't assume.

### A5. `InitAmmo()` — CONFIRMED (behavioral, not crash)
- `playerbot/PlayerbotFactory.cpp:1801-1858` (def), calls at `:54` (`Refresh`) and `:186` (`Randomize`); decl `PlayerbotFactory.h:162-164`.
- Core `Player::SetAmmo` is a commented-out no-op (`src/game/Object/PlayerItemValidation.cpp:1704-1725`) so no crash — but `mangos3.item_template` still has 17 projectile rows, so bots really receive 10×200 legacy arrows as dead bag weight.
- Also: `strategy/actions/EquipAction.cpp:40-42` (`INVTYPE_AMMO` branch). `ReadyCheckAction.cpp:97` ammo check already removed on this branch.
- Fix: delete body + both call sites + EquipAction branch (~70 lines, 2 files, module-only).

### A6. Warlock soul shards counted as items — CONFIRMED
- `strategy/warlock/WarlockActions.h:33` — `"item count","soul shard" < 2` is always true (shards are power in 4.x) → bots channel Drain Soul as finisher forever (wired `GenericWarlockStrategy.cpp:21,:61`, `TankWarlockStrategy.cpp:21`).
- `strategy/warlock/WarlockTriggers.h:49-56` (`:54`) — shard clause always false → fire/spellstone conjure triggers never fire.
- Core power: `POWER_SOUL_SHARDS = 7` (`src/game/Server/SharedDefines.h:173`). Module has no shard value class.
- Fix: `SoulShardsValue` reading `GetPower(POWER_SOUL_SHARDS)` + registration; flip `WarlockActions.h:33` to `shards < 3`; drop the shard clause in `WarlockTriggers.h:54`. Note: "create firestone"/"create spellstone" name strings are NOT in 4.3.4 Spell.dbc (see audit) — the conjure actions need Cata replacements or removal, not just re-gating.

### Adjacent verified facts (not blockers)
- **Talent init (Task C): PARTIALLY WORKS, illegal builds.** `PlayerbotFactory::InitTalents` (`PlayerbotFactory.cpp:378-392`, `:1603-1661`) learns talent spells via `learnSpell` + manual point charge — it never calls `Player::LearnTalent` (`src/game/Object/PlayerTalent.cpp:90-258`), so the 31-point primary-tree lock (`:207`, `REQ_PRIMARY_TREE_TALENTS=31`), row gate (`:201`) and `DependsOn` checks (`:144-164`) are bypassed and — critically — `m_talentsPrimaryTree` is never set (`:231-249`), so the bot gets **no mastery, no spec-defining spells, no armor specialization, wrong mana-regen role mask**. Talents do persist (explains the observed 22-talent restore). Fix is contained: rewrite `InitTalents(uint32)` to drive `bot->LearnTalent(talentId, rank)` row-by-row; core API is complete.
- **Glyphs (Task C): absent; core API complete.** No glyph init anywhere in module. Core has the full surface: `Player::SetGlyph/ApplyGlyph/GetGlyphSlot/InitGlyphsForLevel` via `GlyphMgr` (`GlyphMgr.cpp:32-79,:136`), `MAX_GLYPH_SLOT_INDEX 9` = 3 prime/3 major/3 minor, unlocks 25/50/75, TypeFlags matching mandatory, apply sequence per `SpellEffectObjectCombat.cpp:661-665`. Missing piece is only glyph *selection* data (class→glyph list).
- **Config loading (Task C): the "no loader wired" claim is FALSE.** `PlayerbotAIConfig::Initialize` (`PlayerbotAIConfig.cpp:122-259`) loads its own `aiplayerbot.conf` (`SYSCONFDIR` → `.` on Windows, cwd fallback), wired at `World.cpp:868`, and demonstrably works (boot log + effective overrides). Only caveat: cwd-sensitive on Windows; optional hardening, not a bug for the current layout.

---

## (b) Removed-spell audit — bot spell strings vs 4.3.4 Spell.dbc

Method: extracted every string from `NextAction("…")`, `creators["…"]`, `BEGIN_*SPELL_ACTION(…, "…")`,
`Cast*Action(ai, "…")` across `strategy/**` (786 distinct), checked each against the actual
`server_install/dbc/Spell.dbc` string block (73,253 records). 328 found; 458 not found, of which
most are engine tokens (strategy/value/trigger names) and ike3 composite action names
("X on party" — the base spell is what matters).

**Caveat:** string-presence is necessary but NOT sufficient — a name can survive in the DBC (as an
aura or NPC spell) while the castable player spell is gone. Live evidence: "judgement of light"
string exists, but level-85 paladin bots log `id=0` for it — the learnable spell is gone.
Implementation must verify learnability per class, not just DBC presence.

### Curated offender table

| Bot string | Class | Status (4.3.4) | Replacement / action |
|---|---|---|---|
| aspect of the viper | hunter | REMOVED (4.0.1) | delete; focus model (Q8) |
| viper sting | hunter | REMOVED | delete (A4) |
| pet not happy / hunters pet* mana-family triggers | hunter | mechanic removed | delete happiness triggers (branch already stubbed some) |
| abolish disease | priest | REMOVED | Cure Disease |
| divine spirit | priest | REMOVED | delete (stat folded into Fortitude era changes) |
| lesser heal | priest | REMOVED | Heal |
| abolish poison | druid | REMOVED | Remove Corruption (VERIFY name) |
| dire bear form | druid | REMOVED | Bear Form |
| mangle (bear) / mangle (cat) | druid | name gone (unified) | Mangle |
| swipe (bear) / swipe (cat) | druid | name gone (unified) | Swipe |
| feral charge - bear | druid | string mismatch | VERIFY exact 4.3.4 name ("Feral Charge (Bear)"?) |
| blessing of sanctuary | paladin | REMOVED | delete |
| blessing of wisdom (± on party) | paladin | REMOVED (folded into Might) | Blessing of Might |
| seal of light / seal of wisdom | paladin | REMOVED | delete |
| seal of vengeance | paladin | RENAMED | Seal of Truth |
| judgement of light / wisdom / justice | paladin | not castable (single Judgement) | Judgement |
| purify (± poison/disease variants) | paladin | REMOVED | Cleanse |
| fire/frost/shadow resistance aura | paladin | MERGED | Resistance Aura (VERIFY name); also fixes rfire/rfrost/rshadow strategies |
| divine protection on party | paladin | mechanics changed (self-only) | self-cast only |
| the art of war (code: "art of war") | paladin | string mismatch | "the art of war" (aura check) |
| cleansing totem | shaman | REMOVED | delete |
| grace of air totem | shaman | REMOVED (pre-Cata) | delete |
| lesser healing wave | shaman | RENAMED | Healing Surge |
| remove lesser curse | mage | RENAMED | Remove Curse |
| conjure water / conjure food | mage | MERGED (live-confirmed id=0) | Conjure Refreshment (see Bo #299 adapt) |
| create firestone / create spellstone | warlock | not in DBC strings | VERIFY; likely remove conjure actions (A6) |
| incinirate | warlock | TYPO | Incinerate |
| barskin | druid | TYPO (trigger token) | barkskin — verify usage path before rename |
| soul shard (as item) | warlock | mechanic → power | A6 |
| ammo / projectiles | hunter | mechanic removed | A5 |

Class-token strings not in DBC that are engine names, not spells (no action): aoe, arcane, bdps,
bmana, bear, caster*, cure, dps*, holy, melee*, nc, pull, tank, totems, seal, blessing, boost,
bspeed, bthreat, barmor, bhealth, rnature, shaman weapon, plus all "… on party/attacker/cc/enemy
healer/snare target" composites whose base spell exists.

Full machine output: scratchpad `spell_audit_out.txt` (regenerate with `spell_audit.py`).

---

## (c) Bo Zimmerman port-debt inventory

Baseline: everything ≤ 2026-02-24 sync **should** be present via m1; spot-checks show the sync was
good but not perfect — verify-by-diff each pre-window PR at port time (all are module-only micro-diffs):

| Pre-window PR | Date | Spot-check in our branch |
|---|---|---|
| #213 memory leaks (4 files, ~10 lines) | 2025-12-24 | verify at port |
| #214 taxi crash (CheckMountStateAction, +9) | 2025-12-24 | verify |
| #218 conf hot-reload min/max bots (+30, RandomPlayerbotMgr) | 2025-12-24 | **missing** (no marker) |
| #219 zone spawn by faction + area levels | 2025-12-24 | **present but disabled** — our `CalculateAreaCreatureStats` scan is commented out; our hang-fix stubbed around it. Port m0's working version properly. |
| #226 taxi bad-packet (RememberTaxiAction, +27) | 2026-01-17 | verify |
| #228 perpetual looting (MovementActions, +6) | 2026-01-18 | verify |
| #230 repeated-spell cancel (GenericSpellActions, +13) | 2026-01-18 | **missing** (no marker) |
| #236 follow survives teleport | 2026-02-08 | **PRESENT** (lastFollowState) — N/A |
| #239 boats fix | 2026-02-23 | verify; superseded/extended by #427 |

Since-sync inventory (101 commits). N/A outright: 8 style/line-ending sweeps, #428 (vanilla DBC
schemas), #433 (m0 AH IPC), #324 hunter pet feeding (happiness removed in Cata), #236 (present).
Everything else classified below — **PORTABLE** = mechanic-level, applies with path/API adaptation;
**ADAPTABLE** = needs Cata spell/mechanic substitution.

| Tier | PRs (m0 hash) | Verdict / notes |
|---|---|---|
| 1 Lifecycle/perf/stability | #213, #219 (bcf3dc2), #305 (dc5dba9), #346 (1b5cf08), #364 (a8c0dc9), #372 (8d40d25, r-log authored — exists in m0 not here), #230, #228, #218, #399 (f74bc2f group-disband crash), #365 (d9f2315 safer spawn), #347 (db27fd8) | PORTABLE. #219 must merge with our area-stats stub + hang fix. #372 port verbatim (author = us). |
| 2 Combat correctness | #268 (df72862 DBC range), #295 (435511c spec detection — MUST re-key to Cata primary tree, synergy with Q7), #409 (ca8135c threat/disrupt), #410 (d5057db cc target), #326 (4d3121b), #425 (5ddf3cb), #275 (f926c74 dead-zone), #398 (82b793e close range), #296 (c3635e4 reach), #317 (cb0bc36), #272 (ac4cfad aura checks) | PORTABLE except #295 (ADAPTABLE). #275/#398/#303 directly attack our observed hunter dead-zone problem. |
| 3 Movement | #294 (40cf9e1 rear arc), #270 (37120ec) + #316 (b0e0ba8) cautious, #370 (4548e97) + #323 (9c7fbd2) random-move config, #239/#427 (1f2d619 boats+pets), #214/#226/#304 (42acfa3)/#318 (d221eb0) taxi, e1405da dungeon-exit, #397 (a65cd2c goto), #276 (46f8d75) + #321 (a7ff752) jump, #303 (b5d14f3 hunter shooting pos), #400 (749a1d1 darnassus — verify Cata relevance) | PORTABLE |
| 4 Sustain | #269 (c4bb03f eat/drink SM), #278 (2ea78a4) + #381 (7e53666) food prefs, #282 (833b3e9 bandages), #378 (b149edf) + #379 (6cbdc58) swim/drown, #299 (0f6b039 mage conjure → **Conjure Refreshment**) | PORTABLE except #299 ADAPTABLE |
| 5 Class passes | #285 pal (a2ac466), #286 mage (cca9ef1), #287 druid (631b46b), #288 hunter (4038609), #289 priest (9ecadd5), #290 rogue (fd31c6c), #291 warr (c444fa6), #292 shaman (2b2a3ec), #293 lock (050a010), #368 (d26cfcb shadow), #369 (2e3c174 mage), #377 (e4c2d18 feral), #382 (22e7a76 dispel), #383 (e2bc663 mage curse name), #396 (beb3cdf lock stones), #405 (a8f4c7a shaman cure), #302 (6453389 wingclip), #320 (f59622a feign death), #279 (b91e33d petless), #329 (838f91d blessings) | ALL ADAPTABLE — port the *intent*, substitute Cata 4.3.4 rotations/names from the audit table. Never copy vanilla spell lists. |
| 6 Social/utility | #350 (2d1bf91 remote pull), d25c91b fish with master, #380 (b16ae90 die like master), #412 (3f135f9 guild charters), #407 (7e64a86 quest looting), #330 (caec228 meeting stones), #251 (a177769) + #284 (0916c45) whisper gating, #328 (a42c59d patrollers), #298 (4094e3e add all), #283 (ba6b4e8 trades), #307 (5d83bef), #306 (c3635e4 revive), #308 (92271c6 role strategies), #315 (0389864), #354 (12af499), #404 (30da0bb), #426 (180abf1 no cast emote), e86062d group-leave, #411 (ad3bc80 invite — verify vs our already-fixed invite) | PORTABLE |

Porting rule (Phase 2): read the actual m0 diff first (`git -C /tmp/m0 show <hash>`), re-express
against this branch; never paste code referencing m0-only core APIs; list Cata spell substitutions
in the commit body for ADAPTABLE items.

---

## (d) Ordered implementation queue

Every item: one commit, `[Bots] <summary>`, module-only unless stated, build green
(`-DPLAYERBOTS=ON`, RelWithDebInfo or Release; no new warnings in `src/modules/Bots`), then the
listed acceptance check. Client-free harness for most checks: `aiplayerbot.conf` override block
(RandomBotAutologin=1, MinLevel=MaxLevel=85, 1 bot) + log/DB inspection + PowerShell
`Get-Process mangosd | % CPU` spin check (procedure proven on this branch; cdb available for stacks).

**Tier 0 — Cata blockers (do first, order matters only where noted)**

| # | Item | Files | Acceptance | Blast |
|---|---|---|---|---|
| Q1 | Mana value guard (A3) + ConserveMana eval reorder + ReadyCheck mediumMana typo | StatsValues.cpp, ConserveManaStrategy.cpp, ReadyCheckAction.cpp | 85 warrior/rogue/hunter random bot fights ≥5 min: no NaN behavior, no drink-spam, "low mana" silent for mana-less classes | Low |
| Q2 | Ammo purge (A5) | PlayerbotFactory.{cpp,h}, EquipAction.cpp | fresh `.bot init`: zero `class=6` items in `character_inventory` | Low |
| Q3 | DK stop-gap guard (A1) + availableRaces underflow guard | PlayerbotMgr.cpp, RandomPlayerbotFactory.cpp | `.bot add <DK>` → clear refusal message, no crash; random gen never rolls class 6 | Low |
| Q4 | Race matrix from `playercreateinfo` (A2) | RandomPlayerbotFactory.cpp | fresh random pool contains goblin/worgen and Cata combos; zero rows outside `playercreateinfo`; Draenei weight normal | Low-Med |
| Q5 | Soul-shard power value + warlock gate flips (A6) | StatsValues or new value file, ValueContext.h, WarlockActions.h, WarlockTriggers.h | 85 warlock bot: Drain Soul only when shards<3; no perma-channel | Low |
| Q6 | Removed-spell sweep — one commit per class from audit table (b), incl. typos | strategy/<class>/* per class | per class: specced 85 bot on dummy shows no `id=0` rotation lookups (temp probe build) | Med (mechanical) |
| Q7 | `InitTalents` → `Player::LearnTalent` rewrite (legal builds, primary tree, mastery) | PlayerbotFactory.cpp:1603-1661 region | `.bot init=epic`: ≥31 pts in one tree before off-tree, `characters.talentTree` set, mastery spell known, re-init idempotent | Med |
| Q8 | Hunter focus rewrite (A4 stage 2; stage 1 deletions fold into Q6-hunter) | strategy/hunter/*, StatsValues.cpp, ValueContext.h | 85 hunter sustains ranged rotation on dummy: Steady/Cobra weaving, signature shots on CD, DPS > auto-only baseline | Med |
| Q9 | `InitGlyphs` (prime+major) | PlayerbotFactory.{cpp,h} + class→glyph table | init'd bot has type-matched glyphs in unlocked slots; persists relog (`character_glyphs`) | Med |
| Q10 | (optional) config cwd hardening | PlayerbotAIConfig.cpp | mangosd launched from foreign cwd still loads conf | Low |

**Tier 1-6 — Bo ports, in mission priority order** (inventory table (c) is the queue; within a
tier, order as listed). Special handling:
- #219: port m0's working area-stats implementation to REPLACE our commented scan + hang-era stub in `IsZoneSafeForBot`; keep our cycle-guard semantics. Acceptance: bots spawn only in faction-appropriate, level-banded zones; no world-thread spin (CPU check).
- #372: port verbatim (r-log authored); acceptance: tick budget respected under 50-bot load.
- #295: adapt spec detection to read `m_talentsPrimaryTree` (post-Q7 world) instead of point-counting.
- Class passes (#285-293 etc.): fold each into (or after) its Q6 class commit to avoid double-touching files.
- After each tier: smoke = server boots, random bots spawn, `.bot add` works for all classes (DK refuses cleanly), 5-man group invite/combat/loot run without crash.

**Tier E — Cata-native backlog (post-parity, schedule last)**
- E1: full DK AI (blood tank + frost/unholy DPS; rune + runic-power values).
- E2: resource values as first-class AiObjectContext values: holy power, eclipse, focus (Q8 seed), soul shards (Q5 seed), runes.
- E3: LFD integration — bots fill dungeon-finder roles.
- E4: prime-glyph selection quality pass (Q9 ships the mechanism).
- E5: level-85 gear stat weights (mastery/hit) in factory item scoring.
- E6: delete spell-rank-chain resolution in ChatHelper (Cata has no ranks).

---

*Phase 1 ends here. Awaiting approval before any implementation (Phase 2 rules per mission brief).*
