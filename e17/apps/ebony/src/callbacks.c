#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ebony.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "util.h"
#include "gtk_util.h"

#define MINIMUM_CONTROL_WIDTH 220
void
on_new_bg_activate(GtkMenuItem * menuitem, gpointer user_data)
{
   E_Background_Layer _bl;
   E_Background _bg = NULL;

   if (bg)
      e_bg_free(bg);
   if (!bg_ref)
      rebuild_bg_ref();

   _bg = e_bg_new();
   if (_bg)
   {
      _bl = e_bg_layer_new();
      _bl->type = E_BACKGROUND_TYPE_SOLID;
      _bl->fg.a = _bl->fg.r = _bl->fg.g = _bl->fg.b = 255;
      _bl->size.w = _bl->size.h = 1.0;
      _bg->layers = evas_list_append(_bg->layers, _bl);
      display_bg(_bg);
      gtk_window_set_title(GTK_WINDOW(bg_ref), "Ebony - New Background");
   }
   return;

}


void
on_open_bg_activate(GtkMenuItem * menuitem, gpointer user_data)
{
   GtkWidget *fs;

   fs = gtk_file_selection_new("Load Background File");
   gtk_file_selection_complete(GTK_FILE_SELECTION(fs), "*.bg.db");
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
                      "clicked",
                      GTK_SIGNAL_FUNC(filemenu_load_cancel_clicked),
                      (gpointer) fs);
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC(filemenu_load_ok_clicked),
                      (gpointer) fs);
   gtk_widget_show(fs);
   return;
}


void
on_recent_bg_mi_activate(GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_save_bg_activate(GtkMenuItem * menuitem, gpointer user_data)
{
   gchar errstr[1024];

   if (bg)
   {
      fill_background_images(bg);
      clear_bg_db_keys(bg);
      e_bg_save(bg, bg->file);

      g_snprintf(errstr, 1024, "Saved background: %s", (char *) bg->file);
      if (ebony_status)
         ebony_status_message(errstr, EBONY_STATUS_TO);
   }
   return;

}


void
on_save_bg_as_activate(GtkMenuItem * menuitem, gpointer user_data)
{
   GtkWidget *fs;

   fs = gtk_file_selection_new("Save Background As File ...");
   gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), "new.bg.db");
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
                      "clicked", GTK_SIGNAL_FUNC(save_as_cancel_clicked),
                      (gpointer) fs);
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC(save_as_ok_clicked),
                      (gpointer) fs);
   gtk_widget_show(fs);
   return;

}


void
on_quit_ebony_activate(GtkMenuItem * menuitem, gpointer user_data)
{
   write_ebony_bg_list_db(recent_bgs);
   gtk_main_quit();
}

gboolean
on_bg_evas_configure_event(GtkWidget * widget, GdkEventConfigure * event,
                           gpointer user_data)
{
   evas_set_output_viewport(evas, 0, 0, event->width, event->height);
   evas_set_output_size(evas, event->width, event->height);
   e_bg_resize(bg, event->width, event->height);
   if (bl)
      outline_evas_object(bl->obj);

   DRAW();

   return FALSE;
}


void
on_bg_evas_drag_data_received(GtkWidget * widget,
                              GdkDragContext * drag_context, gint x, gint y,
                              GtkSelectionData * data, guint info, guint time,
                              gpointer user_data)
{

}


void
on_layer_num_spin_button_changed(GtkEditable * editable, gpointer user_data)
{
   E_Background_Layer _bl;

   _bl = e_bg_get_layer_number(bg, (int) get_spin_value("layer_num"));
   if (_bl)
      display_layer_values(_bl);
   else
   {
      _bl = bl;
      bl = NULL;
      set_spin_value("layer_num", get_spin_value("layer_num") - 1);
      bl = _bl;
   }

   return;
}


void
on_layer_add_clicked(GtkButton * button, gpointer user_data)
{
   E_Background_Layer _bl;
   Evas_List l;
   int size = 0;

   if (!bl)
      return;
   if (!bg)
      return;

   _bl = e_bg_layer_new();
   _bl->type = E_BACKGROUND_TYPE_SOLID;
   _bl->size.w = _bl->size.h = 1.0;
   _bl->fg.r = _bl->fg.g = _bl->fg.b = 255;
   _bl->fg.a = 80;
   _bl->obj = evas_add_rectangle(evas);
   evas_set_color(evas, _bl->obj, _bl->fg.r, _bl->fg.g, _bl->fg.b, _bl->fg.a);
   evas_show(evas, _bl->obj);
   bg->layers = evas_list_append(bg->layers, _bl);
   for (l = bg->layers; l; l = l->next)
      size++;
   bl = NULL;
   set_spin_value("layer_num", size);
   display_layer_values(_bl);
   bl = _bl;

}


