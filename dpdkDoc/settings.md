# Установка dpdk:

```sh
sudo apt update
wget https://fast.dpdk.org/rel/dpdk-23.11.1.tar.xz
```

```sh
# Установка основных зависимостей DPDK
sudo apt-get install -y build-essential linux-headers-$(uname -r) gcc make cmake pkg-config libpcap-dev libnuma-dev libelf-dev libdwarf-dev python3-pyelftools meson ninja-build libssl-dev libnl-3-dev libudev-dev
```

```sh
tar xJf dpdk-<version>.tar.xz
cd dpdk-<version>
```

Для настройки сборки DPDK используйте:
```sh
meson setup build
```

или, чтобы включить примеры в сборку, замените команду meson на:
```sh
meson setup -Dexamples=all build
```

После настройки для сборки и установки DPDK в масштабах всей системы используйте:
```sh
cd build
ninja
sudo meson install
sudo ldconfig
```

Последние две команды, указанные выше, обычно необходимо запускать от имени пользователя root, при этом шаг установки meson копирует созданные объекты в их конечные системные расположения, а последний шаг заставляет динамический загрузчик ld.so обновить свой кэш для учета новых объектов.

***В некоторых дистрибутивах Linux, таких как Fedora или Redhat, пути в /usr/local не входят в пути по умолчанию для загрузчика. Поэтому в этих дистрибутивах /usr/local/lib и /usr/local/lib64 следует добавить в файл в /etc/ld.so.conf.d/ перед запуском ldconfig .***

Чтобы решить эту проблему, вам необходимо добавить директории /usr/local/lib и /usr/local/lib64 в конфигурацию ld.so. Это делается следующим образом:

Создайте файл с любым именем (например, local.conf) в директории /etc/ld.so.conf.d/:
```sh
sudo touch /etc/ld.so.conf.d/local.conf
```

Откройте файл local.conf в текстовом редакторе и добавьте следующие строки:
```sh
/usr/local/lib
/usr/local/lib64
```
Эти строки указывают ld.so, что он должен искать библиотеки в этих директориях.

Сохраните файл и выполните команду ldconfig:
```sh
sudo ldconfig
```

# Настройка dpdk:

### Установка драйверов

Убедитесь, что драйверы установлены:
```sh
sudo modprobe uio
sudo modprobe uio_pci_generic
```

Если используете vfio-pci:
```sh
# Я не использовал
sudo modprobe vfio-pci
```

Режим VFIO без IOMMU:
```sh
# Я не использовал
sudo modprobe vfio enable_unsafe_noiommu_mode=1
```

### Привязка сетевой карты

Узнать PCI адрес вашей сетевой карты Virtio:
```sh
lspci | grep Eth
```

Отключение сетевого интерфейса:
```sh
sudo ip link set <interface> down
```

Теперь привяжем сетевую карту к одному из этих драйверов. Используйте утилиту dpdk-devbind для этого. В комплекте с DPDK идет эта утилита:
```sh
sudo ./usertools/dpdk-devbind.py --bind=uio_pci_generic <PCI_ADDRESS>
```

Для vfio-pci:
```sh
# Я не использовал
sudo ./usertools/dpdk-devbind.py --bind=vfio-pci <PCI_ADDRESS>
```

Проверьте статус привязки:
```sh
sudo ./usertools/dpdk-devbind.py --status
```

Отвязать:
```sh
sudo ./usertools/dpdk-devbind.py --unbind <PCI_ADDRESS>
```

***После привязки интерфейса к DPDK, он больше не виден стандартными сетевыми утилитами. Вы должны использовать DPDK-приложения и утилиты для проверки состояния и работы интерфейса. Для назначения IP-адресов и работы с ними, вам нужно настроить соответствующие структуры данных и логику в ваших DPDK-приложениях.***

### Обеспечение достаточного выделения hugepages

```sh
sudo -i
```

Создание точки монтирования для HugePages:
```sh
mkdir -p /mnt/huge
```

