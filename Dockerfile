FROM alpine:3.17 AS build

RUN apk add --no-cache build-base cmake ninja git linux-headers

COPY . /src/

ENV GITHUB_ACTIONS=true

RUN mkdir /build \
  && cmake \
      -G Ninja -S /src -B /build \
      -DCMAKE_BUILD_TYPE=Release \
      -DNO_BUILDCACHE=ON \
      -DPPR_MIMALLOC=ON \
  && cmake \
      --build /build \
      --target ppr-preprocess ppr-backend \
  && install -t /ppr -D \
      /build/ppr-preprocess \
      /build/ppr-backend \
  && cp -r /src/ui /ppr/ \
  && rm -rf /build


FROM alpine:3.17

RUN apk add --no-cache libstdc++ \
  && addgroup -S ppr \
  && adduser -S ppr -G ppr

COPY --from=build /ppr /ppr

WORKDIR /ppr
USER ppr

EXPOSE 8000
VOLUME ["/data"]

CMD ["/ppr/ppr-backend", "-c", "/data/config.ini"]
