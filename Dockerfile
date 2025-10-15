FROM debian AS build
WORKDIR /opt/pond
COPY . .
RUN apt-get update && \
  apt-get install -y build-essential ncurses-dev && \
  make && \
  make install
ENTRYPOINT ["/opt/pond/bin/pond"]

FROM debian AS final
COPY --from=build /opt/pond/bin/pond /app/pond
RUN apt-get update && \
  apt-get install -y libncurses6 && \
  apt-get clean
WORKDIR /app
ENTRYPOINT ["/app/pond"]