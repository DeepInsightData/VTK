# Modules which require software not in the CI image.
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # ospray
<<<<<<< HEAD
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "") # OpenXR
=======
set(VTK_MODULE_ENABLE_VTK_RenderingZSpace NO CACHE STRING "") # zSpace
set(VTK_MODULE_ENABLE_VTK_IOOCCT NO CACHE STRING "") # occt
>>>>>>> bddd94efd3... Improve OpenVR/OpenXR module and test and enable OpenXR in CI
# FindOpenVDB is not installed.
# https://bugzilla.redhat.com/show_bug.cgi?id=1997321
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # openvdb

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_X OFF CACHE BOOL "")
else ()
  set(VTK_USE_X ON CACHE BOOL "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
