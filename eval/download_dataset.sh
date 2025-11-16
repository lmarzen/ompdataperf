#!/bin/bash

cd "$(dirname "$0")"
wget http://www.cs.virginia.edu/~skadron/lava/rodinia/Packages/rodinia_3.1.tar.bz2
tar -xvjf rodinia_3.1.tar.bz2 -C . --strip-components=1 rodinia_3.1/data
rm rodinia_3.1.tar.bz2

