FROM iantorres/boosted:latest

COPY src .

RUN cmake . && \
    make

EXPOSE 8000

ENTRYPOINT ["/srv/bin/Serve"]