# Base container with tools dependencies for the JensenLab tagger
#
# VERSION       1

FROM centos:7.2.1511
MAINTAINER lars.juhl.jensen@gmail.com

# install base dependencies
RUN yum -y install git swig gcc gcc-c++ make python-devel boost boost-devel

WORKDIR /app

# clone and build tagger
RUN git clone https://github.com/larsjuhljensen/tagger.git \
    && cd tagger \
    && make

VOLUME /data
WORKDIR /data
ENTRYPOINT ["/app/tagger/tagcorpus"]
