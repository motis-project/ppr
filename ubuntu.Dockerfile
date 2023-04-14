FROM ubuntu:22.04 AS build

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
      "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main" \
  && wget -nv -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
      | gpg --dearmor - \
      | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' \
      | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      clang-16 \
      cmake \
      libc++-16-dev \
      libc++abi-16-dev \
  && rm -rf /var/lib/apt/lists/*

COPY . /src/

ENV GITHUB_ACTIONS=true

RUN mkdir /build \
  && cmake \
      -GNinja -S /src -B /build \
      -DCMAKE_C_COMPILER=/usr/bin/clang-16 \
      -DCMAKE_CXX_COMPILER=/usr/bin/clang++-16 \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
      -DCMAKE_BUILD_TYPE=Release \
      -DNO_BUILDCACHE=ON \
      -DPPR_MIMALLOC=ON \
  && cmake \
      --build /build \
      --target ppr-preprocess ppr-backend footrouting \
  && install -t /ppr -D \
      /build/ppr-preprocess \
      /build/ppr-backend \
      /build/footrouting \
  && cp -r /src/ui /ppr/ \
  && rm -rf /build



FROM ubuntu:22.04

SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      gnupg \
      software-properties-common \
      wget \
  && wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key \
      | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
  && add-apt-repository \
      "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main" \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      libc++1-16 \
      libc++abi1-16 \
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

CMD ["/ppr/ppr-backend", "-c", "/data/config.ini"]
