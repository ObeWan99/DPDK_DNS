set(SOURCES main.cpp)

add_executable(dpdk_example ${SOURCES})

target_link_libraries(dpdk_example
    ${DPDK_EAL}
    ${DPDK_MEMPOOL}
    ${DPDK_RING}
    ${DPDK_MBUF}
    ${DPDK_ETHDEV}
    ${DPDK_LPM} # Добавляем ссылку на библиотеку rte_lpm
    ${LIB_DL}
    ${LIB_NUMA}
    ${LIB_PTHREAD}
)