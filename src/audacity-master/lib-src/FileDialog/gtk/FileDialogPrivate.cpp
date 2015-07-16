/////////////////////////////////////////////////////////////////////////////
// Name:        gtk/filedlg.cpp
// Purpose:     native implementation of FileDialog
// Author:      Robert Roebling, Zbigniew Zagorski, Mart Raudsepp
// Id:          $Id: FileDialogPrivate.cpp,v 1.7 2009-05-25 11:10:00 llucius Exp $
// Copyright:   (c) 1998 Robert Roebling, 2004 Zbigniew Zagorski, 2005 Mart Raudsepp
// Licence:     wxWindows licence
//
// Modified for Audacity to support an additional button on Save dialogs
//
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

// Include setup.h to get wxUSE flags for compilers that do not support precompilation of headers
#include "wx/setup.h"

#include "../FileDialog.h"

#include <gtk/gtk.h>
#include "private.h"

#include <unistd.h> // chdir

#include "wx/intl.h"
#include "wx/filename.h" // wxFilename
#include "wx/tokenzr.h" // wxStringTokenizer
#include "wx/filefn.h" // ::wxGetCwd
#include "wx/msgdlg.h" // wxMessageDialog
#include "wx/version.h"

//-----------------------------------------------------------------------------
// Pull in generic file dialog
//-----------------------------------------------------------------------------

#include "../generic/FileDialogPrivate.cpp"

//-----------------------------------------------------------------------------
// idle system
//-----------------------------------------------------------------------------

extern void wxapp_install_idle_handler();

//-----------------------------------------------------------------------------
// Open expanders on the dialog (really only the "Browser for other folders")
//-----------------------------------------------------------------------------

extern "C" {
   static void SetExpanded(GtkWidget *widget, gpointer data)
   {
      if (GTK_IS_EXPANDER(widget))
      {
         gtk_expander_set_expanded(GTK_EXPANDER(widget), true);
      }
      else if (GTK_IS_CONTAINER(widget))
      {
         gtk_container_forall(GTK_CONTAINER(widget), SetExpanded, data);
      }
      
      return;
   }
   
   static void gtk_filedialog_show_callback(GtkWidget *widget, FileDialog *dialog)
   {
      gtk_container_forall(GTK_CONTAINER(widget), SetExpanded, NULL);
   }
}

//-----------------------------------------------------------------------------
// "clicked" for OK-button
//-----------------------------------------------------------------------------

extern "C" {
   static void gtk_filedialog_ok_callback(GtkWidget *widget, FileDialog *dialog)
   {
      int style = dialog->GetWindowStyle();
      gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
      
      // gtk version numbers must be identical with the one in ctor (that calls set_do_overwrite_confirmation)
#if GTK_CHECK_VERSION(2,7,3)
      if(gtk_check_version(2,7,3) != NULL)
#endif
         if ((style & wxFD_SAVE) && (style & wxFD_OVERWRITE_PROMPT))
         {
            if ( g_file_test(filename, G_FILE_TEST_EXISTS) )
            {
               wxString msg;
               
               msg.Printf(
                          _("File '%s' already exists, do you really want to overwrite it?"),
                          wxString(wxConvFileName->cMB2WX(filename)).c_str());
               
               wxMessageDialog dlg(dialog, msg, _("Confirm"),
                                   wxYES_NO | wxICON_QUESTION);
               if (dlg.ShowModal() != wxID_YES)
                  return;
            }
         }
      
      // change to the directory where the user went if asked
      if (style & wxFD_CHANGE_DIR)
      {
         // Use chdir to not care about filename encodings
         gchar* folder = g_path_get_dirname(filename);
         chdir(folder);
         g_free(folder);
      }
      
      g_free(filename);
      
      wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK);
      event.SetEventObject(dialog);
      dialog->GetEventHandler()->ProcessEvent(event);
   }
}

//-----------------------------------------------------------------------------
// "clicked" for Cancel-button
//-----------------------------------------------------------------------------

