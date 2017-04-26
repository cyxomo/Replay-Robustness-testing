// feat/freq-computations.cc

// Copyright 2009-2011  Phonexia s.r.o.;  Karel Vesely;  Microsoft Corporation

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <algorithm>
#include <iostream>

#include "feat/feature-functions.h"
#include "feat/feature-window.h"
#include "feat/freq-computations.h"

namespace kaldi {


FreqBanks::FreqBanks(const FreqBanksOptions &opts,
                   const FrameExtractionOptions &frame_opts,
                   BaseFloat vtln_warp_factor):
    htk_mode_(opts.htk_mode) {
  int32 num_bins = opts.num_bins;
  if (num_bins < 3) KALDI_ERR << "Must have at least 3 freq bins";
  BaseFloat sample_freq = frame_opts.samp_freq;
  int32 window_length = static_cast<int32>(frame_opts.samp_freq*0.001*frame_opts.frame_length_ms);
  int32 window_length_padded =
      (frame_opts.round_to_power_of_two ?
       RoundUpToNearestPowerOfTwo(window_length) :
       window_length);
  KALDI_ASSERT(window_length_padded % 2 == 0);
  int32 num_fft_bins = window_length_padded/2;
  BaseFloat nyquist = 0.5 * sample_freq;

  BaseFloat low_freq = opts.low_freq, high_freq;
  if (opts.high_freq > 0.0)
    high_freq = opts.high_freq;
  else
    high_freq = nyquist + opts.high_freq;

  if (low_freq < 0.0 || low_freq >= nyquist
      || high_freq <= 0.0 || high_freq > nyquist
      || high_freq <= low_freq)
    KALDI_ERR << "Bad values in options: low-freq " << low_freq
              << " and high-freq " << high_freq << " vs. nyquist "
              << nyquist;

  BaseFloat fft_bin_width = sample_freq / window_length_padded;
  // fft-bin width [think of it as Nyquist-freq / half-window-length]

  BaseFloat new_low_freq = FreqScale(low_freq);
  BaseFloat new_high_freq = FreqScale(high_freq);

  debug_ = opts.debug_freq;

  // divide by num_bins+1 in next line because of end-effects where the bins
  // spread out to the sides.
  BaseFloat new_freq_delta = (new_high_freq - new_low_freq) / (num_bins + 1);

  bins_.resize(num_bins);
  center_freqs_.Resize(num_bins);

  float ww[40] = {15.963, 3.4, 0.888, 0.173, 0.001, 0.009, 0.001, 0.001, 0.0, 0.002, 0.03, 0.054, 0.04, 0.078, 0.23, 0.519, 0.855, 0.929, 0.901, 1.051, 1.384, 1.535, 1.275, 1.003, 0.952, 0.972, 1.062, 1.05, 0.873, 0.703, 0.542, 0.438, 0.342, 0.19, 0.047, 0.0, 0.034, 0.036, 0.074, 2.34};
  
  for (int32 bin = 0; bin < num_bins; bin++) {
    BaseFloat left_new = new_low_freq + bin * new_freq_delta,
        center_new = new_low_freq + (bin + 1) * new_freq_delta,
        right_new = new_low_freq + (bin + 2) * new_freq_delta;

    //center_freqs_(bin) = InverseFreqScale(center_new);
    // this_bin will be a vector of coefficients that is only
    // nonzero where this new_ bin is active.
    Vector<BaseFloat> this_bin(num_fft_bins);
    int32 first_index = -1, last_index = -1;
    
    for (int32 i = 0; i < num_fft_bins; i++) {
      BaseFloat freq = (fft_bin_width * i);  // Center frequency of this fft
                                             // bin.
      BaseFloat new_ = FreqScale(freq);
      if (new_ > left_new && new_ < right_new) {
        BaseFloat weight;

        if (new_ <= center_new)
          weight = ww[bin] * (new_ - left_new) / (center_new - left_new);
          //weight = (new_ - left_new) / (center_new - left_new);
        else
          weight = ww[bin] * (right_new - new_) / (right_new - center_new);
          //weight = (right_new - new_) / (right_new - center_new);
        this_bin(i) = weight;
        if (first_index == -1)
          first_index = i;
        last_index = i;
      }
    }
    KALDI_ASSERT(first_index != -1 && last_index >= first_index
                 && "You may have set --num-freq-bins too large.");

    bins_[bin].first = first_index;
    int32 size = last_index + 1 - first_index;
    bins_[bin].second.Resize(size);
    bins_[bin].second.CopyFromVec(this_bin.Range(first_index, size));

    // Replicate a bug in HTK, for testing purposes.
    if (opts.htk_mode && bin == 0 && new_low_freq != 0.0)
      bins_[bin].second(0) = 0.0;

  }
  if (debug_) {
    for (size_t i = 0; i < bins_.size(); i++) {
      KALDI_LOG << "bin " << i << ", offset = " << bins_[i].first
                << ", vec = " << bins_[i].second;
    }
  }
}

FreqBanks::FreqBanks(const FreqBanks &other):
    center_freqs_(other.center_freqs_),
    bins_(other.bins_),
    debug_(other.debug_),
    htk_mode_(other.htk_mode_) { }


// "power_spectrum" contains fft energies.
void FreqBanks::Compute(const VectorBase<BaseFloat> &power_spectrum,
                       VectorBase<BaseFloat> *freq_energies_out) const {
  int32 num_bins = bins_.size();
  KALDI_ASSERT(freq_energies_out->Dim() == num_bins);

  for (int32 i = 0; i < num_bins; i++) {
    int32 offset = bins_[i].first;
    const Vector<BaseFloat> &v(bins_[i].second);
    BaseFloat energy = VecVec(v, power_spectrum.Range(offset, v.Dim()));
    // HTK-like flooring- for testing purposes (we prefer dither)
    if (htk_mode_ && energy < 1.0) energy = 1.0;
    (*freq_energies_out)(i) = energy;

    // The following assert was added due to a problem with OpenBlas that
    // we had at one point (it was a bug in that library).  Just to detect
    // it early.
    KALDI_ASSERT(!KALDI_ISNAN((*freq_energies_out)(i)));
  }

  if (debug_) {
    fprintf(stderr, "MEL BANKS:\n");
    for (int32 i = 0; i < num_bins; i++)
      fprintf(stderr, " %f", (*freq_energies_out)(i));
    fprintf(stderr, "\n");
  }
}

void ComputeLifterCoeffs_2(BaseFloat Q, VectorBase<BaseFloat> *coeffs) {
  // Compute liftering coefficients (scaling on cepstral coeffs)
  // coeffs are numbered slightly differently from HTK: the zeroth
  // index is C0, which is not affected.
  for (int32 i = 0; i < coeffs->Dim(); i++)
    (*coeffs)(i) = 1.0 + 0.5 * Q * sin (M_PI * i / Q);
}

}  // namespace kaldi
