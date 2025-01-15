# Установка Gitlab-runner и Container Registry

Работа с демоном gitlab-runner
```sh
sudo systemctl status gitlab-runner

sudo systemctl start gitlab-runner

sudo systemctl stop gitlab-runner

sudo systemctl enable gitlab-runner

sudo gitlab-runner register

sudo systemctl restart gitlab-runner
```

## Добавление и регистрация `gitlab-runner`

1. **Запуск контейнера с GitLab Runner**: 
```sh
docker run -d --name gitlab-runner --restart always \
  -v /srv/gitlab-runner/config:/etc/gitlab-runner \
  -v /var/run/docker.sock:/var/run/docker.sock \
  gitlab/gitlab-runner:latest
```

2. **Регистрация GitLab Runner в GitLab**:
```sh
docker run --rm -it \
  -v /srv/gitlab-runner/config:/etc/gitlab-runner \
  gitlab/gitlab-runner:latest register
```

3. **Поменять поле `volumes` в конфиге `config.toml` на `["/var/run/docker.sock:/var/run/docker.sock", "/cache"]`**:
```sh
sudo nano /srv/gitlab-runner/config/config.toml
```

## Поднятие `Container Registry` в gitlab

```sh
docker login cr.ppg.secdev.space
docker build -t cr.ppg.secdev.space/cert-reports/fuzzing_extensions/pg_exten_fuzz_image -f Dockerfile .
docker push cr.ppg.secdev.space/cert-reports/fuzzing_extensions/pg_exten_fuzz_image
# токен нейм : user
# токен доcтупа : ************************
```

## Поднятие локального `Docker Registry` (по необходимости)

1. **Запустить `Docker Registry`**:
```sh
docker run -d -p 5000:5000 --name registry registry:2
```

2. **Настроить доверие к небезопасному реестру (если не используется HTTPS)**:

    Если ваш GitLab Runner и Docker-клиенты находятся на других машинах, добавьте ваш Registry в список доверенных реестров. Если вы не используете HTTPS, Docker требует явного доверия.

    Откройте файл `/etc/docker/daemon.json` и добавьте IP-адрес вашего Docker Registry в секцию `insecure-registries`.

    ```json
    {
      "insecure-registries": ["192.168.50.***:5000"]
    }
    ```

3. **Чтобы изменения вступили в силу перезапустите демон docker**:
```sh
sudo systemctl restart docker
```

4. **Тегирование и отправка образа в Registry**:
```sh
docker build -t my-image:latest .

docker tag my-image:latest <ваш IP>:5000/my-image:latest

docker push <ваш IP>:5000/my-image:latest
```

5. **Загрузка образа из Registry**:
```sh
docker pull <ваш IP>:5000/my-image:latest
docker pull 192.168.50.***:5000/fuzz_orafce_image
# Убедитесь, что образ успешно загружен в реестр по адресу 192.168.50.***:5000/fuzz_orafce_image
curl -s http://192.168.50.***:5000/v2/fuzz_orafce_image/tags/list
```

# Отправка изменений на gitlab

```sh
git add .

git commit -m "update yaml"

git push ssh-origin orafce-4.10.3
```

===========================================================================

# Отладка в контейнерах

## 1 этап
1. **Проверка установленного расширения в докере**: 
```sh
docker run --rm --name cov -it cr.ppg.secdev.space/cert-reports/fuzzing_extensions/orafce_cov_image /bin/bash
```

2. **Подключение к бд в сингл моде**:
```sh
/usr/local/pgsql_coverage/bin/postgres --single -D /usr/local/pgsql/data postgres
```
3. **Изменить путь поиска схем (search_path) для текущей сессии**:
```sh
SET search_path TO oracle, public;
```

4. **Явно указать типы данных при вызове функции**:
```sh
SELECT orafce_concat2('Hello'::oracle.varchar2, ' World'::oracle.varchar2);
```

## 2 этап локальная отладка на хосте 

1. **Установка postgres с символами afl на хосте**:
```sh
git clone https://github.com/postgres/postgres postgres_afl
cd postgres_afl
git checkout REL_16_4
export ASAN_OPTIONS=detect_leaks=0
CC=afl-clang-fast CXX=afl-clang-fast++ CFLAGS="-fsanitize=address,undefined" ./configure --prefix=/usr/local/pgsql --enable-debug --enable-cassert
make -j$(nproc)
make install
```

2. **Создаём директорию для кластера бд**:
```sh
mkdir -p /usr/local/pgsql/data
chown postgres:postgres /usr/local/pgsql/data
```

3. **Конфигурация бд перед фазингом**:
```sh
sudo su - postgres
/usr/local/pgsql/bin/initdb -D /usr/local/pgsql/data
touch /usr/local/pgsql/data/logfile
/usr/local/pgsql/bin/pg_ctl -D /usr/local/pgsql/data -l /usr/local/pgsql/data/logfile start
# Применяем изменения в конфигурации
sed -i "s/local   all             all                                     md5/local   all             all                                     trust/" /usr/local/pgsql/data/pg_hba.conf
# Останавливаем сервер после применения изменений
echo "session_preload_libraries = 'orafce.so'" >> /usr/local/pgsql/data/postgresql.conf
```

4. **Сборка расширения orafce**: 
```sh
exit
make CC=afl-clang-fast USE_PGXS=1 PG_CONFIG=/usr/local/pgsql/bin/pg_config CFLAGS="-g -O0" clean && \
make CC=afl-clang-fast USE_PGXS=1 PG_CONFIG=/usr/local/pgsql/bin/pg_config CFLAGS="-g -O0" -j$(nproc) && \
make CC=afl-clang-fast USE_PGXS=1 PG_CONFIG=/usr/local/pgsql/bin/pg_config CFLAGS="-g -O0" install
```
**Использование `session_preload_libraries` и функции `_PG_init`**

Фаззинг обертка при этом должна быть помещена в функцию `_PG_init()` в расширения, которая автоматически выполняется при загрузке библиотеки. 

**Добавьте ваше расширение в `postgresql.conf`** через параметр `session_preload_libraries`:

Откройте файл конфигурации PostgreSQL `postgresql.conf`:

```sh
sudo nano /usr/local/pgsql/data/postgresql.conf
```

Найдите строку с параметром `session_preload_libraries` и добавьте ваше расширение:

```bash
session_preload_libraries = 'my_extension'
```

5. **Создадим директории для корпуса и артефактов**:
```sh
mkdir -p in out && \
```

6. **Запуск фазинга**:
```sh
sudo -u postgres psql -d postgres -c "CREATE EXTENSION orafce;"
sudo -u postgres gdb --args /usr/local/pgsql/bin/postgres --single -D /usr/local/pgsql/data postgres

ENV ASAN_OPTIONS=detect_leaks=0:abort_on_error=1:symbolize=0
ENV AFL_IGNORE_PROBLEMS=1
ENV AFL_IGNORE_PROBLEMS_COVERAGE=1
ENV AFL_PRELOAD=/usr/local/pgsql/lib/orafce.so
ENV AFL_EXIT_ON_TIME: 7200

sudo -u postgres afl-fuzz -i in/ -o out/ -t 1000 -- /usr/local/pgsql_afl/bin/postgres --single -D /usr/local/pgsql/data postgres
```