void
on_layer_delete_clicked(GtkButton * button, gpointer user_data)
{
   int l;
   E_Background_Layer _bl;

   bg->layers = evas_list_remove(bg->layers, bl);
   e_bg_layer_free(bg, bl);

   l = (int) get_spin_value("layer_num");
   if (l)
   {
      _bl = e_bg_get_layer_number(bg, (l - 1));
      set_spin_value("layer_num", (l - 1));
   }
   else
   {
      /* was the last entry, make a new base */
      if (!bg->layers)
      {
         _bl = e_bg_layer_new();
         _bl->type = E_BACKGROUND_TYPE_SOLID;
         _bl->fg.a = _bl->fg.r = _bl->fg.g = _bl->fg.b = 255;
         _bl->size.w = _bl->size.h = 1.0;
         _bl->obj = evas_add_rectangle(evas);
         evas_set_color(evas, _bl->obj, _bl->fg.r, _bl->fg.g, _bl->fg.b,
                        _bl->fg.a);
         evas_show(evas, _bl->obj);
         bg->layers = evas_list_append(bg->layers, _bl);
      }
      /* return the front */
      else
      {
         _bl = e_bg_get_layer_number(bg, 0);
      }
   }

   display_layer_values(_bl);
   update_background(bg);
   return;
}


void
on_move_layer_up_clicked(GtkButton * button, gpointer user_data)
{
   if (!bl)
      return;
   if (move_layer_up(bl))
      set_spin_value("layer_num", (int) get_spin_value("layer_num") + 1);
   return;

}


void
on_move_layer_downclicked(GtkButton * button, gpointer user_data)
{
   if (!bl)
      return;
   if (move_layer_down(bl))
      set_spin_value("layer_num", (int) get_spin_value("layer_num") - 1);
   return;
}


void
on_image_file_entry_changed(GtkEditable * editable, gpointer user_data)
{
   char *text;
   char filename[PATH_MAX];

   if (!bl)
      return;

   text = get_entry_text("image_file");
   if ((text) && (strlen(text)))
   {
      snprintf(filename, PATH_MAX, "%s", (char *) text);
      if (bl->file)
         free(bl->file);
      bl->file = strdup(filename);
      if (bl->image)
      {
         imlib_context_set_image(bl->image);
         imlib_free_image_and_decache();
         bl->image = imlib_load_image(bl->file);
      }
      if (bl->obj)
         evas_del_object(evas, bl->obj);
      bl->obj = evas_add_image_from_file(evas, bl->file);
      evas_show(evas, bl->obj);
      bl->size.w = bl->size.h = 1.0;
      bl->fill.w = bl->fill.h = 1.0;
      bl->inlined = 1;
      update_background(bg);
   }
   return;

}


void
on_file_select_button_clicked(GtkButton * button, gpointer user_data)
{
   GtkWidget *fs;
   char buf[1024];

   snprintf(buf, 1024, "%s/", getenv("HOME"));
   fs = gtk_file_selection_new("Add Image File...");
   gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), buf);
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
                      "clicked", GTK_SIGNAL_FUNC(browse_file_cancel_clicked),
                      (gpointer) fs);
   gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC(browse_file_ok_clicked),
                      (gpointer) fs);
   gtk_widget_show(fs);

   return;


}


void
on_inline_image_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->inlined = get_toggled_state("inline_image");
   return;

}


void
on_color_class_changed(GtkEditable * editable, gpointer user_data)
{

}


void
on_scroll_x_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->scroll.x = (float) get_spin_value("scroll_x");
   update_background(bg);
   return;

}


void
on_scroll_y_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->scroll.y = (float) get_spin_value("scroll_y");
   update_background(bg);
   return;

}


void
on_pos_x_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->pos.x = (float) get_spin_value("pos_x");
   update_background(bg);
   return;

}


void
on_pos_y_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->pos.y = (float) get_spin_value("pos_y");
   update_background(bg);
   return;
}


void
on_pos_abs_x_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->abs.x = (int) get_spin_value("abs_x");
   update_background(bg);
   return;

}


void
on_pos_abs_y_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->abs.y = (int) get_spin_value("abs_y");
   update_background(bg);
   return;

}


void
on_size_w_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->size.w = (float) get_spin_value("size_w");
   update_background(bg);
   return;
}


void
on_size_h_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->size.h = (float) get_spin_value("size_h");
   update_background(bg);
   return;

}


