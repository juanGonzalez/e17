/*
 * Copyright (C) 2000-2004 Carsten Haitzler, Geoff Harrison and various contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <ctype.h>
#include "E.h"
#include "timestamp.h"

#define SS(s) ((s) ? (s) : NoText)
static const char   NoText[] = "-NONE-";

static size_t       bufsiz;
static char        *bufptr;

static void
IpcPrintInit(void)
{
   bufsiz = 0;
   bufptr = NULL;
}

static void
IpcPrintFlush(Client * c)
{
   if (bufptr == NULL)
      return;

   if (c)
      CommsSend(c, bufptr);
   Efree(bufptr);
   bufsiz = 0;
   bufptr = NULL;
}

void
IpcPrintf(const char *fmt, ...)
{
   char                tmp[FILEPATH_LEN_MAX];
   int                 len;
   va_list             args;

   va_start(args, fmt);
   len = Evsnprintf(tmp, sizeof(tmp), fmt, args);
   va_end(args);

   bufptr = Erealloc(bufptr, bufsiz + len + 1);
   strcpy(bufptr + bufsiz, tmp);
   bufsiz += len;
}

static EWin        *
IpcFindEwin(const char *windowid)
{
   unsigned int        win;

   if (!strcmp(windowid, "*") || !strcmp(windowid, "%")
       || !strcmp(windowid, "current"))
      return GetContextEwin();

   if (isdigit(windowid[0]))
     {
	sscanf(windowid, "%x", &win);
	return FindEwinByChildren(win);
     }

   if (windowid[0] == '+')
      return FindEwinByPartial(windowid + 1, '+');

   if (windowid[0] == '=')
      return FindEwinByPartial(windowid + 1, '=');

   return FindEwinByPartial(windowid, '=');
}

static int
SetEwinBoolean(const char *txt, char *item, const char *value, int set)
{
   int                 old, new;

   new = old = *item != 0;	/* Remember old value */

   if (value == NULL || value[0] == '\0')
      new = !old;
   else if (!strcmp(value, "on"))
      new = 1;
   else if (!strcmp(value, "off"))
      new = 0;
   else if (!strcmp(value, "?"))
      IpcPrintf("%s: %s", txt, (old) ? "on" : "off");
   else
      IpcPrintf("Error: %s", value);

   if (new != old)
     {
	if (set)
	   *item = new;
	return 1;
     }

   return 0;
}

/* The IPC functions */

static void
IPC_Xinerama(const char *params __UNUSED__, Client * c __UNUSED__)
{
#ifdef HAS_XINERAMA
   if (Mode.display.xinerama_active)
     {
	XineramaScreenInfo *screens;
	int                 num, i;

	screens = XineramaQueryScreens(disp, &num);

	for (i = 0; i < num; i++)
	  {
	     IpcPrintf("Head %d\nscreen # %d\nx origin: %d\ny origin: %d\n"
		       "width: %d\nheight: %d\n\n", i, screens[i].screen_number,
		       screens[i].x_org, screens[i].y_org, screens[i].width,
		       screens[i].height);
	  }
	XFree(screens);
     }
   else
     {
	IpcPrintf("Xinerama is not active on your system");
     }
#else
   IpcPrintf("Xinerama is disabled on your system");
#endif
}

static void
IPC_Nop(const char *params __UNUSED__, Client * c __UNUSED__)
{
   IpcPrintf("nop");
}

static void
IPC_GeneralInfo(const char *params, Client * c)
{
   char                buf[FILEPATH_LEN_MAX];

   buf[0] = 0;

   if (params)
     {
	if (!strcmp(params, "screen_size"))
	  {
	     Esnprintf(buf, sizeof(buf), "screen_size: %d %d", VRoot.w,
		       VRoot.h);
	  }
	else
	  {
	     Esnprintf(buf, sizeof(buf), "Error: unknown info requested");
	  }
     }
   else
     {
	Esnprintf(buf, sizeof(buf), "Error: no info requested");
     }

   if (buf[0])
      CommsSend(c, buf);
}

static void
IPC_ListClassMembers(const char *params, Client * c)
{
   char               *buf = NULL;
   char                buf2[FILEPATH_LEN_MAX];
   int                 num, i;

   if (params)
     {
	if (!strcmp(params, "borders"))
	  {
	     Border            **lst;

	     lst = (Border **) ListItemType(&num, LIST_TYPE_BORDER);
	     for (i = 0; i < num; i++)
	       {
		  buf2[0] = 0;
		  Esnprintf(buf2, sizeof(buf2), "%s\n", lst[i]->name);
		  if (buf)
		     buf = realloc(buf, strlen(buf) + strlen(buf2) + 1);
		  else
		    {
		       buf = malloc(strlen(buf2) + 1);
		       buf[0] = 0;
		    }
		  strcat(buf, buf2);
	       }
	     if (lst)
		Efree(lst);
	  }
	else if (!strcmp(params, "text"))
	  {
	     TextClass         **lst;

	     lst = (TextClass **) ListItemType(&num, LIST_TYPE_TCLASS);
	     for (i = 0; i < num; i++)
	       {
		  buf2[0] = 0;
		  Esnprintf(buf2, sizeof(buf2), "%s\n", lst[i]->name);
		  if (buf)
		     buf = realloc(buf, strlen(buf) + strlen(buf2) + 1);
		  else
		    {
		       buf = malloc(strlen(buf2) + 1);
		       buf[0] = 0;
		    }
		  strcat(buf, buf2);
	       }
	     if (lst)
		Efree(lst);
	  }
	else if (!strcmp(params, "images"))
	  {
	     ImageClass        **lst;

	     lst = (ImageClass **) ListItemType(&num, LIST_TYPE_ICLASS);
	     for (i = 0; i < num; i++)
	       {
		  buf2[0] = 0;
		  Esnprintf(buf2, sizeof(buf2), "%s\n", lst[i]->name);
		  if (buf)
		     buf = realloc(buf, strlen(buf) + strlen(buf2) + 1);
		  else
		    {
		       buf = malloc(strlen(buf2) + 1);
		       buf[0] = 0;
		    }
		  strcat(buf, buf2);
	       }
	     if (lst)
		Efree(lst);
	  }
	else
	   CommsSend(c, "Error: unknown class selected");
     }
   else
      CommsSend(c, "Error: no class selected");

   if (buf)
     {
	CommsSend(c, buf);
	Efree(buf);
     }
}

static void
IPC_DialogOK(const char *params, Client * c __UNUSED__)
{
   if (params)
      DialogOKstr(_("Message"), params);
   else
      IpcPrintf("Error: No text for dialog specified\n");
}

