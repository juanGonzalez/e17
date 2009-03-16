/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

static void _e_bindings_mouse_free(E_Binding_Mouse *bind);
static void _e_bindings_key_free(E_Binding_Key *bind);
static void _e_bindings_edge_free(E_Binding_Edge *bind);
static void _e_bindings_signal_free(E_Binding_Signal *bind);
static void _e_bindings_wheel_free(E_Binding_Wheel *bind);
static int _e_bindings_context_match(E_Binding_Context bctxt, E_Binding_Context ctxt);
static E_Binding_Modifier _e_bindings_modifiers(unsigned int modifiers);
static int _e_ecore_modifiers(E_Binding_Modifier modifiers);
static int  _e_bindings_edge_cb_timer(void *data);

/* local subsystem globals */

static Eina_List *mouse_bindings  = NULL;
static Eina_List *key_bindings    = NULL;
static Eina_List *edge_bindings   = NULL;
static Eina_List *signal_bindings = NULL;
static Eina_List *wheel_bindings  = NULL;

typedef struct _E_Binding_Edge_Data E_Binding_Edge_Data;

struct _E_Binding_Edge_Data
{
   E_Binding_Edge    *bind;
   E_Event_Zone_Edge *ev;
   E_Action          *act;
   E_Object	     *obj;
};

/* externally accessible functions */

EAPI int
e_bindings_init(void)
{
   Eina_List *l;

   for (l = e_config->mouse_bindings; l; l = l->next)
     {
	E_Config_Binding_Mouse *eb;

	eb = l->data;
	e_bindings_mouse_add(eb->context, eb->button, eb->modifiers,
			     eb->any_mod, eb->action, eb->params);
     }

   for (l = e_config->key_bindings; l; l = l->next)
     {
	E_Config_Binding_Key *eb;

	eb = l->data;
	e_bindings_key_add(eb->context, eb->key, eb->modifiers,
			   eb->any_mod, eb->action, eb->params);
     }

   for (l = e_config->edge_bindings; l; l = l->next)
     {
	E_Config_Binding_Edge *eb;

	eb = l->data;
	e_bindings_edge_add(eb->context, eb->edge, eb->modifiers,
			   eb->any_mod, eb->action, eb->params, eb->delay);
     }

   for (l = e_config->signal_bindings; l; l = l->next)
     {
	E_Config_Binding_Signal *eb;

	eb = l->data;
	e_bindings_signal_add(eb->context, eb->signal, eb->source, eb->modifiers,
			      eb->any_mod, eb->action, eb->params);
	/* FIXME: Can this be solved in a generic way? */
	/* FIXME: Only change cursor if action is allowed! */
	if ((eb->action) && (eb->signal) && (eb->source) &&
	    (!strcmp(eb->action, "window_resize")) &&
	    (!strncmp(eb->signal, "mouse,down,", 11)) &&
	    (!strncmp(eb->source, "e.event.resize.", 15)))
	  {
	     char params[32];
	     snprintf(params, sizeof(params), "resize_%s", eb->params);
	     e_bindings_signal_add(eb->context, "mouse,in", eb->source, eb->modifiers,
				   eb->any_mod, "pointer_resize_push", params);
	     e_bindings_signal_add(eb->context, "mouse,out", eb->source, eb->modifiers,
				   eb->any_mod, "pointer_resize_pop", params);
	  }
     }

   for (l = e_config->wheel_bindings; l; l = l->next)
     {
	E_Config_Binding_Wheel *eb;

	eb = l->data;
	e_bindings_wheel_add(eb->context, eb->direction, eb->z, eb->modifiers,
			     eb->any_mod, eb->action, eb->params);
     }

   return 1;
}