void
on_size_abs_w_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->size.absolute.w = get_toggled_state("size_abs_w");
   update_background(bg);
   return;

}


void
on_size_abs_h_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->size.absolute.h = get_toggled_state("size_abs_h");
   update_background(bg);
   return;

}


void
on_size_orig_w_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->size.orig.w = get_toggled_state("size_orig_w");
   update_background(bg);
   return;

}


void
on_size_orig_h_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->size.orig.h = get_toggled_state("size_orig_h");
   update_background(bg);
   return;

}


void
on_fill_w_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->fill.w = (float) get_spin_value("fill_w");
   update_background(bg);
   return;

}


void
on_fill_h_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;
   bl->fill.h = (float) get_spin_value("fill_h");
   update_background(bg);
   return;

}


void
on_fill_orig_w_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->fill.orig.w = get_toggled_state("fill_orig_w");
   update_background(bg);
   return;

}


void
on_fill_orig_h_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   if (!bl)
      return;
   bl->fill.orig.h = get_toggled_state("fill_orig_h");
   update_background(bg);
   return;

}


gboolean
on_color_box_expose_event(GtkWidget * widget, GdkEventExpose * event,
                          gpointer user_data)
{
   GdkColor fg;
   static GdkGC *gc = NULL;

   if (!widget)
      return (FALSE);
   if (!widget->window)
      return (FALSE);
   if (!bl)
      return (FALSE);
   if (!gc)
      gc = gdk_gc_new(widget->window);

   fg.pixel =
      gdk_rgb_xpixel_from_rgb((bl->fg.r << 16) | (bl->fg.g << 8) | (bl->fg.
                                                                    b));

   gdk_gc_set_foreground(gc, &fg);
   gdk_draw_rectangle(widget->window, gc, 1, 0, 0, widget->allocation.width,
                      widget->allocation.height);

   return (TRUE);
}


gboolean
on_color_box_button_release_event(GtkWidget * widget, GdkEventButton * event,
                                  gpointer user_data)
{
   GtkWidget *cs;
   gdouble color[4], temp;

   cs = gtk_color_selection_dialog_new("Select Color");
   gtk_color_selection_set_opacity(GTK_COLOR_SELECTION
                                   (GTK_COLOR_SELECTION_DIALOG(cs)->colorsel),
                                   TRUE);
   gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(cs)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC(cs_ok_button_clicked),
                      (gpointer) cs);
   gtk_signal_connect(GTK_OBJECT
                      (GTK_COLOR_SELECTION_DIALOG(cs)->cancel_button),
                      "clicked", GTK_SIGNAL_FUNC(cs_cancel_button_clicked),
                      (gpointer) cs);

   if (bl)
   {
      color[0] = (temp =
                  ((double) bl->fg.r / (double) 255)) == 0 ? 0.001 : temp;
      color[1] = (temp =
                  ((double) bl->fg.g / (double) 255)) == 0 ? 0.001 : temp;
      color[2] = (temp =
                  ((double) bl->fg.b / (double) 255)) == 0 ? 0.001 : temp;
      color[3] = (temp =
                  ((double) bl->fg.a / (double) 255)) == 0 ? 0.001 : temp;
   }
   gtk_color_selection_set_color(GTK_COLOR_SELECTION
                                 (GTK_COLOR_SELECTION_DIALOG(cs)->colorsel),
                                 color);
   gtk_widget_set_sensitive(widget, FALSE);
   gtk_widget_show(cs);
   return (TRUE);
}


gboolean
on_gradient_one_expose_event(GtkWidget * widget, GdkEventExpose * event,
                             gpointer user_data)
{
   GdkColor fg;
   static GdkGC *gc1 = NULL;
   E_Background_Gradient g = NULL;

   if (!widget)
      return (FALSE);
   if (!widget->window)
      return (FALSE);
   if (!bl)
      return (FALSE);
   if (bl->gradient.colors)
   {
      if (bl->gradient.colors->data)
         g = bl->gradient.colors->data;
   }
   else
   {
      g = (E_Background_Gradient)
         malloc(sizeof(struct _E_Background_Gradient));
      memset(g, 0, sizeof(struct _E_Background_Gradient));
      bl->gradient.colors = evas_list_append(bl->gradient.colors, g);
   }
   if (gc1)
      gdk_gc_destroy(gc1);
   gc1 = gdk_gc_new(widget->window);
   fg.pixel = gdk_rgb_xpixel_from_rgb((g->r << 16) | (g->g << 8) | g->b);
   gdk_gc_set_foreground(gc1, &fg);
   gdk_draw_rectangle(widget->window, gc1, 1, 0, 0, widget->allocation.width,
                      widget->allocation.height);

   return (TRUE);
}