static void
IPC_MoveMode(const char *params, Client * c)
{
   char                buf[FILEPATH_LEN_MAX];

   buf[0] = 0;

   if (params)
     {
	if (!strcmp(params, "opaque"))
	  {
	     Conf.movres.mode_move = 0;
	  }
	else if (!strcmp(params, "lined"))
	  {
	     Conf.movres.mode_move = 1;
	  }
	else if (!strcmp(params, "box"))
	  {
	     Conf.movres.mode_move = 2;
	  }
	else if (!strcmp(params, "shaded"))
	  {
	     Conf.movres.mode_move = 3;
	  }
	else if (!strcmp(params, "semi-solid"))
	  {
	     Conf.movres.mode_move = 4;
	  }
	else if (!strcmp(params, "translucent"))
	  {
	     Conf.movres.mode_move = 5;
	  }
	else if (!strcmp(params, "?"))
	  {
	     if (Conf.movres.mode_move)
	       {
		  if (Conf.movres.mode_move == 1)
		     Esnprintf(buf, sizeof(buf), "movemode: lined");
		  else if (Conf.movres.mode_move == 2)
		     Esnprintf(buf, sizeof(buf), "movemode: box");
		  else if (Conf.movres.mode_move == 3)
		     Esnprintf(buf, sizeof(buf), "movemode: shaded");
		  else if (Conf.movres.mode_move == 4)
		     Esnprintf(buf, sizeof(buf), "movemode: semi-solid");
		  else if (Conf.movres.mode_move == 5)
		     Esnprintf(buf, sizeof(buf), "movemode: translucent");
	       }
	     else
	       {
		  Esnprintf(buf, sizeof(buf), "movemode: opaque");
	       }
	  }
	else
	  {
	     Esnprintf(buf, sizeof(buf), "Error: unknown mode specified");
	  }
     }
   else
     {
	Esnprintf(buf, sizeof(buf), "Error: no mode specified");
     }

   if (buf[0])
      CommsSend(c, buf);
}

static void
IPC_ResizeMode(const char *params, Client * c)
{
   char                buf[FILEPATH_LEN_MAX];

   buf[0] = 0;

   if (params)
     {
	if (!strcmp(params, "opaque"))
	  {
	     Conf.movres.mode_resize = 0;
	  }
	else if (!strcmp(params, "lined"))
	  {
	     Conf.movres.mode_resize = 1;
	  }
	else if (!strcmp(params, "box"))
	  {
	     Conf.movres.mode_resize = 2;
	  }
	else if (!strcmp(params, "shaded"))
	  {
	     Conf.movres.mode_resize = 3;
	  }
	else if (!strcmp(params, "semi-solid"))
	  {
	     Conf.movres.mode_resize = 4;
	  }
	else if (!strcmp(params, "?"))
	  {
	     if (Conf.movres.mode_resize)
	       {
		  if (Conf.movres.mode_resize == 1)
		     Esnprintf(buf, sizeof(buf), "resizemode: lined");
		  else if (Conf.movres.mode_resize == 2)
		     Esnprintf(buf, sizeof(buf), "resizemode: box");
		  else if (Conf.movres.mode_resize == 3)
		     Esnprintf(buf, sizeof(buf), "resizemode: shaded");
		  else if (Conf.movres.mode_resize == 4)
		     Esnprintf(buf, sizeof(buf), "resizemode: semi-solid");
	       }
	     else
	       {
		  Esnprintf(buf, sizeof(buf), "resizemode: opaque");
	       }
	  }
	else
	  {
	     Esnprintf(buf, sizeof(buf), "Error: unknown mode specified");
	  }
     }
   else
     {
	Esnprintf(buf, sizeof(buf), "Error: no mode specified");
     }

   if (buf[0])
      CommsSend(c, buf);
}

static void
IPC_GeomInfoMode(const char *params, Client * c)
{
   char                buf[FILEPATH_LEN_MAX];

   buf[0] = 0;

   if (params)
     {
	if (!strcmp(params, "never"))
	  {
	     Conf.movres.mode_info = 0;
	  }
	else if (!strcmp(params, "center"))
	  {
	     Conf.movres.mode_info = 1;
	  }
	else if (!strcmp(params, "corner"))
	  {
	     Conf.movres.mode_info = 2;
	  }
	else if (!strcmp(params, "?"))
	  {
	     if (Conf.movres.mode_info)
	       {
		  if (Conf.movres.mode_info == 1)
		     Esnprintf(buf, sizeof(buf), "geominfomode: center");
		  else if (Conf.movres.mode_info == 2)
		     Esnprintf(buf, sizeof(buf), "geominfomode: corner");
	       }
	     else
	       {
		  Esnprintf(buf, sizeof(buf), "geominfomode: never");
	       }
	  }
	else
	  {
	     Esnprintf(buf, sizeof(buf), "Error: unknown mode specified");
	  }
     }
   else
     {
	Esnprintf(buf, sizeof(buf), "Error: no mode specified");
     }

   if (buf[0])
      CommsSend(c, buf);
}

static void
IPC_WinList(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX];
   EWin               *const *lst, *e;
   int                 num, i;

   param1[0] = '\0';
   word(params, 1, param1);

   lst = EwinListGetAll(&num);
   for (i = 0; i < num; i++)
     {
	e = lst[i];
	switch (param1[0])
	  {
	  case '\0':
	     IpcPrintf("%#lx : %s\n", e->client.win, SS(e->icccm.wm_name));
	     break;
	  default:
	     IpcPrintf("%#lx : %s :: %d : %d %d : %d %d %dx%d\n",
		       e->client.win, SS(e->icccm.wm_name),
		       (EoIsSticky(e)) ? -1 : EoGetDesk(e), e->area_x,
		       e->area_y, EoGetX(e), EoGetY(e), EoGetW(e), EoGetH(e));
	     break;
	  case 'a':
	     IpcPrintf("%#10lx : %4d %4d %4dx%4d :: %2d : %d %d : %s\n",
		       e->client.win, EoGetX(e), EoGetY(e), EoGetW(e),
		       EoGetH(e), (EoIsSticky(e)) ? -1 : EoGetDesk(e),
		       e->area_x, e->area_y, SS(e->icccm.wm_name));
	     break;
	  }
     }
   if (num <= 0)
      IpcPrintf("No windows\n");
}

#if 0				/* TBD */
static int
doMoveConstrained(EWin * ewin, const char *params)
{
   return ActionMoveStart(ewin, params, 1, 0);
}

static int
doMoveNoGroup(EWin * ewin, const char *params)
{
   return ActionMoveStart(ewin, params, 0, 1);
}

static int
doSwapMove(EWin * ewin, const char *params)
{
   Mode.move.swap = 1;
   return ActionMoveStart(ewin, params, 0, 0);
}

static int
doMoveConstrainedNoGroup(EWin * ewin, const char *params)
{
   return ActionMoveStart(ewin, params, 1, 1);
}
#endif