extern "C" {
   static void gtk_filedialog_cancel_callback(GtkWidget *WXUNUSED(w),
                                              FileDialog *dialog)
   {
      wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL);
      event.SetEventObject(dialog);
      dialog->GetEventHandler()->ProcessEvent(event);
   }
}

extern "C" {
   static void gtk_filedialog_response_callback(GtkWidget *w,
                                                gint response,
                                                FileDialog *dialog)
   {
#if !wxCHECK_VERSION(3, 0, 0)
      wxapp_install_idle_handler();
#endif
      
      if (response == GTK_RESPONSE_ACCEPT)
         gtk_filedialog_ok_callback(w, dialog);
      else if (response == GTK_RESPONSE_CANCEL)
         gtk_filedialog_cancel_callback(w, dialog);
      else // "delete"
      {
         gtk_filedialog_cancel_callback(w, dialog);
         dialog->m_destroyed_by_delete = true;
      }
   }
}

//-----------------------------------------------------------------------------
// "clicked" for extra-button
//-----------------------------------------------------------------------------

extern "C" {
   static void gtk_filedialog_extra_callback(GtkWidget *WXUNUSED(w),
                                             FileDialog *dialog)
   {
      dialog->ClickButton(dialog->GetFilterIndex());
   }
}

//-----------------------------------------------------------------------------
// FileDialog
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(FileDialog, GenericFileDialog)

BEGIN_EVENT_TABLE(FileDialog, GenericFileDialog)
EVT_BUTTON(wxID_OK, FileDialog::OnFakeOk)
END_EVENT_TABLE()

FileDialog::FileDialog(wxWindow *parent, const wxString& message,
                       const wxString& defaultDir,
                       const wxString& defaultFileName,
                       const wxString& wildCard,
                       long style, const wxPoint& pos)
: GenericFileDialog(parent, message, defaultDir, defaultFileName,
                    wildCard, style, pos, 
                    wxDefaultSize,
                    wxFileDialogNameStr,
                    true )
{
   wxASSERT_MSG( !( (style & wxFD_SAVE) && (style & wxFD_MULTIPLE) ), wxT("FileDialog - wxFD_MULTIPLE used on a save dialog" ) );

#if !wxCHECK_VERSION(3, 0, 0)
   m_needParent = false;
#endif
   m_destroyed_by_delete = false;
   
   if (!PreCreation(parent, pos, wxDefaultSize) ||
       !CreateBase(parent, wxID_ANY, pos, wxDefaultSize, style,
                   wxDefaultValidator, wxT("filedialog")))
   {
      wxFAIL_MSG( wxT("FileDialog creation failed") );
      return;
   }
   
   GtkFileChooserAction gtk_action;
   GtkWindow* gtk_parent = NULL;
   if (parent)
      gtk_parent = GTK_WINDOW( gtk_widget_get_toplevel(parent->m_widget) );
   
   const gchar* ok_btn_stock;
   if ( style & wxFD_SAVE )
   {
      gtk_action = GTK_FILE_CHOOSER_ACTION_SAVE;
      ok_btn_stock = GTK_STOCK_SAVE;
   }
   else
   {
      gtk_action = GTK_FILE_CHOOSER_ACTION_OPEN;
      ok_btn_stock = GTK_STOCK_OPEN;
   }
   
   m_widget = gtk_file_chooser_dialog_new(
                                          wxGTK_CONV(m_message),
                                          gtk_parent,
                                          gtk_action,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          ok_btn_stock, GTK_RESPONSE_ACCEPT,
                                          NULL);
   
   // Allow pressing "Enter" key for default action
   gtk_dialog_set_default_response(GTK_DIALOG(m_widget), GTK_RESPONSE_ACCEPT);
   
   if ( style & wxFD_MULTIPLE )
      gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(m_widget), true);
   
   // local-only property could be set to false to allow non-local files to be loaded.
   // In that case get/set_uri(s) should be used instead of get/set_filename(s) everywhere
   // and the GtkFileChooserDialog should probably also be created with a backend,
   // e.g "gnome-vfs", "default", ... (gtk_file_chooser_dialog_new_with_backend).
   // Currently local-only is kept as the default - true:
   // gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(m_widget), true);
   
   g_signal_connect(G_OBJECT(m_widget), "response",
                    G_CALLBACK(gtk_filedialog_response_callback), (gpointer)this);
   g_signal_connect(G_OBJECT(m_widget), "show",
                    G_CALLBACK(gtk_filedialog_show_callback), (gpointer)this);
   
   SetWildcard(wildCard);
   
   if ( style & wxFD_SAVE )
   {
      if ( !defaultDir.empty() )
         gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(m_widget),
                                             wxConvFileName->cWX2MB(defaultDir));
      
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(m_widget),
                                        wxConvFileName->cWX2MB(defaultFileName));
      
