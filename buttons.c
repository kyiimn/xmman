/*

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * xman - X window system manual page display program.
 * Author:    Chris D. Peterson, MIT Project Athena
 * Created:   October 27, 1987
 */

#include "globals.h"
#include "vendor.h"
#include "scroll_motiveP.h"

/* The files with the icon bits in them. */

#include "icon_open.h"
#include "icon_help.h"
#include "iconclosed.h"

static void CreateOptionMenu(ManpageGlobals * man_globals, Widget parent);
static void CreateSectionMenu(ManpageGlobals * man_globals, Widget parent);
static void StartManpage(ManpageGlobals * man_globals, Boolean help,
                         Boolean page);


/*	Function Name: MakeTopBox
 *	Description: This function creates the top menu, in a shell widget.
 *	Arguments: none.
 *	Returns: the top level widget
 */

Widget top;                     /* needed in PopupWarning, misc.c */

void
MakeTopBox(void)
{
    Widget form, label, help_cmd, quit_cmd, manpage_cmd;
    Arg args[8];
    Cardinal n;
    XmString label_str;
    ManpageGlobals *man_globals;

/* create the top icon. */

    n = 0;
    XtSetArg(args[n], XtNiconPixmap,
             XCreateBitmapFromData(XtDisplay(initial_widget),
                                   XtScreen(initial_widget)->root,
                                   (const char *) iconclosed_bits,
                                   iconclosed_width, iconclosed_height));
    n++;
    XtSetArg(args[n], XtNtitle, resources.title);
    n++;
    XtSetArg(args[n], XtNiconic, resources.iconic);
    n++;
    top = XtCreatePopupShell(TOPBOXNAME, topLevelShellWidgetClass,
                             initial_widget, args, n);

    form = XtCreateManagedWidget("form", xmFormWidgetClass, top,
                                 NULL, (Cardinal) 0);

    /* topLabel: spans full width, attached to top of form */
    label_str = XmStringCreateLocalized("Xman");
    n = 0;
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    label = XtCreateManagedWidget("topLabel", xmLabelWidgetClass, form,
                                  args, n);
    XmStringFree(label_str);

    /* Help button: left side, below the label */
    label_str = XmStringCreateLocalized("Help");
    n = 0;
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, label); n++;
    help_cmd = XtCreateManagedWidget(HELP_BUTTON, xmPushButtonWidgetClass,
                                     form, args, n);
    XmStringFree(label_str);
    XtOverrideTranslations(help_cmd,
        XtParseTranslationTable("<Btn1Down>: Arm() <Btn1Up>: Activate() Disarm()"));

    /* Quit button: right of Help, same row */
    label_str = XmStringCreateLocalized("Quit");
    n = 0;
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, help_cmd); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, label); n++;
    quit_cmd = XtCreateManagedWidget(QUIT_BUTTON, xmPushButtonWidgetClass,
                                     form, args, n);
    XmStringFree(label_str);
    XtOverrideTranslations(quit_cmd,
        XtParseTranslationTable("<Btn1Down>: Arm() <Btn1Up>: Activate() Disarm()"));

    /* Manpage button: full width, below Help/Quit row */
    label_str = XmStringCreateLocalized("Manual Page");
    n = 0;
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, help_cmd); n++;
    manpage_cmd = XtCreateManagedWidget(MANPAGE_BUTTON, xmPushButtonWidgetClass,
                                        form, args, n);
    XmStringFree(label_str);
    XtOverrideTranslations(manpage_cmd,
        XtParseTranslationTable("<Btn1Down>: Arm() <Btn1Up>: Activate() Disarm()"));

    help_widget = NULL;         /* We have not seen the help yet. */

    XtRealizeWidget(top);
    /* add WM_COMMAND property */
    XSetCommand(XtDisplay(top), XtWindow(top), saved_argv, saved_argc);

    man_globals =
        (ManpageGlobals *) XtCalloc(ONE, (Cardinal) sizeof(ManpageGlobals));
    MakeSearchWidget(man_globals, top);
    MakeSaveWidgets(man_globals, top);

    SaveGlobals((man_globals->This_Manpage = top), man_globals);
    XtMapWidget(top);
    AddCursor(top, resources.cursors.top);

