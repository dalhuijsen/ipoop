/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: wxwidgets

   Dan Horgan

******************************************************************//**

\file SetTrackInfoCommand.cpp
\brief Definitions for SetTrackInfoCommand and SetTrackInfoCommandType classes

\class SetTrackInfoCommand
\brief Command that sets track information (currently name only)

*//*******************************************************************/

#include "SetTrackInfoCommand.h"
#include "../Project.h"
#include "../Track.h"

wxString SetTrackInfoCommandType::BuildName()
{
   return wxT("SetTrackInfo");
}

void SetTrackInfoCommandType::BuildSignature(CommandSignature &signature)
{
   IntValidator *trackIndexValidator = new IntValidator();
   signature.AddParameter(wxT("TrackIndex"), 0, trackIndexValidator);

   OptionValidator *infoTypeValidator = new OptionValidator();
   infoTypeValidator->AddOption(wxT("Name"));
   signature.AddParameter(wxT("Type"), wxT("Name"), infoTypeValidator);
   Validator *nameValidator = new Validator();
   signature.AddParameter(wxT("Name"), wxT("Unnamed"), nameValidator);
}

Command *SetTrackInfoCommandType::Create(CommandOutputTarget *target)
{
   return new SetTrackInfoCommand(*this, target);
}

bool SetTrackInfoCommand::Apply(CommandExecutionContext context)
{
   wxString mode = GetString(wxT("Type"));

   long trackIndex = GetLong(wxT("TrackIndex"));

   // (Note: track selection ought to be somewhere else)
   long i = 0;
   TrackListIterator iter(context.GetProject()->GetTracks());
   Track *t = iter.First();
   while (t && i != trackIndex)
   {
      t = iter.Next();
      ++i;
   }
   if (i != trackIndex || !t)
   {
      Error(wxT("TrackIndex was invalid."));
      return false;
   }

   if (mode.IsSameAs(wxT("Name")))
   {
      wxString name = GetString(wxT("Name"));
      t->SetName(name);
   }
   else
   {
      Error(wxT("Invalid info type!"));
      return false;
   }
   return true;
}
