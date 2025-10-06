# Установка dpdk:

```sh
sudo apt update
wget dpdk-<version>
```

Установка основных зависимостей DPDK:
```sh
sudo apt-get install -y build-essential linux-headers-$(uname -r) gcc make cmake pkg-config libpcap-dev libnuma-dev libelf-dev libdwarf-dev python3-pyelftools meson ninja-build libssl-dev libnl-3-dev libudev-dev libipsec-mb-dev
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


# Настройка dpdk:

### Установка драйверов

Убедитесь, что драйверы установлены:
```sh
sudo modprobe uio
sudo modprobe uio_pci_generic
```

### Привязка сетевой карты

Узнать PCI адрес вашей сетевой карты Virtio:
```sh
lspci | grep Eth
ethtool -i <interface>
```

Отключение сетевого интерфейса:
```sh
sudo ip link set <interface> down
```

Теперь привяжем сетевую карту к одному из этих драйверов. Используйте утилиту dpdk-devbind для этого. В комплекте с DPDK идет эта утилита:
```sh
sudo ./usertools/dpdk-devbind.py --bind=uio_pci_generic <PCI_ADDRESS>
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

===============================================================================================
===============================================================================================


# Конфиг сетевой карты для spf модулей

## Шаг 1: Добавление параметра в настройки драйвера

1. Откройте или создайте файл конфигурации для драйвера ixgbe:
```sh
sudo nano /etc/modprobe.d/ixgbe.conf
```

2. Этот параметр позволяет драйверу работать с некоторыми SFP+ модулями, которые не сертифицированы или не поддерживаются официально:
```sh
options ixgbe allow_unsupported_sfp=1
```

## Шаг 2: Перезагрузка драйвера

Для применения изменений нужно перезагрузить драйвер ixgbe.

1. Удалите драйвер из ядра:
```sh
sudo modprobe -r ixgbe
```
**Объяснение:**
- modprobe -r удаляет драйвер из текущего состояния ядра.

2. Загрузите драйвер обратно:
```sh
sudo modprobe ixgbe
```

## Шаг 3: Проверка работы

После загрузки драйвера нужно проверить, применились ли настройки и работает ли SFP+ модуль:

1. Проверьте статус устройства с помощью ethtool
```sh
sudo ethtool enp1s0
```

# Привязка карты к драйверу dpdk или к ядру

## Шаг 1: Проверка NIC к ядру

1. Проверка доступных интерфейсов Вы уже выполнили эту проверку с помощью:
```sh
sudo ./usertools/dpdk-devbind.py --status
```

2. Убедитесь, что устройство не используется другим драйвером. Для этого сначала отвяжите устройство:
```sh
sudo ./usertools/dpdk-devbind.py -u 0000:01:00.0
```

3. Привязка интерфейса к драйверу ядра Для привязки устройства обратно к ядру (например, драйверу ixgbe), выполните команду:
```sh
sudo ./usertools/dpdk-devbind.py -b ixgbe 0000:01:00.0
```

## Шаг 2: Проверка статуса NIC

1. Проверка сетевого интерфейса Убедитесь, что интерфейс активен:
```sh
ip link show
```

2. Если интерфейс в состоянии DOWN, активируйте его:
```sh
sudo ip link set <имя_интерфейса> up
```
