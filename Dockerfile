FROM alpine:latest AS builder

RUN apk add --no-cache alpine-sdk

WORKDIR np
COPY . .
RUN make

FROM alpine:latest
COPY --from=builder np/netpipe .
