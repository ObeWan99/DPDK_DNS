#include <iostream>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_ether.h>
#include <cstring>
#include <arpa/inet.h>

#define INTERFACE "0000:00:08.0"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

struct dns_hdr {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

static const struct rte_eth_conf port_conf_default = {
    .rxmode = {
        .mq_mode = RTE_ETH_MQ_RX_NONE,
    },
    .txmode = {
        .mq_mode = RTE_ETH_MQ_TX_NONE,
    },
};


// Функция port_init: Эта функция инициализирует указанный порт Ethernet, 
// настраивая очереди приема и передачи, а также запуская порт. 
// Она также выводит MAC-адрес порта.
static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    struct rte_eth_conf port_conf = port_conf_default;
    const uint16_t nb_rx_queues = 1, nb_tx_queues = 1;
    int ret;
    uint16_t q;

    if (port >= rte_eth_dev_count_avail())
        return -1;

    ret = rte_eth_dev_configure(port, nb_rx_queues, nb_tx_queues, &port_conf);
    if (ret != 0)
        return ret;

    for (q = 0; q < nb_rx_queues; q++) {
        ret = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
        if (ret < 0)
            return ret;
    }

    for (q = 0; q < nb_tx_queues; q++) {
        ret = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE, rte_eth_dev_socket_id(port), NULL);
        if (ret < 0)
            return ret;
    }

    ret = rte_eth_dev_start(port);
    if (ret < 0)
        return ret;

    struct rte_ether_addr addr;
    rte_eth_macaddr_get(port, &addr);

    std::cout << "Port " << port << " MAC Address: "
              << std::hex << (int)addr.addr_bytes[0] << ":"
              << std::hex << (int)addr.addr_bytes[1] << ":"
              << std::hex << (int)addr.addr_bytes[2] << ":"
              << std::hex << (int)addr.addr_bytes[3] << ":"
              << std::hex << (int)addr.addr_bytes[4] << ":"
              << std::hex << (int)addr.addr_bytes[5] << std::endl;

    rte_eth_promiscuous_enable(port);

    return 0;
}

void parse_dns_packet(const uint8_t *data, uint16_t length) {
    if (length < sizeof(dns_hdr)) {
        std::cerr << "Недостаточный размер пакета для заголовка DNS\n";
        return;
    }

    const struct dns_hdr *dns = reinterpret_cast<const struct dns_hdr *>(data);
    bool is_query = !(ntohs(dns->flags) & 0x8000); // Проверяем бит QR
    std::cout << (is_query ? "DNS запрос\n" : "DNS ответ\n");

    // Перемещаемся к полю NAME
    const uint8_t *qname = data + sizeof(dns_hdr);
    const uint8_t *end = data + length;

    // Извлекаем поле NAME
    while (qname < end && *qname != 0) {
        int label_length = *qname;
        qname++;
        if (qname + label_length >= end) {
            std::cerr << "Некорректное поле NAME в DNS пакете\n";
            return;
        }
        for (int i = 0; i < label_length; i++) {
            std::cout << *qname;
            qname++;
        }
        if (*qname != 0) std::cout << ".";
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    struct rte_mempool *mbuf_pool;
    uint16_t portid;
    unsigned long packet_count = 0;

    //Первая задача — инициализация слоя абстракции среды (EAL).
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Ошибка инициализации EAL\n");

    argc -= ret;
    argv += ret;

    if (rte_eth_dev_count_avail() == 0)
        rte_exit(EXIT_FAILURE, "Нет доступных Ethernet портов\n");
    
    //Пул памяти для хранения mbuf (буферов сообщений).
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Не удалось создать пул mbuf\n");

    const char *dev_name = INTERFACE;
    ret = rte_eth_dev_get_port_by_name(dev_name, &portid);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Не удалось найти устройство с PCI-адресом %s\n", dev_name);
    }

    if (port_init(portid, mbuf_pool) != 0)
        rte_exit(EXIT_FAILURE, "Не удалось инициализировать порт %" PRIu16 "\n", portid);

    while (true) {
        struct rte_mbuf *bufs[BURST_SIZE];
        const uint16_t nb_rx = rte_eth_rx_burst(portid, 0, bufs, BURST_SIZE);

        if (nb_rx > 0) {
            packet_count += nb_rx;

            for (int i = 0; i < nb_rx; i++) {
                struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);

                if (rte_be_to_cpu_16(eth_hdr->ether_type) == RTE_ETHER_TYPE_IPV4) {
                    struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

                    if (ipv4_hdr->next_proto_id == IPPROTO_ICMP) {
                        std::cout << "Принят ICMP пакет\n";
                    } else if (ipv4_hdr->next_proto_id == IPPROTO_UDP) {
                        struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((unsigned char *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
                        if (rte_be_to_cpu_16(udp_hdr->src_port) == 53 || rte_be_to_cpu_16(udp_hdr->dst_port) == 53) {
                            std::cout << "Принят DNS пакет\n";
                            parse_dns_packet((uint8_t *)(udp_hdr + 1), rte_be_to_cpu_16(udp_hdr->dgram_len) - sizeof(struct rte_udp_hdr));
                        }
                    }
                }

                rte_pktmbuf_free(bufs[i]);
            }
        }
    }

    rte_eal_cleanup();
    return 0;
}
