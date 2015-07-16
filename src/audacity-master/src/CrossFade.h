/**********************************************************************

  Audacity: A Digital Audio Editor

  CrossFade.h

 (c) 2005 The Audacity Team
  Licensed under the GPL version 2.0

**********************************************************************/
#ifndef __AUDACITY_CROSSFADE__
#define __AUDACITY_CROSSFADE__

/// This defines a crossfader class that
/// accepts a list of WaveClips and can do a mini-mixing
/// to produce the desired crossfading

#include "SampleFormat.h"
#include "Resample.h"
#include "WaveClip.h"


enum FadeType
{
   FT_MIX,
   FT_TRIANGULAR,
   FT_EXPONENTIAL
};

class CrossFader
{

 public:
  CrossFader();
  ~CrossFader();

  //This sets a crossfade mode where the overlapping
  //tracks are simply mixed equally.
  void SetMixCrossFade(){mType = FT_MIX;};
  void SetTriangularCrossFade(){mType = FT_TRIANGULAR;};
  void SetExponentialCrossFade(){mType = FT_EXPONENTIAL;};

  void AddClip( WaveClip * clip);
  void ClearClips();
  //Produces samples according to crossfading rules.
  bool  GetSamples(samplePtr buffer, sampleFormat format,
                   sampleCount start, sampleCount len);

 protected:
  WaveClipList mClips;

 private:

  bool CrossFadeMix(samplePtr buffer, sampleFormat format, sampleCount start, sampleCount len);

  FadeType mType;


};



#endif