#if GTK_CHECK_VERSION(2,7,3)
      if (!gtk_check_version(2,7,3))
         gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(m_widget), FALSE);
#endif
   }
   else
   {
      if ( !defaultFileName.empty() )
      {
         wxString dir;
         if ( defaultDir.empty() )
            dir = ::wxGetCwd();
         else
            dir = defaultDir;
         
         gtk_file_chooser_set_filename(
                                       GTK_FILE_CHOOSER(m_widget),
                                       wxConvFileName->cWX2MB( wxFileName(dir, defaultFileName).GetFullPath() ) );
      }
      else if ( !defaultDir.empty() )
         gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(m_widget),
                                             wxConvFileName->cWX2MB(defaultDir) );
   }
}

FileDialog::~FileDialog()
{
   if (m_destroyed_by_delete)
      m_widget = NULL;
}

void FileDialog::OnFakeOk( wxCommandEvent &event )
{
   if (Validate() && TransferDataFromWindow())
      EndModal(wxID_OK);
}

int FileDialog::ShowModal()
{
   if ( !m_buttonlabel.IsEmpty() )
   {
      GtkWidget *widget;
      wxString label = m_buttonlabel;
      
      label.Replace(wxT("&"), wxT("_"));

      widget = gtk_button_new_with_mnemonic(wxGTK_CONV(label));
      gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(m_widget), widget);
      g_signal_connect(G_OBJECT(widget), "clicked",
                       G_CALLBACK(gtk_filedialog_extra_callback), (gpointer)this);
   }
   
   return wxDialog::ShowModal();
}

bool FileDialog::Show( bool show )
{
   return wxDialog::Show( show );
}

void FileDialog::DoSetSize(int x, int y, int width, int height, int sizeFlags )
{
   if (!m_wxwindow)
      return;
   else
      GenericFileDialog::DoSetSize( x, y, width, height, sizeFlags );
}

wxString FileDialog::GetPath() const
{
   char *f = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(m_widget));
   wxFileName path(wxConvFileName->cMB2WX(f));
   g_free(f);

   if (!path.HasExt())
   {
      if (!(m_dialogStyle & FD_NO_ADD_EXTENSION))
      {  
         int filterIndex = GetFilterIndex();
         if (filterIndex != -1)
         {
            wxStringTokenizer tokenizer(m_patterns[filterIndex], wxT(";"));            
            if (tokenizer.HasMoreTokens())
            {
               wxString extension = tokenizer.GetNextToken().AfterFirst(wxT('.'));
               if (extension.Right(2) == wxT("*"))
               {
                  extension = wxEmptyString;
               }
               path.SetExt(extension);
            }
         }
      }
   }

   return path.GetFullPath();
}

void FileDialog::GetFilenames(wxArrayString& files) const
{
   GetPaths(files);
   for (size_t n = 0; n < files.GetCount(); ++n )
   {
      wxFileName file(files[n]);
      files[n] = file.GetFullName();
   }
}

void FileDialog::GetPaths(wxArrayString& paths) const
{
   paths.Empty();
   if (gtk_file_chooser_get_select_multiple(GTK_FILE_CHOOSER(m_widget)))
   {
      GSList *gpathsi = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(m_widget));
      GSList *gpaths = gpathsi;
      while (gpathsi)
      {
         wxString file(wxConvFileName->cMB2WX((gchar*) gpathsi->data));
         paths.Add(file);
         g_free(gpathsi->data);
         gpathsi = gpathsi->next;
      }
      
      g_slist_free(gpaths);
   }
   else
      paths.Add(GetPath());
}

