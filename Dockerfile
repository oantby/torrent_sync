FROM ubuntu:latest

RUN apt-get -y update
RUN apt-get -y install clang make

COPY . /usr/src/
WORKDIR /usr/src/

RUN CXX=clang++ make -e
CMD ["/bin/bash"]

