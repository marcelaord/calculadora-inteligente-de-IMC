# Imagen base con Drogon + Trantor ya compilados (v1.9.13) e instalados.
FROM drogonframework/drogon:latest AS app

# Fix: Ubuntu devuelve 403 a apt (User-Agent "Debian APT-HTTP/1.3").
RUN printf 'Acquire::http::User-Agent "HealthIQ-build (docker)";\n' \
        > /etc/apt/apt.conf.d/99useragent

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)"

ENV HEALTHIQ_HOST=0.0.0.0 \
    HEALTHIQ_PORT=8080 \
    HEALTHIQ_THREADS=4 \
    HEALTHIQ_LOG_LEVEL=info

EXPOSE 8080

CMD ["/src/build/bin/healthiq-backend"]