void FileDialog::SetMessage(const wxString& message)
{
   m_message = message;
   SetTitle(message);
}

void FileDialog::SetPath(const wxString& path)
{
   if (path.empty()) return;
      
   gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(m_widget), wxConvFileName->cWX2MB(path));
}

void FileDialog::SetDirectory(const wxString& dir)
{
   if (wxDirExists(dir))
   {
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(m_widget), wxConvFileName->cWX2MB(dir));
   }
}

wxString FileDialog::GetDirectory() const
{
   return wxConvFileName->cMB2WX(gtk_file_chooser_get_current_folder( GTK_FILE_CHOOSER(m_widget) ) );
}

void FileDialog::SetFilename(const wxString& name)
{
   if (GetWindowStyle() & wxFD_SAVE)
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(m_widget), wxConvFileName->cWX2MB(name));
   else
      SetPath(wxFileName(GetDirectory(), name).GetFullPath());
}

wxString FileDialog::GetFilename() const
{
   wxFileName name = GetPath();
   return name.GetFullName();
}

void FileDialog::SetWildcard(const wxString& wildCard)
{
   // parse filters
   wxArrayString wildDescriptions, wildFilters;
   if (!wxParseCommonDialogsFilter(wildCard, wildDescriptions, wildFilters))
   {
      wxFAIL_MSG( wxT("FileDialog::SetWildCard - bad wildcard string") );
   }
   else
   {
      // Parsing went fine. Set m_wildCard to be returned by FileDialogBase::GetWildcard
      m_wildCard = wildCard;
      
      GtkFileChooser* chooser = GTK_FILE_CHOOSER(m_widget);
      
      // empty current filter list:
      GSList* ifilters = gtk_file_chooser_list_filters(chooser);
      GSList* filters = ifilters;
      
      while (ifilters)
      {
         gtk_file_chooser_remove_filter(chooser,GTK_FILE_FILTER(ifilters->data));
         ifilters = ifilters->next;
      }
      g_slist_free(filters);
      
      // add parsed to GtkChooser
      for (size_t n = 0; n < wildFilters.GetCount(); ++n)
      {
         GtkFileFilter* filter = gtk_file_filter_new();
         gtk_file_filter_set_name(filter, wxGTK_CONV(wildDescriptions[n]));

         m_patterns.Add(wildFilters[n]);
         wxStringTokenizer exttok(wildFilters[n], wxT(";"));
         while (exttok.HasMoreTokens())
         {
            wxString token = exttok.GetNextToken();
            gtk_file_filter_add_pattern(filter, wxGTK_CONV(token));
         }
         
         gtk_file_chooser_add_filter(chooser, filter);
      }
      
      // Reset the filter index
      SetFilterIndex(0);
   }
}

void FileDialog::SetFilterIndex(int filterIndex)
{
   gpointer filter;
   GtkFileChooser *chooser = GTK_FILE_CHOOSER(m_widget);
   GSList *filters = gtk_file_chooser_list_filters(chooser);
   
   filter = g_slist_nth_data(filters, filterIndex);
   
   if (filter != NULL)
   {
      gtk_file_chooser_set_filter(chooser, GTK_FILE_FILTER(filter));
   }
   else
   {
      wxFAIL_MSG( wxT("FileDialog::SetFilterIndex - bad filter index") );
   }
   
   g_slist_free(filters);
}

int FileDialog::GetFilterIndex() const
{
   GtkFileChooser *chooser = GTK_FILE_CHOOSER(m_widget);
   GtkFileFilter *filter = gtk_file_chooser_get_filter(chooser);
   GSList *filters = gtk_file_chooser_list_filters(chooser);
   gint index = g_slist_index(filters, filter);
   g_slist_free(filters);
   
   if (index == -1)
   {
      wxFAIL_MSG( wxT("FileDialog::GetFilterIndex - bad filter index returned by gtk+") );
      return 0;
   }
   else
      return index;
}
