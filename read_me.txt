Ready data

Complete kaldi configuration
 


Refer to file 'kaldi_segment'

Run The corresponding script

Compute weight process:
************************
run : run_fbank.sh 

produce feats.scp

prepare genuine.scp and replay.scp
(use search_according_to_first_key.pl)

use kaldi command copy-feats

copy-feats scp:genuine.scp ark,t:genuine.ark
copy-feats scp:replay.scp ark,t:replay.ark

run :compute_fw.py 
compute weight over
*************************


