FROM ubuntu:latest

RUN apt-get -y update
RUN apt-get -y install clang

COPY . /usr/src/
WORKDIR /usr/src/

CMD /bin/bash

