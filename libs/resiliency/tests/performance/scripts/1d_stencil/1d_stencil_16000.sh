#!/bin/bash

# Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
#                    LLC (NTESS).
# Copyright (c) 2019 Nikunj Gupta
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

iterations='8192'
n_value='1000'
cores='32 16'

for core in $cores
do
    for iteration in $iterations
    do
        for error_rate in {3..10}
        do
            for total in {1..10}
            do
                .../../..//bin/1d_stencil_replay_benchmarks --n-value=${n_value} -t${core} --iteration=${iteration} --subdomain-width=16000 --subdomains=128 --steps-per-iteration=128 --error-rate=${error_rate} >> 1d_stencil/1d_stencil_${core}_16000_${total}.txt
                .../../..//bin/1d_stencil_checksum_benchmarks --n-value=${n_value} -t${core} --iteration=${iteration} --subdomain-width=16000 --subdomains=128 --steps-per-iteration=128 --error-rate=${error_rate} >> 1d_stencil/1d_stencil_${core}_16000_${total}.txt
                .../../..//bin/1d_stencil_replicate_benchmarks --n-value=${n_value} -t${core} --iteration=${iteration} --subdomain-width=16000 --subdomains=128 --steps-per-iteration=128 --error-rate=${error_rate} >> 1d_stencil/1d_stencil_${core}_16000_${total}.txt
                echo "done 16000_${iteration}_${error_rate}_${core}_${total}"
            done
        done
    done
done