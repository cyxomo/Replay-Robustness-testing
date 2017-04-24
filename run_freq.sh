#!/bin/bash
 
. ./cmd.sh
. ./path.sh
set -e

config=freq.conf

cleanup=true
#number of components
cnum=1024
civ=50
clda=30
dpath=data/digit
rpath=data/replay

if (( 1 == 1 ));then

freqdir=`pwd`/_freq
vaddir=`pwd`/_freq

set -e

steps/make_freq.sh --freq-config conf/$config --nj 20 --cmd "$train_cmd" \
    $dpath/UBM log/_log_freq $freqdir
steps/make_freq.sh --freq-config conf/$config --nj 20 --cmd "$train_cmd" \
    $dpath/Dev log/_log_freq $freqdir

sid/compute_vad_decision.sh --vad-config conf/vad.conf --nj 10 --cmd "$train_cmd" \
    $dpath/UBM log/_log_vad $vaddir
sid/compute_vad_decision.sh --vad-config conf/vad.conf --nj 10 --cmd "$train_cmd" \
    $dpath/Dev log/_log_vad $vaddir

utils/subset_data_dir.sh $dpath/UBM 2000 $dpath/UBM_2k
utils/subset_data_dir.sh $dpath/UBM 4000 $dpath/UBM_4k

fi

if (( 2 == 2 ));then

for phone in 1 2 3 4 5 6 ;do

	freqdir=`pwd`/_freq_${phone}
	vaddir=`pwd`/_freq_${phone}

	for data in model raw replay; do
		steps/make_freq.sh --freq-config conf/$config --nj 10 --cmd "$train_cmd" $rpath/$phone/$data log/_log_freq $freqdir
		sid/compute_vad_decision.sh --vad-config conf/vad.conf --nj 4 --cmd "$train_cmd" $rpath/$phone/$data log/_log_vad $vaddir
	done
done

fi

##### overall speech segments #####
if (( 3 == 3 ));then

sid/train_diag_ubm_novad.sh --nj 10 --cmd "$train_cmd" $dpath/UBM_2k $cnum exp_novad/diag_ubm_${cnum}
sid/train_full_ubm_novad.sh --nj 10 --cmd "$train_cmd" $dpath/UBM_4k exp_novad/diag_ubm_${cnum} exp_novad/full_ubm_${cnum}

# Train the iVector extractor.
sid/train_ivector_extractor_novad.sh --nj 10 --cmd "$train_cmd" \
  --num-iters 6 --ivector_dim $civ exp_novad/full_ubm_${cnum}/final.ubm $dpath/UBM \
  exp_novad/extractor_${cnum}_${civ}

# Extract the iVectors.

sid/extract_ivectors_novad.sh --cmd "$train_cmd" --nj 20 \
   exp_novad/extractor_${cnum}_${civ} $dpath/Dev exp_novad/ivectors_dev_${cnum}_${civ}

for phone in 1 2 3 4 5 6 ;do
        for data in model raw replay; do
                sid/extract_ivectors_novad.sh --cmd "$train_cmd" --nj 20 \
                   exp_novad/extractor_${cnum}_${civ} $rpath/$phone/$data \
                   exp_novad/ivectors_${phone}_${data}_${cnum}_${civ}
        done
done

### Demonstrate what happens if we reduce the dimension with LDA

ivector-compute-lda --dim=$clda  --total-covariance-factor=0.1 \
 "ark:ivector-normalize-length scp:exp_novad/ivectors_dev_${cnum}_${civ}/ivector.scp ark:- |" ark:$dpath/Dev/utt2spk \
   exp_novad/ivectors_dev_${cnum}_${civ}/transform_${clda}.mat

### PLDA training
ivector-compute-plda ark:$dpath/Dev/spk2utt \
  "ark:ivector-normalize-length scp:exp_novad/ivectors_dev_${cnum}_${civ}/ivector.scp  ark:- |" \
    exp_novad/ivectors_dev_${cnum}_${civ}/plda 2>exp_novad/ivectors_dev_${cnum}_${civ}/log/plda.log

### LDA-PLDA training
ivector-compute-plda ark:$dpath/Dev/spk2utt \
  "ark:ivector-normalize-length scp:exp_novad/ivectors_dev_${cnum}_${civ}/ivector.scp ark:- | ivector-transform exp_novad/ivectors_dev_${cnum}_${civ}/transform_${clda}.mat ark:- ark:- | ivector-normalize-length ark:- ark:- |" \
    exp_novad/ivectors_dev_${cnum}_${civ}/plda_lda 2>exp_novad/ivectors_dev_${cnum}_${civ}/log/plda_lda.log

fi


### scoring ###
if (( 4 == 4 ));then

for phone in 1 2 3 4 5 6 ;do
echo Test on phone: $phone

mkdir -p exp_novad/ivectors_${phone}_verify_${cnum}_${civ}
cat exp_novad/ivectors_${phone}_raw_${cnum}_${civ}/ivector.scp exp_novad/ivectors_${phone}_replay_${cnum}_${civ}/ivector.scp > exp_novad/ivectors_${phone}_verify_${cnum}_${civ}/ivector.scp

### Cosine distance ####
trials=$rpath/$phone/a.trials
cat $trials | awk '{print $1, $2}' | \
 ivector-compute-dot-products - \
  scp:exp_novad/ivectors_${phone}_model_${cnum}_${civ}/spk_ivector.scp \
  scp:exp_novad/ivectors_${phone}_verify_${cnum}_${civ}/ivector.scp \
   score/total/foo_cosine_${phone}