static void
IPC_WinOps(const char *params, Client * c __UNUSED__)
{
   EWin               *ewin;
   char                windowid[FILEPATH_LEN_MAX];
   char                operation[FILEPATH_LEN_MAX];
   char                param1[FILEPATH_LEN_MAX];
   unsigned int        val;
   char                on;

   if (params == NULL)
     {
	IpcPrintf("Error: no window specified");
	goto done;
     }

   operation[0] = 0;
   param1[0] = 0;

   windowid[0] = 0;
   word(params, 1, windowid);
   ewin = IpcFindEwin(windowid);
   if (!ewin)
     {
	IpcPrintf("Error: no such window: %s", windowid);
	goto done;
     }

   word(params, 2, operation);
   word(params, 3, param1);

   if (!operation[0])
     {
	IpcPrintf("Error: no operation specified");
	goto done;
     }

   if (!strncmp(operation, "close", 2))
     {
	EwinOpClose(ewin);
     }
   else if (!strcmp(operation, "kill"))
     {
	EwinOpKill(ewin);
     }
   else if (!strncmp(operation, "iconify", 2))
     {
	if (SetEwinBoolean("window iconified", &ewin->iconified, param1, 0))
	   EwinOpIconify(ewin, !ewin->iconified);
     }
#if USE_COMPOSITE
   else if (!strncmp(operation, "opacity", 2))
     {
	if (!strcmp(param1, "?"))
	  {
	     IpcPrintf("opacity: %u", ewin->props.opacity >> 24);
	  }
	else
	  {
	     val = 0xff;
	     sscanf(param1, "%i", &val);
	     EwinOpSetOpacity(ewin, val);
	  }
     }
   else if (!strcmp(operation, "shadow"))	/* Place before "shade" */
     {
	on = EoGetShadow(ewin);
	if (SetEwinBoolean("shadow", &on, param1, 0))
	   EoSetShadow(ewin, !on);
     }
#endif
   else if (!strncmp(operation, "shade", 2))
     {
	if (SetEwinBoolean("window shaded", &ewin->shaded, param1, 0))
	   EwinOpShade(ewin, !ewin->shaded);
     }
   else if (!strncmp(operation, "stick", 2))
     {
	on = EoIsSticky(ewin);
	if (SetEwinBoolean("window sticky", &on, param1, 0))
	   EwinOpStick(ewin, !on);
     }
   else if (!strcmp(operation, "fixedpos"))
     {
	SetEwinBoolean("window fixedpos", &ewin->fixedpos, param1, 1);
     }
   else if (!strcmp(operation, "never_use_area"))
     {
	SetEwinBoolean("window never_use_area", &ewin->never_use_area, param1,
		       1);
     }
   else if (!strcmp(operation, "focusclick"))
     {
	SetEwinBoolean("window focusclick", &ewin->focusclick, param1, 1);
     }
   else if (!strcmp(operation, "neverfocus"))
     {
	SetEwinBoolean("window neverfocus", &ewin->neverfocus, param1, 1);
     }
   else if (!strncmp(operation, "title", 2))
     {
	char               *ptr = strstr(params, "title");

	if (ptr)
	  {
	     ptr += strlen("title");
	     while (*ptr == ' ')
		ptr++;
	     if (strlen(ptr))
	       {
		  if (!strncmp(ptr, "?", 1))
		    {
		       /* return the window title */
		       IpcPrintf("window title: %s", ewin->icccm.wm_name);
		    }
		  else
		    {
		       /* set the new title */
		       if (ewin->icccm.wm_name)
			  Efree(ewin->icccm.wm_name);
		       ewin->icccm.wm_name =
			  Emalloc((strlen(ptr) + 1) * sizeof(char));

		       strcpy(ewin->icccm.wm_name, ptr);
		       XStoreName(disp, ewin->client.win, ewin->icccm.wm_name);
		       EwinBorderUpdateInfo(ewin);
		    }
	       }
	     else
	       {
		  /* error */
		  IpcPrintf("Error: no title specified");
	       }
	  }
     }
   else if (!strcmp(operation, "toggle_width") || !strcmp(operation, "tw"))
     {
	MaxWidth(ewin, param1);
     }
   else if (!strcmp(operation, "toggle_height") || !strcmp(operation, "th"))
     {
	MaxHeight(ewin, param1);
     }
   else if (!strcmp(operation, "toggle_size") || !strcmp(operation, "ts"))
     {
	MaxSize(ewin, param1);
     }
   else if (!strncmp(operation, "raise", 2))
     {
	EwinOpRaise(ewin);
     }
   else if (!strncmp(operation, "lower", 2))
     {
	EwinOpLower(ewin);
     }
   else if (!strncmp(operation, "layer", 2))
     {
	if (!strcmp(param1, "?"))
	  {
	     IpcPrintf("window layer: %d", EoGetLayer(ewin));
	  }
	else
	  {
	     val = atoi(param1);
	     EwinOpSetLayer(ewin, val);
	  }
     }
   else if (!strncmp(operation, "border", 2))
     {
	if (param1[0])
	  {
	     if (!strcmp(param1, "?"))
	       {
		  if (ewin->border)
		    {
		       if (ewin->border->name)
			 {
			    IpcPrintf("window border: %s", ewin->border->name);
			 }
		    }
	       }
	     else
	       {
		  EwinOpSetBorder(ewin, param1);
	       }
	  }
	else
	  {
	     IpcPrintf("Error: no border specified");
	  }
     }
   else if (!strncmp(operation, "desk", 2))
     {
	if (param1[0])
	  {
	     if (!strncmp(param1, "next", 1))
	       {
		  EwinOpMoveToDesk(ewin, EoGetDesk(ewin) + 1);
	       }
	     else if (!strncmp(param1, "prev", 1))
	       {
		  EwinOpMoveToDesk(ewin, EoGetDesk(ewin) - 1);
	       }
	     else if (!strcmp(param1, "?"))
	       {
		  IpcPrintf("window desk: %d", EoGetDesk(ewin));
	       }
	     else
	       {
		  EwinOpMoveToDesk(ewin, atoi(param1));
	       }
	  }
	else
	  {
	     IpcPrintf("Error: no desktop supplied");
	  }
     }
   else if (!strncmp(operation, "area", 2))
     {
	int                 a, b;

	if (param1[0])
	  {
	     if (!strcmp(param1, "?"))
	       {
		  IpcPrintf("window area: %d %d", ewin->area_x, ewin->area_y);
	       }
	     else
	       {
		  sscanf(params, "%*s %*s %i %i", &a, &b);
		  MoveEwinToArea(ewin, a, b);
	       }
	  }
	else
	  {
	     IpcPrintf("Error: no area supplied");
	  }
     }
   else if (!strncmp(operation, "move", 2))
     {
	int                 a, b;

	if (param1[0])
	  {
	     if (!strcmp(param1, "ptr"))
	       {
		  ActionMoveStart(ewin, 1, 0, 0);
	       }
	     else if (!strcmp(param1, "?"))
	       {
		  IpcPrintf("window location: %d %d", EoGetX(ewin),
			    EoGetY(ewin));
	       }
	     else if (!strcmp(param1, "??"))
	       {
		  IpcPrintf("client location: %d %d",
			    EoGetX(ewin) + ewin->border->border.left,
			    EoGetY(ewin) + ewin->border->border.top);
	       }
	     else
	       {
		  sscanf(params, "%*s %*s %i %i", &a, &b);
		  MoveResizeEwin(ewin, a, b, ewin->client.w, ewin->client.h);
	       }
	  }
	else
	  {
	     IpcPrintf("Error: no coords supplied");
	  }
     }
   else if (!strcmp(operation, "resize") || !strcmp(operation, "sz"))
     {
	int                 a, b;

	if (param1[0])
	  {
	     if (!strcmp(param1, "ptr"))
	       {
		  ActionResizeStart(ewin, 0, MODE_RESIZE);
	       }
	     else if (!strcmp(param1, "ptr-h"))
	       {
		  ActionResizeStart(ewin, 0, MODE_RESIZE_H);
	       }
	     else if (!strcmp(param1, "ptr-v"))
	       {
		  ActionResizeStart(ewin, 0, MODE_RESIZE_V);
	       }
	     else if (!strcmp(param1, "?"))
	       {
		  IpcPrintf("window size: %d %d", ewin->client.w,
			    ewin->client.h);
	       }
	     else if (!strcmp(param1, "??"))
	       {
		  IpcPrintf("frame size: %d %d", EoGetW(ewin), EoGetH(ewin));
	       }
	     else
	       {
		  sscanf(params, "%*s %*s %i %i", &a, &b);
		  MoveResizeEwin(ewin, EoGetX(ewin), EoGetY(ewin), a, b);
	       }
	  }
     }
   else if (!strcmp(operation, "move_relative") || !strcmp(operation, "mr"))
     {
	int                 a, b;

	if (param1[0])
	  {
	     sscanf(params, "%*s %*s %i %i", &a, &b);
	     a += EoGetX(ewin);
	     b += EoGetY(ewin);
	     MoveResizeEwin(ewin, a, b, ewin->client.w, ewin->client.h);
	  }
     }
   else if (!strcmp(operation, "resize_relative") || !strcmp(operation, "sr"))
     {
	int                 a, b;

	if (param1[0])
	  {
	     sscanf(params, "%*s %*s %i %i", &a, &b);
	     a += ewin->client.w;
	     b += ewin->client.h;
	     MoveResizeEwin(ewin, EoGetX(ewin), EoGetY(ewin), a, b);
	  }
     }
   else if (!strncmp(operation, "focus", 2))
     {
	if (!strcmp(param1, "?"))
	  {
	     IpcPrintf("focused: %s", (ewin == GetFocusEwin())? "yes" : "no");
	  }
	else
	  {
	     GotoDesktopByEwin(ewin);
	     if (ewin->iconified)
		EwinOpIconify(ewin, 0);
	     if (ewin->shaded)
		EwinOpShade(ewin, 0);
	     EwinOpRaise(ewin);
	     FocusToEWin(ewin, FOCUS_SET);
	  }
     }
   else if (!strncmp(operation, "fullscreen", 2))
     {
	on = ewin->st.fullscreen;
	if (SetEwinBoolean("fullscreen", &on, param1, 0))
	   EwinSetFullscreen(ewin, !on);
     }
   else if (!strncmp(operation, "skiplists", 4))
     {
	if (SetEwinBoolean("skiplists", &ewin->skiptask, param1, 1))
	   EwinOpSkipLists(ewin, ewin->skiptask);
     }
   else if (!strncmp(operation, "zoom", 2))
     {
	if (InZoom())
	   Zoom(NULL);
	else
	   Zoom(ewin);
     }
   else if (!strcmp(operation, "snap"))
     {
	SnapshotEwinSet(ewin, atword(params, 3));
     }
   else
     {
	IpcPrintf("Error: unknown operation");
     }

   RememberImportantInfoForEwin(ewin);

 done:
   return;
}