EAPI int
e_bindings_shutdown(void)
{
   while (mouse_bindings)
     {
	E_Binding_Mouse *bind;
	
	bind = mouse_bindings->data;
	mouse_bindings  = eina_list_remove_list(mouse_bindings, mouse_bindings);
	_e_bindings_mouse_free(bind);
     }
   while (key_bindings)
     {
	E_Binding_Key *bind;
	
	bind = key_bindings->data;
	key_bindings  = eina_list_remove_list(key_bindings, key_bindings);
	_e_bindings_key_free(bind);
     }
   while (edge_bindings)
     {
	E_Binding_Edge *bind;
	
	bind = edge_bindings->data;
	edge_bindings  = eina_list_remove_list(edge_bindings, edge_bindings);
	_e_bindings_edge_free(bind);
     }
   while (signal_bindings)
     {
	E_Binding_Signal *bind;
	
	bind = signal_bindings->data;
	signal_bindings  = eina_list_remove_list(signal_bindings, signal_bindings);
	_e_bindings_signal_free(bind);
     }
   while (wheel_bindings)
     {
	E_Binding_Wheel *bind;
	
	bind = wheel_bindings->data;
	wheel_bindings  = eina_list_remove_list(wheel_bindings, wheel_bindings);
	_e_bindings_wheel_free(bind);
     }
   return 1;
}

EAPI void
e_bindings_mouse_add(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   E_Binding_Mouse *bind;
   
   bind = calloc(1, sizeof(E_Binding_Mouse));
   bind->ctxt = ctxt;
   bind->button = button;
   bind->mod = mod;
   bind->any_mod = any_mod;
   if (action) bind->action = eina_stringshare_add(action);
   if (params) bind->params = eina_stringshare_add(params);
   mouse_bindings = eina_list_append(mouse_bindings, bind);
}

EAPI void
e_bindings_mouse_del(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   Eina_List *l;
   
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (bind->button == button) &&
	    (bind->mod == mod) &&
	    (bind->any_mod == any_mod) &&
	    (((bind->action) && (action) && (!strcmp(bind->action, action))) ||
	     ((!bind->action) && (!action))) &&
	    (((bind->params) && (params) && (!strcmp(bind->params, params))) ||
	     ((!bind->params) && (!params))))
	  {
	     _e_bindings_mouse_free(bind);
	     mouse_bindings = eina_list_remove_list(mouse_bindings, l);
	     break;
	  }
     }
}

EAPI void
e_bindings_mouse_grab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Eina_List *l;

   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     ecore_x_window_button_grab(win, bind->button,
					ECORE_X_EVENT_MASK_MOUSE_DOWN |
					ECORE_X_EVENT_MASK_MOUSE_UP |
					ECORE_X_EVENT_MASK_MOUSE_MOVE,
					_e_ecore_modifiers(bind->mod),
					bind->any_mod);
	  }
     }
}

EAPI void
e_bindings_mouse_ungrab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Eina_List *l;
   
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     ecore_x_window_button_ungrab(win, bind->button,
					  _e_ecore_modifiers(bind->mod), bind->any_mod);
	  }
     }
}

EAPI E_Action *
e_bindings_mouse_down_find(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev, E_Binding_Mouse **bind_ret)
{
   E_Binding_Modifier mod = 0;
   Eina_List *l;

   mod = _e_bindings_modifiers(ev->modifiers);
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if ((bind->button == ev->buttons) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (bind_ret) *bind_ret = bind;
		  return act;
	       }
	  }
     }
   return NULL;
}

EAPI E_Action *
e_bindings_mouse_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev)
{
   E_Action *act;
   E_Binding_Mouse *bind;
   
   act = e_bindings_mouse_down_find(ctxt, obj, ev, &bind);
   if (act)
     {
	if (act->func.go_mouse)
	  act->func.go_mouse(obj, bind->params, ev);
	else if (act->func.go)
	  act->func.go(obj, bind->params);
	return act;
     }
   return act;
}

EAPI E_Action *
e_bindings_mouse_up_find(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev, E_Binding_Mouse **bind_ret)
{
   E_Binding_Modifier mod = 0;
   Eina_List *l;

   mod = _e_bindings_modifiers(ev->modifiers);
   for (l = mouse_bindings; l; l = l->next)
     {
	E_Binding_Mouse *bind;
	
	bind = l->data;
	if ((bind->button == ev->buttons) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (bind_ret) *bind_ret = bind;
		  return act;
	       }
	  }
     }
   return NULL;
}

