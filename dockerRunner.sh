#!/bin/sh
if [ $RUN_TESTS = "1" ]; then
    cmake .; make TradeEngine_test && echo && ctest -VV;
else
    cmake .; make trader && echo && catchsegv ./trader BTC;
fi
