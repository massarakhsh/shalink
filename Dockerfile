FROM golang:1.25.5-alpine3.23 AS builder
ARG VERSION=${CI_COMMIT_TAG}
ARG GITLAB_CI_TOKEN=${GITLAB_CI_TOKEN}
WORKDIR /build
RUN apk add --no-cache git
COPY go.mod go.sum ./
RUN git config --global url."https://vcs.bingo-boom.ru/".insteadOf "git@vcs.bingo-boom.ru:" 
RUN echo "machine vcs.bingo-boom.ru login gitlab-ci password ${GITLAB_CI_TOKEN}" > ~/.netrc
RUN export GOPRIVATE=vcs.bingo-boom.ru/*
RUN go mod download -x
COPY . .
RUN GOOS=linux GOARCH=amd64 go build -ldflags "-X 'main.Version=${VERSION}' -s -w" -o shalink .

FROM alpine:3.23
WORKDIR /app
COPY --from=builder /build/shalink /app/shalink
ENTRYPOINT ["/app/shalink"]
