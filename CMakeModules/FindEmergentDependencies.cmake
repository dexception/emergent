# find all of the other packages that Temt/Emergent depend on

# if you have things in /opt/local/lib that cannot otherwise be found, uncomment this:
# if (NOT WIN32)
#  set(CMAKE_LIBRARY_PATH /opt/local/lib ${CMAKE_LIBRARY_PATH})
# endif (NOT WIN32)

if (QT_USE_5)
  set(CMAKE_PREFIX_PATH ${QTDIR}/lib/cmake)
  message(STATUS "Using Qt5, QTDIR = ${QTDIR}")
  message(STATUS "(if QTDIR = <blank> above, then you probably need to set it in your .bashrc or .cshrc etc -- should be /usr/local/Qt5.x.y for standard install)")

  find_package(Qt5Core)
  find_package(Qt5Gui)
  find_package(Qt5Widgets)
  find_package(Qt5OpenGL)
  find_package(Qt5Xml)
  find_package(Qt5Network)
  find_package(Qt5PrintSupport)
  find_package(Qt5Multimedia)
  find_package(Qt5Svg)

  # subvers
  string(SUBSTRING "${Qt5Gui_VERSION}" 2 1 QT5_MINOR_VERS)
  string(COMPARE GREATER "${QT5_MINOR_VERS}" "5" QT_USE_WEBENGINE)
  message(STATUS "Qt5 version: ${Qt5Gui_VERSION} minor version is ${QT5_MINOR_VERS}, using WebEngine?: ${QT_USE_WEBENGINE}")

    #  qt5_use_modules(Emergent Widgets Network WebKit OpenGL Xml)
#  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  # Add compiler flags for building executables (-fPIE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

  include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5PrintSupport_INCLUDE_DIRS}
    ${Qt5Multimedia_INCLUDE_DIRS} ${Qt5Svg_INCLUDE_DIRS})
  
  add_definitions(${Qt5Widgets_DEFINITIONS})

  set(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES}
    ${Qt5Widgets_LIBRARIES} ${Qt5OpenGL_LIBRARIES} ${Qt5Xml_LIBRARIES}
    ${Qt5Network_LIBRARIES} ${Qt5PrintSupport_LIBRARIES} ${Qt5Multimedia_LIBRARIES}
    ${Qt5Svg_LIBRARIES})

  if (QT_USE_WEBENGINE)
    find_package(Qt5WebEngineCore)
    find_package(Qt5WebEngine)
    find_package(Qt5WebEngineWidgets)

    add_definitions(-DUSE_QT_WEBENGINE)

    include_directories(${Qt5WebEngineCore_INCLUDE_DIRS}
      ${Qt5WebEngine_INCLUDE_DIRS} ${Qt5WebEngineWidgets_INCLUDE_DIRS})

    set(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5WebEngineCore_LIBRARIES} ${Qt5WebEngine_LIBRARIES}
      ${Qt5WebEngineWidgets_LIBRARIES})
  else (QT_USE_WEBENGINE)

    find_package(Qt5WebKit)
    find_package(Qt5WebKitWidgets)
     
    include_directories(${Qt5WebKit_INCLUDE_DIRS} ${Qt5WebKitWidgets_INCLUDE_DIRS})
    set(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5WebKit_LIBRARIES} ${Qt5WebKitWidgets_LIBRARIES})

  endif (QT_USE_WEBENGINE)

  if (QT_USE_3D)
    find_package(Qt53DCore)
    find_package(Qt53DRender)
    find_package(Qt53DInput)
    find_package(Qt53DExtras)

    add_definitions(-DTA_QT3D)

    include_directories(${Qt53DCore_INCLUDE_DIRS} ${Qt53DRenderer_INCLUDE_DIRS}
      ${Qt53DInput_INCLUDE_DIRS} ${Qt53DExtras_INCLUDE_DIRS})

    set(QT_LIBRARIES ${QT_LIBRARIES} ${Qt53DCore_LIBRARIES} ${Qt53DRenderer_LIBRARIES}
      ${Qt53DInput_LIBRARIES} ${Qt53DExtras_LIBRARIES})
  else (QT_USE_3D)
    find_package(Coin REQUIRED)
    find_package(Quarter REQUIRED)
    find_package(OpenGL REQUIRED)

  endif (QT_USE_3D)

  # from http://cebmtpchat.googlecode.com/svn/trunk/CMakeModules/QtSupport.cmake
  SET(QT_BINARY_DIR "${_qt5Core_install_prefix}/bin")
  SET(QT_LIBRARY_DIR "${_qt5Core_install_prefix}/lib")
  SET(QT_PLUGINS_DIR "${_qt5Core_install_prefix}/plugins")
  SET(QT_TRANSLATIONS_DIR "${_qt5Core_install_prefix}/translations")

  # Instruct CMake to run moc automatically when needed.
