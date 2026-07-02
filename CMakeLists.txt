cmake_minimum_required(VERSION 3.16)
project(SmartDisasterRelief LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Required basic components
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    src/main.cpp
    src/core/database.cpp
    src/core/database.h
    src/core/thememanager.cpp
    src/core/thememanager.h
    src/core/appdata.cpp
    src/core/appdata.h
    src/core/filemanager.cpp
    src/core/filemanager.h
    src/core/disastermanagement.h
    src/ui/loginwindow.cpp
    src/ui/loginwindow.h
    src/ui/loginwindow.ui
    src/ui/dashboard.cpp
    src/ui/dashboard.h
    src/ui/dashboard.ui
    src/algorithms/emergency_queue.cpp
    src/algorithms/emergency_queue.h
    src/algorithms/heap_priority.cpp
    src/algorithms/heap_priority.h
    src/algorithms/knapsack.cpp
    src/algorithms/knapsack.h
    src/algorithms/tsp.cpp
    src/algorithms/tsp.h
    src/algorithms/graph.cpp
    src/algorithms/graph.h
    src/algorithms/shelter.cpp
    src/algorithms/shelter.h
    src/algorithms/stack_history.cpp
    src/algorithms/stack_history.h
    src/ui/pages/disaster_page.cpp
    src/ui/pages/disaster_page.h
    src/ui/pages/emergency_page.cpp
    src/ui/pages/emergency_page.h
    src/ui/pages/priority_page.cpp
    src/ui/pages/priority_page.h
    src/ui/pages/resource_page.cpp
    src/ui/pages/resource_page.h
    src/ui/pages/route_page.cpp
    src/ui/pages/route_page.h
    src/ui/pages/shelter_page.cpp
    src/ui/pages/shelter_page.h
    src/ui/pages/zone_page.cpp
    src/ui/pages/zone_page.h
    src/ui/pages/reports_page.cpp
    src/ui/pages/reports_page.h
    src/ui/pages/overview_page.cpp
    src/ui/pages/overview_page.h
    src/ui/pages/hover_scroll_area.cpp
    src/ui/pages/hover_scroll_area.h
    src/ui/pages/alerts_page.cpp
    src/ui/pages/alerts_page.h
    src/ui/pages/users_page.cpp
    src/ui/pages/users_page.h
    src/ui/pages/settings_page.cpp
    src/ui/pages/settings_page.h
)

qt_add_executable(SmartDisasterRelief
    ${PROJECT_SOURCES}
)

target_include_directories(SmartDisasterRelief PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/algorithms
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/pages
)

target_link_libraries(SmartDisasterRelief PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

add_custom_command(TARGET SmartDisasterRelief POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:SmartDisasterRelief>/resources"
)

if(WIN32)
    set_target_properties(SmartDisasterRelief PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