/*
 * Set up ICCCM delete window.
 */
    XtOverrideTranslations
        (top, XtParseTranslationTable("<Message>WM_PROTOCOLS: Quit()"));
    (void) XSetWMProtocols(XtDisplay(top), XtWindow(top), &wm_delete_window, 1);

}

/*	Function Name: CreateManpage
 *	Description: Creates a new manpage.
 *	Arguments: none.
 *	Returns: none.
 */

Widget
CreateManpage(FILE * file)
{
    ManpageGlobals *man_globals;        /* The pseudo global structure. */

    man_globals = InitPsuedoGlobals();
    if (man_globals == NULL)
        return NULL;
    CreateManpageWidget(man_globals, MANNAME, TRUE);

    if (file == NULL)
        StartManpage(man_globals, OpenHelpfile(man_globals), FALSE);
    else {
        OpenFile(man_globals, file);
        StartManpage(man_globals, FALSE, TRUE);
    }
    return (man_globals->This_Manpage);
}

/*	Function Name: InitPsuedoGlobals
 *	Description: Initializes the pseudo global variables.
 *	Arguments: none.
 *	Returns: a pointer to a new pseudo globals structure.
 */

ManpageGlobals *
InitPsuedoGlobals(void)
{
    ManpageGlobals *man_globals;

    /*
     * Allocate necessary memory.
     */

    man_globals =
        (ManpageGlobals *) XtCalloc(ONE, (Cardinal) sizeof(ManpageGlobals));
    if (!man_globals)
        return NULL;

    man_globals->search_widget = NULL;
    man_globals->section_name = (char **) XtMalloc((Cardinal) (sections *
                                                               sizeof(char *)));
    man_globals->manpagewidgets.box = (Widget *) XtCalloc((Cardinal) sections,
                                                          (Cardinal)
                                                          sizeof(Widget));

    /* Initialize the number of screens that will be shown */

    man_globals->both_shown = resources.both_shown_initial;

    return (man_globals);
}

/*	Function Name: CreateManpageWidget
 *	Description: Creates a new manual page widget.
 *	Arguments: man_globals - a new man_globals structure.
 *                 name         - name of this shell widget instance.
 *                 full_instance - if true then create a full manpage,
 *                                 otherwise create stripped down version
 *                                 used for help.
 *	Returns: none
 */

#define MANPAGEARGS 10

