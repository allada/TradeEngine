#!/bin/sh
if [ "$RUN_BASH" = "1" ]; then
    bash
elif [ "$RUN_TESTS" = "1" ]; then
    cmake .; make TradeEngine_test && echo && ctest -VV;
elif [ "$VALGRIND" = "1" ]; then
    cmake .; make trader && echo && valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./trader BTC;
else
    cmake .; make trader && echo && catchsegv ./trader BTC;
fi
