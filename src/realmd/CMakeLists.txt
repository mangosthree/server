#/**
# * MaNGOS is a full featured server for World of Warcraft, supporting
# * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
# *
# * Copyright (C) 2005-2015  MaNGOS project <http://getmangos.eu>
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# *
# * World of Warcraft, and all World of Warcraft or Warcraft art, images,
# * and lore are copyrighted by Blizzard Entertainment, Inc.
# */

set(EXECUTABLE_NAME realmd)

set(EXECUTABLE_SRCS
    AuthCodes.h
    AuthSocket.cpp
    AuthSocket.h
    BufferedSocket.cpp
    BufferedSocket.h
    Main.cpp
    PatchHandler.cpp
    PatchHandler.h
    RealmList.cpp
    RealmList.h
   )

if(WIN32)
  # add resource file to windows build
  set(EXECUTABLE_SRCS ${EXECUTABLE_SRCS} realmd.rc)
endif()

include_directories(
  "${CMAKE_SOURCE_DIR}/src/shared"
  "${CMAKE_SOURCE_DIR}/src/framework"
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}/src/shared"
  "${MYSQL_INCLUDE_DIR}"
  "${ACE_INCLUDE_DIR}"
)

add_executable(${EXECUTABLE_NAME}
  ${EXECUTABLE_SRCS}
)

if(NOT ACE_USE_EXTERNAL)
  add_dependencies(${EXECUTABLE_NAME} ace)
  target_link_libraries(${EXECUTABLE_NAME} ace)
else()
  target_link_libraries(${EXECUTABLE_NAME} ACE)
endif()

target_link_libraries(${EXECUTABLE_NAME}
  shared
  framework
)

if(WIN32)
  target_link_libraries(${EXECUTABLE_NAME}
    optimized ${MYSQL_LIBRARY}
    optimized ${OPENSSL_LIBRARIES}
    debug ${MYSQL_DEBUG_LIBRARY}
    debug ${OPENSSL_DEBUG_LIBRARIES}
  )
  if(PLATFORM MATCHES X86)
    target_link_libraries(${EXECUTABLE_NAME})
  endif()
endif()

if(UNIX)
  target_link_libraries(${EXECUTABLE_NAME}
    ${MYSQL_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${OPENSSL_EXTRA_LIBRARIES}
  )
endif()

set(EXECUTABLE_LINK_FLAGS "")

if(UNIX)
  if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(EXECUTABLE_LINK_FLAGS "-Wl, --no-as-needed -pthread -lrt ${EXECUTABLE_LINK_FLAGS}")
  elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(EXECUTABLE_LINK_FLAGS "-Wl,--no-as-needed -ldl -pthread -lrt ${EXECUTABLE_LINK_FLAGS}")
  endif()
endif()

if(APPLE)
  set(EXECUTABLE_LINK_FLAGS "-framework Carbon ${EXECUTABLE_LINK_FLAGS}")
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS
  "${EXECUTABLE_LINK_FLAGS}"
)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION "${BIN_DIR}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/realmd.conf.dist.in" "${CMAKE_CURRENT_BINARY_DIR}/realmd.conf.dist")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/realmd.conf.dist" DESTINATION "${CONF_INSTALL_DIR}")

if(WIN32 AND MSVC)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/\${BUILD_TYPE}/${EXECUTABLE_NAME}.pdb" DESTINATION "${BIN_DIR}" CONFIGURATIONS Debug)
  add_custom_command(TARGET ${EXECUTABLE_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/realmd.conf.dist" "${CONF_COPY_DIR}"
  )
endif()
