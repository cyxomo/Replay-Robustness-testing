// feat/freq-computations.h

// Copyright 2009-2011  Phonexia s.r.o.;  Microsoft Corporation
//                2016  Johns Hopkins University (author: Daniel Povey)

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

#ifndef KALDI_FEAT_FREQ_COMPUTATIONS_H_
#define KALDI_FEAT_FREQ_COMPUTATIONS_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex>
#include <utility>
#include <vector>

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "matrix/matrix-lib.h"


namespace kaldi {
/// @addtogroup  feat FeatureExtraction
/// @{

struct FrameExtractionOptions;  // defined in feature-window.h


struct FreqBanksOptions {
  int32 num_bins;  // e.g. 25; number of triangular bins
  BaseFloat low_freq;  // e.g. 20; lower frequency cutoff
  BaseFloat high_freq;  // an upper frequency cutoff; 0 -> no cutoff, negative
  // ->added to the Nyquist frequency to get the cutoff.
  bool debug_freq;
  // htk_mode is a "hidden" config, it does not show up on command line.
  // Enables more exact compatibibility with HTK, for testing purposes.  Affects
  // freq-energy flooring and reproduces a bug in HTK.
  bool htk_mode;
  explicit FreqBanksOptions(int num_bins = 25)
      : num_bins(num_bins), low_freq(20), high_freq(0), debug_freq(false), htk_mode(false) {}

  void Register(OptionsItf *opts) {
    opts->Register("num-freq-bins", &num_bins,
                   "Number of triangular freq-frequency bins");
    opts->Register("low-freq", &low_freq,
                   "Low cutoff frequency for freq bins");
    opts->Register("high-freq", &high_freq,
                   "High cutoff frequency for freq bins (if < 0, offset from Nyquist)");
    opts->Register("debug-freq", &debug_freq,
                   "Print out debugging information for freq bin computation");
  }
};


class FreqBanks {
 public:

  static inline BaseFloat InverseFreqScale(BaseFloat inv_freq) {
    return inv_freq;
  }

  static inline BaseFloat FreqScale(BaseFloat freq) {
    return freq;
  }

  FreqBanks(const FreqBanksOptions &opts,
           const FrameExtractionOptions &frame_opts,
           BaseFloat vtln_warp_factor);

  /// Compute Freq energies (note: not log enerties).
  /// At input, "fft_energies" contains the FFT energies (not log).
  void Compute(const VectorBase<BaseFloat> &fft_energies,
               VectorBase<BaseFloat> *freq_energies_out) const;

  int32 NumBins() const { return bins_.size(); }

  // returns vector of central freq of each bin; needed by plp code.
  const Vector<BaseFloat> &GetCenterFreqs() const { return center_freqs_; }

  // Copy constructor
  FreqBanks(const FreqBanks &other);
 private:
  // Disallow assignment
  FreqBanks &operator = (const FreqBanks &other);

  // center frequencies of bins, numbered from 0 ... num_bins-1.
  // Needed by GetCenterFreqs().
  Vector<BaseFloat> center_freqs_;

  // the "bins_" vector is a vector, one for each bin, of a pair:
  // (the first nonzero fft-bin), (the vector of weights).
  std::vector<std::pair<int32, Vector<BaseFloat> > > bins_;

  bool debug_;
  bool htk_mode_;
};


// Compute liftering coefficients (scaling on cepstral coeffs)
// coeffs are numbered slightly differently from HTK: the zeroth
// index is C0, which is not affected.
void ComputeLifterCoeffs_2(BaseFloat Q, VectorBase<BaseFloat> *coeffs);

/// @} End of "addtogroup feat"
}  // namespace kaldi

#endif  // KALDI_FEAT_MEL_COMPUTATIONS_H_
