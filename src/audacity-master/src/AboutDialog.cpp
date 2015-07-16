/**********************************************************************

  Audacity: A Digital Audio Editor

  AboutDialog.cpp

  Dominic Mazzoni
  Vaughan Johnson
  James Crook

********************************************************************//**

\class AboutDialog
\brief The AboutDialog shows the program version and developer credits.

It is a simple scrolling window with an 'OK... Audacious!' button to
close it.

*//*****************************************************************//**

\class AboutDialogCreditItem
\brief AboutDialogCreditItem is a structure used by the AboutDialog to
hold information about one contributor to Audacity.

*//********************************************************************/


#include "Audacity.h"

#include <wx/dialog.h>
#include <wx/html/htmlwin.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/intl.h>

#include "AboutDialog.h"
#include "FileNames.h"
#include "Internat.h"
#include "ShuttleGui.h"
#include "widgets/LinkingHtmlWindow.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(AboutDialogCreditItemsList);

#include "Theme.h"
#include "AllThemeResources.h"

#include "../images/AudacityLogoWithName.xpm"

void AboutDialog::CreateCreditsList()
{
   // The Audacity Team: developers and support
   AddCredit(wxT("Richard Ash"), roleTeamDeveloper);
   AddCredit(wxT("James Crook"), roleTeamDeveloper);
   AddCredit(wxString(wxT("Roger Dannenberg - ")) + _("co-founder"), roleTeamDeveloper);
   AddCredit(wxT("Benjamin Drung"), roleTeamDeveloper);
   AddCredit(wxT("Vaughan Johnson"), roleTeamDeveloper);
   AddCredit(wxT("Paul Licameli"), roleTeamDeveloper);
   AddCredit(wxT("Leland Lucius"), roleTeamDeveloper);
   AddCredit(wxT("Martyn Shaw"), roleTeamDeveloper);

   AddCredit(wxString(wxT("Gale Andrews - ")) + _("quality assurance"), roleTeamSupport);
   AddCredit(wxT("Christian Brochec"), roleTeamSupport);
   AddCredit(wxT("Steve Daulton"), roleTeamSupport);
   AddCredit(wxT("Greg Kozikowski"), roleTeamSupport);
   AddCredit(wxT("Peter Sampson"), roleTeamSupport);
   AddCredit(wxT("Bill Wharrie"), roleTeamSupport);

   // Emeritus: people who were "lead developers" or made an
   // otherwise distinguished contribution, but who are no
   // longer active.
   AddCredit(wxT("Matt Brubeck"), roleEmeritusDeveloper);
   AddCredit(wxT("Michael Chinen"), roleEmeritusDeveloper);
   AddCredit(wxT("Al Dimond"), roleEmeritusDeveloper);
   AddCredit(wxT("Joshua Haberman"), roleEmeritusDeveloper);
   AddCredit(wxT("Ruslan Ijbulatov"), roleEmeritusDeveloper);
   AddCredit(wxString(wxT("Dominic Mazzoni - "))+_("co-founder"), roleEmeritusDeveloper);
   AddCredit(wxT("Markus Meyer"), roleEmeritusDeveloper);
   AddCredit(wxT("Monty Montgomery"), roleEmeritusDeveloper);
   AddCredit(wxT("Shane Mueller"), roleEmeritusDeveloper);
   AddCredit(wxT("Tony Oetzmann"), roleEmeritusSupport);
   AddCredit(wxT("Alexandre Prokoudine"), roleEmeritusSupport);

   // All other contributors
   AddCredit(wxT("Lynn Allan"), roleContributor);
   AddCredit(wxT("David Avery"), roleContributor);
   AddCredit(wxT("David Bailes"), roleContributor);
   AddCredit(wxT("William Bland"), roleContributor);
   AddCredit(wxT("Sami Boukortt"), roleContributor);
   AddCredit(wxT("Jeremy R. Brown"), roleContributor);
   AddCredit(wxT("Alex S. Brown"), roleContributor);
   AddCredit(wxT("Chris Cannam"), roleContributor);
   AddCredit(wxT("Cory Cook"), roleContributor);
   AddCredit(wxT("Craig DeForest"), roleContributor);
   AddCredit(wxT("Mitch Golden"), roleContributor);
   AddCredit(wxT("Brian Gunlogson"), roleContributor);
   AddCredit(wxT("Andrew Hallendorff"), roleContributor);
   AddCredit(wxT("Daniel Horgan"), roleContributor);
   AddCredit(wxT("David Hostetler"), roleContributor);
   AddCredit(wxT("Steve Jolly"), roleContributor);
   AddCredit(wxT("Steven Jones"), roleContributor);
   AddCredit(wxT("Arun Kishore"), roleContributor);
   AddCredit(wxT("Paul Livesey"), roleContributor);
   AddCredit(wxT("Harvey Lubin"), roleContributor);
   AddCredit(wxT("Greg Mekkes"), roleContributor);
   AddCredit(wxT("Abe Milde"), roleContributor);
   AddCredit(wxT("<a href=\"http://www.paulnasca.com/\">Paul Nasca</a>"), roleContributor);
   AddCredit(wxT("Clayton Otey"), roleContributor);
   AddCredit(wxT("Andr\x00e9 Pinto"), roleContributor);
   AddCredit(wxT("Mark Phillips"), roleContributor);
   AddCredit(wxT("Jean Claude Risset"), roleContributor);
   AddCredit(wxT("Edgar-RFT"), roleContributor);
   AddCredit(wxT("Augustus Saunders"), roleContributor);
   AddCredit(wxT("Benjamin Schwartz"), roleContributor);
   AddCredit(wxT("David R. Sky"), roleContributor);
   AddCredit(wxT("Rob Sykes"), roleContributor);
   AddCredit(wxT("Mike Underwood"), roleContributor);
   AddCredit(wxT("Philip Van Baren"), roleContributor);
   AddCredit(wxT("Salvo Ventura"), roleContributor);
   AddCredit(wxT("Jun Wan"), roleContributor);
   AddCredit(wxT("Daniel Winzen"), roleContributor);
   AddCredit(wxT("Tom Woodhams"), roleContributor);
   AddCredit(wxT("Wing Yu"), roleContributor);

   AddCredit(wxT("expat"), roleLibrary);
   AddCredit(wxT("FLAC"), roleLibrary);
   AddCredit(wxT("LAME"), roleLibrary);
   AddCredit(wxT("libmad"), roleLibrary);
   AddCredit(wxT("libsoxr, by Rob Sykes"), roleLibrary);
   #if USE_LV2
      AddCredit(wxT("lilv, serd, sord, and sratom, by David Robillard"), roleLibrary);
      AddCredit(wxT("msinttypes, by Alexander Chemeris"), roleLibrary);
   #endif
   AddCredit(wxT("libsndfile"), roleLibrary);
   AddCredit(wxT("Nyquist"), roleLibrary);
   AddCredit(wxT("Ogg Vorbis"), roleLibrary);
   AddCredit(wxT("PortAudio"), roleLibrary);
   AddCredit(wxT("portsmf"), roleLibrary);
   AddCredit(wxT("sbsms, by Clayton Otey"), roleLibrary);
   AddCredit(wxT("<a href=\"http://www.surina.net/soundtouch/\">SoundTouch</a>, by Olli Parviainen"), roleLibrary);
   AddCredit(wxT("TwoLAME"), roleLibrary);
   AddCredit(wxT("Vamp"), roleLibrary);
   AddCredit(wxT("wxWidgets"), roleLibrary);

   AddCredit(wxT("Dave Beydler"), roleThanks);
   AddCredit(wxT("Brian Cameron"), roleThanks);
   AddCredit(wxT("Jason Cohen"), roleThanks);
   AddCredit(wxT("Dave Fancella"), roleThanks);
   AddCredit(wxT("Steve Harris"), roleThanks);
   AddCredit(wxT("Daniel James"), roleThanks);
   AddCredit(wxT("Daniil Kolpakov"), roleThanks);
   AddCredit(wxT("Robert Leidle"), roleThanks);
   AddCredit(wxT("Logan Lewis"), roleThanks);
   AddCredit(wxT("David Luff"), roleThanks);
   AddCredit(wxT("Jason Pepas"), roleThanks);
   AddCredit(wxT("Jonathan Ryshpan"), roleThanks);
   AddCredit(wxT("Michael Schwendt"), roleThanks);
   AddCredit(wxT("Patrick Shirkey"), roleThanks);
   AddCredit(wxT("Tuomas Suutari"), roleThanks);
   AddCredit(wxT("Mark Tomlinson"), roleThanks);
   AddCredit(wxT("David Topper"), roleThanks);
   AddCredit(wxT("Rudy Trubitt"), roleThanks);
   AddCredit(wxT("StreetIQ.com"), roleThanks);
   AddCredit(wxT("UmixIt Technologies, LLC"), roleThanks);
   AddCredit(wxT("Verilogix, Inc."), roleThanks);
}

// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(AboutDialog, wxDialog)
   EVT_BUTTON(wxID_OK, AboutDialog::OnOK)
END_EVENT_TABLE()

IMPLEMENT_CLASS(AboutDialog, wxDialog)

AboutDialog::AboutDialog(wxWindow * parent)
   :  wxDialog(parent, -1, _("About Audacity"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   SetName(GetTitle());
   this->SetBackgroundColour(theTheme.Colour( clrAboutBoxBackground ));
   icon = NULL;
   ShuttleGui S( this, eIsCreating );
   S.StartNotebook();
   {
      PopulateAudacityPage( S );
      PopulateInformationPage( S );
      PopulateLicensePage( S );
   }
   S.EndNotebook();
   /* i18n-hint: "OK... Audacious" appears on a button at the
    * foot of the 'About Audacity' dialog box, after some text to read.
    * In English it is slightly humorous alternative to an 'OK' button.
    * If the humour doesn't work in your language, then just use whatever
    * you would use for a translation for 'OK' on a button. */
   wxButton *ok = new wxButton(S.GetParent(), wxID_OK, _("OK... Audacious!"));
   ok->SetDefault();
   S.Prop(0).AddWindow( ok );

   Fit();
   this->Centre();
}

#define ABOUT_DIALOG_WIDTH 506

void AboutDialog::PopulateAudacityPage( ShuttleGui & S )
{
   creditItems.DeleteContents(true); // Switch on automatic deletion of list items.
   CreateCreditsList();

   wxString par1Str = _(
"Audacity is a free program written by a worldwide team of volunteer <a href=\"http://audacityteam.org/about/credits\">developers</a>. \
Audacity is <a href=\"http://audacityteam.org/download\">available</a> for Windows, Mac, and GNU/Linux (and other Unix-like systems).");

   // This trick here means that the English language version won't mention using
   // English, whereas all translated versions will.
   wxString par2StrUntranslated = wxT(
"If you find a bug or have a suggestion for us, please write, in English, to our <a href=\"mailto:feedback@audacityteam.org\">feedback address</a>. \
For help, view the tips and tricks on our <a href=\"http://wiki.audacityteam.org/\">wiki</a> or \
visit our <a href=\"http://forum.audacityteam.org/\">forum</a>.");
   wxString par2Str = _(
"If you find a bug or have a suggestion for us, please write, in English, to our <a href=\"mailto:feedback@audacityteam.org\">feedback address</a>. \
For help, view the tips and tricks on our <a href=\"http://wiki.audacityteam.org/\">wiki</a> or \
visit our <a href=\"http://forum.audacityteam.org/\">forum</a>.");

   if( par2Str == par2StrUntranslated )
      par2Str.Replace( wxT(", in English,"), wxT("") );

   wxString translatorCredits;
   /* i18n-hint: The translation of "translator_credits" will appear
    *  in the credits in the About Audacity window.  Use this to add
    *  your own name(s) to the credits.
    *
    *  For example:  "English translation by Dominic Mazzoni." */
   if (_("translator_credits") != wxString(wxT("translator_credits")))
   {
      translatorCredits = _("translator_credits");
   }
   wxString localeStr = wxLocale::GetSystemEncodingName();

   wxString creditStr =
      wxT("<html><head><META http-equiv=\"Content-Type\" content=\"text/html; charset=") +
         localeStr +
         wxT("\"></head>") +
      wxT("<body bgcolor=\"#ffffff\"><center>") +
      wxT("<h3>Audacity ") + wxString(AUDACITY_VERSION_STRING) + wxT("</h3>")+
      _("free, open source, cross-platform software for recording and editing sounds<br>") +
      wxT("<a href=\"http://audacityteam.org/\">http://audacityteam.org/</a>") +
      wxT("<p><br>") + par1Str +
      wxT("<p>") + par2Str +
      wxT("<h3>") + _("Credits") + wxT("</h3>") +
      wxT("<p>") + translatorCredits +

      wxT("<p><b>") + wxString::Format(_("Audacity Developers")) + wxT("</b><br>") +
      GetCreditsByRole(roleTeamDeveloper) +

      wxT("<p><b>") + wxString::Format(_("Audacity Support Team")) + wxT("</b><br>") +
      GetCreditsByRole(roleTeamSupport) +

      wxT("<p><b>") + _("Emeritus Developers") + wxT("</b><br>") +
      GetCreditsByRole(roleEmeritusDeveloper) +

      wxT("<p><b>") + _(" Emeritus Team Members") + wxT("</b><br>") +
      GetCreditsByRole(roleEmeritusSupport) +

      wxT("<p><b>") + _("Other Contributors") + wxT("</b><br>") +
      GetCreditsByRole(roleContributor) +

      wxT("<p><b>") +  _("Audacity is based on code from the following projects:") + wxT("</b><br>") +
      GetCreditsByRole(roleLibrary) +

      wxT("<p><b>") +  _("Special thanks:") + wxT("</b><br>") +
      GetCreditsByRole(roleThanks) +

      wxT("<p><br>") + _("<b>Audacity&reg;</b> software is copyright")+
      wxT("&copy; 1999-2015 Audacity Team.<br>") +
      _("The name <b>Audacity&reg;</b> is a registered trademark of Dominic Mazzoni.") +
      wxT("</center></font></body></html>");


   this->SetBackgroundColour(theTheme.Colour( clrAboutBoxBackground ));


   // New way to add to About box....
   S.StartNotebookPage( wxT("Audacity") );
   S.StartVerticalLay(1);

   //v For now, change to AudacityLogoWithName via old-fashioned way, not Theme.
   logo = new wxBitmap((const char **) AudacityLogoWithName_xpm); //v

   // JKC: Resize to 50% of size.  Later we may use a smaller xpm as
   // our source, but this allows us to tweak the size - if we want to.
   // It also makes it easier to revert to full size if we decide to.
   const float fScale=0.5f;// smaller size.
   wxImage RescaledImage( logo->ConvertToImage() );
   // wxIMAGE_QUALITY_HIGH not supported by wxWidgets 2.6.1, or we would use it here.
   RescaledImage.Rescale( int(LOGOWITHNAME_WIDTH * fScale), int(LOGOWITHNAME_HEIGHT *fScale) );
   wxBitmap RescaledBitmap( RescaledImage );

   icon =
       new wxStaticBitmap(S.GetParent(), -1,
                          //*logo, //v
                          //v theTheme.Bitmap(bmpAudacityLogo), wxPoint(93, 10), wxSize(215, 190));
                          //v theTheme.Bitmap(bmpAudacityLogoWithName),
                          RescaledBitmap,
                          wxDefaultPosition,
                          wxSize(int(LOGOWITHNAME_WIDTH*fScale), int(LOGOWITHNAME_HEIGHT*fScale)));
   delete logo;
   S.Prop(0).AddWindow( icon );

   HtmlWindow *html = new LinkingHtmlWindow(S.GetParent(), -1,
                                         wxDefaultPosition,
                                         wxSize(ABOUT_DIALOG_WIDTH, 359),
                                         wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);
   html->SetFocus();
   html->SetPage(creditStr);

   /* locate the html renderer where it fits in the dialogue */
   S.Prop(1).AddWindow( html, wxEXPAND );

   S.EndVerticalLay();
   S.EndNotebookPage();
}

/** \brief: Fills out the "Information" tab of the preferences dialogue
 *
 * Provides as much information as possible about build-time options and
 * the libraries used, to try and make Linux support easier. Basically anything
 * about the build we might wish to know should be visible here */
void AboutDialog::PopulateInformationPage( ShuttleGui & S )
{
   wxString informationStr;   // string to build up list of information in
   S.StartNotebookPage( _("Build Information") );  // start the tab
   S.StartVerticalLay(2);  // create the window
   HtmlWindow *html = new LinkingHtmlWindow(S.GetParent(), -1, wxDefaultPosition,
                           wxSize(ABOUT_DIALOG_WIDTH, 264),
                           wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);
   // create a html pane in it to put the content in.
   wxString enabled = _("Enabled");
   wxString disabled = _("Disabled");
   wxString blank = wxT("");

   /* this builds up the list of information to go in the window in the string
    * informationStr */
   informationStr = wxT("<h2><center>");
   informationStr += _("Build Information");
   informationStr += wxT("</center></h2>\n");
   // top level heading
   informationStr += wxT("<h3>");
   informationStr += _("File Format Support");
   informationStr += wxT("</h3>\n<p>");
   // 2nd level headings to split things up a bit


   informationStr += wxT("<table>");   // start table of libraries


   #ifdef USE_LIBMAD
   /* i18n-hint: This is what the library (libmad) does - imports MP3 files */
   AddBuildinfoRow(&informationStr, wxT("libmad"), _("MP3 Importing"), enabled);
   #else
   AddBuildinfoRow(&informationStr, wxT("libmad"), _("MP3 Importing"), disabled);
   #endif

   /* i18n-hint: Ogg is the container format. Vorbis is the compression codec.
    * Both are proper nouns and shouldn't be translated */
   #ifdef USE_LIBVORBIS
   AddBuildinfoRow(&informationStr, wxT("libvorbis"),
         _("Ogg Vorbis Import and Export"), enabled);
   #else
   AddBuildinfoRow(&informationStr, wxT("libvorbis"),
         _("Ogg Vorbis Import and Export"), disabled);
   #endif

   #ifdef USE_LIBID3TAG
   AddBuildinfoRow(&informationStr, wxT("libid3tag"), _("ID3 tag support"),
         enabled);
   #else
   AddBuildinfoRow(&informationStr, wxT("libid3tag"), _("ID3 tag support"),
         disabled);
   #endif

   /* i18n-hint: FLAC stands for Free Lossless Audio Codec, but is effectively
    * a proper noun and so shouldn't be translated */
   # if USE_LIBFLAC
   AddBuildinfoRow(&informationStr, wxT("libflac"), _("FLAC import and export"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("libflac"), _("FLAC import and export"),
         disabled);
   # endif

   # if USE_LIBTWOLAME
   AddBuildinfoRow(&informationStr, wxT("libtwolame"), _("MP2 export"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("libtwolame"), _("MP2 export"),
         disabled);
   # endif

   # if USE_QUICKTIME
   AddBuildinfoRow(&informationStr, wxT("QuickTime"), _("Import via QuickTime"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("QuickTime"), _("Import via QuickTime"),
         disabled);
   # endif

   #ifdef USE_FFMPEG
   AddBuildinfoRow(&informationStr, wxT("ffmpeg"), _("FFmpeg Import/Export"), enabled);
   #else
   AddBuildinfoRow(&informationStr, wxT("ffmpeg"), _("FFmpeg Import/Export"), disabled);
   #endif

   #ifdef USE_GSTREAMER
   AddBuildinfoRow(&informationStr, wxT("gstreamer"), _("Import via GStreamer"), enabled);
   #else
   AddBuildinfoRow(&informationStr, wxT("gstreamer"), _("Import via GStreamer"), disabled);
   #endif

   informationStr += wxT("</table>\n");  //end table of file format libraries
   informationStr += wxT("<h3>");
   /* i18n-hint: Libraries that are essential to audacity */
   informationStr += _("Core Libraries");
   informationStr += wxT("</h3>\n<table>");  // start table of features

   AddBuildinfoRow(&informationStr, wxT("libsoxr"),
         _("Sample rate conversion"), enabled);

   AddBuildinfoRow(&informationStr, wxT("PortAudio"),
         _("Audio playback and recording"), wxString(wxT("v19")));

   informationStr += wxT("<tr><td>");  // start new row
   // wxWidgets version:
   informationStr += wxVERSION_STRING;
   informationStr += wxT("</td><td/><td>");
   informationStr += wxT("</td></tr>\n");   // end of row

   informationStr += wxT("</table>\n");  //end table of libraries
   informationStr += wxT("<h3>");
   informationStr += _("Features");
   informationStr += wxT("</h3>\n<table>");  // start table of features

   # if USE_NYQUIST
   AddBuildinfoRow(&informationStr, wxT("Nyquist"), _("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("Nyquist"), _("Plug-in support"),
         disabled);
   # endif

   # if USE_LADSPA
   AddBuildinfoRow(&informationStr, wxT("LADSPA"), _("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("LADSPA"), _("Plug-in support"),
         disabled);
   # endif

   # if USE_VAMP
   AddBuildinfoRow(&informationStr, wxT("Vamp"), _("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("Vamp"), _("Plug-in support"),
         disabled);
   # endif

   # if USE_AUDIO_UNITS
   AddBuildinfoRow(&informationStr, wxT("Audio Units"), _("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("Audio Units"), _("Plug-in support"),
         disabled);
   # endif

   # if USE_VST
   AddBuildinfoRow(&informationStr, wxT("VST"), _("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("VST"), _("Plug-in support"),
         disabled);
   # endif

   # if USE_LV2
   AddBuildinfoRow(&informationStr, wxT("LV2"), _("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("LV2"), _("Plug-in support"),
         disabled);
   # endif

   # if USE_PORTMIXER
   AddBuildinfoRow(&informationStr, wxT("PortMixer"), _("Sound card mixer support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("PortMixer"), _("Sound card mixer support"),
         disabled);
   # endif

   # if USE_SOUNDTOUCH
   AddBuildinfoRow(&informationStr, wxT("SoundTouch"), _("Pitch and Tempo Change support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("SoundTouch"), _("Pitch and Tempo Change support"),
         disabled);
   # endif

   # if USE_SBSMS
   AddBuildinfoRow(&informationStr, wxT("SBSMS"), _("Extreme Pitch and Tempo Change support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, wxT("SBSMS"), _("Extreme Pitch and Tempo Change support"),
         disabled);
   # endif

   informationStr += wxT("</table>\n");   // end of table of features

   informationStr += wxT("<h3>");
   /* i18n-hint: Information about when audacity was compiled */
   informationStr += _("Build Information");
   informationStr += wxT("</h3>\n<table>");

   // Current date
   AddBuildinfoRow(&informationStr, _("Program build date: "), __TDATE__);

// Uncomment the next two lines to test hyperlinks work from here.
//   AddBuildinfoRow(&informationStr, wxT("Link Test:"), 
//      wxT("<a href=\"https:web.audacityteam.org\">Click bait</a>") );

   AddBuildinfoRow(&informationStr, _("Commit Id:"),
#include "RevisionIdent.h"
);

#ifdef __WXDEBUG__
   AddBuildinfoRow(&informationStr, _("Build type:"), _("Debug build"));
#else
   AddBuildinfoRow(&informationStr, _("Build type:"), _("Release build"));
#endif

   // Install prefix
   /* i18n-hint: The directory audacity is installed into (on *nix systems) */
   AddBuildinfoRow(&informationStr, _("Installation Prefix: "), \
         wxT(INSTALL_PREFIX));

   // Location of settings
   AddBuildinfoRow(&informationStr,_("Settings folder: "), \
      FileNames::DataDir());
   // end of table
   informationStr += wxT("</table>\n");

   html->SetPage(informationStr);   // push the page into the html renderer
   S.Prop(2).AddWindow( html, wxEXPAND ); // make it fill the page
   // I think the 2 here goes with the StartVerticalLay() call above?
   S.EndVerticalLay();     // end window
   S.EndNotebookPage(); // end the tab
}


void AboutDialog::PopulateLicensePage( ShuttleGui & S )
{
   S.StartNotebookPage( _("GPL License") );
   S.StartVerticalLay(1);
   HtmlWindow *html = new LinkingHtmlWindow(S.GetParent(), -1,
                                         wxDefaultPosition,
                                         wxSize(ABOUT_DIALOG_WIDTH, 264),
                                         wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);

// I tried using <pre> here to get a monospaced font,
// as is normally used for the GPL.
// However can't reduce the font size in that case.  It looks
// better proportionally spaced.
//
// The GPL is not to be translated....
   wxString PageText=
wxT("		    <center>GNU GENERAL PUBLIC LICENSE\n</center>")
wxT("		       <center>Version 2, June 1991\n</center>")
wxT("<p><p>")
wxT(" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n")
wxT(" 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA\n")
wxT(" Everyone is permitted to copy and distribute verbatim copies\n")
wxT(" of this license document, but changing it is not allowed.\n")
wxT("\n")
wxT("			   <center>Preamble\n</center>")
wxT("<p><p>\n")
wxT("  The licenses for most software are designed to take away your\n")
wxT("freedom to share and change it.  By contrast, the GNU General Public\n")
wxT("License is intended to guarantee your freedom to share and change free\n")
wxT("software--to make sure the software is free for all its users.  This\n")
wxT("General Public License applies to most of the Free Software\n")
wxT("Foundation's software and to any other program whose authors commit to\n")
wxT("using it.  (Some other Free Software Foundation software is covered by\n")
wxT("the GNU Library General Public License instead.)  You can apply it to\n")
wxT("your programs, too.\n")
wxT("<p><p>\n")
wxT("  When we speak of free software, we are referring to freedom, not\n")
wxT("price.  Our General Public Licenses are designed to make sure that you\n")
wxT("have the freedom to distribute copies of free software (and charge for\n")
wxT("this service if you wish), that you receive source code or can get it\n")
wxT("if you want it, that you can change the software or use pieces of it\n")
wxT("in new free programs; and that you know you can do these things.\n")
wxT("<p><p>\n")
wxT("  To protect your rights, we need to make restrictions that forbid\n")
wxT("anyone to deny you these rights or to ask you to surrender the rights.\n")
wxT("These restrictions translate to certain responsibilities for you if you\n")
wxT("distribute copies of the software, or if you modify it.\n")
wxT("<p><p>\n")
wxT("  For example, if you distribute copies of such a program, whether\n")
wxT("gratis or for a fee, you must give the recipients all the rights that\n")
wxT("you have.  You must make sure that they, too, receive or can get the\n")
wxT("source code.  And you must show them these terms so they know their\n")
wxT("rights.\n")
wxT("<p><p>\n")
wxT("  We protect your rights with two steps: (1) copyright the software, and\n")
wxT("(2) offer you this license which gives you legal permission to copy,\n")
wxT("distribute and/or modify the software.\n")
wxT("<p><p>\n")
wxT("  Also, for each author's protection and ours, we want to make certain\n")
wxT("that everyone understands that there is no warranty for this free\n")
wxT("software.  If the software is modified by someone else and passed on, we\n")
wxT("want its recipients to know that what they have is not the original, so\n")
wxT("that any problems introduced by others will not reflect on the original\n")
wxT("authors' reputations.\n")
wxT("<p><p>\n")
wxT("  Finally, any free program is threatened constantly by software\n")
wxT("patents.  We wish to avoid the danger that redistributors of a free\n")
wxT("program will individually obtain patent licenses, in effect making the\n")
wxT("program proprietary.  To prevent this, we have made it clear that any\n")
wxT("patent must be licensed for everyone's free use or not licensed at all.\n")
wxT("<p><p>\n")
wxT("  The precise terms and conditions for copying, distribution and\n")
wxT("modification follow.\n")
wxT("<p><p>\n")
wxT("		   <center>GNU GENERAL PUBLIC LICENSE\n</center>")
wxT("   <center>TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n</center>")
wxT("<p><p>\n")
wxT("  0. This License applies to any program or other work which contains\n")
wxT("a notice placed by the copyright holder saying it may be distributed\n")
wxT("under the terms of this General Public License.  The \"Program\", below,\n")
wxT("refers to any such program or work, and a \"work based on the Program\"\n")
wxT("means either the Program or any derivative work under copyright law:\n")
wxT("that is to say, a work containing the Program or a portion of it,\n")
wxT("either verbatim or with modifications and/or translated into another\n")
wxT("language.  (Hereinafter, translation is included without limitation in\n")
wxT("the term \"modification\".)  Each licensee is addressed as \"you\".\n")
wxT("<p><p>\n")
wxT("Activities other than copying, distribution and modification are not\n")
wxT("covered by this License; they are outside its scope.  The act of\n")
wxT("running the Program is not restricted, and the output from the Program\n")
wxT("is covered only if its contents constitute a work based on the\n")
wxT("Program (independent of having been made by running the Program).\n")
wxT("Whether that is true depends on what the Program does.\n")
wxT("<p><p>\n")
wxT("  1. You may copy and distribute verbatim copies of the Program's\n")
wxT("source code as you receive it, in any medium, provided that you\n")
wxT("conspicuously and appropriately publish on each copy an appropriate\n")
wxT("copyright notice and disclaimer of warranty; keep intact all the\n")
wxT("notices that refer to this License and to the absence of any warranty;\n")
wxT("and give any other recipients of the Program a copy of this License\n")
wxT("along with the Program.\n")
wxT("<p><p>\n")
wxT("You may charge a fee for the physical act of transferring a copy, and\n")
wxT("you may at your option offer warranty protection in exchange for a fee.\n")
wxT("<p><p>\n")
wxT("  2. You may modify your copy or copies of the Program or any portion\n")
wxT("of it, thus forming a work based on the Program, and copy and\n")
wxT("distribute such modifications or work under the terms of Section 1\n")
wxT("above, provided that you also meet all of these conditions:\n")
wxT("<p><p>\n")
wxT("<blockquote>")
wxT("    a) You must cause the modified files to carry prominent notices\n")
wxT("    stating that you changed the files and the date of any change.\n")
wxT("<p><p>\n")
wxT("    b) You must cause any work that you distribute or publish, that in\n")
wxT("    whole or in part contains or is derived from the Program or any\n")
wxT("    part thereof, to be licensed as a whole at no charge to all third\n")
wxT("    parties under the terms of this License.\n")
wxT("<p><p>\n")
wxT("    c) If the modified program normally reads commands interactively\n")
wxT("    when run, you must cause it, when started running for such\n")
wxT("    interactive use in the most ordinary way, to print or display an\n")
wxT("    announcement including an appropriate copyright notice and a\n")
wxT("    notice that there is no warranty (or else, saying that you provide\n")
wxT("    a warranty) and that users may redistribute the program under\n")
wxT("    these conditions, and telling the user how to view a copy of this\n")
wxT("    License.  (Exception: if the Program itself is interactive but\n")
wxT("    does not normally print such an announcement, your work based on\n")
wxT("    the Program is not required to print an announcement.)\n")
wxT("</blockquote>")
wxT("<p><p>\n")
wxT("These requirements apply to the modified work as a whole.  If\n")
wxT("identifiable sections of that work are not derived from the Program,\n")
wxT("and can be reasonably considered independent and separate works in\n")
wxT("themselves, then this License, and its terms, do not apply to those\n")
wxT("sections when you distribute them as separate works.  But when you\n")
wxT("distribute the same sections as part of a whole which is a work based\n")
wxT("on the Program, the distribution of the whole must be on the terms of\n")
wxT("this License, whose permissions for other licensees extend to the\n")
wxT("entire whole, and thus to each and every part regardless of who wrote it.\n")
wxT("<p><p>\n")
wxT("Thus, it is not the intent of this section to claim rights or contest\n")
wxT("your rights to work written entirely by you; rather, the intent is to\n")
wxT("exercise the right to control the distribution of derivative or\n")
wxT("collective works based on the Program.\n")
wxT("<p><p>\n")
wxT("In addition, mere aggregation of another work not based on the Program\n")
wxT("with the Program (or with a work based on the Program) on a volume of\n")
wxT("a storage or distribution medium does not bring the other work under\n")
wxT("the scope of this License.\n")
wxT("<p><p>\n")
wxT("  3. You may copy and distribute the Program (or a work based on it,\n")
wxT("under Section 2) in object code or executable form under the terms of\n")
wxT("Sections 1 and 2 above provided that you also do one of the following:\n")
wxT("<p><p>\n")
wxT("<blockquote>")
wxT("    a) Accompany it with the complete corresponding machine-readable\n")
wxT("    source code, which must be distributed under the terms of Sections\n")
wxT("    1 and 2 above on a medium customarily used for software interchange; or,\n")
wxT("<p><p>\n")
wxT("    b) Accompany it with a written offer, valid for at least three\n")
wxT("    years, to give any third party, for a charge no more than your\n")
wxT("    cost of physically performing source distribution, a complete\n")
wxT("    machine-readable copy of the corresponding source code, to be\n")
wxT("    distributed under the terms of Sections 1 and 2 above on a medium\n")
wxT("    customarily used for software interchange; or,\n")
wxT("<p><p>\n")
wxT("    c) Accompany it with the information you received as to the offer\n")
wxT("    to distribute corresponding source code.  (This alternative is\n")
wxT("    allowed only for noncommercial distribution and only if you\n")
wxT("    received the program in object code or executable form with such\n")
wxT("    an offer, in accord with Subsection b above.)\n")
wxT("</blockquote>")
wxT("<p><p>\n")
wxT("The source code for a work means the preferred form of the work for\n")
wxT("making modifications to it.  For an executable work, complete source\n")
wxT("code means all the source code for all modules it contains, plus any\n")
wxT("associated interface definition files, plus the scripts used to\n")
wxT("control compilation and installation of the executable.  However, as a\n")
wxT("special exception, the source code distributed need not include\n")
wxT("anything that is normally distributed (in either source or binary\n")
wxT("form) with the major components (compiler, kernel, and so on) of the\n")
wxT("operating system on which the executable runs, unless that component\n")
wxT("itself accompanies the executable.\n")
wxT("<p><p>\n")
wxT("If distribution of executable or object code is made by offering\n")
wxT("access to copy from a designated place, then offering equivalent\n")
wxT("access to copy the source code from the same place counts as\n")
wxT("distribution of the source code, even though third parties are not\n")
wxT("compelled to copy the source along with the object code.\n")
wxT("<p><p>\n")
wxT("  4. You may not copy, modify, sublicense, or distribute the Program\n")
wxT("except as expressly provided under this License.  Any attempt\n")
wxT("otherwise to copy, modify, sublicense or distribute the Program is\n")
wxT("void, and will automatically terminate your rights under this License.\n")
wxT("However, parties who have received copies, or rights, from you under\n")
wxT("this License will not have their licenses terminated so long as such\n")
wxT("parties remain in full compliance.\n")
wxT("<p><p>\n")
wxT("  5. You are not required to accept this License, since you have not\n")
wxT("signed it.  However, nothing else grants you permission to modify or\n")
wxT("distribute the Program or its derivative works.  These actions are\n")
wxT("prohibited by law if you do not accept this License.  Therefore, by\n")
wxT("modifying or distributing the Program (or any work based on the\n")
wxT("Program), you indicate your acceptance of this License to do so, and\n")
wxT("all its terms and conditions for copying, distributing or modifying\n")
wxT("the Program or works based on it.\n")
wxT("<p><p>\n")
wxT("  6. Each time you redistribute the Program (or any work based on the\n")
wxT("Program), the recipient automatically receives a license from the\n")
wxT("original licensor to copy, distribute or modify the Program subject to\n")
wxT("these terms and conditions.  You may not impose any further\n")
wxT("restrictions on the recipients' exercise of the rights granted herein.\n")
wxT("You are not responsible for enforcing compliance by third parties to\n")
wxT("this License.\n")
wxT("<p><p>\n")
wxT("  7. If, as a consequence of a court judgment or allegation of patent\n")
wxT("infringement or for any other reason (not limited to patent issues),\n")
wxT("conditions are imposed on you (whether by court order, agreement or\n")
wxT("otherwise) that contradict the conditions of this License, they do not\n")
wxT("excuse you from the conditions of this License.  If you cannot\n")
wxT("distribute so as to satisfy simultaneously your obligations under this\n")
wxT("License and any other pertinent obligations, then as a consequence you\n")
wxT("may not distribute the Program at all.  For example, if a patent\n")
wxT("license would not permit royalty-free redistribution of the Program by\n")
wxT("all those who receive copies directly or indirectly through you, then\n")
wxT("the only way you could satisfy both it and this License would be to\n")
wxT("refrain entirely from distribution of the Program.\n")
wxT("<p><p>\n")
wxT("If any portion of this section is held invalid or unenforceable under\n")
wxT("any particular circumstance, the balance of the section is intended to\n")
wxT("apply and the section as a whole is intended to apply in other\n")
wxT("circumstances.\n")
wxT("<p><p>\n")
wxT("It is not the purpose of this section to induce you to infringe any\n")
wxT("patents or other property right claims or to contest validity of any\n")
wxT("such claims; this section has the sole purpose of protecting the\n")
wxT("integrity of the free software distribution system, which is\n")
wxT("implemented by public license practices.  Many people have made\n")
wxT("generous contributions to the wide range of software distributed\n")
wxT("through that system in reliance on consistent application of that\n")
wxT("system; it is up to the author/donor to decide if he or she is willing\n")
wxT("to distribute software through any other system and a licensee cannot\n")
wxT("impose that choice.\n")
wxT("<p><p>\n")
wxT("This section is intended to make thoroughly clear what is believed to\n")
wxT("be a consequence of the rest of this License.\n")
wxT("<p><p>\n")
wxT("  8. If the distribution and/or use of the Program is restricted in\n")
wxT("certain countries either by patents or by copyrighted interfaces, the\n")
wxT("original copyright holder who places the Program under this License\n")
wxT("may add an explicit geographical distribution limitation excluding\n")
wxT("those countries, so that distribution is permitted only in or among\n")
wxT("countries not thus excluded.  In such case, this License incorporates\n")
wxT("the limitation as if written in the body of this License.\n")
wxT("<p><p>\n")
wxT("  9. The Free Software Foundation may publish revised and/or new versions\n")
wxT("of the General Public License from time to time.  Such new versions will\n")
wxT("be similar in spirit to the present version, but may differ in detail to\n")
wxT("address new problems or concerns.\n")
wxT("<p><p>\n")
wxT("Each version is given a distinguishing version number.  If the Program\n")
wxT("specifies a version number of this License which applies to it and \"any\n")
wxT("later version\", you have the option of following the terms and conditions\n")
wxT("either of that version or of any later version published by the Free\n")
wxT("Software Foundation.  If the Program does not specify a version number of\n")
wxT("this License, you may choose any version ever published by the Free Software\n")
wxT("Foundation.\n")
wxT("<p><p>\n")
wxT("  10. If you wish to incorporate parts of the Program into other free\n")
wxT("programs whose distribution conditions are different, write to the author\n")
wxT("to ask for permission.  For software which is copyrighted by the Free\n")
wxT("Software Foundation, write to the Free Software Foundation; we sometimes\n")
wxT("make exceptions for this.  Our decision will be guided by the two goals\n")
wxT("of preserving the free status of all derivatives of our free software and\n")
wxT("of promoting the sharing and reuse of software generally.\n")
wxT("<p><p>\n")
wxT("			    <center>NO WARRANTY\n</center>")
wxT("<p><p>\n")
wxT("  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n")
wxT("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n")
wxT("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n")
wxT("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n")
wxT("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n")
wxT("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n")
wxT("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n")
wxT("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n")
wxT("REPAIR OR CORRECTION.\n")
wxT("<p><p>\n")
wxT("  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n")
wxT("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n")
wxT("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n")
wxT("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n")
wxT("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n")
wxT("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n")
wxT("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n")
wxT("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n")
wxT("POSSIBILITY OF SUCH DAMAGES.\n");

   html->SetPage( PageText );

   S.Prop(1).AddWindow( html, wxEXPAND );

   S.EndVerticalLay();
   S.EndNotebookPage();
}

void AboutDialog::AddCredit(const wxString& description, Role role)
{
   AboutDialogCreditItem* item = new AboutDialogCreditItem();
   item->description = description;
   item->role = role;
   creditItems.Append(item);
}

wxString AboutDialog::GetCreditsByRole(AboutDialog::Role role)
{
   wxString s;

   for (AboutDialogCreditItemsList::compatibility_iterator p=creditItems.GetFirst(); p; p = p->GetNext())
   {
      AboutDialogCreditItem* item = p->GetData();
      if (item->role == role)
      {
         s += item->description;
         s += wxT("<br>");
      }
   }

   // Strip last <br>, if any
   if (s.Right(4) == wxT("<br>"))
      s = s.Left(s.Length() - 4);

   return s;
}

/** \brief Add a table row saying if a library is used or not
 *
 * Used when creating the build information tab to show if each optional
 * library is enabled or not, and what it does */
void AboutDialog::AddBuildinfoRow( wxString* htmlstring, const wxChar * libname, const wxChar * libdesc, wxString status)
{
   *htmlstring += wxT("<tr><td>");
   *htmlstring += libname;
   *htmlstring += wxT("</td><td>(");
   *htmlstring += libdesc;
   *htmlstring += wxT(")</td><td>");
   *htmlstring += status;
   *htmlstring += wxT("</td></tr>");
}

/** \brief Add a table row saying if a library is used or not
 *
 * Used when creating the build information tab to show build dates and
 * file paths */
void AboutDialog::AddBuildinfoRow( wxString* htmlstring, const wxChar * libname, const wxChar * libdesc)
{
   *htmlstring += wxT("<tr><td>");
   *htmlstring += libname;
   *htmlstring += wxT("</td><td>");
   *htmlstring += libdesc;
   *htmlstring += wxT("</td></tr>");
}


AboutDialog::~AboutDialog()
{
   delete icon;
//   delete logo;
}

void AboutDialog::OnOK(wxCommandEvent & WXUNUSED(event))
{
   EndModal(wxID_OK);
}
