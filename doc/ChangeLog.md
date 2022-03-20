MaNGOS Three Changelog
======================
This change log references the relevant changes (bug and security fixes) done
in recent versions.

0.22 (2021-01-01 to 2022-02-26) - "Answer the Volcano's Call"
------------------------------------------------------
* [Appveyor] Remove no-longer needed file    
* [Build] Add OpemSSL1.0.2j installers    
* [Build] Added MySQL 5.7 support    
* [Build] Attempt to make travis happy    
* [Build] Cmake fixes from Zero    
* [Build] Enhanced Build System    
* [Build] Force building SOAP and PlayerBots in testing    
* [Build] Improved Build System, based on the work of H0zen (#131)
* [Build] Updated EasyBuild to v1.5c    
* [Core/Soap] Fix memory leak on shutdown (#159)    
* [DB] Set min db levels    
* [DbDocs] The Big DB documentation update    
* [DEP] Added some ACE fixes    
* [DEP] Attempt to fix Stormlib build    
* [DEP] Stormlib updated to latest version    
* [DEP] Update Stormlib v9.21    
* [DEP] Updated ACE library    
* [DEP] Updated g3glite to latest version    
* [DEP] Updated library    
* [DEP] Updated recast to latest version    
* [Deps] Adjusted to use the standard Deps library    
* [Deps] Corrected case sensitive library name for unix compatibility    
* [Deps] Update Dep Library    
* [DEPS] Update zlib version to 1.2.8    
* [EasyBuild] Added EasyBuild subModule    
* [EASYBUILD] Added temp patch files back    
* [EasyBuild] EasyBuild updated to V2    
* [Easybuild] Fixed a crash. Thanks     MadMaxMangos for finding.
* [EasyBuild] Hotfix Revert OpenSSL binaries.    
* [EasyBuild] ignore easybuild created debug files    
* [EasyBuild] Minor update to include some additional checks    
* [EasyBuild] Remove static files    
* [EasyBuild] Updated base versions of libraries    
* [EasyBuild] Updated MySQL and Cmake library locations    
* [EasyBuild] Updated to Support modified build system and enhancements    
* [Eluna] Add conditionals around code    
* [Eluna] Applied a load of missing hooks    
* [Eluna] Patched to allow compilation    
* [Eluna] sync    
* [Extractors] Correct Version mentioned in Readme's    
* [Extractors] Some cleanup/sync in prep of merge    
* [Extractors] Updated extractors to fix movement bug. Will need to reextract    
* [FIX] correct easybuild patch script    
* [Fix] Correct incorrectly renamed GAMEOBJECT_TYPE    
* [FIX] Make Travis happy.
* [Install] Changes ported from Zero    
* [Install] Extended lazymangos.sh to support M3    
* [Linux] Fix playerbots in getmangos.sh. Thanks Tom Peters    
* [Linux] Fix Sync up script    
* [Linux] Minor script fix    
* [Maps] Allow map id's above 999. Thanks Meltie2013 & Foereaper    
* [Port] Internal Timer Rewrite & Introduce GameTime by Meltie2013
* [Ported] GM Commands files reorganisation. Based on Elmsroth's work    
* [Realm] Sync up    
* [Realm] Updated Realmd required version    
* [Realmd] Correct the submodule revision    
* [Realmd] Resolve SRP6a authentication bypass issue. Thanks     DevDaemon
* [realmd] Resolved authentication bypass. Thanks namreeb    
* [Rel21] Stage 1 of updates for Rel21 Build System    
* [Rel21] Stage 2 of updates for Rel21 Build System    
* [SD3] Added SD3 as a submodule    
* [SD3] first commit with SD3 inplace - not complete yet    
* [SD3] Fix BRD issues and server crash. Thanks 
* [SD3] Fix item_petrov_cluster_bombs    
* [SD3] Fix quest Kodo Roundup (#69)    
* [SD3] Ported a lot of SD3 updates from main SD3 module    
* [SD3] Update Onyxia script    
* [SD3] Update SD3 submodule    
* [SD3] Updated for BRD arena fix    
* [SD3] Updated ScriptDev3    
* [Styling] Another sweep on styling cleanups    
* [Styling] Fix some minor errors    
* [Styling] Remove tabs    
* [Styling] Remove trailing spaces    
* [Styling] Some styling corrections    
* [Styling] Trailing space cleanup    
* [Submodule] Updated submodules    
* [Submodules] Updates dep and SD3    
* [Sync] From Project Sync from MangosTwo    
* [Tools] Extraction Project Sync    
* [Tools] Fixed movemaps compile    
* [Warden] Refactor to match other cores    
* Add Character Date Create Column (#102)    
* Add Codacy badge and status    
* Add Disables table based on the work of     Olion on Zero
* Add mangosd full versioning information on windows    
* Add missing file from last commit    
* Add missing quote which broke links in README    
* add new mangos 'family' icons. Thanks UnkleNuke for the original design    
* Add possibility to write mangos command via a whisper.    
* Add realmd full versioning information on windows    
* Add some missed Warden code    
* Add state for GM command completed quests. Thanks H0zen for assistance    
* Add support for new comment column    
* Add Ubuntu 19.04 case for Prerequisites install (#77)    
* Added AppVeyor Build Status    
* Added compile status to readme    
* Added deps submodule    
* Added missing call to LoadDB2Stores( )    
* Added Patch removal script    
* Added Realmd submodule    
* Added support for Red Hat 'Experimental'
* Adding new distribution support (Fedora) (#16)    
* Adding safeguard to prevent possible duping of gold in mail. (#131)    
* Adding support for database updates. Only last folder (alphabetically sorted) will be takenxw    
* Adding support for Player Bots submodule in installer. (#20)    
* Adding support for Ubuntu: Curl dependencies added
* Adding support of several known Linux distribution for dependancies setup (#175)    
* Adding support when several WoW clients path are detected. Only the first one is selected
* Adds custom emote to wyrmthalak script    
* AddSC_scholomance() added to ScriptLoader    
* AHBot rollup from previous cores    
* All required patch files are now being applied (#123)    
* Ammend Core name    
* And more build errors fixed    
* Another attempt to fix OSX. Thanks 
* apply latest Dep library    
* Apply MAC / Arm fixes for building    
* Apply style fix    
* Appveyor supplied fix for openSSL 1.0    
* At last, changing opcode to enable flying (#7)
* AutoBroadcast system.    
* Avoid increasing player count per team when re-logging (#165)    
* Build error and warning fixes    
* Build error fix - undefined SFileExtractFile (#65)    
* Build fixes made and tabs turned  into spaces    
* Cata areas added to SD3    
* Change function names to match other cores    
* Changed doxygen settings (#5)    
* Changing the Glancing Blow chance to 25% (#18)    
* ChatChannels fix - test PR    
* Check for Buildings folder    
* Check player exiting waiting area while starting battleground    
* Checking of TrainerTemplateId fixed    
* Clean up readme a little    
* Code change for the acquisition of the locale    
* Core updated to use the latest version of ScriptDev3    
* Correct Function Naming across cores pt1    
* Correct Typo for default status    
* Corrected path to Stormlib.h    
* Correction for unix    
* Correction made to BuildChatPacket(( ) declaration    
* Correction made to DBC file format strings    
* Correction made to file name    
* Correction made to function name    
* Correction made to script    
* Corrections made to the extractor    
* Crash on exit to login screen resolved    
* Create a docker container image and runing it with docker-compose (#164)    
* creature_template table's format string corrected    
* debug recv Command added    
* Description of the meaning of the format strings added    
* Disable command locale loading as its hard crashing    
* Disable OSX build checking until we have an OSX dev to get them fixed    
* Displaying of the portait window fixed    
* Drop Unused PostgreSQL (#25)    
* Eliminated tabs!    
* ensure bins are marked as executable (#108)    
* Fix a crash when using an invalid display id with command
* Fix applied for Quest Log, Bags, and money issues
* Fix Baradin Hold and Alizabal script loading    
* Fix core startup message when db version is same as core (#150)    
* Fix crash at startUp due to command localization loading    
* Fix crash in BIH module due to uninitialized member variable. (#172)    
* Fix crash when using command helps (#93)    
* Fix Eluna build after https://github.com/mangosthree/server/commit/b13453cc89c53d1f95107ee744821235af468671    
* Fix Eluna crash.    
* fix error ‘atoi’ was not declared in this scope. Thanks to drarkanex for pointing    
* Fix floating point model for VS 2015 (#52)    
* Fix for Powers defaults always being 0    
* Fix Gameobject loading issue    
* Fix Git Action Errors    
* Fix incorrect logout error.
* Fix instance cleanup at startup (#99)    
* Fix Latest MariaDB cmake detection    
* Fix login issue. Thanks rochet    
* Fix mac build (#84)    
* Fix Merge error    
* Fix misleading core startup message (#147)    
* Fix missing bzip2 dependency for CentOS (#187)    
* Fix missing delimiter    
* Fix OpenSSL travis for mac    
* Fix OSX build on Travis (#94)    
* Fix part of NPC localized text cannot be displayed.    
* Fix pdump write command and add check to pdump load (#106)    
* Fix player kicking    
* Fix possible problem with 'allow two side interaction' and loot.    
* Fix potential NullPointerException on C'Thun (#107)    
* Fix PvPstats table to fit with its web app    
* fix reference to dockerFiles to match with real files name (#92)    
* Fix SD3 Compile fail pt1 of 2    
* Fix send mail and send item commands    
* Fix server crash. Thank H0zen/mpfans    
* Fix server startup. Thanks Rochet and 
* Fix simultaneous connection contention issue    
* Fix some codacy detected issues    
* Fix some code porting discrepancies    
* fix some incorrect slashes    
* Fix static linking in shared libs    
* Fix typo in VMap BIH generation    
* Fix unknown packet 0x720E    
* Fix VS debug build    
* Fix VS2010/2012 build. Thanks H0zen for help    
* Fix VS2017 build. Needs cmake 3.8.0 minimum    
* Fix whisper blocking    
* FIX-CENTOS-BUILD Added epel repo    
* FIX-CENTOS-BUILD Fixed centos 7 build    
* Fixed  chest game objects interactions    
* Fixed build errors related to the static_assert instructions (#64)    
* Fixed dbc and map extractor    
* Fixed memory issue with msbuild build    
* Fixed possible logout problems in Major Cities and Inns.    
* Fixed some additional found nullptrs    
* Fixed the targeting error that would not allow Hai'shulud to be summoned.    
* Fixed travis url change. Thanks Alarnis    
* Fixed vs2012 build    
* Fixes build issues for Linux (#17)    
* Fixes Error "There is no game at this location" (#172)
* Fixing a bug that made the DBC enUS being ignored while they are valid. (#122)    
* Format specifiers was not correct in lootmgr    
* Function added back    
* g++ was not installed without build-essential (#80)    
* GameObject port pt1    
* GroupHandler: prevent cheater self-invite    
* Hai'shulud script updated. Updated Hai'shulud to the correct database entry (#22038).
* Implement Action Workflows (#24)    
* Implement command localization    
* Implement OpenSSL 1.1.x support    
* Implement quest_relations table. Based on work by     Hozen
* Implement Update Timer & Remove Ancient Timers
* Implement Whitespace Cleaner (#27)    
* Improve user experience & clean up (#135)    
* Improving Build system and removing Common.h clutter    
* Incorrect cast fixed (#120)    
* Kill whitespaces in files (#26)    
* LfgDungeonsEntryfmt structure corrected    
* LFGMgr.cpp and LFGMgr.h added to Three    
* linux/getmangos.sh: default to build client tools (#19)    
* LoadCreatureClassLvlStats() fixed    
* Lots of cmangos commirs applied    
* made function isActiveObject consistant to other cores    
* Make git actions happy    
* Make GM max speed customisable through mangosd.conf    
* Make Mangos compatible with newer MySQL pt3.
* Make Mangos Three compatible with newer MySQL.
* Mana tombs added to ScriptLoader    
* Many fixes made to get the server to run again    
* Minor file changes    
* Minor styling change    
* Minor styling tidy up    
* Minor typo corrected (#184)    
* Missing code related to Warden added    
* Missing function added back (#62)    
* Missing script fixed    
* Modify default options    
* More SQL delimiting for modern servers (#166)    
* Move project files about to match currect build system    
* Move the license file    
* MySQL CMake Update (#29)    
* New RNG engine for MaNGOS    
* New thread pool reactor implementation and refactoring world daemon. (#8)    
* Now we can inspect player when GM mode is ON (#98)    
* nuke tabs    
* OpenSSL Version Check & Ensure Version    
* Overwritten CMakeLists entried put back    
* Patch up tree    
* Player health regen fixed - temporary fix    
* Port OSX fix from Zero    
* quest_template columns now read and stored in the correct order    
* raise minimum cmake version to 3.12    
* Refactor Logout Request Packet
* Refactored db_scripts The unity! - Based on the original work of H0zen for Zero.    
* Regex requires gcc 4.9 or higher    
* Remove -rdynamic. on Unix/Linux.    
* Remove duplicate eluna world update call    
* Remove icon folder and fix broken links    
* Remove in-house SSL script    
* Remove last remnants of obsolete npc_gossip table    
* Remove obsolete code    
* Remove obsolete SD2 folder    
* Remove old SD3    
* Remove PostGre support in line with other cores    
* Remove Remnants of Two obsolete tables: npc_trainer_template & npc_vendor_template    
* remove some leftover PostGre references    
* Remove some Visual Studio configurations
* Remove travis badge/checks and add github actions    
* Remove unneeded folder    
* Remove unused include directives    
* Removed OpenSSL1.1.x blocker    
* Removed umwanted text from CMakeList file    
* Removed unwanted "return"
* Replace deprecated std::auto_ptr with std::shared_ptr (#133)    
* Replacing aptitude by apt-get on Ubuntu by default.
* Reved some unused files    
* Revision.h update.    
* Rewrite Soap Server (#130)    
* Script added - npc_kharan_mighthammer    
* Script refactoring done as per previous cores    
* Scripts set up to work for Cata    
* SD3 compile fix pt2 of 2. Thanks 
* Server Banner and Status redone    
* Server start up error fixes    
* Server-owned world channel    
* Several major improvements to Linux installer. (#15)    
* show email in .pinfo command. Based on the work of Salja
* Some code cleanup.    
* some sync from previous cores    
* SpellEffect.dbc - correction to format string    
* StormLib ver 9.2 implemented    
* StormLib's CMakeList reverted to pre last OSX issue fix    
* Style cleanup from the Mangos Futures Team    
* Synchronized Conf files for easier comparison (#96)    
* The tools folder structure redone    
* Tidy up AHBot prices code    
* Time Format Update (#32)    
* Trimming Ubuntu dependencies (#17)    
* Unused code removed    
* Update Badges and cleanup Dependancy Descriptions    
* Update Dep submodule to the main Rel21 module    
* Update deprecated ROW_FORMAT    
* update easybuild patcher    
* Update getmangos.sh (#3)    
* update incorrect copyright information    
* Update mangosd.conf.dist.in    
* Update README.md (#109)    
* Update Waypoint System and Commands    
* Updated Dep Submodule    
* Updated dep/StormLib branch to lastest commit    
* Updated patch scripts    
* Updated Readme - Changed Appveyor to test the correct branch    
* Updated Readme.md and icons    
* Updated to use the latest version of ScriptDev3    
* Updating Debian Sources (#169)    
* URL update    
* VMAP creation fixed - files are now created    
* Warden added    
* Weather system changed to that of the previous cores.    
* Weather system fixed    
* World Delay Timer can be view in server info command.    
* WorldTime will be replaced by GameTime which is more accurate.


0.21 (2016-04-01 to 2021-01-01) - "The Battle for the Elemental Planes"
---------------------------------------------------------
Many Thanks to all the groups and individuals who contributed to this release.


Code Changes:
=============
* [12778] Add 5th field in reputation_spillover_template for Bilgewater Cartel/Gilneas spillover reputation.
* [Appveyor] Remove no-longer needed file
* [Build] Add OpemSSL1.0.2j installers
* [Build] Added MySQL 5.7 support
* [Build] Attempt to make travis happy
* [Build] Cmake fixes from Zero
* [Build] Enhanced Build System
* [BUILD] Fixed SARC4 errors in shared.vcxproj project
* [Build] Fixed up Stormlib
* [Build] Force building SOAP and PlayerBots in testing
* [Build] Improved Build System, based on the work of H0zen (#131)
* [Build] Larger build system update
* [Build] More Project build cleanup
* [Build] Most errors fixed
* [Build] Some cleanup to the Windows build - not complete
* [Build] updated build tools
* [Core] Enable using table creature_linking with empty table creature_linking_template [c12670]
* [Core] Fix tabs
* [Core] REalmdb sync with Zero
* [Core] Remove another obsolete util
* [Core] Remove obsolete directive
* [Core] Removed duplicate files
* [Core] Updated build defintions. Thanks lfxgroove
* [DB] Database table update
* [DB] Rename scripted_event_id table as scripted_event
* [DB] Set min db levels
* [DbDocs] The Big DB documentation update
* [DEP] Update Stormlib v9.21
* [DEP] Updated library
* [Deps] Adjusted to use the standard Deps library
* [Deps] Corrected case sensitive library name for unix compatibility
* [Deps] Update Dep Library
* [DEPS] Update zlib version to 1.2.8
* [Easybuild] Fixed a crash. Thanks MadMaxMangos for finding.
* [EasyBuild] Hotfix Revert OpenSSL binaries.
* [EasyBuild] ignore easybuild created debug files
* [EasyBuild] Minor update to include some additional checks
* [EasyBuild] Remove static files
* [EasyBuild] Updated base versions of libraries
* [EasyBuild] Updated MySQL and Cmake library locations
* [EasyBuild] Updated to Support modified build system and enhancements
* [Eluna] Add conditionals around code
* [Eluna] Applied a load of missing hooks
* [Eluna] Patched to allow compilation
* [Extractors] Updated extractors to fix movement bug. Will need to reextract
* [FIX] correct easybuild patch script
* [Fix] Correct incorrectly renamed GAMEOBJECT_TYPE
* [FIX] Fixed a logic error in EasyBuild
* [FIX] Make Travis happy. Thanks H0zen
* [Install] Changes ported from Zero
* [Install] Extended lazymangos.sh to support M3
* [Linux] Fix playerbots in getmangos.sh. Thanks Tom Peters
* [Linux] Minor script fix
* [m]  corrected some text typos
* [m] Fixed VS2013 MMaps build
* [m] Updated website URL
* [Realm] Update realm daemon so that it only presents clients with realms that it is compatible with.
* [Realm] Updated Realmd required version
* [Realmd] Added submodule back in
* [Realmd] Correct the submodule revision
* [Realmd] Resolve SRP6a authentication bypass issue. Thanks DevDaemon
* [realmd] Resolved authentication bypass. Thanks namreeb
* [Rel21] Stage 1 of updates for Rel21 Build System
* [Rel21] Stage 2 of updates for Rel21 Build System
* [SD3] Added SD3 as a submodule
* [SD3] first commit with SD3 inplace - not complete yet
* [SD3] Fix BRD issues and server crash. Thanks H0zen
* [SD3] Fix item_petrov_cluster_bombs
* [SD3] Fix quest Kodo Roundup (#69)
* [SD3] Update Onyxia script
* [SD3] Updated for BRD arena fix
* [Sync] Build system and docs proj sync
* [Sync] Project header sync
* [Sync] Project sync
* [Sync] Project Sync plus Revision changes
* [Sync] Some long overdue project sync
* [Sync] Some long overdue project sync pt2
* [Sync] Some minor cross project sync
* [Sync] Some minor project sync
* [Sync] Some minor updates from Two
* [Sync] Some project sync
* [Tools] Extraction Project Sync
* [Tools] Fixed movemaps compile
* [Warden] Refactor to match other cores
* 13 cmangos commits implemented
* 50 plus cmangos updates implemented (to c12832)
* A few more build error fixes
* Add Codacy badge and status
* Add Disables table based on the work of Olion on Zero
* Add mangosd full versioning information on windows
* Add missing file from last commit
* add new mangos 'family' icons. Thanks UnkleNuke for the original design
* Add pet support at first login for hunter and warlock
* Add positive exception for spell 56099
* Add possibility to write cmangos command via a whisp.
* Add realmd full versioning information on windows
* Add state for GM command completed quests. Thanks H0zen for assistance
* Add support for new comment column
* Add Temp patch for easybuild
* Add Ubuntu 19.04 case for Prerequisites install (#77)
* Added AppVeyor Build Status
* Added compile status to readme
* Added deps submodule
* Added Linux build helper
* Added mangosproof
* Added missing call to LoadDB2Stores( )
* added missing dependencies for travis
* Added missing folders for mangosd cmake project also reordered this list
* added missing header
* added missing tool
* Added Patch removal script
* Added Realmd submodule
* Adding new distribution support (Fedora) (#16)
* Adding support for Player Bots submodule in installer. (#20)
* Adding support for Ubuntu: Curl dependencies added - Adding support when several WoW clients path are detected. Only the first one is selected - Adding support for database updates. Only last folder (alphabetically sorted) will be takenxw
* Adding support of several known Linux distribution for dependancies setup (#175)
* Adds custom emote to wyrmthalak script
* AddSC_scholomance() added to ScriptLoader
* AHBot rollup from previous cores
* All required patch files are now being applied (#123)
* Ammend Core name
* An ACE build fix.  Allows for building on FreeBSD 10 with the clang stack
* And more build errors fixed
* Another attempt to fix OSX pt2. Thanks H0zen
* Another attempt to fix OSX. Thanks H0zen
* another error correction
* Applied dep and realm updates
* apply latest Dep library
* Apply style fix
* Appveyor supplied fix for openSSL 1.0
* At last, changing opcode to enable flying (#7)
* AutoBroadcast system.
* Build error and warning fixes
* Build error fix - undefined SFileExtractFile (#65)
* Build fixes made and tabs turned  into spaces
* Cata areas added to SD3
* changed ```MAX_ACCOUNT_STR``` from 16 to 32 chars
* Changed doxygen settings (#5)
* Changed email return for item that can't be equiped anymore. Before the email was sent with an empty body and the subject was to long to be displayed in the player email. Now the Email is sent with the subject 'Item could not be loaded to inventory.' and the body as the subject message before. (#71)
* ChatChannels fix - test PR
* Check for Buildings folder
* Check player exiting waiting area while starting battleground
* Checking of TrainerTemplateId fixed
* Clean up readme a little
* Cmangos Cata commits applied
* Cmangos commits applied
* Code change for the acquisition of the locale
* code style
* Core updated to use the latest version of ScriptDev3
* correct "/home/travis/build/mangosthree/server/CMakeLists.txt:117 (if)"
* Correct Typo for default status
* corrected cmake build
* Corrected path to Stormlib.h
* corrected tabs
* corrected two more headers
* Correction for unix
* Correction made to BuildChatPacket(( ) declaration
* Correction made to DBC file format strings
* Correction made to file name
* Correction made to function name
* Correction made to script
* Correction, build fix
* Corrections made (oops)
* Corrections made to the extractor
* Crash on exit to login screen resolved
* Create a docker container image and runing it with docker-compose (#164)
* creature_template table's format string corrected
* dammit
* debug recv Command added
* dep folder is up one dir
* Description of the meaning of the format strings added
* Disable OSX build checking until we have an OSX dev to get them fixed
* disabled CMSG_SET_TITLE_SUFFIX handled - no real opcode for this handler was not found
* Displaying of the portait window fixed
* Dropped the new SD2 Module in
* eh i thought i pushed this definition already...
* Eliminated tabs!
* Enabled CMSG_SET_CHANNEL_WATCH
* enabled SMSG_CALENDAR_SEND_EVENT
* ensure bins are marked as executable (#108)
* finally fixed clangs...
* Fix applied for Quest Log, Bags, and money issues
* Fix build and tidy up file
* Fix build errors after previous commit
* Fix CMSG_DISMISS_CONTROLLED_VEHICLE
* Fix crash at startUp due to command localization loading
* Fix crash in BIH module due to uninitialized member variable. (#172)
* Fix crash when using command helps
* Fix crash when using command helps (#93)
* Fix Death Knight rune cooldown
* Fix Eluna build after https://github.com/mangosthree/server/commit/b13453cc89c53d1f95107ee744821235af468671
* Fix Eluna crash.
* fix error ÔÇÿatoiÔÇÖ was not declared in this scope. Thanks to drarkanex for pointing
* Fix error on INSERT query in mangos.sql. Fixes #21.
* Fix floating point model for VS 2015 (#52)
* Fix for Powers defaults always being 0
* Fix Gameobject loading issue
* Fix instance cleanup at startup (#99)
* Fix Latest MariaDB cmake detection
* Fix login issue. Thanks rochet
* Fix mac build
* Fix mangos three build errors - eluna version before fix is https://github.com/ElunaLuaEngine/Eluna/commit/6804f6e90f5ed3bccbad3dcea5a3d97b2db09249
* Fix Merge error
* Fix Merge error pt2. damn git
* Fix missing delimiter
* Fix OpenSSL travis for mac
* Fix OSX build on Travis (#94)
* Fix part of NPC localized text cannot be displayed.
* Fix pdump write command and add check to pdump load (#106)
* Fix player kicking
* Fix possible problem with 'allow two side interaction' and loot.
* Fix potential NullPointerException on C'Thun (#107)
* Fix PvPstats table to fit with its web app
* fix reference to dockerFiles to match with real files name (#92)
* Fix SD3 Compile fail pt1 of 2
* Fix send mail and send item commands
* Fix server crash. Thank H0zen/mpfans
* Fix server startup. Thanks Rochet and H0zen
* Fix simultaneous connection contention issue
* Fix socket gems
* Fix some codacy detected issues
* Fix some code porting discrepancies
* Fix sound BG (SMSG_PLAY_SOUND)
* Fix static linking in shared libs
* Fix typo
* Fix typo in VMap BIH generation
* Fix unholy blight in DKs
* Fix VS debug build
* Fix VS2017 build. Needs cmake 3.8.0 minimum
* Fix whisper blocking
* FIX-CENTOS-BUILD Added epel repo
* FIX-CENTOS-BUILD Fixed centos 7 build
* Fixed  chest game objects interactions
* Fixed " invalid suffix on literal; C++11 requires a space between literal and identifier [-Wreserved-user-defined-literal]"
* Fixed a couple of typos
* fixed banresult
* Fixed build
* Fixed build errors related to the static_assert instructions (#64)
* Fixed clang build
* Fixed dbc and map extractor
* Fixed memory issue with msbuild build
* fixed more build warnings reported by clang/gcc
* fixed more headers
* fixed one more forgotten merge conflict
* fixed other issue reported via clang
* Fixed SMSG_SHOW_MAILBOX opcode, moved to mailhandler.cpp
* Fixed some additional found nullptrs
* fixed travis build for gcc
* Fixed travis issue where gcc+4.8 is not installed.
* fixed unix build
* Fixed version
* Fixed vs2012 build
* Fixes Error "There is no game at this location" (#172)
* Fixes the build problem with g3d on FreeBSD 10 in System.h
* Fixes the hash definition in ObjectGuid.h. This looks like it allows mangos to compile on FreeBSD 10 with clang
* Fixing a bug that made the DBC enUS being ignored while they are valid. (#122)
* force clang to use libcxx
* Forced clang compiler to use c++11 - fixes clang build
* Format specifiers was not correct in lootmgr
* Function added back
* g++ was not installed without build-essential (#80)
* GroupHandler: prevent cheater self-invite
* Hai'shulud script updated. Updated Hai'shulud to the correct database entry (#22038). Fixed the targeting error that would not allow Hai'shulud to be sumoned.
* Implement CAST_FLAG_HEAL_PREDICTION
* Implement CMSG_DISMISS_CRITTER handler
* Implement command localization
* Implement OpenSSL 1.1.x support
* Implement profession skillgains != 1 (+2, +3)
* Implement quest_relations table. Based on work by 
* Implemented CMSG_GROUP_REQUEST_JOIN_UPDATES and updated SMSG_REAL_GROUP_UPDATE
* Implemented SMSG_START_TIMER (Implement bg countdown timer)
* Implemented vehicle opcodes that allow seat changing
* Improving Build system and removing Common.h clutter
* Incorrect cast fixed (#120)
* Initial project location adjustment
* its nasty thing when vbox even can't load freebsd to make full tests with clang, linux situation is same. There still exists pitty code style issues where clang still issues...
* I've updated some Opcodes
* lets build release mode on travis (reduces warnings list, to devs - use debian based server to test it with gc if wants debug messages)
* LfgDungeonsEntryfmt structure corrected
* LFGMgr.cpp and LFGMgr.h added to Three
* Linux build error fix
* linux/getmangos.sh: default to build client tools (#19)
* LoadCreatureClassLvlStats() fixed
* Lots of cmangos commirs applied
* made function isActiveObject consistant to other cores
* Make Mangos Three compatible with newer MySQL. Based by work by leprasmurf
* Mana tombs added to ScriptLoader
* Mant more cmangos Cata commits applied
* Many cmangos commits applied
* Many fixes made to get the server to run again
* Many, many cmangos Cata commits applied
* Merge branch 'master' into sqlDelimit
* Merge branch 'master' of https://github.com/mangosthree/server
* Merge pull request #1 from leprasmurf/sqlDelimit
* Minor file changes
* Minor styling tidy up
* Minor typo corrected (#184)
* Missing change for clang build fix
* missing changes for last commit
* missing code for last commit
* Missing code related to Warden added
* Missing delimit
* Missing Delimiter
* Missing function added back (#62)
* missing part for last
* Missing script fixed
* Modify default options
* More build errors and warnings fixed
* More build errors fixed
* more c++11 standarts income
* more code corrections
* More delimiting
* more packets updates
* More SQL delimiting for modern servers (#166)
* more travis options
* Move the license file
* New thread pool reactor implementation and refactoring world daemon. (#8)
* Now that tools work, build them by default
* Now we can inspect player when GM mode is ON (#98)
* nuke tabs
* Older versions of clang ( 3.6 and older) still uses tr1 namespace for some types
* On travis we do not need install prefix
* once again i forget about save button
* Over 100 camangos Cata commits applied (to c12950)
* Overwritten CMakeLists entried put back
* partial fix for calendar
* Partially apply https://github.com/mangoszero/server/commit/f81e455e3a68012ce7e7e9c78100e7907fab8672
* Player health regen fixed - temporary fix
* Player/Emotes: Fix infinite dance and read map animation
* Port OSX fix from Zero
* Project tidy up and sync
* quest_template columns now read and stored in the correct order
* Refactored db_scripts The unity! - Based on the original work of H0zen for Zero.
* Regex requires gcc 4.9 or higher
* Rel21 prep
* Remove deps in prep for submodule
* Remove duplicate eluna world update call
* Remove last remnants of obsolete npc_gossip table
* Remove obsolete code
* Remove -rdynamic. on Unix/Linux.
* Remove realmd in prep for submodule
* Remove Remnants of Two obsolete tables: npc_trainer_template & npc_vendor_template
* remove unused file
* Remove unused include directives pt1
* Removed createprojects.bat
* removed dublicate variable definition
* Removed local realmd repo and added universal realmd submodule
* removed one more soap option usage from cmake lists
* Removed OpenSSL1.1.x blocker
* Removed soap options from cmake build There is already option to enable or disable it in configs This feature should come with full mangos package
* Removed some useless CMake options Removed commented "possible" bindings folder
* Removed umwanted text from CMakeList file
* Removed unwanted "return"
* Replacing aptitude by apt-get on Ubuntu by default. Added support for Red Har 'Experimental'
* Script added - npc_kharan_mighthammer
* Script refactoring done as per previous cores
* Scripts set up to work for Cata
* SD3 compile fix pt2 of 2. Thanks H0zen
* Second set of Moves and Year update
* Server Banner and Status redone
* Server start up error fixes
* Server-owned world channel
* Several major improvements to Linux installer. (#15)
* Should limit the change to just clang since gcc does not have a problem building Mangos on FreeBSD 9 and lower with the current code
* show email in .pinfo command. Based on the work of 
* slap som,e warnings and other correction
* slapped more old warnings
* SMSG_ARENA_TEAM_CHANGE_FAILED
* SMSG_SET_PLAYER_DECLINED_NAMES_RESULT structure
* Some adjustments for last pull request
* Some build errors and warning fixed
* Some build errors fixed
* Some build errors fixed
* Some code cleanup.
* some sync from previous cores
* SpellEffect.dbc - correction to format string
* StormLib ver 9.2 implemented
* StormLib's CMakeList reverted to pre last OSX issue fix
* Style cleanup from the Mangos Futures Team
* Synchronized Conf files for easier comparison (#96)
* Temporally commented out two packets in movement handler to solve compilation Until these two packets are not updated they will stay as commented
* The rest of the fix for ACE_Wrappers compiling on FreeBSD 10
* The tools folder structure redone
* This Fixes the build problem in the database code revolving around the UNORDERED_MAP type
* This is bad, this is BADDDD! :D
* this should be commited for public yet
* Trimming Ubuntu dependencies (#17)
* Unused code removed
* Update CMakelist.txt
* Update Dep submodule to the main Rel21 module
* Update deprecated ROW_FORMAT
* Update getmangos.sh
* Update getmangos.sh (#3)
* Update mangosd.conf.dist.in
* Update map list that allow mounts
* Update missed year changes
* Update Opcodes.cpp
* Update Opcodes.h
* Update Player.cpp
* Update Player.cpp
* Update README.md (#109)
* Update README.md (#96)
* update SMSG_FORCED_DEATH_UPDATE
* Update SMSG_IGNORE_DIMINISHING_RETURNS_CHEAT
* update SMSG_MOVE_UPDATE_CAN_FLY
* Update SMSG_PLAYER_SKINNED
* Update Waypoint System and Commands
* Updated CMSG_CANCEL_GROWTH_AURA
* updated CMSG_CHAT_FILTERED
* Updated Dep Submodule
* Updated dep/StormLib branch to lastest commit
* updated linked header source
* Updated opcode handlers for last 
* Updated patch scripts
* Updated Readme - Changed Appveyor to test the correct branch
* Updated README.md
* Updated Readme.md and icons
* updated SMSG_BATTLEFIELD_LIST structure (rbg needs to be merged from two) updated CMSG_BATTLEMASTER_HELLO
* Updated SMSG_DAMAGE_CALC_LOG ps: set all non handled packets to have unhandled packet state
* updated SMSG_ECHO_PARTY_SQUELCH
* updated SMSG_EXPECTED_SPAM_RECORDS
* Updated SMSG_GAMEOBJECT_RESET_STATE
* updated SMSG_GM_MESSAGECHAT, CMSG_COMMENTATOR_ENABLE, SMSG_COMMENTATOR_STATE_CHANGED, CMSG_COMMENTATOR_GET_MAP_INFO, SMSG_COMMENTATOR_MAP_INFO, CMSG_COMMENTATOR_GET_PLAYER_INFO, SMSG_COMMENTATOR_PLAYER_INFO, CMSG_COMMENTATOR_ENTER_INSTANCE, CMSG_COMMENTATOR_EXIT_INSTANCE, CMSG_COMMENTATOR_INSTANCE_COMMAND packets
* updated SMSG_LFG_DISABLED
* Updated SMSG_MINIGAME_SETUP, SMSG_MINIGAME_STATE, CMSG_MINIGAME_MOVE
* Updated SMSG_OFFER_PETITION_ERROR
* updated SMSG_PLAY_TIME_WARNING * this packet is not handled in server * this packet should activate specific UI which inform that you play too long * issue related to this opcode was created as todo feature.
* Updated SMSG_RWHOIS to 4.3.4 15595 Nulled all currently unknown opcodes
* updated SMSG_SET_FACTION_ATWAR, SMSG_SET_FACTION_NOT_VISIBLE opcodes
* Updated SMSG_SHOW_MAILBOX
* Updated some emotes list to cata Updated release build info
* Updated to use the latest version of ScriptDev3
* Updated to World database revision 21 4 8
* Updating Debian Sources (#169)
* Updating to latest version of ScriptDev3
* URL update
* VMAP creation fixed - files are now created
* Warden added
* Warden issue - disabled to prevent crash
* Weather issue - disabled to prevent crash
* Weather system changed to that of the previous cores.
* Weather system fixed

 
DB Changes:
===========
* [French] Updated Translations
* [Locale] Fix 'replace_BaseEnglish_with_xxx' file
* [Locale] Fix up installation script
* [Locale] Updated Translations submodule
* [Locales] Add multi translations. Thanks Elmsroth and Gromchek
* [Localisation] Format updates and minor changes. Thanks all authors
* [Localisation] Multiple updates. Thanks Gromchek and everyone else
* [Localisation] Updated various texts
* [Localisation] various Text translations. Thanks Gromcheck/Elmsroth
* [Localisation] Various updates. Thanks galathil and other contributors
* [Russian] Added some new translations
* [Trans] Updated the trnalsations submodule
* Add missing Fel Fire aura to Warbringer Arix'Amal.
* Add some missing phase defs
* Add some old fixes
* Agatha, Aradne and Daschla should fly.
* Allows for auto update with DB installer
* Anton 24291
* Asghar 22025 EventAI
* Bishop Street script rework.
* Brainwashed Noble 596
* carriage returns gossip_menu_option
* carriage returns quest
* Clefthoof Calf 19183 EventAI added.
* compress files that are not required in normal use
* Crazed Dragonhawk EventAI
* Create Rel21_12_002_structure_fix.sql
* D.E.H.T.A. Quest Series - Borean Tundra
* DB script for command localization
* Durkot Wolfbrother & Armorer Orkuruk
* Fel Reaver 18733 - Remove Fel Reaver Warning Aura on Death
* Fel Reaver 18733 EventAI update pt2.
* Felfire Diemetradon 21408 AI.
* Feral Dragonhawk Hatchling EventAI
* Fix a load of startup errors
* Fix NPC positions
* fix typo in quest beer basted boar ribs
* Fix typo 'Skuller' -> 'Skulker'
* Fixed a type on creature_ai_texts.
* Fixed installdatabase.bat script not creating the DB if defaults left in
* Fixed text for item 10022
* game_tele Northrend Update
* game_tele updates
* GO. 183853 and 183854 rotation updated.
* Gossip script updated
* Haaleshi Talonguard 16967
* Happy New Year 2021 from everyone at getMangos.eu :tada:
* Illidari Jailor AI script
* Illidari Ravager
* Illidari Succubus 22860 EventAI
* Implement script for quest 12049
* Implement script for Warbringer Arix'Amal.
* Improve Broken Skeleton 16805.
* Keeper of the Cistern 20795
* Large Bonfire damage range lowered updated.
* Laris Geardawdl complete rework.
* Mammoth Shark EventAI
* Minor banner update
* Moaki Bottom Thresher 26511
* Moa'ki Harbor added to game_tele location list.
* NPC 23691 scripted
* NPC 29579 script updated
* NPC_24464_scripted.
* Overlord Gorefist 18160
* Prevent Training Dummy Movement.
* q.11314 'The Fallen Sisters'
* q.11593 'The Honored Dead'
* q.12182 'To Venomspite!'
* Quest 10422 fixed.
* Quest 26919, kill credit fix.
* Quest 'Dead Man's Debt" script.
* Quests - Borean Tundra  updates.
* Remove comment from previous commit to avoid confusion.
* Remove duplicate Kaskala Supplies object.
* Remove unnecessary spacing from mangos_string
* Remove XP gain from Unstable Fel Imps
* Rename two commands to match core
* River Thresher 27617
* Saronite Animus EventAI
* Shatterhorn script reworked
* Skull pile GO z position fixed.
* Slaag 22199 - Frenzy should be used at any HP%
* Start tidying up the outstanding update files
* Summon Fissure movement update
* The Big Command Help Sync Pt2
* The big Command help syncup
* Tok'kar z_position update.
* Update deprecated ROW_FORMAT
* Update reward text for quest 5064
* Update script for creatures 25618 and 25730.
* Updated script to apply update from Rel21 folder (#98)
* Valiance Keep Rifleman
* Venomspite quest relations.
* Vindicator Haylen AI script.
* Warden refactor
* WIP: Setup database with docker (#166)
* Wrath Lord 20929
* Zabrajin/Swamprat/Telredor-Guard Orebor Harborage Defenâ€¦  â€¦der 18909/18910/18922/18943


MaNGOS 0.18 to 0.20 No definitive changelog exists


MaNGOS 0.17   (17 November 2012)
================================
 MaNGOS 0.17 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

==== Game Features ====
* Add Auction House Bot
* Add Creature linking
* Add Pathfinding
* Add Outdoor PvP for zones Eastern Plaguelands, Silithus, Hellfire Peninsula, Terorkkar Forest, Zangarmarsh, Halaa and Venture Bay
* Add support for Gameobject group loot
* Add support for Vehicles
* Improve liquid handling
* Add support for dynamic collision

==== Server Features ====
* Add Prepared SQL-Statements
* Add a bunch of new commands for DB scripts and rewrite the engine
* Map system re-engineered. Instance maps are updated normally like non instance maps, memory usage reduced
* Optimize server performance by reducing frequency of visibility update calls
* Multiple MySQL connections to database.
* New system for parallelizing client packet processing
* Move vmaps to vmap version 3.0 and then to 4.0
* Add PersistantState system
* Implement proper work pool/gameevent spawns in Map instances
* Implement server side spells
* Implement a new conditions system that is more flexible
* Improve overall spell targeting and implement many spells
* Generic code style fixes

* Updated MySQL client libs to 5.1.49.
* Updated ACE framework to 5.8.3.
* Updated G3DLite library to 8.0.1

==== Statistics ====
* Total number of changes: 2132 revisions (2273 commits)
* Lots of bugs from forum and issues from github fixed


MaNGOS 0.16   (2 July 2010)

 MaNGOS 0.16 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Implemented support dual talents spec work.
 * Added: Implemented support weekly quests cooldowns.
 * Added: player/group tap of attacked creature, and other loot related changes and fixes.
 * Added: Implemented proper way enchanting in trade slot.
 * Improved: Pool system work, especially with pool creatures/gameobjects as event parts.
 * Improved: Pets speed and movement flags at attack/follow mode switches.
 * Improved: Better work gameobjects at use.
 * Improved: Fixed problem with non-attackable pets/totems at arenas.
 * Improved: Fixed long time existed problem with "partly-resurrection" character with periodic HoT auras in some cases.
 * Improved: More achievement types support added.
 * Improved: More proper use movement/spline flags and movement packets let more correct show creature movement in some cases.
 * Improved: shareable quest work and related quest objective complete events.
 * Improved: Recalculate bounding radius and combat reach distance at model scale.
 * plus lots of fixes for auras, effects, spells, talents, glyphs and more.

 ==== Server Features ====

 * Added: Implemented SOAP in MaNGOS
 * Added: Camera framework for proper mind vision like spells use and grid code cleanups including remove unused cell-level thread locking.
 * Added: New table character_stats for external tools.
 * Added: More user friendly output at DB version check fail.
 * Improved: Update used utf8 cpp library version up to 2.2.4
 * Improved: Character `data` field values finally replaced by normal table fields.
 * Improved: Increase reserved stack size for mangosd up to 4Mb at Windows
 * Improved: Rewritten realmd and mangosd RA code use ACE network classes and drop Sockets lib.
 * Improved: OpenSSL lib upgrade to OpenSSL 1.0.0.
 * Improved: Use ACE config library instead dropped dotconfpp.
 * Improved: G3DLite lib upgrade to G3DLite 8.0b4
 * plus lots of improvements in chat/console commands, server startup DB integrity checks.

 ==== Statistics ====
 * Fixed Bugs: 130 tickets and many bugs reported at forum resolved
 * Total number of changes: 1000 revisions (1010 commits)


MaNGOS 0.15   (10 January 2010)

 MaNGOS 0.15 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Implement AV battleground.
 * Added: Implement threat level show in client.
 * Added: Implement dead visible creatures support.
 * Added: Implement new dungeon/raid difficulties work.
 * Added: Implement mails sending at player levelup.
 * Added: Implement new DB-based gossip system for creatures/gameobjects.
 * Improved: More proper creature movement animation in fly. Improvements in movement generators.
 * Improved: Fixes in pool system that must allow more wide use pool of pools mode.
 * Improved: Rewrited battleground spirit guids code for more correct work.
 * Improved: More achievement types support. Including with DB/script-based achievement requirement checks.
 * plus lots of fixes for auras, effects, spells, talents, glyphs and more.

 ==== Server Features ====

 * Added: More chat/console commands and config options.
 * Added: Implement realmd support realms for different client versions.
 * Added: New memory allocator for MaNGOS, based on Intel Threading Building Blocks library.
 * Improved: Some guid generation and object stores made map local. More improvements can be possible in future for this part.
 * Improved: Player updates move to map local code.
 * Improved: More other changes for thread safe work code in future per-map threads.
 * Improved: More wide use ACE features instead explicitly hardcoded.
 * Improved: Rewritten mail preparing code, and cleanup mail system code in general.
 * Improved: More checks at loading DB data for problems detection.
 * Improved: Some optimizations in amount data send to client.
 * Improved: Avoid characters save calls in some redundant cases.
 * Improved: Progress in move before used at loading values from `data` field to own fields/tables.

 ==== Statistics ====
 * Fixed Bugs: 100 tickets and many bugs reported at forum resolved
 * Total number of changes: 547 revisions (560 commits)


MaNGOS 0.14   (06 October 2009)

 MaNGOS 0.14 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Support for 'on demand' regeneration of power instead fixed tick mode.
 * Added: Allow NPCs to apply taunt.
 * Added: Update killing quest objectives base at creature_template KillCredit fields.
 * Added: Implement support `spell_proc_item_enchant` table for ppm item enchantments triggering at hit.
 * Added: Implement new arena rating-system for season 6.
 * Added: Allow save and restore at login forms/stances.
 * Added: Implement far sight like spells work for long distance.
 * Added: Implement proper storage and use of character specific account data.
 * Added: Implement battleground bonusweekends call to arms.
 * Added: Implement DB-based store battleground/arena gameobjects/creatures and related event spawn/despawn requirement.
 * Improved: Fixes and optimizations in spell cooldown code.
 * Improved: Fixes and improvements in EventAI code let better EventAI creature scripting.
 * Improved: Fixes in combat log packets let show more correct info.
 * Improved: More achievements types support implemented.
 * Improved: Fixed well known walk-after-taxi bug.
 * Improved: More improvements in mind control code.
 * Improved: More improvements in gameobjects spawn/despawn work dependent from gameobject template data.
 * Improved: Changed formula used for calculation creature damage base at attackpower.
 * Improved: Propertly storing/restoring BG entry point including mount/taxi state
 * Improved: More correct work timed quests, other quest code improvements.
 * Improved: Implement more strict checks for fake messages in chat.
 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====

 * Added: Implement support spell casting precasts and aura boost casts for simplify and support related spell cast dependencies.
 * Added: Implement possibility to reset client cache data from server side.
 * Added: Implement --enable-builtin-ace option for explicit select builtin/system ACE in Unix/Linux build.
 * Added: Implement check DBs versions (required_* fields) at mangosd/realmd loading.
 * Improved: More checks at server startup for DB content.
 * Improved: Some fields moved from `data` field of characters table to own fields. More planned move to final get rid from `data` field.
 * Improved: Finish switch to uint32 spell ids use in code.
 * Improved: Optimizations in map loading and broadcasting. New cell search algorithm implemented.
 * Improved: Fixed memory leaks (mostly at server shutdown) and code cleanups.
 * Improved: Use exceptions instead of explicit size checking for each packet, added checks for full read received packets.
 * Improved: Optimizations for different guild data loading and use.
 * Improved: Fixed redundant calculation of v and s on every login.
 * Improved: Fixed race conditions in LockedQueue.
 * Improved: Fixed long existed bug with vmaps unloading.

 ==== Statistics ====
 * Fixed Bugs: 60 tickets and many bugs reported at forum resolved
 * Total number of changes: 600 revisions (635 commits)

MaNGOS 0.13   (10 June 2009)

 MaNGOS 0.13 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Support for heroic class character creating and functionality.
 * Added: Support for area phase system.
 * Added: Support for achievement system and most achievement types implemented.
 * Added: Support for pet's talents learning and pet's spells auto-learning at level change.
 * Added: Support for one potion per-combat functionality work.
 * Added: Support for glyphs work.
 * Added: Support for explicit recipe discovery abilities.
 * Added: Support for heroic raid instance mode.
 * Added: Support for currencies tab.
 * Added: Support for prismatic sockets in items.
 * Added: Support for feral attack power bonus from items.
 * Added: Support for gameobjects and creatures grouping (pools of them).
 * Added: Support for deep ocean fatigue timer.
 * Added: Basic support for mind control like spells. Required more work for full functionality.
 * Added: Support for spellclick units.
 * Added: Support for mobs fleeing and run for getting assistance.
 * Added: support for bind to account items.
 * Improved: Implement proper hide out of range (at another map) transports.
 * Improved: Updated spell casting pushback system.
 * Improved: Allows saving characters in Battle Grounds.
 * Improved: Implemented new BattleGroundQueue invitation system. Now it supports premade group versus premade group matches.
 * Improved: Allow mini-pet has been questgivers or gossip holders, mini-pet use DB data for stats and flags.
 * Improved: Show arrows/etc at creature use ranged weapon and more support for proper scripting creatures with ranged weapon.
 * Improved: Support in DB set time ordering details and reward quest emotes.
 * Improved: Support for use spell data for spellranges at friendly targets.

 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====

 * Added: `spell_bonus_data` table for store damage/healing bonus coefficients.
 * Added: `player_xp_for_level` table added for store XP requirements for player levels instead outdated formula.
 * Added: Implement mangosd stop and pause if it work as Windows service.
 * Added: Implement active objects support.
 * Added: Implement precompiled headers use in build at Windows that speedup builds at Windows in big degree.
 * Added: Implement support for PostgreSQL connection using Unix sockets.
 * Added: EventAI (DB-based creature AI scripting engine) contributed by ScriptDev2 team
 * Added: Tool for convert sql queries/statements from mysql to pgsql.
 * Improved: Rewritten spell proc system added that allow resolve many limitations old spell proc system
 * Improved: Redesigned .map files structure with result more compact files with better data quality.
 * Improved: Better security checks in generic way for chat commands.
 * Improved: Taxi and specially quest taxi paths work.
 * Improved: Better support C++ scripting in script DLL including improvements in AI calls system.
 * Improved: Better support for localization.
 * Improved: Implement item use target requirements store in table `item_required_target`.


 ==== Statistics ====
 * Fixed Bugs: 41 ticket and many bugs reported at forum resolved
 * Total number of changes: 1057 revisions (1550 commits)

MaNGOS 0.12   (Dec 22 2008)

 MaNGOS 0.12 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Support for different difficult instances, proper instances reset and proper group/player binding to instances.
 * Added: Support for arena system.
 * Added: Support for declined names (with Cyrillic client).
 * Added: Support for auction pending mail and 1 hour delay in money delivery to owner.
 * Added: Support for possibility reject warlock's player summon by targeted player.
 * Added: Support for talent based pet auras.
 * Added: Support for auto-reward quests.
 * Added: Support for trap arming delay in combat state.
 * Added: Support for show enchantment item effect on login screen.
 * Added: Support for restoration buff in battlegrounds.
 * Added: Support for dependent spell disabling (instead removing) at talent reset until talent re-learning.
 * Added: Support for guild/arena team leaders delete protection.
 * Added: Support for areatrigger scripting.
 * Added: Support for honor rewards from quests.
 * Added: Support for realmd reconnect from login character list.
 * Added: Support for configurable assistance aggro delaying.
 * Improved: Better duels work.
 * Improved: Improvements in battleground system work including really instancing battlegrounds.
 * Improved: Better support game localization for players and GMs.
 * Improved: More server-side re-checks for client side primary checked data for prevent cheating.
 * Improved: Player can't dodge attack from behind.
 * Improved: Correctly animate eating/drinking in cases sitting at chairs/etc.
 * Improved: Better guardians related functionality work.
 * Improved: Expected instant logout in taxi flight at logout request.
 * Improved: Re-implement raid ready check code.
 * Improved: Select fishing bobber activating time in more standard way.
 * Improved: Better random/waypoint creature movement work including fly case.
 * Improved: Evade distance is now based on the distance from the position that the creature was at when it entered combat.
 * Improved: Implemented further support for contested guards.
 * Improved: Show available daily quests at map as blue marks.
 * Improved: Limit distance for listen say/yell text.
 * Improved: Removed dismount at login hack that now unneeded.
 * Improved: Better support for low character experience gain in group with high character.
 * Improved: Default behavior of pets for creatures changed to REACT_DEFENSIVE.
 * Improved: Better work with combo points on miss/parry/immune.
 * Improved: Allow have team dependent graveyards at entrance map for instances.
 * Improved: Allowed switching INVTYPE_HOLDABLE items during combat.
 * Improved: Make flying creatures fall on ground when killed.
 * Improved: More correct way of calculating fall damage by using fall distance and not fall time.
 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====

 * Added: Support for load and use multiply DBC locales data at server.
 * Added: Store in `spell_chain` additional spell dependences from another spell chain for learning possibility checks.
 * Added: New chat command for reload settings from mangosd.conf file without server restart need.
 * Added: Set icon for realmd/mangosd binaries at Windows build.
 * Added: Start use Adaptive Communication Environment (ACE) framework v.5.6.6 in server network code.
 * Added: Migrate from SVN sourceforge.net to GIT github.com repository for project sources store.
 * Added: Script name indexing for creature, gameobject, item, areatrigger and instance script.
 * Added: The possibility to use custom process return values.
 * Improved: More DB tables loading at server startup and less unneeded DB access in server work time.
 * Improved: Console support for chat like commands features. Merge chat/console command list to single.
 * Improved: Implement possibility output detailed and DB error log messages from scripting code.
 * Improved: Better localization support for DB scripts and scripting library.
 * Improved: Referenced loot data moved to new table for better sharing like data from other loot tables.
 * Improved: Prevent overwriting realmd.conf and mangosd.conf on make install at Unix/Linux.
 * Improved: Update Visual Leak Detector lib to Version 1.9g (beta).
 * Improved: New revisions numbering and sql updates names scheme  with tools support used after switch to GIT.
 * Improved: Use DBC data for creating initial character items.

 ==== Statistics ====
 * Fixed Bugs: many :)
 * Total number of changes: 731 (323 in git)

MaNGOS 0.11   (Jul 22 2008)

 MaNGOS 0.11 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Support for show possible blocked damage, armor penetration, negative stats and other data in client.
 * Added: Support for show aura charges in client.
 * Added: Support for spell casting delay at damage dependence from casting partly interrupts amount, apply interrupts in spell direct damage case also.
 * Added: Support for form requirement check for item/itemset casts at equip. Correctly cast at equip and cast/remove at form switch.
 * Added: Support for correct speed bonus stacking.
 * Added: Support for instant honor updating.
 * Added: Support for cap experience received from high level creatures.
 * Added: Support mining/herbalism mode for skinning of special creatures.
 * Added: Support for more battleground types, many improvements in battlegrounds work.
 * Added: Support for quests work with player title rewards, check for shareable quests base at quest flags.
 * Added: Support for quest rewarding with mail send after specific delay with possible items sent.
 * Added: Support for boss +3 target dependent level for attack rolls and chances.
 * Added: Support for dodge chance calculations depends from agility and level (use dbc based values) instead wrong hardcoded values.
 * Added: Support for defense/weapon skills maxed in PvP fight.
 * Added: Support for bilinear interpolation use for height calculation between existed map point data for better ground height selection for point.
 * Added: Support for corpse reclaim delay dependence from character death count in last period.
 * Added: Support for creatures movement flags storing in DB and setup from scripts, also store extra flags to mark creatures that can't parry/block/counterattack or not provide xp at kill.
 * Added: Support for offhand expertize.
 * Added: Support for generation of pet passive auras by DBC data.
 * Improved: Re-Implemented group/friend/enemy/pet area auras.
 * Improved: Finished fill spell_affect DB table allow have related spells correct working. Now only required update table at client switch.
 * Improved: Allow DoTs/HoTs ticks from dead caster if its not required caster alive state.
 * Improved: Better spell targeting selection code.
 * Improved: Removed outdated honor diminishing return support.
 * Improved: Fixed well known unexpected sit at stun bug.
 * Improved: More correct show and work with faction reputation.
 * Improved: Finish update to expected level dependent mana/health stats data for player's classes.
 * Improved: Better creature corpse despawn time selection. Corpse instant despawn after skinning.
 * Improved: Better check in front spell target requirement.
 * Improved: Show parry/dodge/block/crit calculation in client for same level creature enemy.
 * Improved: Correctly show item random property in all cases, including random property with suffix factor, in inspect and in equipped state at other player.
 * Improved: Use correct way to check binding items and check enchants apply in trade slot.
 * Improved: Rewritten mana regen calculation, also implement creature in combat mana regeneration delay.
 * Improved: Fixed long time existed problem with unexpected auto start melee attack after spell casting.
 * Improved: Re-implemented diminishing returns system.
 * Improved: Re-implemented fear movement.
 * Improved: Rewritten distract/dispel/spell steal effects.
 * Improved: Updated pet name generation data in DB.
 * Improved: Better work combat enter/leave system in PvP/PvE cases.
 * Improved: Many items and item set effects start work.
 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====

 * Added: New sql/tools directory for useful SQL queries for check and restore DB integrity.
 * Added: Own counter for HIGHGUID_PET low guids. Use it for pets. This let have lot more range for pet guids, and then more creature guid for instances spawns/totems.
 * Added: Replace existed random generators by Mersenne Twister random number generator.
 * Added: Support for Russian, Spanish Mexico, traditional and simplified Chinese clients to ad.exe.
 * Added: Support for using the failed_logins field in the account table. Support IP/account temporary ban at max allowed failed logins for account.
 * Added: Support for --enable-doxygen to enable doc generation at configure use at Unix/Linux.
 * Added: Anti-freeze system.
 * Added: AD.EXE now also extracts DBC files. It also now searches for patch files dynamically.
 * Improved: Updated MySQL client libs to 5.0.56.
 * Improved: Better checking DB data at server loading.
 * Improved: Apply reserver names check to pet/charter names also.
 * Improved: Move hardcoded base fishing skill for zone from code to new DB table.
 * Improved: Removed `spell_learn_skill` and `PlayerCreateInfo_skill` tables, use DBC data instead.
 * Improved: Many improvements in Gamemaster chat commands.
 * Improved: Many improvements in config options.
 * Improved: Better scripting support (DB base and C++ based scripts).

 ==== Statistics ====
 * Fixed Bugs: 194
 * Total number of changes: 708

MaNGOS 0.10   (Apr 18 2008)

 MaNGOS 0.10 - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====

 * Added: Implement talent inspecting.
 * Added: Implement unique equipped items support, including gems.
 * Added: Check instances where mount using allowed. Still need implement in-building check.
 * Added: Implement master looter loot mode.
 * Added: Implement quest related lootable gameobject activating only for characters with active quest.
 * Added: Implement multi-map taxi flights, allow teleport to instances/battleground in taxi flight time.
 * Added: Allow equip 2hand weapon in case swap with equipped mainhand/offhand weapon pair.
 * Added: Implement player confirmation request at player summon attempt.
 * Added: Implement support items with limited lifetime.
 * Added: Implement chance additional items crafting with appropriate profession specialization.
 * Added: Implement FFA PvP zones and FFA PvP server type.
 * Added: Implement Guild Banks.
 * Added: Implement expertise and better rating work.
 * Added: Implement dialog status show for gameobject like yellow exclamation signs above gameobject if quest available.
 * Added: Implement money requirement show for gossip box open.
 * Added: Apply reputation discounts to trainer spell prices.
 * Added: Implement death at fall to void/textures.
 * Added: Support dead by default creatures.
 * Added: Implemented work some nice items like "safe" teleport items.
 * Added: Implement correct stacking elixirs/flasks.
 * Added: Implement Fishing hole gameobjects work.
 * Added: Implement support correctly sitting at chairs with different height and slots.
 * Added: Implement normalized weapon damage use where it expected.
 * Improved: Limit player money amount by 214748g 36s 46c.
 * Improved: Allow join to LFG channel if character registered in LFG tool.
 * Improved: Better detection and calculation in fall damage.
 * Improved: Update XP generation for high levels and use coefficients dependent from new/old continents position.
 * Improved: Better immunity/resisted/interrupting spell work.
 * Improved: Better check area/zone/map/mapset limited spells at cast and zone leave.
 * Improved: Fixed long time existed problems with client crash at group member using transports.
 * Improved: Add/update lot spell_affect/spell_proc_event tables content that let work many talents, spells, and item effects.
 * Improved: Better mail system work, use it for problematic items at character loading, and sending honor marks if need.
 * Improved: Use correct coefficients for damage/healing spell bonuses for many spells.
 * Improved: Restore work weather system.
 * Improved: More correct spell affects stacking at target, and spell icons stacking in spellbook.
 * Improved: More correct work explicit target positive aura rank auto-selection and implemented area auras rank auto-selection.
 * Improved: Correct work permanent skill bonuses.
 * Improved: Prevent lost money at unexpected double pay for learned spell with lags.
 * Improved: More correct show for other players character state under different spell affects and in time spell casting.
 * Improved: More correct random item enchantment bonuses selection and work code.
 * Improved: Better battlegrounds work.
 * Improved: Implement default open doors/pushed buttons support.
 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====

 * Added: broken spells check at use and loading.
 * Added: Implement pid file support.
 * Added: Extract and include svn revision version data in binaries at build.
 * Added: Implement binding socket to specific network interface (IP address) instead all interfaces using new config option.
 * Added: Implement support 64-bit binaries building at Windows.
 * Added: Enable the LARGEADDRESSAWARE flag for Visual Studio 2003 mangosd and realmd projects.
 * Improved: Allow Gamemasters see any creature/gameobject including invisible/stealth, allow select unselectable units.
 * Improved: Many server-side check for casts (while shapeshifted, other caster and target reqirents) to prevent cheating.
 * Improved: better loot system implementation including more loot conditions support and simplify loot managment for DB creators.
 * Improved: better DB content error reporting at server load.`
 * Improved: Many improvements in Gamemaster chat commands.
 * Improved: Update sockets library to v.2.2.9 version.
 * Improved: More arena support but not full yet.
 * Improved: Less amount data sent to clients, including in spell casting time.
 * Improved: Fixed/finish PostgreSql support.
 * Improved: Better scripting support (DB base and C++ based scripts).

 ==== Statistics ====
 * Fixed Bugs: 784
 * Total number of changes: 804

MaNGOS 0.9    (Dec 14 2007)

 MaNGOS 0.9 - Codename "Flight Master" - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====
 * Added: support for recipe discovery,
 * Added: support for allowing use of an item only in a specific map or area,
 * Added: support for free-for-all quest/non-quest loot items, additional loot conditions,
 * Improved: a more correct implementation of XP gain (level dependent) when in a group,
 * Improved: fixed creature killed from DoTs but remaining stuck with 1/3 health,
 * Improved: spell and melee crit chance now calculated from DBC data and combat rating coefficients,
 * Improved: a more correct implementation of mana/health regeneration calculation, including quick health regeneration in a polymorphed state,
 * Improved: better support for quests with multiple speakto goals, less dependent on DB for quest flag calculation,
 * Improved: more functionality added to battlegrounds, including correctly showing battleground scores,
 * Improved: a more correct implementation of reputation gain for other factions from the same team,
 * Improved: better support for simple database scripts (quest-start/end, buttons, spells)
 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====
 * Added: support for running mangosd/realmd as Windows services,
 * Added: support for auto-generation of mangosd/realmd crash reports (Windows version only),
 * Added: support for Visual Studio 2008 Express and Pro,
 * Added: support for new Char.log for basic character operations, including save dump of character data on deletion, and logging client IP,
 * Added: support for players queue at login,
 * Improved: better DB content error reporting at server load,
 * Improved: division of Mangos DataBase to MangosDB(WorldDB) and CharactersDB.
 * Improved: better support for older autoconf / automake versions,
 * Improved: existing chat and console commands for server gamemasters/administrators, and added new commands.

 ==== Statistics ====
 * Fixed Bugs: 161
 * Total number of changes: 228

MaNGOS 0.8    (Oct 16, 2007)

 MaNGOS 0.8 - Codename "Innkeeper" - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 Important License Change Notice: MaNGOS still is licensed under the terms
 of the GPL v2, but we now have added an exception to officially allow our
 users to link MaNGOS against the OpenSSL libraries.

 ==== Game Features ====
 * Added: a new threat manager was introduced,
 * Added: log more GM activities,
 * Added: many new features for creatures and game objects working,
 * Added: support for client build 6898, aka version 2.1.3,
 * Added: support for custom creature equipment and display,
 * Added: support for daily quests,
 * Added: support for different fishing loot in sub-zones,
 * Added: support for gender specific models,
 * Added: support for instance specific scripts and data,
 * Added: support for localization of names, texts, etc.,
 * Added: support for multiple battleground instances,
 * Added: support for scripted game object buttons,
 * Improved: battlegrounds should be mostly working, only a few issues left,
 * Improved: dungeon system has seen a few improvements,
 * Improved: formulas for most aspects of the game,
 * Improved: many player level up values have been corrected,
 * Improved: pet and demon handling has seen a lot of improvements,
 * Improved: properly divide loot and reputation in groups,
 * Rewritten: battleground queue system,
 * Rewritten: invisibility detection,
 * plus lots of fixes for auras, effects, spells, talents, and more.

 ==== Server Features ====
 * Added: support for database transactions,
 * Added: support for height maps -- named vmaps -- to tackle the LOS issue,
 * Added: support for OpenBSD and FreeBSD building,
 * Fixed: lots of memory leaks closed,
 * Fixed: Numerous bug fixes to the core,
 * Improved: database queries adding performance boosts here and there,

 ==== Statistics ====
 * Fixed Bugs: 528
 * Total number of changes: 558

MaNGOS 0.7    (Jul 9, 2007)

 MaNGOS 0.7 - Codename "Eye of the Storm" -adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

==== Game Features ====
 * Fixed: random item enchantment display in the auction/mail/group loot dialogs. The item properties are also properly applied to the items at creation.
 * Added: support for opening gameobjects/items using their specific keys. Implemented key requirements for entering instances.
 * Added: a threat based aggro system along with threat bonuses from spells.
 * Implemented many additional spell effects and auras. The spell system is one step closer to completeness.
 * Improved: creature/player hostility checks using more accurate faction hostility data. Hostility is checked when casting spells including area-of-effect spells.
 * Fixed: parry/dodge related code.
 * Improved rage generation formula on blocked attacks/armor reduction etc.
 * Fixed: creature movement problems after creature stun/roots and creature orientation in some cases.
 * Fixed: some small problems in the player inventory system.
 * Added: support for all resurrection methods for various classes. Resurrection from corpse is also implemented.
 * Fixed: showing cooldown for items and spells, server side check and saving to the database is also implemented.
 * Added: support for gift creation.
 * Improved: the pet system and more improvements will follow in next release.
 * Added: keyring support.
 * Improved: many features related to raids/groups.
 * Added: support for in game guild creation. Guild system can be considered finished. Arena team creation  is also partly implemented.
 * Added: target immunity. The targetable flag and the creature type limitation for spell targeting, which is now check when creating the target list, allows area spells against specific creature types like undeads/demons to work.
 * Added: support for diminishing duration of stun/fear/etc effects.
 * Rewrote: pet stable code along with improvements and bug fixes.
 * Added: support for who list filters.
 * Added: the instance system.
 * Fixed: the weather system, now common weather is shown for all players in the same zone.
 * Improved: the mail system and implemented the delay for mails containing items.
 * Added: an initial version of the battleground system. One type of battleground is mostly done, still needs more work.
 * Added: the jewelry profession, prospecting and support for inserting gems in sockets including meta gems bonuses.
 * Added: support for multi-mining veins.
 * Added: support for auto-renaming of characters on login at the request of GMs.
 * Added: a new, more correct visibility system, invisibility is also implemented correctly now.
 * Improved: durability cost calculation.

 ==== Server Features ====
 * Added: full support for 2.0.x client branch.
 * Added/improved: many GM-commands.
 * Added: many checks for DB data at server load to simplify detecting problems and DB development.
 * Moved: many data stored in code to the DB and cached most static DB data at server load to speed up runtime access to it.
 * Added: support for saving next spawn time for creatures/GOs which is now used it at grid/server load.
 * Improved: the script system allowing more item/creature/GO scripts with more features to be written easily.
 * Added: size checking for all packets received from the to prevent crashes at wrong data. Many other received data check were also added to prevent cheating.
 * Improved: the compatibility with 64-bit platforms.
 * Added: support for account password encryption in the DB to increase secure security.
 * Added: support for a log directory and the date info is now added to log name.
 * Updated: the network library to a recent version.
 * Fixed: many memory leaks and crases sources.
 * Added: DBC locale support that can be set from mangosd.conf.

 ==== Statistics ====
 * Fixed Bugs: 390
 * Total number of changes: 923

MaNGOS 0.6    (Jan 29, 2007)

 MaNGOS 0.6 - Codename "Black Dragonflight" - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 ==== Game Features ====
 * Creature and game object respawning has finally been fixed and is considered finished.
 * Many improvements to decrease time for saving player data, and transaction support for save operations has been added.
 * Many item fixes for using / equipping/ enhancing function (some errors still remain and have to be resolved).
 * All professions work now (many fixes have been implemented, you may now enjoy fishing).
 * Mail and auction house system has been rewritten, finally should work as it supposed to be.
 * Spell system has received a HUGE number of improvements, and should feel much better. (Still many problems are left to be fixed, most notable include procflag spell casting, also many effects and auras are not implemented yet).
 * Pet-stable implemented. Many improvements in hunter and warlock pet system (Problems with pet casting exists, various other minor bugs too).
 * Rest system can be considered finished.
 * Quest system has been rewritten (some problems remain unresolved).
 * Implemented PvP system, and support PvE and PvP realm types.
 * Duel system has been rewritten and can be considered complete (minor bugs still remain: e.g. characters sometimes may be killed in duels by channeled spells, or after duels finished by pets still attacking).
 * Guild system improvements, including guild charter, guild master, and guild tabard work. (Some bugs reported about losing tabard style, etc).
 * Some taxi system fixes and improvements. Switched to use DBC only data for taxi system work.
 * Group related code has been rewritten, and extended to support raid groups.
 * Loot code improvements and implementing group loot modes (except master loot mode).
 * Correct implementation for creature skinning has been added.
 * Unlearning talents, and unlearning spells with first rank received from unlearned talent implemented.
 * Transports (ships/zeppelins) system implemented (still have some synchronization problems).
 * Many fixes in combat system (Still have many problems).
 * Enchanting in trade slot implemented as last not implemented part of trade system. Many other fixes.
 * Rogue stealth for other players implemented and many fixes in druid forms.

 ==== Server Features ====
 * Full support for 1.12.x client branch has been implemented.
 * Many GM-commands added and improved including GM mode state and GM invisibility.
 * Many cheating preventing checks added and code rewrites.
 * DB-based scripting support added for quest emote scripts and spell event scripts.
 * Many improvements in less client-server data amount transferring.

 ==== Statistics ====
 * Fixed Bugs: 306
 * Total number of changes: 874

MaNGOS 0.5    (Sep 20, 2006)

 MaNGOS 0.5 - Codename "Stable Master" - adds further improvements to the
 server core as well as to the majority of game classes and the game content
 database.

 Most noteable changes include cleaning up the database backend, adding proper
 support for game clients of version 1.10.2, and closing lots of threading and
 memory related bugs. Cross-platform support has been improved as well, MaNGOS
 should build and run on FreeBSD as well.

 Feature-wise, support for pets, totems, more spells, talents, etc. have been
 added, as well as lots of quest related features.

 ==== Statistics ====
 * Fixed Bugs: 544
 * Total number of changes: 1828.

MaNGOS 0.1    (Dec 04, 2005)

 MaNGOS 0.1 - Codename "Lightbringer" -adds further improvements to the server
 core as well as to the majority of game classes and the game content database.
 A complete list of all updated items follows below:

 === Game Features ===
 * Add all AI files to the build process on Windows
 * Added: Better item information updates.
 * Added: Check on death for invalid duel status.
 * Added: Client now shows full Creature information.
 * Added: Creature::_RealtimSetCreatureInfo procedure which only sets changed values for realtime usage.
 * Added: DEBUG_LOG for logging debug messages. Works with --debug-info switch on Linux and debug build on Windows.
 * Added: Extra information for NPC and item information transmissions.
 * Added: GM command .modify spell spellflatID,val,mark.
 * Added: Guild structures, creation, saving data to DB.
 * Added: Initial support for binding heart stones to a location.
 * Added: Initial support for Guilds. Loading from DB, class, creation and management functions, plus some opcodes supported.
 * Added: Initial support for item stacks.
 * Added item page text display for detailed item info.
 * Added: Level 3 command for Guild creation.
 * Added: Linux Makefiles will now install mangosd.conf to sysconfdir when running "make install" after build.
 * Added pragma's to disable stupid compiler warnings. Code now compiles cleanly.
 * Added: Random generation of damage values for weapons based on their level.
 * Added: RandomMovementGenerator. Template not yet implemented.
 * Added SharedDefines.h (and updated some enums with more values).
 * Added: Sheath code.
 * Added: Some DB cleaning tools, hard-coded damage can be removed now.
 * Added: some movement related classes.
 * Added: SQL tables for guilds.
 * Added: Support for additional spells.
 * Added support for AIM system ( Artificial Intelligence and Movement ).
 * Added: Support for client 1.8.3.
 * Added support for Guild System, still has some bugs, about 85% done.
 * Added support for Honor System, initial support is done, calculations need love.
 * Added support for IP logging of players.
 * Added: Support for page texts.
 * Added: Support for QuestAreaPoints.
 * Added support for reputation.
 * Added: Support for tutorials.
 * added the opcode name in the world.log for bether cheking
 * Added: Weapon damage genrator now adds extra damage types for some items.
 * Add Tools,DBC Editer,you can use it to edit all .dbc File,
 * AI delivery
 * Fix duel flag object position
 * Fixed and sped up the players array code.
 * Fixed: Armor settings.
 * Fixed: Bug fixes for crash and other stuff.
 * Fixed: Character bug on login closed.
 * Fixed character creation bug
 * Fixed: Commented wrong lines in last commit. Now correct ones commented.
 * Fixed: Creation of item spells.
 * Fixed: Creature::SaveToDB() code fixed. I messed it up while trying to sort out NPC corpse issue. Now back to normal.
 * Fixed dead NPC issue.
 * Fixed: Double Jump bug fixed with a temporary solution.
 * Fixed: Fixed duplicate inclusion of Opcodes.cpp and Opcodes_1_7_x.cpp in game and mangosd directory for VC7 build.
 * Fixed: Friendly NPCs attacking.
 * Fixed Game Objects, now signs other objects all display.
 * fixed gametickets at last,added error handling,character can have only 1 gmticket
 * Fixed: Handle the bad data for guid and LOW/HIGH GUID.
 * Fixed: Intel C++ VC project now compiles.
 * Fixed: Item query code fixed. Item now display most stats (90%).
 * Fixed: ItemQuery opcode. This prevents a crash when talking to some vendors.
 * Fixed: Minor fixes for Creatures health, added some comments.
 * Fixed: NPC texts.
 * Fixed: One of the lines in ObjectAccessor.cpp wa removed by accident in changeset #356, causing nearby creatures not roaming, thus not attacking for aggressors.
 * Fixed: Proper comparison for maxhealth.
 * Fixed: Release build for 1.8 and default Grid ON
 * Fixed: Resolve dead NPCs, maxhealth setting.
 * Fixed Skill check for equiping Items.
 * Fixed: Small fix for Windows build in ObjectAccessor::Update(const uint32 &diff).
 * Fixed: Talent modifiers.
 * Fixed: Talent percent work.
 * Fixed: Vendors now load and display items.
 * Fix: now the player can only equip item, if have the proper skill
 * Fix two player in the same zone cores. Fix mem leaks in 1.8 mask deletion. And fix core dump for npc handler due to GUI only takes lower part.
 * Function _ApplyItemMods() is protected... then i created a public function ApplyItemMods() that calls it... Object Oriented Project, guys!
 * Function SetStanding() has been created. Now needs to put it on places where the standing of the reputation is increased.
 * Initial delivery of the AI framework..
 * Major CPU usage improvements with grid system disabled.
 * msg of ignore list fixed
 * Now Faction.dbc is being loaded.
 * Progress bar code enhanced.
 * Put back compression. Apparently the core problem of two players is NOT solved cuz I can still replicate it.
 * Removed: All ENABLE_GRID_SYSTEM defines removed.
 * Removed: Some operation out of Creature::Updated. Resolve dead NPCs.
 * Reputation: first step. List of factions has been created. Some opcodes are working now.
 * Reputation: second step. _LoadReputation, _SaveReputation, LoadReputationFromDBC functions have been created. Now all reputation factions are into factions list.
 * Reputation System is now fixed. All functions are fixed. Load and save to DB are fine. The file reputation.sql has been updated.
 * Reputation table has been created... update your Data Base!
 * Resolved: Outstanding issues in phase 2 of grid system, still some left.
 * Small reputation table sql fix for compatability. Remove latin character requirement.
 * Started adding Enchant spell code.
 * Started writing local items cache. (disabled)
 * Still working on reputation... now FactionTemplate.dbc is loaded.
 * Trainer code fixed. Items now disappear when you learn them.
 * Trainer code update.
 * Updated: Added guild sql files to Linux Makefile.
 * Updated: Adjusted Item Query code.
 * Updated: AtWar flag will now bet set for hostile fractions by LoadReputationFromDBC function.
 * Updated: Changed transmission of item information.
 * Updated: Creature display updates.
 * Updated: Deliver Grid system phase 2 for VC7. The Grid System (TGS) completed.
 * Updated: Display nicer statistics on daemon startup.
 * Updated: FactionTemplate now hashed.
 * Updated Game Objects. Looting works, loot template missing, support for Herbs, Mines, Locks missing.
 * Updated: Item text pages now display additional information.
 * Updated mail support. Now fully works.
 * Updated: More debug cleanings.
 * Updated: More grid optimizations.
 * Updated: only updated creatures/objects near adjacent cell of where player stands. Also intersection of cell between player should update once.
 * Updated: On Quest completion your faction reputation will increase properly.
 * Updated: Quest and NPC text loading modified to suit the new tables.
 * Updated: Quest code will now read the Creature_ID from the table.
 * Updated: Removed some obsolete code.
 * Updated: Rewrote urand() procedure to fix conflicted SVN.
 * Updated: _SetCreatureTemplate() must be run in an update. When set on creation, it has no effect.
 * Updated: Spell time recution talents now fully work.
 * Updated: The Grid System (TGS) is now on by default.  Not support grid off. Next, (1) deliver phase 2 stuff and (2) remove ifdef and ALL old classes.
 * Updated: TRUNCATE is faster than DELETE

 === Server Features ===
 * Added CLI interface for server. Needs to be enabled on compile-time.
 * Reorganized Spell System, separated effects to a diferent file, for better fixing.

 ==== Statistics ====
 * Fixed Bugs: #14, #17, #20, #22, #23, #24, #25, #26
 * Total number of changes: 193.

MaNGOS 0.0.3  (Nov 15, 2005)

 MaNGOS 0.0.3 - Codename "Mango Carpet" - was mainly a bug fix release, which
 never was published on the web, and just marks a point in development where
 a damn lot of bugs had been fixed, but we still felt the need for further work
 to be done before a release could be pushed to the world.

MaNGOS 0.0.2  (Oct 31, 2005)

 MaNGOS 0.0.2 - Codename "Library" - adds another bunch of improvements, bug
 fixes and major enhancements to the overall functionality of the daemon. A
 complete list of all updated items follows below:

 ==== Game Features ====
 * Added support for area exploration.
 * Added support for duels.
 * Added support for ticket system.
 * Added support for trading.
 * Added support for NPC movement when there are no waypoints defined.
 * Added support for NPC gossip, now handling options, and providing default options.
 * Added attack code for creatures.
 * Added default data for realm list.
 * Fixed character logout. Players can only log out when not in combat.
 * Fixed friends and ignore lists.
 * Fixed game objects to have proper sizes.
 * Fixed item swapping.
 * Fixed player factions.
 * Fixed vendors. They now work without requiring gossip texts defined, as long as they have objects to sell.
 * Updated command descriptions to be more meaningful.
 * Updated default data for player creation. Actions, items, skills and spells moved to database.

 ==== Server Features ====
 * Added support for building with Intel C++ compiler on Windows.
 * Added support for building with debug info on Linux. Use --with-debug-info switch to include debug info.
 * Added support for building with 1.8.x protocol as default protocol. Use the 1.8.x build configurations in Visual Studio or --enable-18x switch on Linux.
 * Added support for building with 1.7.x protocol. Use the 1.7.x build configurations in Visual Studio or --enable-17x switch on Linux.
 * Added support for internal httpd server added for those not running Apache. --enable-httpd-integrated will add it. Windows solution available.
 * Added support for displaying progress bars for daemon startup loading.
 * Added support for on demand Grid Loading/Unloading system, which is disabled by default.
 * Added core application framework.
 * Added support for console commands (setgm,ban,create,info)
 * Added command line switch -c for pointing to a custom configuration file. By default file from _MANGOSD_CONFIG (defined in src/mangosd/Master.h.in) will be used.
 * Fixed ZThread build process.
 * Fixed segmentation fault on zone map test due to access of array out of bound. Also, change m_ZoneIDmap to use bitset instead of the 4 bytes bool.
 * Fixed memory leak problems. The creation of new TYPE[] must delete with [] for correctness otherwise n-1 members are leaked.

 ==== Statistics ====
 * Fixed Bugs: #4, #7, #12, #13, #16, #18, #19
 * Total number of changes: 225.

MaNGOS 0.0.1  (Sept 13, 2005)

 MaNGOS 0.0.1 - Codename "Endeavour" - contains a great number of new features,
 improvements and bug fixes. The following list contains all of them:

 * NPC gossips now hash by Guid instead of ID. ID is meaningless and we should revisit its usage.
 * Fixed client crash issue. GameObject still not interactive. Flags issues.
 * Introduced new gameobjecttemplate table as well new map files.
 * Added SCP-to-SQL converter to contrib/scp-to-sql/.
 * MySQL 4.0 branch now is minimum requirement.
 * Server causes client to core on unintialized memory. Also, remove some debug statement which causes problem when the DB is large
 * Creature loot now reads from the creatureloot table. Use a new algorithm to select loot items that mimic the probabilities assigned in each item.
 * Fixed configuration file, added proper settings for packet logging.
 * Added default data for player creation and command help.
 * Added GM command: .addspw #entry-id. Spawns a creature from creaturetemplate table by given #entry-id.
 * Server randomly cores if DBC file failed to load. Fixes by intializing all DBC class internal variables.
 * Daemon version and path to daemon configuration now set by build system on compile time.
 * Allow connections from client release 4544
 * Update solution and project files for latest Visual Studio .NET 2005 Beta 2 release.
 * Fixed compiler error for gcc 4.0 or higher. Calling templated base class methods has to be explicit from 4.0 or higher on.
 * Added contrib/ subdirectory for third-party tools.
 * Applied MaNGOS code indention schema.
 * Initial code checked into repository.

 ==== Statistics ====
 * Fixed Bugs: #2, #3, #9, #10, #11
 * Total number of changes: 53.

