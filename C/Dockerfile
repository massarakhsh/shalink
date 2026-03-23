FROM ubuntu:22.04 as builder
ARG GITLAB_CI_TOKEN=${GITLAB_CI_TOKEN}
RUN apt-get update && apt-get install -y make build-essential
WORKDIR /build
COPY . .
RUN make build

FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /build/shalink /app/

ENTRYPOINT ["shalink"]