#  set(CMAKE_AUTOMOC ON)
else (QT_USE_5)
  find_package(Qt4 REQUIRED QtCore QtGui QtOpenGL QtXml QtNetwork QtWebKit QtSvg)
  find_package(Coin REQUIRED)
  find_package(Quarter REQUIRED)
  find_package(OpenGL REQUIRED)

  # setup QT_LIBRARIES, defines, etc through options, and the QT_USE_FILE thing does automagic
  set(QT_USE_QT3SUPPORT 0)
  set(QT_USE_QTOPENGL 1)
  set(QT_USE_QTXML 1)
  set(QT_USE_QTNETWORK 1)
  set(QT_USE_QTWEBKIT 1)
  set(QT_USE_QTSVG 1)
  include(${QT_USE_FILE})

  include_directories(${QT_INCLUDES})

endif (QT_USE_5)

# subversion
if (WIN32)
  # Give FindSubversionLibrary a hint to where the libs are installed on Windows.
  # TODO: verify this just works on its own on Linux/Mac.
  set(SUBVERSION_INSTALL_PATH "${EMER_SVN_LIBS_DIR}")
endif()
find_package(SubversionLibrary REQUIRED)


# readline / termcap
if (NOT WIN32)
  find_package(Readline REQUIRED)
  if (APPLE)  
    FIND_LIBRARY(CARBON_LIBRARY Carbon)
    FIND_LIBRARY(OBJC_LIBRARY objc)
  else (APPLE)
    find_package(Termcap)
  endif (APPLE)
endif (NOT WIN32)


# ODE, GSL
find_package(ODE)
find_package(GSL)


# ZLIB
if (WIN32)
  if (QT_USE_5)
    find_package(ZLIB)
    message(STATUS "Found ZLIB for Windows in = ${ZLIB_LIBRARIES}")
  else (QT_USE_5)
    set(ZLIB_LIBRARIES "")
  endif (QT_USE_5)
else (WIN32)
  find_package(ZLIB)
endif (WIN32)


# SndFile (optional)
find_package(SndFile)
if(SNDFILE_FOUND)
  set(EMERGENT_OPT_LIBRARIES ${EMERGENT_OPT_LIBRARIES} ${SNDFILE_LIBRARY})
  include_directories(${SNDFILE_INCLUDE_DIR})
  add_definitions(-DTA_SNDFILE)
  message(STATUS "Found optional libsndfile in ${SNDFILE_LIBRARY} -- enabling it for sound file I/O -- otherwise on Mac OS X no sound file I/O is avail")
else (SNDFILE_FOUND)
  if (APPLE)
    message(STATUS "Did NOT found optional libsndfile on Mac OS X -- NO sound file I/O is available without it!")
  endif (APPLE)
endif(SNDFILE_FOUND)


# HPC profiling
if(HPCPROF_BUILD)
  set(EMERGENT_OPT_LIBRARIES ${EMERGENT_OPT_LIBRARIES} -L/usr/local/lib/hpctoolkit -lhpctoolkit)
  add_definitions(-DHPCPROF_COMPILE)
endif(HPCPROF_BUILD)


##############################
# CUDA (set -DCUDA_BUILD flag at compile time)
IF(CUDA_BUILD)
  set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
  find_package(CUDA REQUIRED)
  # turn this on for debugging
#  set(CUDA_VERBOSE_BUILD ON)
  FIND_CUDA_HELPER_LIBS(curand)