EAPI E_Action *
e_bindings_mouse_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev)
{
   E_Action *act;
   E_Binding_Mouse *bind;
   
   act = e_bindings_mouse_up_find(ctxt, obj, ev, &bind);
   if (act)
     {
	if (act->func.end_mouse)
	  act->func.end_mouse(obj, bind->params, ev);
	else if (act->func.end)
	  act->func.end(obj, bind->params);
	return act;
     }
   return act;
}

EAPI void
e_bindings_key_add(E_Binding_Context ctxt, const char *key, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   E_Binding_Key *bind;
   
   bind = calloc(1, sizeof(E_Binding_Key));
   bind->ctxt = ctxt;
   bind->key = eina_stringshare_add(key);
   bind->mod = mod;
   bind->any_mod = any_mod;
   if (action) bind->action = eina_stringshare_add(action);
   if (params) bind->params = eina_stringshare_add(params);
   key_bindings = eina_list_append(key_bindings, bind);
}

EAPI E_Binding_Key *
e_bindings_key_get(const char *action)
{
   Eina_List *l;
   
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if (bind->action && action && !strcmp(action, bind->action))
	  return bind;
     }
   return NULL;
}

EAPI void
e_bindings_key_del(E_Binding_Context ctxt, const char *key, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   Eina_List *l;
   
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (key) && (bind->key) && (!strcmp(bind->key, key)) &&
	    (bind->mod == mod) &&
	    (bind->any_mod == any_mod) &&
	    (((bind->action) && (action) && (!strcmp(bind->action, action))) ||
	     ((!bind->action) && (!action))) &&
	    (((bind->params) && (params) && (!strcmp(bind->params, params))) ||
	     ((!bind->params) && (!params))))
	  {
	     _e_bindings_key_free(bind);
	     key_bindings = eina_list_remove_list(key_bindings, l);
	     break;
	  }
     }
}

EAPI void
e_bindings_key_grab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Eina_List *l;

   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     ecore_x_window_key_grab(win, bind->key,
				     _e_ecore_modifiers(bind->mod), bind->any_mod);
	  }
     }
}

EAPI void
e_bindings_key_ungrab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Eina_List *l;
   
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     ecore_x_window_key_ungrab(win, bind->key,
				       _e_ecore_modifiers(bind->mod), bind->any_mod);
	  }
     }
}

EAPI E_Action *
e_bindings_key_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Key *ev)
{
   E_Binding_Modifier mod = 0;
   Eina_List *l;

   mod = _e_bindings_modifiers(ev->modifiers);
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if ((bind->key) && (!strcmp(bind->key, ev->keyname)) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
	
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       if (act->func.go_key)
			 act->func.go_key(obj, bind->params, ev);
		       else if (act->func.go)
			 act->func.go(obj, bind->params);
		       return act;
		    }
		  return NULL;
	       }
	  }
     }
   return NULL;
}

EAPI E_Action *
e_bindings_key_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Key *ev)
{
   E_Binding_Modifier mod = 0;
   Eina_List *l;

   mod = _e_bindings_modifiers(ev->modifiers);
   for (l = key_bindings; l; l = l->next)
     {
	E_Binding_Key *bind;
	
	bind = l->data;
	if ((bind->key) && (!strcmp(bind->key, ev->keyname)) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       if (act->func.end_key)
			 act->func.end_key(obj, bind->params, ev);
		       else if (act->func.end)
			 act->func.end(obj, bind->params);
		       return act;
		    }
		  return NULL;
	       }
	  }
     }
   return NULL;
}

