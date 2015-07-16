/**********************************************************************

  Audacity: A Digital Audio Editor

  ViewInfo.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_VIEWINFO__
#define __AUDACITY_VIEWINFO__

#include "SelectedRegion.h"

const double gMaxZoom = 6000000,
             gMinZoom = 0.001;

class Track;

struct ViewInfo {

   // Current selection

   SelectedRegion selectedRegion;

   // Scroll info

   Track *track;                // first visible track
   int vpos;                    // vertical scroll pos

   double h;                    // h pos in secs
   double screen;               // screen width in secs
   double total;                // total width in secs
   double zoom;                 // pixels per second

   // Current horizontal scroll bar positions, in pixels
   wxInt64 sbarH;
   wxInt64 sbarScreen;
   wxInt64 sbarTotal;

   // Internal wxScrollbar positions are only int in range, so multiply
   // the above values with the following member to get the actual
   // scroll bar positions as reported by the horizontal wxScrollbar's members
   // i.e. units are scroll increments per pixel
   double sbarScale;

   // Vertical scroll step
   int scrollStep;

   // Other stuff, mainly states (true or false) related to autoscroll and
   // drawing the waveform. Maybe this should be put somewhere else?

   bool bUpdateTrackIndicator;
};

#endif
