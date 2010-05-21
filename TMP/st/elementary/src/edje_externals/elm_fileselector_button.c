#include "private.h"

typedef struct _Elm_Params_fileselector_button
{
   Elm_Params base;
   Evas_Object *icon;

   struct {
      const char *path;
      Eina_Bool is_save:1;
      Eina_Bool is_save_set:1;
      Eina_Bool folder_only:1;
      Eina_Bool folder_only_set:1;
      Eina_Bool expandable:1;
      Eina_Bool expandable_set:1;
      Eina_Bool inwin_mode:1;
      Eina_Bool inwin_mode_set:1;
   } fs;
} Elm_Params_fileselector_button;

static void
external_fileselector_button_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_fileselector_button *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->base.label)
     elm_fileselector_button_label_set(obj, p->base.label);
   if (p->icon) elm_fileselector_button_icon_set(obj, p->icon);
   if (p->fs.path) elm_fileselector_button_selected_set(obj, p->fs.path);
   if (p->fs.is_save_set)
     elm_fileselector_button_is_save_set(obj, p->fs.is_save);
   if (p->fs.folder_only_set)
     elm_fileselector_button_folder_only_set(obj, p->fs.folder_only);
   if (p->fs.expandable_set)
     elm_fileselector_button_expandable_set(obj, p->fs.expandable);
   if (p->fs.inwin_mode_set)
     elm_fileselector_button_inwin_mode_set(obj, p->fs.inwin_mode);
}

static Eina_Bool
external_fileselector_button_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_fileselector_button_label_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *icon = external_common_param_icon_get(obj, param);
	     if (icon)
	       {
		  elm_fileselector_button_icon_set(obj, icon);
		  return EINA_TRUE;
	       }
	  }
     }
   else if (!strcmp(param->name, "path"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_fileselector_button_selected_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "save"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_fileselector_button_is_save_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "folder only"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_fileselector_button_folder_only_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "expandable"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_fileselector_button_expandable_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "inwin mode"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_fileselector_button_inwin_mode_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_fileselector_button_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_fileselector_button_label_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	/* not easy to get icon name back from live object */
	return EINA_FALSE;
     }
   else if (!strcmp(param->name, "path"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_fileselector_button_selected_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "save"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_fileselector_button_is_save_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "folder only"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_fileselector_button_folder_only_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "expandable"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_fileselector_button_expandable_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "inwin mode"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_fileselector_button_inwin_mode_get(obj);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_fileselector_button_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_fileselector_button *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = external_common_params_parse(Elm_Params_fileselector_button,
				      data, obj, params);
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "path"))
	  mem->fs.path = eina_stringshare_add(param->s);
	else if (!strcmp(param->name, "save"))
	  {
	     mem->fs.is_save = !!param->i;
	     mem->fs.is_save_set = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "folder only"))
	  {
	     mem->fs.folder_only = !!param->i;
	     mem->fs.folder_only_set = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "expandable"))
	  {
	     mem->fs.expandable = !!param->i;
	     mem->fs.expandable_set = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "inwin mode"))
	  {
	     mem->fs.inwin_mode = !!param->i;
	     mem->fs.inwin_mode_set = EINA_TRUE;
	  }
     }

   return mem;
}

 static void
external_fileselector_button_params_free(void *params)
{
   Elm_Params_fileselector_button *mem = params;

   if (mem->icon)
     evas_object_del(mem->icon);
   if (mem->fs.path)
     eina_stringshare_del(mem->fs.path);
   external_common_params_free(params);
}

static Edje_External_Param_Info external_fileselector_button_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("path"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("save"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("folder only"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("expandable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("inwin mode"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(fileselector_button, "fileselector_button");
DEFINE_EXTERNAL_TYPE_SIMPLE(fileselector_button, "File Selector Button");
