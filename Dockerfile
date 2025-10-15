FROM alpine AS build
WORKDIR /opt/pond
COPY . .
RUN apk add --update alpine-sdk ncurses-dev && \
  make

FROM alpine AS final
COPY --from=build /opt/pond/bin/pond /app/pond
RUN apk add --update --no-cache ncurses

WORKDIR /app
ENTRYPOINT ["/app/pond"]