static void
IPC_Remember(const char *params, Client * c __UNUSED__)
{
   int                 window;
   EWin               *ewin;

   if (!params)
     {
	IpcPrintf("Error: no parameters\n");
	goto done;
     }

   window = 0;
   sscanf(params, "%x", &window);
   ewin = FindItem(NULL, window, LIST_FINDBY_ID, LIST_TYPE_EWIN);
   if (!ewin)
     {
	IpcPrintf("Error: Window not found: %#x\n", window);
	goto done;
     }

   SnapshotEwinSet(ewin, atword(params, 2));

 done:
   return;
}

static void
IPC_ForceSave(const char *params __UNUSED__, Client * c __UNUSED__)
{
   autosave();
}

static void
IPC_Restart(const char *params __UNUSED__, Client * c __UNUSED__)
{
   SessionExit("restart");
}

static void
IPC_RestartWM(const char *params, Client * c __UNUSED__)
{
   char                buf[FILEPATH_LEN_MAX];

   if (params)
     {
	Esnprintf(buf, sizeof(buf), "restart_wm %s", params);
	SessionExit(buf);
     }
   else
     {
	IpcPrintf("Error: no window manager specified");
     }
}

static void
IPC_Exit(const char *params, Client * c __UNUSED__)
{
   SessionExit(params);
}

static void
IPC_Copyright(const char *params __UNUSED__, Client * c __UNUSED__)
{
   IpcPrintf("Copyright (C) 2000-2004 Carsten Haitzler and Geoff Harrison,\n"
	     "with various contributors (Isaac Richards, Sung-Hyun Nam, "
	     "Kimball Thurston,\n"
	     "Michael Kellen, Frederic Devernay, Felix Bellaby, "
	     "Michael Jennings,\n"
	     "Christian Kreibich, Peter Kjellerstedt, Troy Pesola, Owen Taylor, "
	     "Stalyn,\n" "Knut Neumann, Nathan Heagy, Simon Forman, "
	     "Brent Nelson,\n"
	     "Martin Tyler, Graham MacDonald, Jessse Michael, "
	     "Paul Duncan, Daniel Erat,\n"
	     "Tom Gilbert, Peter Alm, Ben Frantzdale, "
	     "Hallvar Helleseth, Kameran Kashani,\n"
	     "Carl Strasen, David Mason, Tom Christiansen, and others\n"
	     "-- please see the AUTHORS file for a complete listing)\n\n"
	     "Permission is hereby granted, free of charge, to "
	     "any person obtaining a copy\n"
	     "of this software and associated documentation files "
	     "(the \"Software\"), to\n"
	     "deal in the Software without restriction, including "
	     "without limitation the\n"
	     "rights to use, copy, modify, merge, publish, distribute, "
	     "sub-license, and/or\n"
	     "sell copies of the Software, and to permit persons to "
	     "whom the Software is\n"
	     "furnished to do so, subject to the following conditions:\n\n"
	     "The above copyright notice and this permission notice "
	     "shall be included in\n"
	     "all copies or substantial portions of the Software.\n\n"
	     "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF "
	     "ANY KIND, EXPRESS OR\n"
	     "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
	     "MERCHANTABILITY,\n"
	     "FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. "
	     "IN NO EVENT SHALL\n"
	     "THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
	     "LIABILITY, WHETHER\n"
	     "IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
	     "OUT OF OR IN\n"
	     "CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS "
	     "IN THE SOFTWARE.\n");
}

static void
IPC_About(const char *params __UNUSED__, Client * c __UNUSED__)
{
   About();
}

static void
IPC_Version(const char *params __UNUSED__, Client * c __UNUSED__)
{
   IpcPrintf(_("Enlightenment Version : %s\n" "code is current to    : %s\n"),
	     e_wm_version, E_CHECKOUT_DATE);
}

static void
IPC_MemDebug(const char *params __UNUSED__, Client * c __UNUSED__)
{
#if !USE_LIBC_MALLOC
   EDisplayMemUse();
#endif
}

