#! /bin/sh

createdb conntest

../pg_ddm -d ctest6000.ini
../pg_ddm -d ctest7000.ini

./asynctest

# now run conntest.sh on another console
