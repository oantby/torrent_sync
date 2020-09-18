FROM alpine:latest

RUN apk update && apk add g++ make

COPY . /usr/src/
WORKDIR /usr/src/

RUN make -e