static void
IPC_Hints(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX];
   char                param2[FILEPATH_LEN_MAX];

   param1[0] = 0;
   param2[0] = 0;

   word(params, 1, param1);
   word(params, 2, param2);

   if (!strcmp(param1, "xroot"))
     {
	if (!strncmp(param2, "norm", 4))
	   Conf.hints.set_xroot_info_on_root_window = 0;
	else if (!strncmp(param2, "root", 4))
	   Conf.hints.set_xroot_info_on_root_window = 1;
	autosave();
     }

   IpcPrintf("Set _XROOT* hints: %s\n",
	     (Conf.hints.set_xroot_info_on_root_window) ? "root" : "normal");
}

static void
IPC_Debug(const char *params, Client * c __UNUSED__)
{
   char                param[1024];
   int                 l;
   const char         *p;

   if (!params)
      return;

   p = params;
   l = 0;
   sscanf(p, "%1000s %n", param, &l);
   p += l;

   if (!strncmp(param, "event", 2))
     {
	EventDebugInit(p);
     }
   else if (!strncmp(param, "grab", 2))
     {
	Window              win;

	l = 0;
	sscanf(p, "%1000s %n", param, &l);
	p += l;
	if (!strncmp(param, "allow", 2))
	  {
	     l = 0;
	     sscanf(p, "%d", &l);
	     XAllowEvents(disp, l, CurrentTime);
	     IpcPrintf("XAllowEvents\n");
	  }
	else if (!strncmp(param, "unset", 2))
	  {
	     GrabPointerRelease();
	     IpcPrintf("Ungrab\n");
	  }
	else
	  {
	     sscanf(param, "%li", &win);
	     GrabPointerSet(win, ECSR_ACT_RESIZE, 1);
	     IpcPrintf("Grab %#lx\n", win);
	  }
     }
   else if (!strncmp(param, "sync", 2))
     {
	l = 0;
	sscanf(p, "%1000s %n", param, &l);
	p += l;
	if (!strncmp(param, "on", 2))
	  {
	     XSynchronize(disp, True);
	     IpcPrintf("Sync on\n");
	  }
	else if (!strncmp(param, "off", 2))
	  {
	     XSynchronize(disp, False);
	     IpcPrintf("Sync off\n");
	  }
     }
}

static void
IPC_Set(const char *params, Client * c __UNUSED__)
{
   ConfigurationSet(params);
}

static void
IPC_Show(const char *params, Client * c __UNUSED__)
{
   ConfigurationShow(params);
}

static void
IPC_Reply(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX], param2[FILEPATH_LEN_MAX];

   word(params, 1, param1);

   if (!strcmp(param1, "imageclass"))
     {
	/* Reply format "reply imageclass NAME 24243" */
	word(params, 2, param1);
	word(params, 3, param2);
	HonorIclass(param1, atoi(param2));
     }
}

static void
EwinShowInfo1(const EWin * ewin)
{
   Border              NoBorder;
   const Border       *border;

   border = ewin->border;
   if (border == NULL)
     {
	border = &NoBorder;
	memset(&NoBorder, 0, sizeof(Border));
     }

   IpcPrintf("***CLIENT***\n"
	     "CLIENT_WIN_ID:          %#10lx\n"
	     "FRAME_WIN_ID:           %#10lx\n"
	     "CONTAINER_WIN_ID:       %#10lx\n"
	     "FRAME_X,Y:              %5i , %5i\n"
	     "FRAME_WIDTH,HEIGHT:     %5i , %5i\n"
	     "BORDER_NAME:            %s\n"
	     "BORDER_BORDER:          %5i , %5i , %5i , %5i\n"
	     "DESKTOP_NUMBER:         %5i\n"
	     "MEMBER_OF_GROUPS:       %5i\n"
	     "DOCKED:                 %5i\n"
	     "STICKY:                 %5i\n"
	     "VISIBLE:                %5i\n"
	     "ICONIFIED:              %5i\n"
	     "SHADED:                 %5i\n"
	     "ACTIVE:                 %5i\n"
	     "LAYER:                  %5i\n"
	     "NEVER_USE_AREA:         %5i\n"
	     "FLOATING:               %5i\n"
	     "CLIENT_WIDTH,HEIGHT:    %5i , %5i\n"
	     "ICON_WIN_ID:            %#10lx\n"
	     "ICON_PIXMAP,MASK_ID:    %#10lx , %#10lx\n"
	     "CLIENT_GROUP_LEADER_ID: %#10lx\n"
	     "CLIENT_NEEDS_INPUT:     %5i\n"
	     "TRANSIENT:              %5i\n"
	     "TITLE:                  %s\n"
	     "CLASS:                  %s\n"
	     "NAME:                   %s\n"
	     "COMMAND:                %s\n"
	     "MACHINE:                %s\n"
	     "ICON_NAME:              %s\n"
	     "IS_GROUP_LEADER:        %5i\n"
	     "NO_RESIZE_HORIZONTAL:   %5i\n"
	     "NO_RESIZE_VERTICAL:     %5i\n"
	     "SHAPED:                 %5i\n"
	     "MIN_WIDTH,HEIGHT:       %5i , %5i\n"
	     "MAX_WIDTH,HEIGHT:       %5i , %5i\n"
	     "BASE_WIDTH,HEIGHT:      %5i , %5i\n"
	     "WIDTH,HEIGHT_INC:       %5i , %5i\n"
	     "ASPECT_MIN,MAX:         %5.5f , %5.5f\n"
	     "MWM_BORDER:             %5i\n"
	     "MWM_RESIZEH:            %5i\n"
	     "MWM_TITLE:              %5i\n"
	     "MWM_MENU:               %5i\n"
	     "MWM_MINIMIZE:           %5i\n"
	     "MWM_MAXIMIZE:           %5i\n",
	     ewin->client.win, EoGetWin(ewin), ewin->win_container,
	     EoGetX(ewin), EoGetY(ewin), EoGetW(ewin), EoGetH(ewin),
	     border->name,
	     border->border.left, border->border.right,
	     border->border.top, border->border.bottom,
	     EoGetDesk(ewin),
	     ewin->num_groups, ewin->docked, EoIsSticky(ewin),
	     ewin->shown, ewin->iconified, ewin->shaded,
	     ewin->active, EoGetLayer(ewin), ewin->never_use_area,
	     EoIsFloating(ewin), ewin->client.w, ewin->client.h,
	     ewin->client.icon_win,
	     ewin->client.icon_pmap, ewin->client.icon_mask,
	     ewin->client.group,
	     ewin->client.need_input, ewin->client.transient,
	     SS(ewin->icccm.wm_name), SS(ewin->icccm.wm_res_class),
	     SS(ewin->icccm.wm_res_name), SS(ewin->icccm.wm_command),
	     SS(ewin->icccm.wm_machine), SS(ewin->icccm.wm_icon_name),
	     ewin->client.is_group_leader,
	     ewin->client.no_resize_h, ewin->client.no_resize_v,
	     ewin->client.shaped,
	     ewin->client.width.min, ewin->client.height.min,
	     ewin->client.width.max, ewin->client.height.max,
	     ewin->client.base_w, ewin->client.base_h,
	     ewin->client.w_inc, ewin->client.h_inc,
	     ewin->client.aspect_min, ewin->client.aspect_max,
	     ewin->client.mwm_decor_border, ewin->client.mwm_decor_resizeh,
	     ewin->client.mwm_decor_title, ewin->client.mwm_decor_menu,
	     ewin->client.mwm_decor_minimize, ewin->client.mwm_decor_maximize);
}

