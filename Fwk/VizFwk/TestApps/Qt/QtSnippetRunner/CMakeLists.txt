cmake_minimum_required(VERSION 2.8)

project(QtSnippetRunner)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${CEE_BASE_CXX_FLAGS}")

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-long-long")
endif()


find_package(OpenGL)

find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
include(${QT_USE_FILE})


include_directories(${LibCore_SOURCE_DIR})
include_directories(${LibGeometry_SOURCE_DIR})
include_directories(${LibRender_SOURCE_DIR})
include_directories(${LibViewing_SOURCE_DIR})
include_directories(${LibGuiQt_SOURCE_DIR})
include_directories(${LibUtilities_SOURCE_DIR})
include_directories(${SnippetsBasis_SOURCE_DIR})

set(CEE_LIBS SnippetsBasis freetype LibFreeType LibUtilities LibGuiQt LibViewing LibRender LibGeometry LibIo LibCore)

include_directories(${SnippetsModules_SOURCE_DIR})
set(CEE_LIBS LibStructGrid ${CEE_LIBS})


set(CEE_SOURCE_FILES
QSRCommandLineArgs.cpp
QSRMain.cpp
QSRMainWindow.cpp
QSRPropertiesPanel.cpp
QSRRunPanel.cpp
QSRSnippetWidget.cpp
QSRStdInclude.cpp
QSRTranslateEvent.cpp
)

# Headers that need MOCing
set(MOC_HEADER_FILES
QSRMainWindow.h
QSRPropertiesPanel.h
QSRRunPanel.h
QSRSnippetWidget.h
)

# Run MOC on the headers
add_definitions(-DCVF_USING_CMAKE)
set(MOC_SOURCE_FILES)
qt4_wrap_cpp(MOC_SOURCE_FILES ${MOC_HEADER_FILES} )


add_executable(${PROJECT_NAME} ${CEE_SOURCE_FILES} ${MOC_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${CEE_LIBS} ${OPENGL_LIBRARIES} ${QT_LIBRARIES})


