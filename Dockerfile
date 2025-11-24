FROM alpine:3.20 AS builder
WORKDIR /src
RUN apk update && \
    apk add --no-cache gcc musl-dev hiredis-dev make linux-headers

COPY service/main.c .
RUN gcc -o app main.c -lhiredis

FROM alpine:3.20
WORKDIR /app
RUN apk update && \
    apk add --no-cache hiredis
COPY --from=builder /src/app .
RUN chmod +x app