void
CreateManpageWidget(ManpageGlobals * man_globals,
                    String name, Boolean full_instance)
{
    Arg arglist[MANPAGEARGS];
    Cardinal num_args;
    Widget mytop, pane, hpane, mysections = NULL;
    XmString label_str;
    Arg args[4];
    Cardinal n;
    ManPageWidgets *mpw = &(man_globals->manpagewidgets);

    num_args = (Cardinal) 0;
    XtSetArg(arglist[num_args], XtNwidth, default_width);
    num_args++;
    XtSetArg(arglist[num_args], XtNheight, default_height);
    num_args++;

    mytop = XtCreatePopupShell(name, topLevelShellWidgetClass, initial_widget,
                               arglist, num_args);

    man_globals->This_Manpage = mytop;
    num_args = 0;
    if (full_instance)
        XtSetArg(arglist[num_args], XtNiconPixmap,
                 XCreateBitmapFromData(XtDisplay(mytop), XtScreen(mytop)->root,
                                       (const char *) icon_open_bits,
                                       icon_open_width, icon_open_height));
    else
        XtSetArg(arglist[num_args], XtNiconPixmap,
                 XCreateBitmapFromData(XtDisplay(mytop), XtScreen(mytop)->root,
                                       (const char *) icon_help_bits,
                                       icon_help_width, icon_help_height));
    num_args++;
    XtSetValues(mytop, arglist, num_args);

    pane = XtCreateManagedWidget("vertPane", xmPanedWindowWidgetClass, mytop,
                                 NULL, (Cardinal) 0);

    hpane = XtCreateManagedWidget("horizPane", xmPanedWindowWidgetClass,
                                  pane, NULL, (Cardinal) 0);

    CreateOptionMenu(man_globals, mytop);

    n = 0;
    XtSetArg(args[n], XmNsubMenuId, man_globals->option_menu); n++;
    label_str = XmStringCreateLocalized("Options");
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    (void) XtCreateManagedWidget("options", xmCascadeButtonWidgetClass,
                                 hpane, args, n);
    XmStringFree(label_str);

    if (full_instance) {
        CreateSectionMenu(man_globals, mytop);
        n = 0;
        XtSetArg(args[n], XmNsubMenuId, man_globals->section_menu); n++;
        label_str = XmStringCreateLocalized("Sections");
        XtSetArg(args[n], XmNlabelString, label_str); n++;
        mysections = XtCreateManagedWidget("sections", xmCascadeButtonWidgetClass,
                                           hpane, args, n);
        XmStringFree(label_str);
    }
    else {
        mysections = XtCreateManagedWidget("sections", xmCascadeButtonWidgetClass,
                                            hpane, NULL, (Cardinal) 0);
        XtSetSensitive(mysections, FALSE);
    }

    label_str = XmStringCreateLocalized(SHOW_BOTH);
    n = 0;
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    XtSetValues(man_globals->both_screens_entry, args, n);
    XmStringFree(label_str);

    if (full_instance) {
        MakeSearchWidget(man_globals, mytop);
        MakeSaveWidgets(man_globals, mytop);
    }
    else {
        XtSetArg(arglist[0], XtNsensitive, FALSE);
        XtSetValues(man_globals->dir_entry, arglist, ONE);
        XtSetValues(man_globals->manpage_entry, arglist, ONE);
        XtSetValues(man_globals->help_entry, arglist, ONE);
        XtSetValues(man_globals->search_entry, arglist, ONE);
        XtSetValues(man_globals->both_screens_entry, arglist, ONE);
    }

    label_str = XmStringCreateLocalized("manualTitle");
    n = 0;
    XtSetArg(args[n], XmNlabelString, label_str); n++;
    man_globals->label = XtCreateManagedWidget("manualTitle", xmLabelWidgetClass,
                                               hpane, args, n);
    XmStringFree(label_str);

    if (full_instance) {
        n = 0;
        XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;

        mpw->directory = XtCreateWidget(DIRECTORY_NAME, xmScrolledWindowWidgetClass,
                                         pane, args, n);

        man_globals->current_directory = INITIAL_DIR;
        MakeDirectoryBox(man_globals, mpw->directory,
                         mpw->box + man_globals->current_directory,
                         man_globals->current_directory);
        XtManageChild(mpw->box[man_globals->current_directory]);
    }

    mpw->manpage = XtCreateWidget(MANUALPAGE, scrollMotiveWidgetClass,
                                    pane, NULL, (Cardinal) 0);

    {
        ScrollMotiveWidget smw = (ScrollMotiveWidget) mpw->manpage;
        smw->scroll.fonts = XmanLoadManpageFonts(XtDisplay(mpw->manpage),
                                                  XScreenNumberOfScreen(XtScreen(mpw->manpage)));
        smw->scroll.font_height = XftGetFontHeight(smw->scroll.fonts->normal);
        smw->scroll.h_width = XftGetFontWidth(smw->scroll.fonts->normal);
    }
}

/*	Function Name: StartManpage
 *	Description: Starts up a new manpage.
 *	Arguments: man_globals - the pseudo globals variable.
 *                 help - is this a help file?
 *                 page - Is there a page to display?
 *	Returns: none.
 */

