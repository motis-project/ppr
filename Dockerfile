FROM ubuntu:20.04 AS build

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
  && wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
  && add-apt-repository \
      "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-11 main" \
  && wget -nv -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
      | gpg --dearmor - \
      | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
  && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main' \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      clang-11 \
      cmake \
      libc++-11-dev \
      libc++abi-11-dev \
  && rm -rf /var/lib/apt/lists/*

COPY . /src/

RUN mkdir /build \
  && cmake \
      -GNinja -S /src -B /build \
      -DCMAKE_C_COMPILER=/usr/bin/clang-11 \
      -DCMAKE_CXX_COMPILER=/usr/bin/clang++-11 \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
      -DCMAKE_BUILD_TYPE=Release \
      -DNO_BUILDCACHE=ON \
  && cmake \
      --build /build \
      --target ppr-preprocess ppr-backend \
  && install -t /ppr -D \
      /build/ppr-preprocess \
      /build/ppr-backend \
  && cp -r /src/ui /ppr/ \
  && rm -rf /build



FROM ubuntu:20.04

SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      gnupg \
      software-properties-common \
      wget \
  && wget -nv -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
  && add-apt-repository \
      "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-11 main" \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qq \
      --no-install-recommends \
      libc++1-11 \
      libc++abi1-11 \
  && DEBIAN_FRONTEND=noninteractive apt-get purge --auto-remove -y \
      gnupg \
      software-properties-common \
      wget \
  && rm -rf /var/lib/apt/lists/* \
  && useradd --user-group --create-home --shell /bin/bash ppr

COPY --from=build /ppr /ppr

WORKDIR /ppr
USER ppr

CMD ["/bin/bash"]
