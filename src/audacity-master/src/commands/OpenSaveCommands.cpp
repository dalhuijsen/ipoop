/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Stephen Parry

******************************************************************//**

\file OpenSaveCommands.cpp
\brief Contains definitions for the OpenProjectCommand and SaveProjectCommand classes

*//*******************************************************************/

#include "OpenSaveCommands.h"
#include "../Project.h"
#include "../export/Export.h"

// OpenProject

wxString OpenProjectCommandType::BuildName()
{
   return wxT("OpenProject");
}

void OpenProjectCommandType::BuildSignature(CommandSignature &signature)
{
   BoolValidator *addToHistoryValidator(new BoolValidator());
   signature.AddParameter(wxT("AddToHistory"), true, addToHistoryValidator);
   Validator *filenameValidator(new Validator());
   signature.AddParameter(wxT("Filename"), wxT(""), filenameValidator);
}

Command *OpenProjectCommandType::Create(CommandOutputTarget *target)
{
   return new OpenProjectCommand(*this, target);
}

bool OpenProjectCommand::Apply(CommandExecutionContext context)
{
   wxString fileName = GetString(wxT("Filename"));
   bool addToHistory  = GetBool(wxT("AddToHistory"));
   wxString oldFileName = context.GetProject()->GetFileName();
   if(fileName == wxEmptyString)
   {
      context.GetProject()->OnOpen();
   }
   else
   {
      context.GetProject()->OpenFile(fileName,addToHistory);
   }
   wxString newFileName = context.GetProject()->GetFileName();

   // Because Open does not return a success or failure, we have to guess
   // at this point, based on whether the project file name has
   // changed and what to...
   return newFileName != wxEmptyString && newFileName != oldFileName;
}

OpenProjectCommand::~OpenProjectCommand()
{ }

// SaveProject

wxString SaveProjectCommandType::BuildName()
{
   return wxT("SaveProject");
}

void SaveProjectCommandType::BuildSignature(CommandSignature &signature)
{
   BoolValidator *saveCompressedValidator(new BoolValidator());
   BoolValidator *addToHistoryValidator(new BoolValidator());

   signature.AddParameter(wxT("Compress"), false, saveCompressedValidator);
   signature.AddParameter(wxT("AddToHistory"), true, addToHistoryValidator);

   Validator *filenameValidator(new Validator());
   signature.AddParameter(wxT("Filename"), wxT(""), filenameValidator);
}

Command *SaveProjectCommandType::Create(CommandOutputTarget *target)
{
   return new SaveProjectCommand(*this, target);
}

bool SaveProjectCommand::Apply(CommandExecutionContext context)
{
   wxString fileName = GetString(wxT("Filename"));
   bool saveCompressed  = GetBool(wxT("Compress"));
   bool addToHistory  = GetBool(wxT("AddToHistory"));
   if(fileName == wxEmptyString)
      return context.GetProject()->SaveAs(saveCompressed);
   else
      return context.GetProject()->SaveAs(fileName,saveCompressed,addToHistory);
}

SaveProjectCommand::~SaveProjectCommand()
{ }