# this is pretty aggressive -- just for testing
  if (NOT WIN32)
    if (APPLE)
      set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; --ptxas-options=-v -gencode=arch=compute_20,code=sm_20 -gencode=arch=compute_30,code=sm_30 -gencode=arch=compute_35,code=sm_35 -gencode=arch=compute_50,code=sm_50 -gencode=arch=compute_52,code=sm_52 -gencode=arch=compute_52,code=compute_52 --use_fast_math -O3)
      # this is more standard and is the default
      #  set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; --ptxas-options=-v -arch=compute_20 -code=sm_20,sm_21,sm_30 --use_fast_math -O3)
    else (APPLE)
      #Linux appears to need the -fPIC options to compile with cuda 
      set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; --ptxas-options=-v -gencode=arch=compute_20,code=sm_20 -gencode=arch=compute_30,code=sm_30 -gencode=arch=compute_35,code=sm_35 -gencode=arch=compute_50,code=sm_50 --use_fast_math -O3 -Xcompiler -fPIC)
      # this is more standard and is the default
      #  set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; --ptxas-options=-v -arch=compute_20 -code=sm_20,sm_21,sm_30 --use_fast_math -O3 -Xcompiler -fPIC)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif (APPLE)
  else (NOT WIN32)
    #Todo:Are these the correct flags for windows?
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; -arch=compute_30 -code=sm_30 --use_fast_math -O3)
    # this is more standard and is the default
    #  set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; -arch=compute_20 -code=sm_20,sm_21,sm_30 --use_fast_math -O3)    
  endif (NOT WIN32)
  include_directories(${CUDA_INCLUDE_DIRS})
  set(EMERGENT_OPT_LIBRARIES ${EMERGENT_OPT_LIBRARIES} ${CUDA_LIBRARIES} ${CUDA_curand_LIBRARY})
  add_definitions(-DCUDA_COMPILE)
ENDIF(CUDA_BUILD)

# NOTE: could also do BISON but it is not really required so not worth the hassle
#find_package(BISON)

include_directories(${COIN_INCLUDE_DIR} ${QUARTER_INCLUDE_DIR}
  ${ODE_INCLUDE_DIR}
  ${GSL_INCLUDE_DIR}
  ${SUBVERSION_INCLUDE_DIRS}
)
if (WIN32)
  #note: valid in main app and plugin contexts:
  include_directories(${EMERGENT_SHARE_DIR}/3rdparty/include
    $ENV{COINDIR}/include
  )
else (WIN32)
  include_directories(${READLINE_INCLUDE_DIR} )
  # Termcap on Fedora Core
  if (NOT APPLE)  
    include_directories(${TERMCAP_INCLUDE_DIR} )
  endif (NOT APPLE)
endif (WIN32)

# Windows dll macros
if (WIN32)
  add_definitions(-DCOIN_DLL -DQUARTER_DLL)
endif (WIN32)

# all dependency libraries to link to -- used automatically in EMERGENT_LINK_LIBRARIES
# specify in executables
set(EMERGENT_DEP_LIBRARIES ${COIN_LIBRARY} ${QUARTER_LIBRARY} ${QT_LIBRARIES}
    ${ODE_LIBRARY} ${GSL_LIBRARIES}
    ${OPENGL_LIBRARIES} ${ZLIB_LIBRARIES}
    ${SUBVERSION_LIBRARIES} ${EMERGENT_OPT_LIBRARIES}
)
if (NOT WIN32)
  set(EMERGENT_DEP_LIBRARIES ${EMERGENT_DEP_LIBRARIES}
    ${READLINE_LIBRARY}
    )

  if (APPLE)  # Termcap on Fedora Core
    set(EMERGENT_DEP_LIBRARIES ${EMERGENT_DEP_LIBRARIES}
      ${CARBON_LIBRARY} ${OBJC_LIBRARY}
      )
  else (APPLE)
    set(EMERGENT_DEP_LIBRARIES ${EMERGENT_DEP_LIBRARIES}
      ${READLINE_LIBRARY} ${TERMCAP_LIBRARY}
      )
  endif (APPLE)
endif (NOT WIN32)

# theoretically you're not supposed to do this, but we don't pass build type in at
# run time, so this should be OK -- it is used for maketa flags

get_directory_property(defstr COMPILE_DEFINITIONS)
foreach(d ${defstr})
  if(WIN32)
    set(defs "${defs} /D${d}")
  else(WIN32)
    set(defs "${defs} -D${d}")
  endif(WIN32)
endforeach(d ${defstr})
message(STATUS "Compile definitions: ${defs}")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(EMERGENT_FULL_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${defs}" )
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
  set(EMERGENT_FULL_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${defs}")
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
  set(EMERGENT_FULL_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${defs}")
else(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(EMERGENT_FULL_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${defs}")
endif(CMAKE_BUILD_TYPE MATCHES "Debug")

message(STATUS "FULL CMAKE_CXX_FLAGS: ${EMERGENT_FULL_CXX_FLAGS}")

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "FULL INCLUDE_DIRECTORIES: ${dirs}")