EAPI void
e_bindings_edge_add(E_Binding_Context ctxt, E_Zone_Edge edge, E_Binding_Modifier mod, int any_mod, const char *action, const char *params, float delay)
{
   E_Binding_Edge *bind;
   
   bind = calloc(1, sizeof(E_Binding_Edge));
   bind->ctxt = ctxt;
   bind->edge = edge;
   bind->mod = mod;
   bind->any_mod = any_mod;
   bind->delay = delay;
   if (action) bind->action = eina_stringshare_add(action);
   if (params) bind->params = eina_stringshare_add(params);
   edge_bindings = eina_list_append(edge_bindings, bind);
}

EAPI E_Binding_Edge *
e_bindings_edge_get(const char *action, E_Zone_Edge edge)
{
   Eina_List *l;
   
   for (l = edge_bindings; l; l = l->next)
     {
	E_Binding_Edge *bind;
	
	bind = l->data;
	if ((bind->edge == edge) && bind->action &&
	      action && !strcmp(action, bind->action))
	  return bind;
     }
   return NULL;
}

EAPI void
e_bindings_edge_del(E_Binding_Context ctxt, E_Zone_Edge edge, E_Binding_Modifier mod, int any_mod, const char *action, const char *params, float delay)
{
   Eina_List *l;
   
   for (l = edge_bindings; l; l = l->next)
     {
	E_Binding_Edge *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (bind->edge == edge) &&
	    (bind->mod == mod) &&
	    ((bind->delay * 1000) == (delay * 1000)) &&
	    (bind->any_mod == any_mod) &&
	    (((bind->action) && (action) && (!strcmp(bind->action, action))) ||
	     ((!bind->action) && (!action))) &&
	    (((bind->params) && (params) && (!strcmp(bind->params, params))) ||
	     ((!bind->params) && (!params))))
	  {
	     _e_bindings_edge_free(bind);
	     edge_bindings = eina_list_remove_list(edge_bindings, l);
	     break;
	  }
     }
}

EAPI E_Action *
e_bindings_edge_in_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev)
{
   E_Binding_Modifier mod = 0;
   E_Desk *current = NULL;
   E_Action *act = NULL;
   Eina_List *l;
   
   current = e_desk_at_xy_get(ev->zone, ev->zone->desk_x_current, ev->zone->desk_y_current);
   if (current->fullscreen_borders && (!e_config->fullscreen_flip)) return NULL;

   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   for (l = edge_bindings; l; l = l->next)
     {
	E_Binding_Edge *bind;
	
	bind = l->data;
	if (((bind->edge == ev->edge)) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  act = e_action_find(bind->action);
		  if (act)
		    {
		       E_Binding_Edge_Data *ed = E_NEW(E_Binding_Edge_Data, 1);
		       E_Event_Zone_Edge *ev2 = E_NEW(E_Event_Zone_Edge, 1);

		       /* The original event will be freed before it can be
			* used again */
		       ev2->zone  = ev->zone;
		       ev2->edge  = ev->edge;
		       ev2->x     = ev->x;
		       ev2->y     = ev->y;

		       ed->bind = bind;
		       ed->obj  = obj;
		       ed->act  = act;
		       ed->ev   = ev2;
		       bind->timer = ecore_timer_add(((double) bind->delay), _e_bindings_edge_cb_timer, ed);
		    }
	       }
	  }
     }
   return act;
}

EAPI E_Action *
e_bindings_edge_out_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev)
{
   E_Binding_Modifier mod = 0;
   E_Action *act = NULL;
   Eina_List *l;
   
   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   for (l = edge_bindings; l; l = l->next)
     {
	E_Binding_Edge *bind;
	
	bind = l->data;
	if ((bind->edge == ev->edge) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  if (bind->timer)
		    ecore_timer_del(bind->timer);
		  bind->timer = NULL;

		  act = e_action_find(bind->action);
		  if (act && act->func.end)
		    act->func.end(obj, bind->params);
	       }
	  }
     }
   return act;
}

