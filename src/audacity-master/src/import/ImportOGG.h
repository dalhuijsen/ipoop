/**********************************************************************

  Audacity: A Digital Audio Editor

  ImportOGG.h

  Joshua Haberman

**********************************************************************/

#ifndef __AUDACITY_IMPORT_OGG__
#define __AUDACITY_IMPORT_OGG__

class ImportPluginList;
class UnusableImportPluginList;

void GetOGGImportPlugin(ImportPluginList *importPluginList,
                        UnusableImportPluginList *unusableImportPluginList);

#endif