python local/format.py score/total/foo_cosine_${phone} $phone score/total/foo_cosine_${phone}.trials
local/score.sh score/total/foo_cosine_${phone}.trials

### LDA distance ####
trials=$rpath/$phone/a.trials
cat $trials | awk '{print $1, $2}' | \
 ivector-compute-dot-products - \
  "ark:ivector-transform exp_novad/ivectors_dev_${cnum}_${civ}/transform_${clda}.mat scp:exp_novad/ivectors_${phone}_model_${cnum}_${civ}/spk_ivector.scp ark:- | ivector-normalize-length ark:- ark:- |" \
  "ark:ivector-transform exp_novad/ivectors_dev_${cnum}_${civ}/transform_${clda}.mat scp:exp_novad/ivectors_${phone}_verify_${cnum}_${civ}/ivector.scp ark:- | ivector-normalize-length ark:- ark:- |" \
  score/total/foo_lda_${phone}

python local/format.py score/total/foo_lda_${phone} $phone score/total/foo_lda_${phone}.trials
local/score.sh score/total/foo_lda_${phone}.trials

### PLDA scoring ###

trials=$rpath/$phone/a.trials
ivector-plda-scoring --num-utts=ark:exp_novad/ivectors_${phone}_model_${cnum}_${civ}/num_utts.ark \
   "ivector-copy-plda --smoothing=0.0 exp_novad/ivectors_dev_${cnum}_${civ}/plda - |" \
   "ark:ivector-subtract-global-mean scp:exp_novad/ivectors_${phone}_model_${cnum}_${civ}/spk_ivector.scp ark:- |" \
   "ark:ivector-subtract-global-mean scp:exp_novad/ivectors_${phone}_verify_${cnum}_${civ}/ivector.scp ark:- |" \
   "cat '$trials' | awk '{print \$1, \$2}' |" score/total/foo_plda_${phone}

python local/format.py score/total/foo_plda_${phone} $phone score/total/foo_plda_${phone}.trials
local/score.sh score/total/foo_plda_${phone}.trials

### LDA-PLDA scoring ####

trials=$rpath/$phone/a.trials
ivector-plda-scoring --num-utts=ark:exp_novad/ivectors_${phone}_model_${cnum}_${civ}/num_utts.ark \
   "ivector-copy-plda --smoothing=0.0 exp_novad/ivectors_dev_${cnum}_${civ}/plda_lda - |" \
   "ark:ivector-transform exp_novad/ivectors_dev_${cnum}_${civ}/transform_${clda}.mat scp:exp_novad/ivectors_${phone}_model_${cnum}_${civ}/spk_ivector.scp ark:- | ivector-normalize-length ark:- ark:- | ivector-subtract-global-mean ark:- ark:- |" \
   "ark:ivector-transform exp_novad/ivectors_dev_${cnum}_${civ}/transform_${clda}.mat scp:exp_novad/ivectors_${phone}_verify_${cnum}_${civ}/ivector.scp ark:- | ivector-normalize-length ark:- ark:- | ivector-subtract-global-mean ark:- ark:- |" \
   "cat '$trials' | awk '{print \$1, \$2}' |" score/total/foo_plda_lda_${phone}

python local/format.py score/total/foo_plda_lda_${phone} $phone score/total/foo_plda_lda_${phone}.trials
local/score.sh score/total/foo_plda_lda_${phone}.trials

done

cat score/total/foo_cosine_1.trials score/total/foo_cosine_2.trials score/total/foo_cosine_3.trials score/total/foo_cosine_4.trials score/total/foo_cosine_5.trials score/total/foo_cosine_6.trials > score/total/foo_cosine_all.trials
local/score.sh score/total/foo_cosine_all.trials

cat score/total/foo_lda_1.trials score/total/foo_lda_2.trials score/total/foo_cosine_3.trials score/total/foo_cosine_4.trials score/total/foo_lda_5.trials score/total/foo_lda_6.trials > score/total/foo_lda_all.trials
local/score.sh score/total/foo_lda_all.trials

cat score/total/foo_plda_1.trials score/total/foo_plda_2.trials score/total/foo_cosine_3.trials score/total/foo_cosine_4.trials score/total/foo_plda_5.trials score/total/foo_plda_6.trials > score/total/foo_plda_all.trials
local/score.sh score/total/foo_plda_all.trials

cat score/total/foo_plda_lda_1.trials score/total/foo_plda_lda_2.trials score/total/foo_cosine_3.trials score/total/foo_cosine_4.trials score/total/foo_plda_lda_5.trials score/total/foo_plda_lda_6.trials > score/total/foo_plda_lda_all.trials
local/score.sh score/total/foo_plda_lda_all.trials

fi

echo Done

if $cleanup; then
  echo Cleaning up data
  rm -r _freq* log score/total/* exp_novad
  rm -r data/digit/UBM_*
  rm -r data/digit/UBM/split*
  rm -r data/digit/UBM/feats.scp data/digit/UBM/vad.scp
  rm -r data/digit/Dev/split*  
  rm -r data/digit/Dev/feats.scp data/digit/Dev/vad.scp
  rm -r data/replay/*/*/split* data/replay/*/*/feats.scp data/replay/*/*/vad.scp
fi

