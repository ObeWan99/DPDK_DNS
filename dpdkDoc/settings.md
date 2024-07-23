# Установка dpdk:

```sh
sudo apt update
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

# Настрйока IPv4(запасной вариант)
nano /etc/network/interfaces

auto eth0
iface eth0 inet dhcp