Монтирование hugetlbfs:
```sh
mount -t hugetlbfs nodev /mnt/huge
```

Выделение HugePages:
```sh
echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```
Эта команда выделяет 1024 hugepages размером 2MB каждая.

Проверка настроек:
```sh
grep Huge /proc/meminfo
```

```sh
mount | grep hugetlbfs
```

```sh
exit
```

### Если вы хотите, чтобы эти настройки сохранялись при перезагрузке, добавьте соответствующие записи в /etc/fstab и /etc/sysctl.conf.

Добавление в /etc/fstab:
```sh
echo "nodev /mnt/huge hugetlbfs defaults 0 0" >> /etc/fstab
```

Добавление в /etc/sysctl.conf:
```sh
echo "vm.nr_hugepages=1024" >> /etc/sysctl.conf
sysctl -p
```

# Чтобы изолировать ядра

## надо тут поработать ещё ...
Проверить загрузку ядер:
```sh
mpstat -P ALL
```

# Настройка OVS

```sh
mkdir -p /usr/local/etc/openvswitch

ovsdb-tool create /usr/local/etc/openvswitch/conf.db \
    vswitchd/vswitch.ovsschema
```

```sh
# Создание директории для сокетов Open vSwitch
mkdir -p /usr/local/var/run/openvswitch

# Запуск сервера базы данных Open vSwitch
ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock \
    --remote=db:Open_vSwitch,Open_vSwitch,manager_options \
    --pidfile --detach --log-file

mkdir -p /usr/local/var/log/openvswitch
```

```sh
# Позволяет оболочке находить скрипты Open vSwitch, расположенные в этой директории
export PATH=$PATH:/usr/local/share/openvswitch/scripts
```

```sh
# DB_SOCK: Это пользовательская переменная окружения, которая создается для хранения пути к сокету базы данных Open vSwitch
export DB_SOCK=/usr/local/var/run/openvswitch/db.sock
```

```sh
# Установка параметра dpdk-init в базе данных Open vSwitch
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
```
- ovs-vsctl: Это утилита командной строки для управления базой данных Open vSwitch.
- --no-wait: Этот параметр говорит утилите не ждать подтверждения от сервера базы данных.
- set: Эта команда устанавливает значение параметра в базе данных.
- Open_vSwitch: Это название таблицы в базе данных Open vSwitch.
- .: Это указывает на текущую строку в таблице.
- other_config:dpdk-init=true: Этот параметр устанавливает значение dpdk-init в true в разделе other_config для текущей строки в таблице Open_vSwitch. Это включает инициализацию DPDK (Data Plane Development Kit) в Open vSwitch.

```sh
# Запуск сервиса Open vSwitch
ovs-ctl --no-ovsdb-server --db-sock="$DB_SOCK" start
```
- ovs-ctl: Это скрипт для управления службой Open vSwitch.
- --no-ovsdb-server: Этот параметр говорит скрипту не запускать ovsdb-server, поскольку мы уже запустили его ранее.
- --db-sock="$DB_SOCK": Этот параметр указывает скрипту использовать сокет базы данных, определенный в переменной DB_SOCK.
- start: Эта команда запускает службу Open vSwitch.


```sh
# Проверка состояния службы Open vSwitch
ovs-ctl status

# Перезапуск службы Open vSwitch
ovs-ctl restart
```

```sh
# Создать мост br0
ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev

# Привзять к мосту порт dpdk0
ovs-vsctl add-port br0 dpdk0 -- set Interface dpdk0 \
    type=dpdk options:dpdk-devargs=<PCI_ADDRESS>

# Тот же IP, что был до этого у сетевого порта
ip addr add <IP_ADDRESS> dev br0

# Активировать мост br0
ip link set br0 up

# Этот интерфейс является внутренним для OVS и обычно не используется напрямую пользователем. Он создается и управляется демоном ovs-vswitchd автоматически при запуске
ip link set ovs-netdev up

# После настройки перезапустить службу ovs
ovs-ctl restart
```