EAPI void
e_bindings_signal_add(E_Binding_Context ctxt, const char *sig, const char *src, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   E_Binding_Signal *bind;
   
   bind = calloc(1, sizeof(E_Binding_Signal));
   bind->ctxt = ctxt;
   if (sig) bind->sig = eina_stringshare_add(sig);
   if (src) bind->src = eina_stringshare_add(src);
   bind->mod = mod;
   bind->any_mod = any_mod;
   if (action) bind->action = eina_stringshare_add(action);
   if (params) bind->params = eina_stringshare_add(params);
   signal_bindings = eina_list_append(signal_bindings, bind);
}

EAPI void
e_bindings_signal_del(E_Binding_Context ctxt, const char *sig, const char *src, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   Eina_List *l;
   
   for (l = signal_bindings; l; l = l->next)
     {
	E_Binding_Signal *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (((bind->sig) && (sig) && (!strcmp(bind->sig, sig))) ||
	     ((!bind->sig) && (!sig))) &&
	    (((bind->src) && (src) && (!strcmp(bind->src, src))) ||
	     ((!bind->src) && (!src))) &&
	    (bind->mod == mod) &&
	    (bind->any_mod == any_mod) &&
	    (((bind->action) && (action) && (!strcmp(bind->action, action))) ||
	     ((!bind->action) && (!action))) &&
	    (((bind->params) && (params) && (!strcmp(bind->params, params))) ||
	     ((!bind->params) && (!params))))
	  {
	     _e_bindings_signal_free(bind);
	     signal_bindings = eina_list_remove_list(signal_bindings, l);
	     break;
	  }
     }
}

EAPI E_Action  *
e_bindings_signal_find(E_Binding_Context ctxt, E_Object *obj, const char *sig, const char *src, E_Binding_Signal **bind_ret)
{
   E_Binding_Modifier mod = 0;
   Eina_List *l;
   
   if (strstr(sig, "MOD:Shift")) mod |= E_BINDING_MODIFIER_SHIFT;
   if (strstr(sig, "MOD:Control")) mod |= E_BINDING_MODIFIER_CTRL;
   if (strstr(sig, "MOD:Alt")) mod |= E_BINDING_MODIFIER_ALT;
   if (strstr(sig, "MOD:Super")) mod |= E_BINDING_MODIFIER_WIN;
   for (l = signal_bindings; l; l = l->next)
     {
	E_Binding_Signal *bind;
	
	bind = l->data;
	if ((e_util_glob_match(sig, bind->sig)) &&
	    (e_util_glob_match(src, bind->src)) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (bind_ret) *bind_ret = bind;
		  return act;
	       }
	  }
     }
   return NULL;
}

EAPI E_Action *
e_bindings_signal_handle(E_Binding_Context ctxt, E_Object *obj, const char *sig, const char *src)
{
   E_Action *act;
   E_Binding_Signal *bind;

   if (sig[0] == 0) sig = NULL;
   if (src[0] == 0) src = NULL;
   act = e_bindings_signal_find(ctxt, obj, sig, src, &bind);
   if (act)
     {
	if (act->func.go_signal)
	  act->func.go_signal(obj, bind->params, sig, src);
	else if (act->func.go)
	  act->func.go(obj, bind->params);
	return act;
     }
   return act;
}

EAPI void
e_bindings_wheel_add(E_Binding_Context ctxt, int direction, int z, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   E_Binding_Wheel *bind;
   
   bind = calloc(1, sizeof(E_Binding_Wheel));
   bind->ctxt = ctxt;
   bind->direction = direction;
   bind->z = z;
   bind->mod = mod;
   bind->any_mod = any_mod;
   if (action) bind->action = eina_stringshare_add(action);
   if (params) bind->params = eina_stringshare_add(params);
   wheel_bindings = eina_list_append(wheel_bindings, bind);
}