static void
StartManpage(ManpageGlobals * man_globals, Boolean help, Boolean page)
{
    Widget dir = man_globals->manpagewidgets.directory;
    Widget manpage = man_globals->manpagewidgets.manpage;
    Widget label = man_globals->label;
    Arg arglist[1];

/*
 * If there is a helpfile then put up both screens if both_show is set.
 */

    if (page || help) {
        if (help)
            strlcpy(man_globals->manpage_title, "Xman Help",
                    sizeof(man_globals->manpage_title));

        if (man_globals->both_shown) {
            XtManageChild(dir);
            man_globals->dir_shown = TRUE;

            XtSetArg(arglist[0], XmNpreferredPaneSize,
                     resources.directory_height);
            XtSetValues(dir, arglist, (Cardinal) 1);

            XtSetArg(arglist[0], XtNsensitive, FALSE);
            XtSetValues(man_globals->manpage_entry, arglist, ONE);
            XtSetValues(man_globals->dir_entry, arglist, ONE);

            {
                XmString str = XmStringCreateLocalized(SHOW_ONE);
                XtSetArg(arglist[0], XmNlabelString, str);
                XtSetValues(man_globals->both_screens_entry, arglist, ONE);
                XmStringFree(str);
            }
            ChangeLabel(label,
                        man_globals->section_name[man_globals->
                                                  current_directory]);
        }
        else {
            ChangeLabel(label, man_globals->manpage_title);
        }
        XtManageChild(manpage);
        man_globals->dir_shown = FALSE;
    }
/*
 * Since There is file to display, put up directory and do not allow change
 * to manpage, show both, or help.
 */
    else {
        XtManageChild(dir);
        man_globals->dir_shown = TRUE;
        XtSetArg(arglist[0], XtNsensitive, FALSE);
        XtSetValues(man_globals->manpage_entry, arglist, ONE);
        XtSetValues(man_globals->help_entry, arglist, ONE);
        XtSetValues(man_globals->both_screens_entry, arglist, ONE);
        man_globals->both_shown = FALSE;
        ChangeLabel(label,
                    man_globals->section_name[man_globals->current_directory]);
    }

/*
 * Start 'er up, and change the cursor.
 */

    XtRealizeWidget(man_globals->This_Manpage);
    SaveGlobals(man_globals->This_Manpage, man_globals);
    XtMapWidget(man_globals->This_Manpage);
    AddCursor(man_globals->This_Manpage, resources.cursors.manpage);
    XtSetArg(arglist[0], XtNtransientFor, man_globals->This_Manpage);
    XtSetValues(XtParent(man_globals->standby), arglist, (Cardinal) 1);
    XtSetValues(XtParent(man_globals->save), arglist, (Cardinal) 1);
    XtRealizeWidget(XtParent(man_globals->standby));
    XtRealizeWidget(XtParent(man_globals->save));
    AddCursor(XtParent(man_globals->standby), resources.cursors.top);
    AddCursor(XtParent(man_globals->save), resources.cursors.top);

/*
 * Set up ICCCM delete window.
 */
    XtOverrideTranslations
        (man_globals->This_Manpage,
         XtParseTranslationTable("<Message>WM_PROTOCOLS: RemoveThisManpage()"));
    (void) XSetWMProtocols(XtDisplay(man_globals->This_Manpage),
                           XtWindow(man_globals->This_Manpage),
                           &wm_delete_window, 1);

}

/*      Function Name: MenuDestroy
 *      Description: free's data associated with menu when it is destroyed.
 *      Arguments: w - menu widget.
 *                 free_me - data to free.
 *                 junk - not used.
 *      Returns: none.
 */

/* ARGSUSED */
static void
MenuDestroy(Widget w, XtPointer free_me, XtPointer junk)
{
    XtFree((char *) free_me);
}

/*      Function Name:   CreateOptionMenu
 *      Description: Create the option menu.
 *      Arguments: man_globals - the manual page globals.
 *                 parent - the button that activates the menu.
 *      Returns: none.
 */

