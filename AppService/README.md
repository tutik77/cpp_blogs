## Blog platform (C++ / Drogon) — запуск и проверка

### Требования

- **Docker / Docker Desktop** установлен и запущен.
- Сервис аутентификации уже работает на хосте по адресу `http://localhost:3000`.
- Git Bash, WSL или другая оболочка, где можно запускать `bash` (для скрипта тестов).

### Запуск приложения в Docker

1. Перейди в директорию проекта:

   ```bash
   cd "C:/Users/Николай/Documents/prog/blog_platform_cpp/AppService"
   ```

2. Собери и подними сервисы (Postgres + app_service):

   ```bash
   docker compose up --build
   ```

   Это:
   - Соберёт Docker-образ для `app_service` (C++/Drogon).
   - Поднимет контейнер `postgres_app` с БД `app_service` и применит миграции из `migrations/001_initial_schema.sql`.
   - Поднимет контейнер `app_service` и пробросит порт **3001** на хост.

3. После успешного старта API блога будет доступен по адресу:

   ```text
   http://localhost:3001
   ```

### Быстрая проверка работы (smoke-тесты)

В репозитории есть простой скрипт smoke-тестов на `curl`:

```bash
tests/smoke_tests.sh
```

Он проверяет:

- `GET /users/1/followers` — публичный эндпоинт без авторизации (ожидает HTTP 200 и корректный JSON).
- `GET /feed` без заголовка `Authorization` — должен вернуть HTTP 401 (проверка фильтра авторизации).

#### Как запустить smoke-тесты

1. Убедись, что Docker-сервисы уже запущены:

   ```bash
   docker compose up --build
   # или, если уже собирал, достаточно:
   docker compose up
   ```

2. В отдельном терминале в корне проекта запусти:

   ```bash
   bash tests/smoke_tests.sh
   ```

3. При успешной проверке увидишь сообщение:

   ```text
   All smoke tests passed ✔
   ```

### Полезные команды

- **Посмотреть логи приложения:**

  ```bash
  docker compose logs app_service
  ```

- **Посмотреть логи Postgres:**

  ```bash
  docker compose logs postgres_app
  ```

- **Остановить и удалить контейнеры:**

  ```bash
  docker compose down
  ```


