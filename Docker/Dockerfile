FROM ubuntu:22.04

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    nlohmann-json3-dev \
    valgrind

COPY . .

RUN make

CMD ["./WorkflowEngine"]