/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityHeaders.h

  Dominic Mazzoni

  This is not a normal include file - it's currently only used
  on Mac OS X as a "precompiled header" file that's automatically
  included by all source files, resulting in roughly a 2x increase
  in compilation speed.

  When gcc 3.4 is released, it will have precompiled header support
  on other platforms, and this file could be adapted to support
  precompiled headers on Linux, etc.

**********************************************************************/

#include "Audacity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>




#include <wx/wx.h>
#include <wx/bitmap.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/image.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/progdlg.h>
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/tooltip.h>

#ifdef __WXMAC__
#include <wx/mac/private.h>
#endif

#include "audacity/Types.h"

#include "AColor.h"
#include "AudacityApp.h"
#include "AudioIO.h"
#include "BlockFile.h"
#include "Diags.h"
#include "DirManager.h"
#include "Envelope.h"
#include "FFT.h"
#include "FileFormats.h"
#include "FreqWindow.h"
#include "ImageManipulation.h"
#include "Internat.h"
#include "LabelTrack.h"
#include "Mix.h"
#include "NoteTrack.h"
#include "Prefs.h"
#include "Project.h"
#include "Resample.h"
#include "SampleFormat.h"
#include "Sequence.h"
#include "TimeTrack.h"
#include "Track.h"
#include "UndoManager.h"
#include "ViewInfo.h"
#include "WaveTrack.h"
#include "widgets/ASlider.h"
#include "widgets/ProgressDialog.h"
#include "widgets/Ruler.h"
#include "xml/XMLTagHandler.h"

// PRL:  These lines allow you to remove Project.h above.
// They must be included before the definition of macro new below.
#include <set>
#include <map>

//#ifdef __WXMSW__
// Enable this to diagnose memory leaks too!
//    #include <wx/msw/msvcrt.h>      // redefines the new() operator
//#endif

#if __WXMSW__
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif
