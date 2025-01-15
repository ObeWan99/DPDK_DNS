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
# токен нейм : obewan-run
# токен доcтупа : glpat-ct5vpM_gsz6sdpPJ-pDV
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
      "insecure-registries": ["192.168.50.102:5000"]
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
docker pull 192.168.50.102:5000/fuzz_orafce_image
# Убедитесь, что образ успешно загружен в реестр по адресу 192.168.50.102:5000/fuzz_orafce_image
curl -s http://192.168.50.102:5000/v2/fuzz_orafce_image/tags/list
```

# Отправка изменений на gitlab

```sh
git add .

git commit -m "update yaml"

git push ssh-origin orafce-4.10.3
```