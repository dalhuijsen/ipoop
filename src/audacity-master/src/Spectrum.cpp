/**********************************************************************

  Audacity: A Digital Audio Editor

  Spectrum.cpp

  Dominic Mazzoni

*******************************************************************//*!

\file Spectrum.cpp
\brief Functions for computing Spectra.

*//*******************************************************************/

#include <math.h>

#include "Spectrum.h"
#include "FFT.h"

bool ComputeSpectrum(const float * data, int width,
                     int windowSize,
                     double WXUNUSED(rate), float *output,
                     bool autocorrelation, int windowFunc)
{
   if (width < windowSize)
      return false;

   if (!data || !output)
      return true;

   float *processed = new float[windowSize];

   int i;
   for (i = 0; i < windowSize; i++)
      processed[i] = float(0.0);
   int half = windowSize / 2;

   float *in = new float[windowSize];
   float *out = new float[windowSize];
   float *out2 = new float[windowSize];

   int start = 0;
   int windows = 0;
   while (start + windowSize <= width) {
      for (i = 0; i < windowSize; i++)
         in[i] = data[start + i];

      WindowFunc(windowFunc, windowSize, in);

      if (autocorrelation) {
         // Take FFT
#ifdef EXPERIMENTAL_USE_REALFFTF
         RealFFT(windowSize, in, out, out2);
#else
         FFT(windowSize, false, in, NULL, out, out2);
#endif
         // Compute power
         for (i = 0; i < windowSize; i++)
            in[i] = (out[i] * out[i]) + (out2[i] * out2[i]);

         // Tolonen and Karjalainen recommend taking the cube root
         // of the power, instead of the square root

         for (i = 0; i < windowSize; i++)
            in[i] = powf(in[i], 1.0f / 3.0f);

         // Take FFT
#ifdef EXPERIMENTAL_USE_REALFFTF
         RealFFT(windowSize, in, out, out2);
#else
         FFT(windowSize, false, in, NULL, out, out2);
#endif

      }
      else
         PowerSpectrum(windowSize, in, out);

      // Take real part of result
      for (i = 0; i < half; i++)
        processed[i] += out[i];

      start += half;
      windows++;
   }

   if (autocorrelation) {

      // Peak Pruning as described by Tolonen and Karjalainen, 2000
      /*
       Combine most of the calculations in a single for loop.
       It should be safe, as indexes refer only to current and previous elements,
       that have already been clipped, etc...
      */
      for (i = 0; i < half; i++) {
        // Clip at zero, copy to temp array
        if (processed[i] < 0.0)
            processed[i] = float(0.0);
        out[i] = processed[i];
        // Subtract a time-doubled signal (linearly interp.) from the original
        // (clipped) signal
        if ((i % 2) == 0)
           processed[i] -= out[i / 2];
        else
           processed[i] -= ((out[i / 2] + out[i / 2 + 1]) / 2);

        // Clip at zero again
        if (processed[i] < 0.0)
            processed[i] = float(0.0);
      }

      // Reverse and scale
      for (i = 0; i < half; i++)
         in[i] = processed[i] / (windowSize / 4);
      for (i = 0; i < half; i++)
         processed[half - 1 - i] = in[i];
   } else {
      // Convert to decibels
      // But do it safely; -Inf is nobody's friend
      for (i = 0; i < half; i++){
         float temp=(processed[i] / windowSize / windows);
         if (temp > 0.0)
            processed[i] = 10*log10(temp);
         else
            processed[i] = 0;
      }
   }

   for(i=0;i<half;i++)
      output[i] = processed[i];
   delete[]in;
   delete[]out;
   delete[]out2;
   delete[]processed;

   return true;
}