static void
CreateOptionMenu(ManpageGlobals * man_globals, Widget parent)
{
    Widget menu, entry;
    int i;
    XmString label_str;
    Arg args[4];
    Cardinal n;
    static const char *option_names[] = {
        DIRECTORY,
        MANPAGE,
        HELP,
        SEARCH,
        BOTH_SCREENS,
        REMOVE_MANPAGE,
        OPEN_MANPAGE,
        SHOW_VERSION,
        QUIT
    };

    menu = XmCreatePulldownMenu(parent, OPTION_MENU, NULL, (Cardinal) 0);
    man_globals->option_menu = menu;

    for (i = 0; i < NUM_OPTIONS; i++) {
        label_str = XmStringCreateLocalized((String) option_names[i]);
        n = 0;
        XtSetArg(args[n], XmNlabelString, label_str); n++;
        entry = XtCreateManagedWidget(option_names[i], xmPushButtonWidgetClass,
                                      menu, args, n);
        XmStringFree(label_str);
        XtAddCallback(entry, XmNactivateCallback, OptionCallback,
                      (XtPointer) man_globals);
        switch (i) {
        case 0:
            man_globals->dir_entry = entry;
            break;
        case 1:
            man_globals->manpage_entry = entry;
            break;
        case 2:
            man_globals->help_entry = entry;
            break;
        case 3:
            man_globals->search_entry = entry;
            break;
        case 4:
            man_globals->both_screens_entry = entry;
            break;
        case 5:
            man_globals->remove_entry = entry;
            break;
        case 6:
            man_globals->open_entry = entry;
            break;
        case 7:
            man_globals->version_entry = entry;
            break;
        case 8:
            man_globals->quit_entry = entry;
            break;
        default:
            Error(("CreateOptionMenu: Unknown id=%d\n", i));
            break;
        }
    }
}

/*      Function Name: CreateSectionMenu
 *      Description: Create the Section menu.
 *      Arguments: man_globals - the manual page globals.
 *                 parent - the button that activates the menu.
 *      Returns: none.
 */

static void
CreateSectionMenu(ManpageGlobals * man_globals, Widget parent)
{
    Widget menu, entry;
    int i;
    MenuStruct *menu_struct;
    Arg args[2];
    Cardinal n;
    XmString label_str;
    char entry_name[BUFSIZ];

    menu = XmCreatePulldownMenu(parent, SECTION_MENU, NULL, (Cardinal) 0);
    man_globals->section_menu = menu;

    for (i = 0; i < sections; i++) {
        label_str = XmStringCreateLocalized(manual[i].blabel);
        n = 0;
        XtSetArg(args[n], XmNlabelString, label_str); n++;
        snprintf(entry_name, sizeof(entry_name), "section%d", i);

        entry = XtCreateManagedWidget(entry_name, xmPushButtonWidgetClass,
                                       menu, args, n);
        XmStringFree(label_str);
        menu_struct = (MenuStruct *) XtMalloc(sizeof(MenuStruct));
        menu_struct->data = (caddr_t) man_globals;
        menu_struct->number = i;
        XtAddCallback(entry, XmNactivateCallback, DirPopupCallback,
                      (caddr_t) menu_struct);
        XtAddCallback(entry, XtNdestroyCallback, MenuDestroy,
                      (caddr_t) menu_struct);
    }
}

/*	Function Name: MakeDirectoryBox
 *	Description: make a directory box.
 *	Arguments: man_globals - the pseudo global structure for each manpage.
 *                 parent - this guys parent widget.
 *                 dir_disp - the directory display widget.
 *                 section - the section number.
 *	Returns: none.
 */

void
MakeDirectoryBox(ManpageGlobals * man_globals, Widget parent, Widget * dir_disp,
                 int section)
{
    Widget list_w;
    Arg args[8];
    Cardinal n;
    XmStringTable str_list;
    char *name, label_name[BUFSIZ];
    int i;

    if (*dir_disp != NULL)      /* If we have one, don't make another. */
        return;

    name = manual[section].blabel;      /* Set the section name */
    snprintf(label_name, sizeof(label_name), "Directory of: %s", name);
    man_globals->section_name[section] = XtNewString(label_name);

    str_list = (XmStringTable) XtMalloc(
        (manual[section].nentries + 1) * sizeof(XmString));
    for (i = 0; i < manual[section].nentries; i++) {
        str_list[i] = XmStringCreateLocalized(
            CreateManpageName(manual[section].entries[i], section,
                              manual[section].flags));
    }
    str_list[i] = NULL;

    n = 0;
    XtSetArg(args[n], XmNitems, str_list); n++;
    XtSetArg(args[n], XmNitemCount, manual[section].nentries); n++;
    XtSetArg(args[n], XmNvisibleItemCount, 10); n++;
    XtSetArg(args[n], XmNselectionPolicy, XmBROWSE_SELECT); n++;
    list_w = XtCreateManagedWidget("dirList", xmListWidgetClass,
                                   parent, args, n);

    for (i = 0; i < manual[section].nentries; i++) {
        XmStringFree(str_list[i]);
    }
    XtFree((char *) str_list);

    XtAddCallback(list_w, XmNdefaultActionCallback,
                  DirectoryHandler, (XtPointer) man_globals);
    XtAddCallback(list_w, XmNsingleSelectionCallback,
                  DirectoryHandler, (XtPointer) man_globals);

    *dir_disp = list_w;
}

