FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y --no-install-recommends g++-8 gcc-8 make pkg-config build-essential cmake

WORKDIR /root
COPY CMakeLists.txt CMakeLists.txt
RUN echo "target_link_libraries(bombtag -lstdc++ -lstdc++fs)" >> CMakeLists.txt
COPY main.cpp main.cpp

RUN mkdir /root/build
WORKDIR /root/build
RUN CC=gcc-8 CXX=g++-8 cmake .. && make

CMD ["bash"]