static void
EwinShowInfo2(const EWin * ewin)
{
   Border              NoBorder;
   const Border       *border;

   border = ewin->border;
   if (border == NULL)
     {
	border = &NoBorder;
	memset(&NoBorder, 0, sizeof(Border));
     }

   IpcPrintf("WM_NAME                 %s\n"
	     "WM_ICON_NAME            %s\n"
	     "WM_CLASS name.class     %s.%s\n"
	     "WM_WINDOW_ROLE          %s\n"
	     "WM_COMMAND              %s\n"
	     "WM_CLIENT_MACHINE       %s\n"
	     "Client window           %#10lx   x,y %4i,%4i   wxh %4ix%4i\n"
	     "Container window        %#10lx\n"
	     "Frame window            %#10lx   x,y %4i,%4i   wxh %4ix%4i\n"
#if USE_COMPOSITE
	     "Named pixmap            %#10lx\n"
#endif
	     "Border                  %s   lrtb %i,%i,%i,%i\n"
	     "Icon window, pixmap, mask %#10lx, %#10lx, %#10lx\n"
	     "Is group leader  %i  Window group leader %#lx   Client leader %#10lx\n"
	     "Has transients   %i  Transient type  %i  Transient for %#10lx\n"
	     "No resize H/V    %i/%i       Shaped      %i\n"
	     "Base, min, max, inc w/h %ix%i, %ix%i, %ix%i %ix%i\n"
	     "Aspect min, max         %5.5f, %5.5f\n"
	     "Struts                  lrtb %i,%i,%i,%i\n"
	     "MWM border %i resizeh %i title %i menu %i minimize %i maximize %i\n"
	     "NeedsInput   %i   TakeFocus    %i   FocusNever   %i   FocusClick   %i\n"
	     "NeverUseArea %i   FixedPos     %i\n"
	     "Desktop      %i   Layer        %i(%i)\n"
	     "Iconified    %i   Sticky       %i   Shaded       %i   Docked       %i\n"
	     "State        %i   Shown        %i   Active       %i   Floating     %i\n"
	     "Member of groups        %i\n",
	     SS(ewin->icccm.wm_name),
	     SS(ewin->icccm.wm_icon_name),
	     SS(ewin->icccm.wm_res_name), SS(ewin->icccm.wm_res_class),
	     SS(ewin->icccm.wm_role),
	     SS(ewin->icccm.wm_command),
	     SS(ewin->icccm.wm_machine),
	     ewin->client.win,
	     ewin->client.x, ewin->client.y, ewin->client.w, ewin->client.h,
	     ewin->win_container,
	     EoGetWin(ewin),
	     EoGetX(ewin), EoGetY(ewin), EoGetW(ewin), EoGetH(ewin),
#if USE_COMPOSITE
	     EoGetPixmap(ewin),
#endif
	     border->name, border->border.left, border->border.right,
	     border->border.top, border->border.bottom,
	     ewin->client.icon_win,
	     ewin->client.icon_pmap, ewin->client.icon_mask,
	     ewin->client.is_group_leader, ewin->client.group,
	     ewin->client.client_leader, ewin->has_transients,
	     ewin->client.transient, ewin->client.transient_for,
	     ewin->client.no_resize_h, ewin->client.no_resize_v,
	     ewin->client.shaped, ewin->client.base_w, ewin->client.base_h,
	     ewin->client.width.min, ewin->client.height.min,
	     ewin->client.width.max, ewin->client.height.max,
	     ewin->client.w_inc, ewin->client.h_inc,
	     ewin->client.aspect_min, ewin->client.aspect_max,
	     ewin->strut.left, ewin->strut.right,
	     ewin->strut.top, ewin->strut.bottom,
	     ewin->client.mwm_decor_border, ewin->client.mwm_decor_resizeh,
	     ewin->client.mwm_decor_title, ewin->client.mwm_decor_menu,
	     ewin->client.mwm_decor_minimize, ewin->client.mwm_decor_maximize,
	     ewin->client.need_input, ewin->client.take_focus,
	     ewin->neverfocus, ewin->focusclick,
	     ewin->never_use_area, ewin->fixedpos, EoGetDesk(ewin),
	     EoGetLayer(ewin), ewin->o.ilayer,
	     ewin->iconified, EoIsSticky(ewin), ewin->shaded,
	     ewin->docked, ewin->state, ewin->shown, ewin->active,
	     EoIsFloating(ewin), ewin->num_groups);
}

static void
IPC_EwinInfo(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX];
   EWin               *ewin;

   if (params == NULL)
      return;

   sscanf(params, "%1000s", param1);

   if (!strcmp(param1, "all"))
     {
	EWin               *const *lst;
	int                 i, num;

	lst = EwinListGetAll(&num);
	for (i = 0; i < num; i++)
	   EwinShowInfo1(lst[i]);
     }
   else
     {
	ewin = IpcFindEwin(param1);
	if (ewin)
	   EwinShowInfo1(ewin);
	else
	   IpcPrintf("No matching EWin found\n");
     }
}

static void
IPC_EwinInfo2(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX];
   EWin               *ewin;

   if (params == NULL)
      return;

   sscanf(params, "%1000s", param1);

   if (!strcmp(param1, "all"))
     {
	EWin               *const *lst;
	int                 i, num;

	lst = EwinListGetAll(&num);
	for (i = 0; i < num; i++)
	   EwinShowInfo2(lst[i]);
     }
   else
     {
	ewin = IpcFindEwin(param1);
	if (ewin)
	   EwinShowInfo2(ewin);
	else
	   IpcPrintf("No matching EWin found\n");
     }
}

static void
IPC_MiscInfo(const char *params __UNUSED__, Client * c __UNUSED__)
{
   IpcPrintf("stuff:\n");

   if (Mode.focuswin)
      IpcPrintf("Focus - win=%#lx\n", Mode.focuswin->client.win);

   IpcPrintf("Pointer grab on=%d win=%#lx\n",
	     Mode.grabs.pointer_grab_active, Mode.grabs.pointer_grab_window);
}

static void
IPC_Reparent(const char *params, Client * c __UNUSED__)
{
   char                param1[FILEPATH_LEN_MAX];
   char                param2[FILEPATH_LEN_MAX];
   EWin               *ewin, *enew;

   if (params == NULL)
      return;

   sscanf(params, "%100s %100s", param1, param2);

   ewin = IpcFindEwin(param1);
   enew = IpcFindEwin(param2);
   if (!ewin || !enew)
      IpcPrintf("No matching client or target EWin found\n");
   else
      EwinReparent(ewin, enew->client.win);
}

static void
IPC_Slideout(const char *params, Client * c __UNUSED__)
{
   Slideout           *s;

   if (!params)
      return;

   s = FindItem(params, 0, LIST_FINDBY_NAME, LIST_TYPE_SLIDEOUT);
   if (s)
     {
	SoundPlay("SOUND_SLIDEOUT_SHOW");
	SlideoutShow(s, GetContextEwin(), Mode.context_win);
     }
}