EAPI void
e_bindings_wheel_del(E_Binding_Context ctxt, int direction, int z, E_Binding_Modifier mod, int any_mod, const char *action, const char *params)
{
   Eina_List *l;
   
   for (l = wheel_bindings; l; l = l->next)
     {
	E_Binding_Wheel *bind;
	
	bind = l->data;
	if ((bind->ctxt == ctxt) &&
	    (bind->direction == direction) &&
	    (bind->z == z) &&
	    (bind->mod == mod) &&
	    (bind->any_mod == any_mod) &&
	    (((bind->action) && (action) && (!strcmp(bind->action, action))) ||
	     ((!bind->action) && (!action))) &&
	    (((bind->params) && (params) && (!strcmp(bind->params, params))) ||
	     ((!bind->params) && (!params))))
	  {
	     _e_bindings_wheel_free(bind);
	     wheel_bindings = eina_list_remove_list(wheel_bindings, l);
	     break;
	  }
     }
}

EAPI void
e_bindings_wheel_grab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Eina_List *l;

   for (l = wheel_bindings; l; l = l->next)
     {
	E_Binding_Wheel *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     int button = 0;

	     if (bind->direction == 0)
	       {
		  if (bind->z < 0) button = 4;
		  else if (bind->z > 0) button = 5;
	       }
	     else if (bind->direction == 1)
	       {
		  if (bind->z < 0) button = 6;
		  else if (bind->z > 0) button = 7;
	       }
	     if (button != 0)
	       ecore_x_window_button_grab(win, button,
					  ECORE_X_EVENT_MASK_MOUSE_DOWN,
					  _e_ecore_modifiers(bind->mod), bind->any_mod);
	  }
     }
}

EAPI void
e_bindings_wheel_ungrab(E_Binding_Context ctxt, Ecore_X_Window win)
{
   Eina_List *l;
   
   for (l = wheel_bindings; l; l = l->next)
     {
	E_Binding_Wheel *bind;
	
	bind = l->data;
	if (_e_bindings_context_match(bind->ctxt, ctxt))
	  {
	     int button = 0;
	     
	     if (bind->direction == 0)
	       {
		  if (bind->z < 0) button = 4;
		  else if (bind->z > 0) button = 5;
	       }
	     else if (bind->direction == 1)
	       {
		  if (bind->z < 0) button = 6;
		  else if (bind->z > 0) button = 7;
	       }
	     if (button != 0)
	       ecore_x_window_button_ungrab(win, button,
					    _e_ecore_modifiers(bind->mod), bind->any_mod);
	  }
     }
}

EAPI E_Action *
e_bindings_wheel_find(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Wheel *ev, E_Binding_Wheel **bind_ret)
{
   E_Binding_Modifier mod = 0;
   Eina_List *l;

   mod = _e_bindings_modifiers(ev->modifiers);
   for (l = wheel_bindings; l; l = l->next)
     {
	E_Binding_Wheel *bind;
	
	bind = l->data;
	if ((bind->direction == ev->direction) &&
	    (((bind->z < 0) && (ev->z < 0)) || ((bind->z > 0) && (ev->z > 0))) &&
	    ((bind->any_mod) || (bind->mod == mod)))
	  {
	     if (_e_bindings_context_match(bind->ctxt, ctxt))
	       {
		  E_Action *act;
		  
		  act = e_action_find(bind->action);
		  if (bind_ret) *bind_ret = bind;
		  return act;
	       }
	  }
     }
   return NULL;
}

EAPI E_Action *
e_bindings_wheel_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Wheel *ev)
{
   E_Action *act;
   E_Binding_Wheel *bind;

   act = e_bindings_wheel_find(ctxt, obj, ev, &bind);
   if (act)
     {
	if (act->func.go_wheel)
	  act->func.go_wheel(obj, bind->params, ev);
	else if (act->func.go)
	  act->func.go(obj, bind->params);
	return act;
     }
   return act;
}

/* local subsystem functions */

static void
_e_bindings_mouse_free(E_Binding_Mouse *bind)
{
   if (bind->action) eina_stringshare_del(bind->action);
   if (bind->params) eina_stringshare_del(bind->params);
   free(bind);
}

static void
_e_bindings_key_free(E_Binding_Key *bind)
{
   if (bind->key) eina_stringshare_del(bind->key);
   if (bind->action) eina_stringshare_del(bind->action);
   if (bind->params) eina_stringshare_del(bind->params);
   free(bind);
}

