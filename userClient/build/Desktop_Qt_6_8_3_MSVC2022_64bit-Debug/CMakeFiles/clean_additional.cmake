# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\SmartFarmDashboard_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\SmartFarmDashboard_autogen.dir\\ParseCache.txt"
  "SmartFarmDashboard_autogen"
  )
endif()
