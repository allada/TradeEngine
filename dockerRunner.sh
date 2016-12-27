#!/bin/sh
PARENT_PROGRAM="catchsegv"
if [ "$VALGRIND" = "1" ]; then
    PARENT_PROGRAM="valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all -v"
fi

if [ "$RUN_BASH" = "1" ]; then
    bash
elif [ "$RUN_TESTS" = "1" ]; then
    cmake .; make TradeEngine_test && echo && $PARENT_PROGRAM ctest -VV;
else
    cmake .; make trader && echo && $PARENT_PROGRAM ./trader BTC;
fi