static void
_e_bindings_edge_free(E_Binding_Edge *bind)
{
   if (bind->action) eina_stringshare_del(bind->action);
   if (bind->params) eina_stringshare_del(bind->params);
   if (bind->timer)
     {
	E_Binding_Edge_Data *ed;

	ed = ecore_timer_del(bind->timer);
	E_FREE(ed);
     }
   free(bind);
}

static void
_e_bindings_signal_free(E_Binding_Signal *bind)
{
   if (bind->sig) eina_stringshare_del(bind->sig);
   if (bind->src) eina_stringshare_del(bind->src);
   if (bind->action) eina_stringshare_del(bind->action);
   if (bind->params) eina_stringshare_del(bind->params);
   free(bind);
}

static void
_e_bindings_wheel_free(E_Binding_Wheel *bind)
{
   if (bind->action) eina_stringshare_del(bind->action);
   if (bind->params) eina_stringshare_del(bind->params);
   free(bind);
}

static int
_e_bindings_context_match(E_Binding_Context bctxt, E_Binding_Context ctxt)
{
   if (bctxt == E_BINDING_CONTEXT_ANY) return 1;
   if (ctxt == E_BINDING_CONTEXT_UNKNOWN) return 0;
   if (bctxt == ctxt) return 1;
   return 0;
}

static E_Binding_Modifier
_e_bindings_modifiers(unsigned int modifiers)
{
   E_Binding_Modifier mod = 0;

   if (modifiers & ECORE_EVENT_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (modifiers & ECORE_EVENT_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (modifiers & ECORE_EVENT_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (modifiers & ECORE_EVENT_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   /* FIXME: there is a good reason numlock was ignored. sometimes people
    * have it on, sometimes they don't, and often they have no idea. waaaay
    * back in E 0.1->0.13 or so days this caused issues thus numlock,
    * scrollock and capslock are not usable modifiers.
    * 
    * if we REALLY want to be able to use numlock we need to add more binding
    * flags and config that says "REALLY pay attention to numlock for this
    * binding" field in the binding (like there is a "any_mod" flag - we need a
    * "num_lock_respect" field)
    * 
    * also it should be an E_BINDING_MODIFIER_LOCK_NUM as the ecore lock flag
    * may vary from system to system as different xservers may have differing
    * modifier masks for numlock (it is queried at startup).
    * 
   if (ev->modifiers & ECORE_X_LOCK_NUM) mod |= ECORE_X_LOCK_NUM;
    */

   return mod;
}

static int
_e_ecore_modifiers(E_Binding_Modifier modifiers)
{
   int mod = 0;

   if (modifiers & E_BINDING_MODIFIER_SHIFT) mod |= ECORE_EVENT_MODIFIER_SHIFT;
   if (modifiers & E_BINDING_MODIFIER_CTRL) mod |= ECORE_EVENT_MODIFIER_CTRL;
   if (modifiers & E_BINDING_MODIFIER_ALT) mod |= ECORE_EVENT_MODIFIER_ALT;
   if (modifiers & E_BINDING_MODIFIER_WIN) mod |= ECORE_EVENT_MODIFIER_WIN;
   /* see comment in e_bindings on numlock
      if (modifiers & ECORE_X_LOCK_NUM) mod |= ECORE_X_LOCK_NUM;
   */

   return mod;
}

static int
_e_bindings_edge_cb_timer(void *data)
{
   E_Binding_Edge_Data *ed;
   E_Event_Zone_Edge *ev;
   E_Binding_Edge *bind;
   E_Action *act;
   E_Object *obj;

   ed = data;
   bind = ed->bind;
   act = ed->act;
   obj = ed->obj;
   ev = ed->ev;

   E_FREE(ed);

   if (act->func.go_edge)
     act->func.go_edge(obj, bind->params, ev);
   else if (act->func.go)
     act->func.go(obj, bind->params);

   bind->timer = NULL;

   /* Duplicate event */
   E_FREE(ev);

   return 0;
}

