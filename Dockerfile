# Build TIVIDY GUI

FROM ubuntu:22.04 AS build

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN make tividy_gui


# Run TIVIDY GUI

FROM ubuntu:22.04 AS runtime

WORKDIR /app

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/tividy_gui ./tividy_gui

COPY --from=build /app/web ./web

EXPOSE 8080

CMD ["./tividy_gui", "--port", "8080", "--web", "/app/web"]