#if 0
static int
doWarpPointer(EWin * edummy __UNUSED__, const char *params)
{
   int                 dx, dy;

   if (params)
     {
	sscanf(params, "%i %i", &dx, &dy);
	XWarpPointer(disp, None, None, 0, 0, 0, 0, dx, dy);
     }

   return 0;
}
#endif

/* the IPC Array */

/* the format of an IPC member of the IPC array is as follows:
 * {
 *    NameOfMyFunction,
 *    "command_name",
 *    "quick-help explanation",
 *    "extended help data"
 *    "may go on for several lines, be sure\n"
 *    "to add line feeds when you need them and to \"quote\"\n"
 *    "properly"
 * }
 *
 * when you add a function into this array, make sure you also add it into
 * the declarations above and also put the function in this file.  PLEASE
 * if you add a new function in, add help to it also.  since my end goal
 * is going to be to have this whole IPC usable by an end-user or to your
 * scripter, it should be easy to learn to use without having to crack
 * open the source code.
 * --Mandrake
 */
static void         IPC_Help(const char *params, Client * c);

IpcItem             IPCArray[] = {
   {
    IPC_Help,
    "help", "?",
    "Gives you this help screen",
    "Additional parameters will retrieve help on many topics - "
    "\"help <command>\"." "\n" "use \"help all\" for a list of commands.\n"},
   {
    IPC_Version,
    "version", "ver",
    "Displays the current version of Enlightenment running",
    NULL},
   {
    IPC_Nop,
    "nop", NULL,
    "IPC No-operation - returns nop",
    NULL},
   {
    IPC_Copyright,
    "copyright", NULL,
    "Displays copyright information for Enlightenment",
    NULL},
   {
    IPC_About, "about", NULL, "Show E info", NULL},
   {
    IPC_Restart,
    "restart", NULL,
    "Restart Enlightenment",
    NULL},
   {
    IPC_RestartWM,
    "restart_wm", NULL,
    "Restart another window manager",
    "Use \"restart_wm <wmname>\" to start another window manager.\n"
    "Example: \"restart_wm fvwm\"\n"},
   {
    IPC_Exit,
    "exit", "q",
    "Exit Enlightenment",
    NULL},
   {
    IPC_ForceSave,
    "save_config", "s",
    "Force Enlightenment to save settings now",
    NULL},
   {
    IPC_WinOps,
    "win_op", "wop",
    "Change a property of a specific window",
    "Use \"win_op <windowid> <property> <value>\" to change the "
    "property of a window\nYou can use the \"window_list\" "
    "command to retrieve a list of available windows\n"
    "You can use ? after most of these commands to receive the current\n"
    "status of that flag\n"
    "available win_op commands are:\n"
    "  win_op <windowid> <close/kill>\n"
    "  win_op <windowid> <fixedpos/never_use_area>\n"
    "  win_op <windowid> <focus/focusclick/neverfocus>\n"
    "  win_op <windowid> <fullscreen/iconify/shade/stick>\n"
    "  win_op <windowid> <raise/lower>\n"
    "  win_op <windowid> skiplists\n"
    "  win_op <windowid> snap <what>\n"
    "         <what>: all, none, border, command, desktop, dialog, group, icon,\n"
    "                 layer, location, opacity, shade, shadow, size, sticky\n"
    "  win_op <windowid> noshadow\n"
    "  win_op <windowid> toggle_<width/height/size> <conservative/available/xinerama>\n"
    "          (or none for absolute)\n"
    "  win_op <windowid> border <BORDERNAME>\n"
    "  win_op <windowid> desk <desktochangeto/next/prev>\n"
    "  win_op <windowid> area <x> <y>\n"
    "  win_op <windowid> <move/resize> <x> <y>\n"
    "          (you can use ? and ?? to retreive client and frame locations)\n"
    "  win_op <windowid> title <title>\n"
    "  win_op <windowid> layer <0-100,4=normal>\n"
    "<windowid> may be substituted with \"current\" to use the current window\n"},
   {
    IPC_WinList,
    "window_list", "wl",
    "Get a list of currently open windows",
    "the list will be returned in the following "
    "format - \"window_id : title\"\n"
    "you can get an extended list using \"window_list extended\"\n"
    "returns the following format:\n\"window_id : title :: "
    "desktop : area_x area_y : x_coordinate y_coordinate\"\n"},
#if 0
   {
    IPC_FX,
    "fx", NULL,
    "Toggle various effects on/off",
    "Use \"fx <effect> <mode>\" to set the mode of a particular effect\n"
    "Use \"fx <effect> ?\" to get the current mode\n"
    "the following effects are available\n"
    "ripples <on/off> (ripples that act as a water effect on the screen)\n"
    "deskslide <on/off> (slide in desktops on desktop change)\n"
    "mapslide <on/off> (slide in new windows)\n"
    "raindrops <on/off> (raindrops will appear across your desktop)\n"
    "menu_animate <on/off> (toggles the animation of menus "
    "as they appear)\n"
    "animate_win_shading <on/off> (toggles the animation of "
    "window shading)\n"
    "window_shade_speed <#> (number of pixels/sec to shade a window)\n"
    "dragbar <on/off/left/right/top/bottom> (changes " "location of dragbar)\n"
    "tooltips <on/off/#> (changes state of tooltips and "
    "seconds till popup)\n"
    "autoraise <on/off/#> (changes state of autoraise and "
    "seconds till raise)\n"
    "edge_resistance <#/?/off> (changes the amount (in 1/100 seconds)\n"
    "   of time to push for resistance to give)\n"
    "edge_snap_resistance <#/?> (changes the number of pixels that "
    "a window will\n   resist moving against another window\n"
    "audio <on/off> (changes state of audio)\n"
    "-  seconds for tooltips and autoraise can have less than one second\n"
    "   (i.e. 0.5) or greater (1.3, 3.5, etc)\n"},
#endif
   {
    IPC_MoveMode,
    "move_mode", "smm",
    "Toggle the Window move mode",
    "use \"move_mode <opaque/lined/box/shaded/semi-solid/translucent>\" "
    "to set\nuse \"move_mode ?\" to get the current mode\n"},
   {
    IPC_ResizeMode,
    "resize_mode", "srm",
    "Toggle the Window resize mode",
    "use \"resize_mode <opaque/lined/box/shaded/semi-solid>\" "
    "to set\nuse \"resize_mode ?\" to get the current mode\n"},
   {
    IPC_GeomInfoMode,
    "geominfo_mode", "sgm",
    "Change position of geometry info display during Window move or resize",
    "use \"geominfo_mode <center/corner/never>\" "
    "to set\nuse \"geominfo_mode ?\" to get the current mode\n"},
   {
    IPC_DialogOK,
    "dialog_ok", "dok",
    "Pop up a dialog box with an OK button",
    "use \"dialog_ok <message>\" to pop up a dialog box\n"},
   {
    IPC_ListClassMembers,
    "list_class", "cl",
    "List all members of a class",
    "use \"list_class <classname>\" to get back a list of class members\n"
    "available classes are:\n" "actions\n" "borders\n" "text\n" "images\n"},
   {
    IPC_GeneralInfo,
    "general_info", NULL,
    "Retrieve some general information",
    "use \"general_info <info>\" to retrieve information\n"
    "available info is: screen_size\n"},
   {
    IPC_MemDebug,
    "dump_mem_debug", NULL,
    "Dumps memory debugging information out to e.mem.out",
    "Use this command to have E dump its current memory debugging table\n"
    "to the e.mem.out file. NOTE: please read comments at the top of\n"
    "memory.c to see how to enable this. This will let you hunt memory\n"
    "leaks, over-allocations of memory, and other " "memory-related problems\n"
    "very easily with all pointers allocated stamped with a time, call\n"
    "tree that led to that allocation, file and line, "
    "and the chunk size.\n"},
   {
    IPC_Xinerama,
    "xinerama", NULL,
    "Return xinerama information about your current system",
    NULL},
   {
    SnapIpcFunc,
    "list_remember", "rl",
    "Retrieve a list of remembered windows and their attributes",
    SnapIpcText},
   {
    IPC_Hints,
    "hints", NULL,
    "Set hint options",
    "usage:\n" "  hints xroot <normal/root>\n"},
   {
    IPC_Debug,
    "debug", NULL,
    "Set debug options",
    "usage:\n" "  debug events <EvNo>:<EvNo>...\n"},
   {
    IPC_Set, "set", NULL, "Set configuration parameter", NULL},
   {
    IPC_Show, "show", "sh", "Show configuration parameter(s)", NULL},
   {
    IPC_Reply, "reply", NULL, "TBD", NULL},
   {
    IPC_EwinInfo, "get_client_info", NULL, "Show client window info", NULL},
   {
    IPC_EwinInfo2, "win_info", "wi", "Show client window info", NULL},
   {
    IPC_MiscInfo, "dump_info", NULL, "TBD", NULL},
   {
    IPC_Reparent,
    "reparent", "rep",
    "Reparent window",
    "usage:\n" "  reparent <windowid> <new parent>\n"},
   {
    IPC_Slideout, "slideout", NULL, "Show slideout", NULL},
   {
    IPC_Remember,
    "remember", NULL,
    "Remembers parameters for client windows (obsolete)",
    "  remember <windowid> <parameter>...\n"
    "For compatibility with epplets only. In stead use\n"
    "  wop <windowid> snap <parameter>...\n"},
};