gboolean
on_gradient_one_button_release_event(GtkWidget * widget,
                                     GdkEventButton * event,
                                     gpointer user_data)
{
   GtkWidget *cs;
   gdouble color[4], temp;
   E_Background_Gradient g;

   cs = gtk_color_selection_dialog_new("Select Color");
   gtk_color_selection_set_opacity(GTK_COLOR_SELECTION
                                   (GTK_COLOR_SELECTION_DIALOG(cs)->colorsel),
                                   TRUE);
   gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(cs)->ok_button),
                      "clicked",
                      GTK_SIGNAL_FUNC(gradient_one_cs_ok_button_clicked),
                      (gpointer) cs);
   gtk_signal_connect(GTK_OBJECT
                      (GTK_COLOR_SELECTION_DIALOG(cs)->cancel_button),
                      "clicked",
                      GTK_SIGNAL_FUNC(gradient_one_cs_cancel_button_clicked),
                      (gpointer) cs);

   if (bl)
   {
      if ((bl->gradient.colors) && (bl->gradient.colors))
      {
         g = (E_Background_Gradient) bl->gradient.colors->data;
      }
      else
      {
         g = (E_Background_Gradient)
            malloc(sizeof(struct _E_Background_Gradient));
         memset(g, 0, sizeof(struct _E_Background_Gradient));

         bl->gradient.colors = evas_list_append(bl->gradient.colors, g);
      }
      color[0] = (temp = ((double) g->r / (double) 255)) == 0 ? 0.001 : temp;
      color[1] = (temp = ((double) g->g / (double) 255)) == 0 ? 0.001 : temp;
      color[2] = (temp = ((double) g->b / (double) 255)) == 0 ? 0.001 : temp;
      color[3] = (temp = ((double) g->a / (double) 255)) == 0 ? 0.001 : temp;
   }
   gtk_color_selection_set_color(GTK_COLOR_SELECTION
                                 (GTK_COLOR_SELECTION_DIALOG(cs)->colorsel),
                                 color);
   gtk_widget_set_sensitive(widget, FALSE);
   gtk_widget_show(cs);
   return (TRUE);
}


gboolean
on_gradient_two_button_release_event(GtkWidget * widget,
                                     GdkEventButton * event,
                                     gpointer user_data)
{
   GtkWidget *cs;
   gdouble color[4];
   E_Background_Gradient g;

   cs = gtk_color_selection_dialog_new("Select Color");
   gtk_color_selection_set_opacity(GTK_COLOR_SELECTION
                                   (GTK_COLOR_SELECTION_DIALOG(cs)->colorsel),
                                   TRUE);
   gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(cs)->ok_button),
                      "clicked",
                      GTK_SIGNAL_FUNC(gradient_two_cs_ok_button_clicked),
                      (gpointer) cs);
   gtk_signal_connect(GTK_OBJECT
                      (GTK_COLOR_SELECTION_DIALOG(cs)->cancel_button),
                      "clicked",
                      GTK_SIGNAL_FUNC(gradient_two_cs_cancel_button_clicked),
                      (gpointer) cs);

   if (bl)
   {
      gdouble temp;

      if ((bl->gradient.colors) && (bl->gradient.colors->next))
      {
         g = (E_Background_Gradient) bl->gradient.colors->next->data;
      }
      else
      {
         g = (E_Background_Gradient)
            malloc(sizeof(struct _E_Background_Gradient));
         memset(g, 0, sizeof(struct _E_Background_Gradient));

         bl->gradient.colors = evas_list_append(bl->gradient.colors, g);
      }
      color[0] = (temp = ((double) g->r / (double) 255)) == 0 ? 0.001 : temp;
      color[1] = (temp = ((double) g->g / (double) 255)) == 0 ? 0.001 : temp;
      color[2] = (temp = ((double) g->b / (double) 255)) == 0 ? 0.001 : temp;
      color[3] = (temp = ((double) g->a / (double) 255)) == 0 ? 0.001 : temp;
   }
   gtk_color_selection_set_color(GTK_COLOR_SELECTION
                                 (GTK_COLOR_SELECTION_DIALOG(cs)->colorsel),
                                 color);
   gtk_widget_set_sensitive(widget, FALSE);
   gtk_widget_show(cs);
   return (TRUE);

   return FALSE;
}


