#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "xse::common" for configuration "Release"
set_property(TARGET xse::common APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(xse::common PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/common.lib"
  )

list(APPEND _cmake_import_check_targets xse::common )
list(APPEND _cmake_import_check_files_for_xse::common "${_IMPORT_PREFIX}/lib/common.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
