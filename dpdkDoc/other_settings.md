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

# Eve setup
```sh
cd /opt/unetlab/addons/qemu/

mkdir linux-astra-1.7

cd $_

# создает новый виртуальный жесткий диск с именем hda.qcow2
qemu-img create -f qcow2 hda.qcow2 20G

mv /path/to/1.7.0-11.06.2021_12.40.iso ./cdrom.iso

chmod -R 775 astra-1.7/

chown  -R root:root astra-1.7/

# запускает скрипт unl_wrapper с параметром -a fixpermissions, который исправляет права доступа для файлов и директорий в EVE-NG
/opt/unetlab/wrappers/unl_wrapper -a fixpermissions

cd /opt/unetlab/tmp/0/24ff17f7-923d-4e31-9379-cfc1a5cb1b34/1/

qemu-img commit hda.qcow2 
```

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