```sh
# Проверить конфиг моста br0
ovs-vsctl show
```

===========================================================================================================================

# Download trex

# Setup TREX
- выолнить настройку DPDK, конфгурация сетевых интерфейсов и их привязка к DPDK.

- выполнить конфигурацию trex.

```sh
sudo nano /etc/trex_cfg.yaml
```

```sh
- version: 2              # Configuration file version, should be 2 for TRex
  interfaces: ["0000:00:04.0", "dummy"]  # List of interfaces' PCI addresses
  port_limit: 2           # Number of ports to use, should be even: 2, 4, 6, etc.
  port_bandwidth_gb: 10   # Optional, specify bandwidth in Gbps for each port (adjust as needed)
  platform:
      master_thread_id: 0
      latency_thread_id: 1
      dual_if:
             - socket   : 0
               threads  : [2,3]
```

# Run TREX

```sh
# запуск сервера TRex
sudo ./t-rex-64 -i --no-watchdog
```

```sh
# подключения к серверу TRex через CLI
./trex-console
```

# TREX console

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

# Eve setup

cd /opt/unetlab/tmp/0/24ff17f7-923d-4e31-9379-cfc1a5cb1b34/1/

cd /opt/unetlab/addons/qemu/
mkdir linux-astra-1.7
cd $_
qemu-img create -f qcow2 hda.qcow2 20G
mv /path/to/1.7.0-11.06.2021_12.40.iso ./cdrom.iso

chmod -R 775 astra-1.7/
chown  -R root:root astra-1.7/
/opt/unetlab/wrappers/unl_wrapper -a fixpermissions

cd /opt/unetlab/tmp/0/24ff17f7-923d-4e31-9379-cfc1a5cb1b34/1/
qemu-img commit hda.qcow2 
   
===========================================================================================================================

# Inet

## server

```sh
ssh root@172.16.0.176
	1234
iptables -t nat -I POSTROUTING -o enx9cebe8b4d37f -s 172.16.0.xxx -j MASQUERADE
```

## client

```sh
sudo ip route del
sudo ip route add default via 172.16.0.176
sudo nano /etc/resolv.conf 
```

# Hostst

- dpdk-host
	user@172.16.0.187
	192.168.0.1
- client-host
	user@172.16.0.183
	192.168.0.2
- proxmox
	obewan@172.16.0.146


# Apt setup

```sh
sudo nano /etc/apt/sources.list

	deb https://download.astralinux.ru/astra/stable/1.7_x86-64/repository-main/ 1.7_x86-64 main contrib non-free
	deb https://download.astralinux.ru/astra/stable/1.7_x86-64/repository-update/ 1.7_x86-64 main contrib non-free
	deb https://download.astralinux.ru/astra/stable/1.7_x86-64/repository-base/ 1.7_x86-64 main contrib non-free
	deb https://download.astralinux.ru/astra/stable/1.7_x86-64/repository-extended/ 1.7_x86-64 main contrib non-free


	deb [trusted=yes] http://deb.debian.org/debian buster main contrib non-free
	deb-src [trusted=yes] http://deb.debian.org/debian buster main contrib non-free
	deb [trusted=yes] http://security.debian.org/debian-security buster/updates main contrib non-free
	deb-src [trusted=yes] http://security.debian.org/debian-security buster/updates main contrib non-free

sudo nano /etc/apt/apt.conf.d/99disable-ssl-verification	
    Acquire::https::dl.astralinux.ru::Verify-Peer "false";
```

# Perfomance tests
```sh
apt install iperf3
```

## client (client-host)
```sh
iperf3 -c 192.168.0.1 -B 192.168.0.2
```

## server (dpdk-host)
```sh
iperf3 -s -B 192.168.0.1
```

```
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-10.00  sec  19.2 GBytes  16.5 Gbits/sec    0             sender
[  5]   0.00-10.00  sec  19.2 GBytes  16.5 Gbits/sec                  receiver
```