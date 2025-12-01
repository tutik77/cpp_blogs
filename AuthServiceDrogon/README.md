## Запуск AuthService (Docker, Windows)

### Предварительные требования

- **Docker Desktop** (включая `docker-compose`).

### Вариант 1. Один шаг через docker compose

1. Перейти в каталог сервиса:

   ```powershell
   cd C:\Users\Николай\Documents\prog\blog_platform_cpp\AuthServiceDrogon
   ```

2. Запустить Postgres + AuthService одной командой:

   ```powershell
   docker compose up --build
   ```

   Будет поднят Postgres с БД `auth_service`, автоматически применена миграция `create_user_table_migration_09_11_1624.sql` и собран/запущен контейнер `auth_service` на порту `3000`.

### Вариант 2. Ручные шаги (как раньше)

1. **Клонировать репозиторий и перейти в папку проекта**

   ```powershell
   git clone <URL-НА-РЕПОЗИТОРИЙ>
   cd AuthServiceDrogon
   ```

2. **Поднять Postgres и pgAdmin**

   ```powershell
   docker-compose -f docker-compose-dev.yaml up -d
   ```

3. **Создать таблицу `users` (миграция, нужна только при первом запуске)**

   ```powershell
   type migrations\create_user_table_migration_09_11_1624.sql | docker exec -i AuthServiceTable psql -U root -d auth_service
   ```

4. **Собрать Docker-образ сервиса**

   ```powershell
   docker build -t auth_service .
   ```

5. **Запустить сервис аутентификации**

   ```powershell
   docker run --rm -p 3000:3000 --network authservicedrogon_postgres --name auth_service_container auth_service
   ```

6. **Проверить, что сервис жив (смоук‑тесты)**

   В новом окне PowerShell:

   ```powershell
   cd C:\Users\Николай\Documents\prog\blog_platform_cpp\AuthServiceDrogon
   .\tests\smoke_auth.ps1
   ```

### Эндпоинты

- **Healthcheck**: `GET /v1/Auth/healthcheck`
- **Регистрация**: `POST /v1/Auth/reg`
- **Логин**: `POST /v1/Auth/login`


