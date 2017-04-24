#!/bin/bash
 
. ./cmd.sh
. ./path.sh
set -e

config=fbank.conf

dpath=data/digit

if (( 1 == 1 ));then

fbankdir=`pwd`/_fbank_freq

set -e

steps/make_fbank.sh --fbank-config conf/$config --nj 20 --cmd "$train_cmd" \
    $dpath/UBM log/_log_fbank_freq $fbankdir

fi

