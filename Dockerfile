FROM trader_base_1.1008

WORKDIR /trader

RUN rm -R /trader/CMakeFiles &> /dev/null
RUN rm /trader/CMakeCache.txt &> /dev/null

VOLUME ["/trader"]

ENTRYPOINT bash -c "sh ./dockerRunner.sh"
