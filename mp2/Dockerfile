FROM gcc:latest
RUN apt-get update && apt-get install -y valgrind
COPY ./docker/entrypoint.sh /
RUN chmod +x entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
