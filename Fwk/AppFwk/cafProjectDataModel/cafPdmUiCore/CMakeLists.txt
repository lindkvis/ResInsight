cmake_minimum_required (VERSION 2.8)

project (cafPdmUiCore)		

# Qt
find_package ( Qt4 COMPONENTS QtCore QtGui )
include (${QT_USE_FILE})

include_directories (
    ..
)

set( PROJECT_FILES

    cafInternalPdmFieldTypeSpecializations.h
    cafInternalPdmUiFieldCapability.h
    cafInternalPdmUiFieldCapability.inl
    cafInternalPdmUiCommandSystemInterface.h

    cafPdmUiEditorHandle.cpp
    cafPdmUiEditorHandle.h
    cafPdmUiFieldEditorHandle.cpp
    cafPdmUiFieldEditorHandle.h
    cafPdmUiFieldHandle.cpp
    cafPdmUiFieldHandle.h
    cafPdmUiFieldSpecialization.h
    cafPdmUiItem.cpp
    cafPdmUiItem.h
    cafPdmUiModelChangeDetector.cpp
    cafPdmUiModelChangeDetector.h
    cafPdmUiObjectEditorHandle.cpp
    cafPdmUiObjectEditorHandle.h
    cafPdmUiObjectHandle.cpp
    cafPdmUiObjectHandle.h
    cafPdmUiOrdering.cpp
    cafPdmUiOrdering.h
    cafPdmUiCommandSystemProxy.cpp
    cafPdmUiCommandSystemProxy.h
    cafPdmUiTreeOrdering.cpp
    cafPdmUiTreeOrdering.h
    cafUiTreeItem.h
    cafPdmUiGroup.cpp
    cafPdmUiGroup.h

    cafSelectionManager.cpp
    cafSelectionManager.h
    cafSelectionManagerTools.h
)

add_library( ${PROJECT_NAME}
	${PROJECT_FILES}
)

target_link_libraries ( ${PROJECT_NAME}
    cafPdmCore
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

source_group("" FILES ${PROJECT_FILES})
