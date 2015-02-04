MaNGOS Three [![Build Status](https://travis-ci.org/mangosthree/server.png)](https://travis-ci.org/mangosthree/server)
============
*MaNGOS Three* is a full featured server for [World of Warcraft][2], in its Cataclysm
version supporting clients from the [patch 4.3.4][50] branch, specifically patch
4.3.4.

World of Warcraft, and all World of Warcraft or Warcraft art, images, and lore are
copyrighted by [Blizzard Entertainment, Inc.][1].


Build status
------------
Each update to the *MaNGOS Three* sources is run through build tests using the
Travis CI build service. The current build status can be viewed on the *MaNGOS Zero*
[build status page][114], and for your convenience the build status also is shown
at the top of this README when viewing the repository on [github][111]. If it's
green, a successful build has been performed under Linux.

ROADMAP and goals for 0.20.0 release
------------
* Completely synchronize fixes between mangos repositories (zero, one, two)
* Implement void storage
* Implement archaelogy
* warden system
* Implement passive anticheat (warden system is not enough to stop jockers...).
* Phasing system port from trinitycore
* Add missing opcodes.
* Implement linked_spell_trigger database table implementation (this will accessible in all mangos) for simple spells implementation.
* Implement new gossip script wrapper in scriptmgr to make code style better.
* Implement new event in EVENTAI which would let better integrate gossips
* Rework/retest all classes spells

Requirements
------------


Compilation Guides
------------------
Please see our compilation and installation guides at our [Wiki][20]

License
-------
*MaNGOS Three* is open source, and licensed under the terms of the GNU GPL version 2.

  Copyright (C) 2005-2015  MaNGOS <http://getmangos.co.uk>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  You can find the full license text in the file COPYING delivered with this
  package.

### Exceptions to GPL

  Some third-party libraries MaNGOS uses have other licenses, that must be
  uphold.  These libraries are located within the dep/ directory

  In addition, as a special exception, MaNGOS gives permission to link the code
  of its release of MaNGOS with the OpenSSL project's "OpenSSL" library
  (or with modified versions of it that use the same license as the "OpenSSL"
  library), and distribute the linked executables. You must obey the GNU
  General Public License in all respects for all of the code used other than
  "OpenSSL".

[1]: http://blizzard.com/ "Blizzard Entertainment Inc. · we love you!"
[2]: http://battle.net/wow/ "World of Warcraft"

[10]: http://a.dependency.net/ "A · dependency"

[20]: https://github.com/mangoswiki/Wiki/wiki/MaNGOS%20Installation/ "Wiki"

[50]: http://www.wowpedia.org/Patch_4.3.4 "WoW Cataclysm· Patch 4.3.4 release notes"

[100]: http://getmangos.co.uk/ "MaNGOS Community Project Website"
[101]: http://community.getmangos.co.uk/ "MaNGOS Community Discussion Forums"

[110]: http://github.com/mangosthree "MaNGOS Three · github organization"
[111]: http://github.com/mangosthree/server "MaNGOS Three · server repository"
[112]: http://github.com/mangosthree/scripts "MaNGOS Three · script extensions repository"
[113]: http://github.com/mangosthree/database "MaNGOS Three · content database repository"
[114]: https://travis-ci.org/mangosthree/server/ "MaNGOS Three · build status"

[201]: http://www.microsoft.com/express/ "Visual Studio Express · free, limited edition"
[202]: http://gcc.gnu.org/ "GCC"
[203]: http://clang.llvm.org/ "Clang"

[251]: http://www.cmake.org/ "CMake · Cross Platform Make"
