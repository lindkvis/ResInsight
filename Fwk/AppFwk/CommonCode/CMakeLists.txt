cmake_minimum_required (VERSION 2.8.12)

project (CommonCode)

# Qt
find_package ( Qt4 COMPONENTS QtCore QtGui QtMain QtOpenGl )
include (${QT_USE_FILE})

# Open GL
find_package( OpenGL )

# These headers need to go through Qt's MOC compiler
set( QOBJECT_HEADERS    
    cafMessagePanel.h
    )

qt4_wrap_cpp( MOC_FILES_CPP ${QOBJECT_HEADERS} )

add_library( ${PROJECT_NAME}
	cafEffectCache.cpp
	cafEffectCache.h
	cafEffectGenerator.cpp
	cafEffectGenerator.h
	cafLog.cpp
	cafLog.h
	cafMessagePanel.cpp
	cafMessagePanel.h
	cafMouseState.cpp
	cafMouseState.h
	cafUtils.cpp
	cafUtils.h
	cvfStructGrid.cpp
	cvfStructGrid.h
	cvfCellRange.cpp
	cvfCellRange.h
	cafColorTable.cpp
	cafColorTable.h
	
	cvfStructGridGeometryGenerator.cpp
	cvfStructGridGeometryGenerator.h
	cvfStructGridScalarDataAccess.h
    cafVecIjk.cpp
    cafVecIjk.h

    ${MOC_FILES_CPP}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries ( ${PROJECT_NAME}
    LibCore
    LibGeometry
    LibViewing
    LibRender
    LibGuiQt

    cafPdmCore

    ${QT_LIBRARIES}
)

source_group("" FILES ${PROJECT_FILES})
