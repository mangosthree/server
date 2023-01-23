# This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)

if (NOT OpenSSL_FOUND AND NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL was not found!")
else()
    if (OpenSSL_FOUND AND NOT OPENSSL_VERSION)
        set(OPENSSL_VERSION ${OpenSSL_VERSION})
    endif()

    if (OPENSSL_VERSION VERSION_LESS 1.1)
        message(SEND_ERROR "OpenSSL: This project requires OpenSSL version 1.1.0 or higher")
    endif()
endif()

add_library(openssl)

target_include_directories(openssl
    PUBLIC
    OpenSSL::Crypto
    OpenSSL::SSL)