static int          ipc_item_count = 0;
static const IpcItem **ipc_item_list = NULL;

static const IpcItem **
IPC_GetList(int *pnum)
{
   int                 i, num;
   const IpcItem     **lst;

   if (ipc_item_list)
     {
	/* Must be re-generated if modules are ever added/removed */
	*pnum = ipc_item_count;
	return ipc_item_list;
     }

   num = sizeof(IPCArray) / sizeof(IpcItem);
   lst = (const IpcItem **)Emalloc(num * sizeof(IpcItem *));
   for (i = 0; i < num; i++)
      lst[i] = &IPCArray[i];

   ModulesGetIpcItems(&lst, &num);

   ipc_item_count = num;
   ipc_item_list = lst;
   *pnum = num;
   return lst;
}

/* The IPC Handler */
/* this is the function that actually loops through the IPC array
 * and finds the command that you were trying to run, and then executes it.
 * you shouldn't have to touch this function
 * - Mandrake
 */
int
HandleIPC(const char *params, Client * c)
{
   int                 i;
   int                 num;
   char                w[FILEPATH_LEN_MAX];
   const IpcItem     **lst, *ipc;

   if (EventDebug(EDBUG_TYPE_IPC))
      Eprintf("HandleIPC: %s\n", params);

   IpcPrintInit();

   lst = IPC_GetList(&num);

   word(params, 1, w);

   for (i = 0; i < num; i++)
     {
	ipc = lst[i];
	if (!(ipc->nick && !strcmp(w, ipc->nick)) && strcmp(w, ipc->name))
	   continue;

	word(params, 2, w);
	if (w)
	   ipc->func(atword(params, 2), c);
	else
	   ipc->func(NULL, c);

	IpcPrintFlush(c);
	CommsFlush(c);
	return 1;
     }

   return 0;
}

int
EFunc(const char *params)
{
   return HandleIPC(params, NULL);
}

static int
ipccmp(void *p1, void *p2)
{
   return strcmp(((IpcItem *) p1)->name, ((IpcItem *) p2)->name);
}

static void
IPC_Help(const char *params, Client * c __UNUSED__)
{
   char                buf[FILEPATH_LEN_MAX];
   int                 i, num;
   const IpcItem     **lst, *ipc;
   const char         *nick;

   lst = IPC_GetList(&num);

   IpcPrintf(_("Enlightenment IPC Commands Help\n"));

   if (!params)
     {
	IpcPrintf(_("Use \"help all\" for descriptions of each command\n"
		    "Use \"help <command>\" for an individual description\n\n"));
	IpcPrintf(_("Commands currently available:\n"));

	Quicksort((void **)lst, 0, num - 1, ipccmp);

	for (i = 0; i < num; i++)
	  {
	     ipc = lst[i];
	     nick = (ipc->nick) ? ipc->nick : "";
	     IpcPrintf("  %-16s %-4s ", ipc->name, nick);
	     if ((i % 3) == 2)
		IpcPrintf("\n");
	  }
	if (i % 3)
	   IpcPrintf("\n");
     }
   else if (!strcmp(params, "all"))
     {
	IpcPrintf(_
		  ("Use \"help full\" for full descriptions of each command\n"));
	IpcPrintf(_("Use \"help <command>\" for an individual description\n"));
	IpcPrintf(_("Commands currently available:\n"));
	IpcPrintf(_("         <command>     : <description>\n"));

	for (i = 0; i < num; i++)
	  {
	     ipc = lst[i];
	     nick = (ipc->nick) ? ipc->nick : "";
	     IpcPrintf("%18s %4s: %s\n", ipc->name, nick, ipc->help_text);
	  }
     }
   else if (!strcmp(params, "full"))
     {
	IpcPrintf(_("Commands currently available:\n"));
	IpcPrintf(_("         <command>     : <description>\n"));

	for (i = 0; i < num; i++)
	  {
	     IpcPrintf("----------------------------------------\n");
	     ipc = lst[i];
	     nick = (ipc->nick) ? ipc->nick : "";
	     IpcPrintf("%18s %4s: %s\n", ipc->name, nick, ipc->help_text);
	     if (ipc->extended_help_text)
		IpcPrintf(ipc->extended_help_text);
	  }
     }
   else
     {
	for (i = 0; i < num; i++)
	  {
	     ipc = lst[i];
	     if (strcmp(params, ipc->name) &&
		 (ipc->nick == NULL || strcmp(params, ipc->nick)))
		continue;

	     if (ipc->nick)
		sprintf(buf, " (%s)", ipc->nick);
	     else
		buf[0] = '\0';

	     IpcPrintf(" : %s%s\n--------------------------------\n%s\n",
		       ipc->name, buf, ipc->help_text);
	     if (ipc->extended_help_text)
		IpcPrintf(ipc->extended_help_text);
	  }
     }
}
