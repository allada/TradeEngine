FROM trader_base_1.1006

WORKDIR /trader

RUN rm -R /trader/CMakeFiles &> /dev/null
RUN rm /trader/CMakeCache.txt &> /dev/null

VOLUME ["/trader"]

ENTRYPOINT bash -c "cmake .; make && echo && catchsegv ./trader BTC"
