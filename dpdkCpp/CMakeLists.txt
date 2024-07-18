cmake_minimum_required(VERSION 3.10)
project(dpdk_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

# Установите путь к DPDK SDK и целевой путь сборки
set(RTE_SDK "/usr/src/dpdk-stable-23.11.1/build")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")

# Пути к заголовочным файлам и библиотекам DPDK
include_directories(${RTE_SDK}/include)
link_directories(${RTE_SDK}/lib)

# Найдите и подключите необходимые библиотеки DPDK
find_library(DPDK_EAL NAMES rte_eal HINTS ${RTE_SDK}/lib)
find_library(DPDK_MEMPOOL NAMES rte_mempool HINTS ${RTE_SDK}/lib)
find_library(DPDK_RING NAMES rte_ring HINTS ${RTE_SDK}/lib)
find_library(DPDK_MBUF NAMES rte_mbuf HINTS ${RTE_SDK}/lib)
find_library(DPDK_ETHDEV NAMES rte_ethdev HINTS ${RTE_SDK}/lib)
find_library(DPDK_LPM NAMES rte_lpm HINTS ${RTE_SDK}/lib)

# Добавьте дополнительные системные библиотеки
find_library(LIB_DL NAMES dl)
find_library(LIB_NUMA NAMES numa)
find_library(LIB_PTHREAD NAMES pthread)

# Соберите исполняемый файл
add_subdirectory(src)
cmake_minimum_required(VERSION 3.10)

# project(dpdk_example LANGUAGES CXX)
# project('dpdk-app', 'c')

# set(CMAKE_CXX_STANDARD 14)

# dpdk = dependency('libdpdk')
# sources = files('main.c')
# executable('dpdk-app', sources, dependencies: dpdk)