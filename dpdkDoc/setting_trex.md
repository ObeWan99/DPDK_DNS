# Download trex

```sh
sudo apt update
sudo apt install -y build-essential libnuma-dev python3-pip python3-dev libpcap-dev linux-headers-$(uname -r) git cmake libelf-dev
wget --no-check-certificate https://trex-tgn.cisco.com/trex/release/latest
tar xvf latest
cp  v.3.06 /tmp/
```

# Setup TREX

- выолнить настройку DPDK, конфгурация сетевых интерфейсов и их привязка к DPDK.

- выполнить конфигурацию trex.

```sh
sudo nano /etc/trex_cfg.yaml
```

```sh
- version: 2              # Configuration file version, should be 2 for TRex
  interfaces: ["0000:01:00.0", "dummy"]  # List of interfaces' PCI addresses
  port_limit: 2           # Number of ports to use, should be even: 2, 4, 6, etc.
  port_bandwidth_gb: 10   # Optional, specify bandwidth in Gbps for each port (adj>
  platform:
      master_thread_id: 0
      latency_thread_id: 1
      dual_if:
             - socket   : 0
               threads  : [2, 3, 4, 5, 6, 7]
```

# Run TREX server

Запуск сервера TRex:
```sh
sudo ./t-rex-64 -i --no-watchdog
```

Подключения к серверу TRex через CLI:
```sh
./trex-console

start -f stl/syn_attack.py --force -m 10000mbps
```

# Run TREX console

```sh
service

# настйрока L3 трафика
l3 -p 0 --src 192.168.0.2 --dst 192.168.0.1

service --off

# настройка атрибутов порта
portattr

# запуск скрипта генерации трафика в 10 мегабит
start -f stl/syn_attack.py --force -m 10mbps

# обновление параметра генерации трафика 1 мегабит
update -m 1mbps - change speed

# остановка трафика
stop - stop traffic gen

# удаление текущих настроек
clear
```

```sh
# отправка трафика из pcap
push --port 0 -f /home/user/cmds_over_dns_txt_queries_and_reponses_ONLY.pcap

# параметры для команды push
-d 10 -c 10000 --force
```