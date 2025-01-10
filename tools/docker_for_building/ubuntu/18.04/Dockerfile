FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get clean && \
  apt-get update && \
  apt-get install -y --no-install-recommends apt-utils software-properties-common && \
  add-apt-repository ppa:git-core/ppa && \
  apt-get update && \
  apt-get install -y --no-install-recommends curl \
  lsb-release \
  iwyu \
  libssl-dev \
  p7zip-full \
  p7zip-rar \
  git \
  tree \
  zip \
  unzip \
  libasan5 \
  autoconf \
  libtool \
  uuid-dev \
  libmsgpack-dev \
  libmysqlclient-dev \
  libmysqlcppconn-dev \
  libcrypto++-dev \
  libexpat1-dev \
  libxdamage1 \
  make \
  vim \
  libxcomposite-dev \
  libdrm-dev \
  libavformat-dev \
  libavfilter-dev \
  libx264-dev \
  build-essential \
  zlib1g-dev \
  libncurses5-dev \
  libgdbm-dev \
  libnss3-dev \
  libssl-dev \
  libreadline-dev \
  libffi-dev \
  wget \
  gpg-agent

# =======================================
# Installing cmake
#
# The default version of cmake of Ubuntu 18.04 is 3.10, but APTIMA needs cmake version greater than 3.14, so we need to install it from cmake official site.

RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
  && apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" \
  && apt-get update \
  && apt-get install -y --no-install-recommends cmake

# =======================================
# Installing gcc13
#
# NOTE: Install gcc after cmake, while cmake will auto-install gcc7.
# Install gcc13 as default.

RUN apt-get purge -y gcc gcc-7 g++-7 && \
  apt-add-repository -y ppa:ubuntu-toolchain-r/test && \
  apt-get update && \
  apt-get install -y --no-install-recommends gcc-13 g++-13 && \
  apt-get install -y gcc-9-multilib g++-13-multilib && \
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
  update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# =======================================
# Install golang

# /root/go is GOPATH.

ENV PATH="$PATH:/usr/local/go/bin:/root/go/bin"

# APTIMA go binding needs to be compatible with GO 1.20, so we need to install GO
# 1.20 to check the compatibility.
RUN curl -OL https://go.dev/dl/go1.22.3.linux-amd64.tar.gz && \
  rm -rf /usr/local/go && tar -C /usr/local -xvf go1.22.3.linux-amd64.tar.gz && rm go1.22.3.linux-amd64.tar.gz && \
  go install golang.org/dl/go1.20.12@latest && go1.20.12 download

# =======================================
# Install cargo for rust.

RUN curl https://sh.rustup.rs -sSf | sh -s -- -y && \
  /root/.cargo/bin/rustup install nightly && \
  /root/.cargo/bin/rustup default nightly && \
  /root/.cargo/bin/cargo install --force cbindgen

ENV PATH="/root/.cargo/bin:$PATH"

# =======================================
# Installing Python3.10 from source code.

ENV PYTHON_VERSION=3.10.14

RUN wget https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz && \
  tar -zxvf Python-${PYTHON_VERSION}.tgz && \
  cd Python-${PYTHON_VERSION} && \
  ./configure --with-lto --with-computed-gotos --with-system-ffi --prefix=/usr --disable-test-modules --enable-shared && \
  make -j && \
  make altinstall && \
  rm -rf Python-{PYTHON_VERSION}*

RUN ln -sf /usr/bin/python3.10-config /usr/bin/python3-config && \
  ln -sf /usr/bin/python3.10 /usr/bin/python3 && \
  ln -sf /usr/bin/python3.10 /usr/bin/python && \
  ln -sf /usr/bin/pip3.10 /usr/bin/pip3 && \
  cp /usr/lib/python3/dist-packages/lsb_release.py /usr/bin && \
  pip3 install --upgrade setuptools && \
  pip3 install --upgrade pip && \
  pip3 install requests oss2 && \
  pip3 install --use-pep517 python-dotenv jinja2

# =======================================
# Cleanup

RUN apt-get clean && rm -rf /var/lib/apt/lists/* && rm -rf /tmp/*
