FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git \
    gcc \
    g++ \
    cmake \
    libjsoncpp-dev \
    uuid-dev \
    openssl \
    libssl-dev \
    zlib1g-dev \
    postgresql-client \
    libpq-dev \
    libc-ares-dev \
    libbrotli-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# Install Drogon
RUN git clone --depth 1 https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && rm -rf drogon

# Install jwt-cpp
RUN git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && \
    mkdir build && cd build && \
    cmake .. -DJWT_BUILD_EXAMPLES=OFF -DJWT_BUILD_TESTS=OFF && \
    make install && \
    cd ../.. && rm -rf jwt-cpp

WORKDIR /app

COPY controllers ./controllers
COPY models ./models
COPY filters ./filters
COPY migrations ./migrations
COPY CMakeLists.txt .
COPY config-docker.json ./config.json
COPY main.cpp .

# Build project
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

RUN mkdir -p uploads

EXPOSE 3001

CMD ["./build/app_service"]
