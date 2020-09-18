FROM alpine:latest AS builder

RUN apk update && apk add g++ make

COPY . /usr/src/
WORKDIR /usr/src/

RUN make -e

FROM alpine:latest

COPY --from=builder /usr/src/torrent_tree /usr/src/flatten_tree /usr/src/
WORKDIR /usr/src/
