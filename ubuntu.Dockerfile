FROM ubuntu:24.04 AS build

SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      apt-transport-https \
      ca-certificates \
      git \
      gnupg \
      ninja-build \
      software-properties-common \
      wget \
  && wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key \
      | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
  && add-apt-repository \
      "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-18 main" \
  && wget -nv -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
      | gpg --dearmor - \
      | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' \
      | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      clang-18 \
      cmake \
      libc++-18-dev \
      libc++abi-18-dev \
  && rm -rf /var/lib/apt/lists/*

COPY . /src/

ENV GITHUB_ACTIONS=true

RUN mkdir /build \
  && cmake \
      -GNinja -S /src -B /build \
      --preset=clang-18-release \
      -DNO_BUILDCACHE=ON \
  && cmake \
      --build /build \
      --target ppr-preprocess ppr-backend footrouting \
  && install -t /ppr -D \
      /build/ppr-preprocess \
      /build/ppr-backend \
      /build/footrouting \
  && cp -r /src/ui /ppr/ \
  && rm -rf /build



FROM ubuntu:24.04

SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      gnupg \
      software-properties-common \
      wget \
      tini \
  && wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key \
      | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
  && add-apt-repository \
      "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-18 main" \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      libc++1-18 \
      libc++abi1-18 \
  && DEBIAN_FRONTEND=noninteractive apt-get purge --auto-remove -y \
      gnupg \
      software-properties-common \
      wget \
  && rm -rf /var/lib/apt/lists/* \
  && useradd --user-group --create-home --shell /bin/bash ppr

COPY --from=build /ppr /ppr

WORKDIR /ppr
USER ppr

EXPOSE 8000
VOLUME ["/data"]

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/ppr/ppr-backend", "-c", "/data/config.ini"]
