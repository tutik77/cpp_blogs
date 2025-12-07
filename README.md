# Blog Platform (C++ / Drogon)

Простая блог-платформа на C++ с использованием фреймворка Drogon. Состоит из двух микросервисов (AuthService, AppService), PostgreSQL баз данных и современного Vanilla JS фронтенда.

[![Docker](https://img.shields.io/badge/Docker-Enabled-blue)](https://www.docker.com/)
[![C++](https://img.shields.io/badge/C%2B%2B-17/20-brightgreen)](https://isocpp.org/)
[![Drogon](https://img.shields.io/badge/Drogon-WebFramework-orange)](https://github.com/drogonframework/drogon)

## Архитектура

Frontend (localhost:8080) <-> AppService (localhost:3001) <-> AuthService (localhost:3000)
AppService -> Postgres (app_service)
AuthService -> Postgres (auth_service)

## Быстрый запуск (все сервисы)

### Предварительные требования
- Docker Desktop с docker-compose
- Git Bash / WSL / PowerShell
- Порты 3000, 3001, 8080 свободны

### 1. AuthService (порт 3000)
- cd AuthServiceDrogon﻿
- docker compose up --build﻿
- Healthcheck: http://localhost:3000/v1/Auth/healthcheck

### 2. AppService (порт 3001) 
- cd AppService﻿
- docker compose up --build﻿
- API: http://localhost:3001

### 3. Frontend (порт 8080)
- cd frontend﻿
- python -m http.server 8080
Открыть: http://localhost:8080

## Детальные инструкции

### AuthService (порт 3000)

#### Быстрый запуск (одна команда)
- cd C:\Users\Николай\Documents\prog\blog_platform_cpp\AuthServiceDrogon﻿
- docker compose up --build
- git clone <URL-РЕПОЗИТОРИЯ>﻿
- cd AuthServiceDrogon﻿
- docker-compose -f docker-compose-dev.yaml up -d﻿
- type migrations\create_user_table_migration_09_11_1624.sql | docker exec -i AuthServiceTable psql -U root -d auth_service﻿
- docker build -t auth_service .﻿
- docker run --rm -p 3000:3000 --network authservicedrogon_postgres --name auth_service_container auth_service

#### Эндпоинты
- GET /v1/Auth/healthcheck
- POST /v1/Auth/reg - регистрация  
- POST /v1/Auth/login - вход

### AppService (порт 3001)

#### Требования
- Docker Desktop запущен
- AuthService работает на http://localhost:3000

#### Запуск
- cd "C:/Users/Николай/Documents/prog/blog_platform_cpp/AppService"﻿
- docker compose up --build

#### Проверка
- bash tests/smoke_tests.sh
- Результат: All smoke tests passed ✔

### Frontend

#### Запуск
- cd frontend﻿
- start index.html﻿
или
- python -m http.server 8080
- Открыть: http://localhost:8080

## Полная проверка
1. curl http://localhost:3000/v1/Auth/healthcheck
2. bash AppService/tests/smoke_tests.sh  
3. Открыть http://localhost:8080
4. Регистрация → Пост → Лайк → Комментарий


## Команды
- docker compose logs auth_service﻿
- docker compose logs app_service﻿
- docker compose logs postgres_app﻿
- docker compose down
  
## Технологии
- Backend: C++17/20, Drogon, PostgreSQL
- Frontend: Vanilla JS (ES6+), CSS, HTML5, Fetch API
- DevOps: Docker, docker-compose