/*	Function Name: SaveOkCallback
 *	Description: Callback for the OK button on the save dialog.
 *                 Invokes SaveFormattedPage with "Save" parameter.
 *	Arguments: w - the MessageBox widget
 *                 client_data - not used
 *                 call_data - not used
 *	Returns: none.
 */

/*ARGSUSED*/
static void
SaveOkCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    String params[1] = {"Save"};
    Cardinal num_params = 1;

    SaveFormattedPage(w, NULL, params, &num_params);
}

/*	Function Name: SaveCancelCallback
 *	Description: Callback for the Cancel button on the save dialog.
 *                 Invokes SaveFormattedPage with "Cancel" parameter.
 *	Arguments: w - the MessageBox widget
 *                 client_data - not used
 *                 call_data - not used
 *	Returns: none.
 */

/*ARGSUSED*/
static void
SaveCancelCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    String params[1] = {"Cancel"};
    Cardinal num_params = 1;

    SaveFormattedPage(w, NULL, params, &num_params);
}

/*	Function Name: MakeSaveWidgets.
 *	Description: This functions creates two popup widgets, the please
 *                   standby widget and the would you like to save widget.
 *	Arguments: man_globals - the pseudo globals structure for each man page
 *                 parent - the realized parent for both popups.
 *	Returns: none.
 */

void
MakeSaveWidgets(ManpageGlobals * man_globals, Widget parent)
{
    Widget shell, dialog;       /* misc. widgets. */
    Arg args[4];
    Cardinal n;
    XmString label_str;

/* make the please stand by popup widget. */
    n = 0;
    if (XtIsRealized(parent)) {
        XtSetArg(args[n], XtNtransientFor, parent);
        n++;
    }
    shell = XtCreatePopupShell("pleaseStandBy", transientShellWidgetClass,
                               parent, args, n);

    label_str = XmStringCreateLocalized("Please Stand By");
    n = 0;
    XtSetArg(args[n], XmNmessageString, label_str); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION); n++;
    man_globals->standby = XmCreateMessageBox(shell, "standbyDialog", args, n);
    XmStringFree(label_str);
    XtUnmanageChild(XmMessageBoxGetChild(man_globals->standby,
                                         XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(man_globals->standby,
                                         XmDIALOG_HELP_BUTTON));
    XtManageChild(man_globals->standby);

/* make the would you like to save popup widget. */
    n = 0;
    if (XtIsRealized(parent)) {
        XtSetArg(args[n], XtNtransientFor, parent);
        n++;
    }
    man_globals->save = XtCreatePopupShell("likeToSave",
                                           transientShellWidgetClass,
                                           parent, args, n);

    label_str = XmStringCreateLocalized("Would you like to save this page?");
    n = 0;
    XtSetArg(args[n], XmNmessageString, label_str); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_QUESTION); n++;
    XtSetArg(args[n], XmNokLabelString,
             XmStringCreateLocalized(FILE_SAVE)); n++;
    XtSetArg(args[n], XmNcancelLabelString,
             XmStringCreateLocalized(CANCEL_FILE_SAVE)); n++;
    dialog = XmCreateMessageBox(man_globals->save, "saveDialog", args, n);
    XmStringFree(label_str);
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    XtAddCallback(dialog, XmNokCallback, SaveOkCallback, NULL);
    XtAddCallback(dialog, XmNcancelCallback, SaveCancelCallback, NULL);
    XtManageChild(dialog);

    if (XtIsRealized(parent)) {
        XtRealizeWidget(shell);
        AddCursor(shell, resources.cursors.top);
        XtRealizeWidget(man_globals->save);
        AddCursor(man_globals->save, resources.cursors.top);
    }
}