gboolean
on_gradient_two_expose_event(GtkWidget * widget, GdkEventExpose * event,
                             gpointer user_data)
{
   E_Background_Gradient g = NULL;
   GdkColor fg;
   static GdkGC *gc2 = NULL;

   if (!widget)
      return (FALSE);
   if (!widget->window)
      return (FALSE);
   if (!bl)
      return (FALSE);
   if ((bl->gradient.colors) && (bl->gradient.colors->next))
   {
      if (bl->gradient.colors->next->data)
         g = bl->gradient.colors->next->data;
   }
   else
   {
      g = (E_Background_Gradient)
         malloc(sizeof(struct _E_Background_Gradient));
      memset(g, 0, sizeof(struct _E_Background_Gradient));
      bl->gradient.colors = evas_list_append(bl->gradient.colors, g);
   }
   if (gc2)
      gdk_gc_destroy(gc2);
   gc2 = gdk_gc_new(widget->window);
   fg.pixel = gdk_rgb_xpixel_from_rgb((g->r << 16) | (g->g << 8) | g->b);
   gdk_gc_set_foreground(gc2, &fg);
   gdk_draw_rectangle(widget->window, gc2, 1, 0, 0, widget->allocation.width,
                      widget->allocation.height);

   return (TRUE);
}


gboolean
on_main_win_delete_event(GtkWidget * widget, GdkEvent * event,
                         gpointer user_data)
{
   write_ebony_bg_list_db(recent_bgs);
   gtk_main_quit();
   return FALSE;
}


void
on_layer_outline_invert_activate(GtkMenuItem * menuitem, gpointer user_data)
{
   fprintf(stderr, "I should be inverting outline colors\n");
}


gboolean
on_evas_expose_event(GtkWidget * widget, GdkEventExpose * event,
                     gpointer user_data)
{
   evas_update_rect(evas, event->area.x, event->area.y, event->area.width,
                    event->area.height);
   DRAW();
   return FALSE;
}


void
on_layer_type_toggled(GtkToggleButton * togglebutton, gpointer user_data)
{
   GtkWidget *w, *ww;

   w = gtk_object_get_data(GTK_OBJECT(win_ref), "type_image");
   ww = gtk_object_get_data(GTK_OBJECT(win_ref), "type_color");
   if ((w == GTK_WIDGET(togglebutton)))
   {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
      {
         if (bl)
            bl->type = E_BACKGROUND_TYPE_IMAGE;
         advanced_widgets_show_for_image();
      }
   }
   else if ((ww == GTK_WIDGET(togglebutton)))
   {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
      {
         if (bl)
            bl->type = E_BACKGROUND_TYPE_SOLID;
         advanced_widgets_show_for_color();
      }
   }
   else                         /* type_gradient */
   {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)))
      {
         if (bl)
            bl->type = E_BACKGROUND_TYPE_GRADIENT;
         advanced_widgets_show_for_gradient();
      }
   }
}


void
on_gradient_angle_changed(GtkEditable * editable, gpointer user_data)
{
   if (!bl)
      return;

   bl->gradient.angle = get_spin_value("gradient_angle");
   evas_set_angle(evas, bl->obj, bl->gradient.angle);
   update_background(bg);
   return;

}

gboolean
on_win_bg_delete_event(GtkWidget * widget, GdkEvent * event,
                       gpointer user_data)
{
   if (bg)
   {
      e_bg_free(bg);
      bg = NULL;
      bl = NULL;
   }
   if (evas)
   {
      evas_free(evas);
   }
   gtk_widget_destroy(widget);
   bg_ref = NULL;
   evas = NULL;
   ebony_status = NULL;
   return FALSE;
}

gboolean
on_win_bg_button_press_event(GtkWidget * widget, GdkEventKey * event,
                             gpointer user_data)
{
   guint mods;

   mods = event->state;

   if (mods & GDK_CONTROL_MASK)
   {
      /* check out gdkkeysyms.h for other key values */
      switch (event->keyval)
      {
        case GDK_q:
           gtk_main_quit();
           break;
        case GDK_w:
           gtk_widget_destroy(bg_ref);
           on_win_bg_delete_event(bg_ref, NULL, NULL);
           break;
        case GDK_n:
           new_bg(NULL, NULL);
           break;
        case GDK_l:
        case GDK_o:
           on_open_bg_activate(NULL, NULL);
           break;
        case GDK_s:
           on_save_bg_activate(NULL, NULL);
        default:
           break;
      }
   }
   return FALSE;
}

void
on_scale_scroll_request(GtkWidget * widget, gpointer user_data)
{
   if (!bg)
      return;
   if (!bl)
      return;


   if ((bl->scroll.x) || (bl->scroll.y))
   {
      e_bg_set_scroll(bg, (int) get_range_value("xscale"),
                      (int) get_range_value("yscale"));
      DRAW();
   }
}
