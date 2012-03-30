#include "efx_private.h"

typedef struct Efx_Zoom_Data
{
   EFX *e;
   Evas_Map *map;
   Ecore_Animator *anim;
   Efx_Effect_Speed speed;
   double ending_zoom;
   double current_zoom;
   double starting_zoom;
   Evas_Point focus;
   Efx_End_Cb cb;
   void *data;
} Efx_Zoom_Data;

static void
_obj_del(Efx_Zoom_Data *ezd, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if (ezd->anim) ecore_animator_del(ezd->anim);
   ezd->e->zoom_data = NULL;
   if ((!ezd->e->rotate_data) && (!ezd->e->spin_data))
     {
        eina_hash_del_by_key(_efx_object_manager, ezd->e->obj);
        free(ezd->e);
     }
   free(ezd);
}

static void
_zoom(EFX *e, Evas_Object *obj, double zoom, Evas_Coord x, Evas_Coord y)
{
   Evas_Map *map;

   map = evas_map_new(4);
   evas_map_smooth_set(map, EINA_FALSE);
   evas_map_util_points_populate_from_object(map, obj);
   //DBG("ZOOM: %g: %d,%d", zoom, x, y);
   evas_map_util_zoom(map, zoom, zoom, x, y);
   if (e->rotate_data) _efx_rotate_calc(e->rotate_data, map);
   else if (e->spin_data) _efx_spin_calc(e->spin_data, map);
   evas_object_map_set(obj, map);
   evas_object_map_enable_set(obj, EINA_TRUE);
   evas_map_free(map);
}

static Eina_Bool
_zoom_cb(Efx_Zoom_Data *ezd, double pos)
{
   Evas_Coord x, y, w, h;
   double zoom;

   zoom = ecore_animator_pos_map(pos, ezd->speed, 0, 0);
   if (ezd->focus.x == -1)
     {
        evas_object_geometry_get(ezd->e->obj, &x, &y, &w, &h);
        x += (w / 2);
        y += (h / 2);
     }
   else
     {
        x = ezd->focus.x;
        y = ezd->focus.y;
     }
   ezd->current_zoom = (zoom * (ezd->ending_zoom - ezd->starting_zoom)) + ezd->starting_zoom;
   //DBG("total: %g || zoom (pos %g): %g || endzoom: %g || startzoom: %g", ezd->current_zoom, zoom, pos, ezd->ending_zoom, ezd->starting_zoom);
   _zoom(ezd->e, ezd->e->obj, ezd->current_zoom, x, y);

   if (pos != 1.0) return EINA_TRUE;

   if (ezd->cb) ezd->cb(ezd->data, ezd->ending_zoom, ezd->e->obj);
   return EINA_TRUE;
}

static void
_zoom_stop(Evas_Object *obj, Eina_Bool reset)
{
   EFX *e;
   Efx_Zoom_Data *ezd;

   e = eina_hash_find(_efx_object_manager, obj);
   if ((!e) || (!e->zoom_data)) return;
   ezd = e->zoom_data;
   if (reset)
     {
        Evas_Coord x, y, w, h;
        evas_object_geometry_get(ezd->e->obj, &x, &y, &w, &h);
        x += (w / 2);
        y += (h / 2);
        _zoom(e, obj, 1.0, x, y);
        evas_object_event_callback_del_full(obj, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_obj_del, ezd);
        _obj_del(ezd, NULL, NULL, NULL);
        INF("reset zooming object %p", obj);
     }
   else
     {
        ecore_animator_del(ezd->anim);
        ezd->anim = NULL;
        INF("stopped zooming object %p", obj);
     }
}

void
_efx_zoom_calc(void *data, Evas_Map *map)
{
   Efx_Zoom_Data *ezd = data;
   Evas_Coord x, y, w, h;
   if (ezd->focus.x == -1)
     {
        evas_object_geometry_get(ezd->e->obj, &x, &y, &w, &h);
        x += (w / 2);
        y += (h / 2);
     }
   else
     {
        x = ezd->focus.x;
        y = ezd->focus.y;
     }
   evas_map_util_zoom(map, ezd->current_zoom, ezd->current_zoom, x, y);
}

Eina_Bool
efx_zoom(Evas_Object *obj, Efx_Effect_Speed speed, double starting_zoom, double ending_zoom, Evas_Point *zoom_point, double total_time, Efx_End_Cb cb, const void *data)
{
   EFX *e;
   Efx_Zoom_Data *ezd;
 
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);
   if (ending_zoom <= 0.0) return EINA_FALSE;
   if (starting_zoom <= 0.0) return EINA_FALSE;
   if (total_time < 0.0) return EINA_FALSE;
   if (speed > EFX_EFFECT_SPEED_SINUSOIDAL) return EINA_FALSE;
   if (zoom_point)
     {
        if (zoom_point->x < 0) return EINA_FALSE;
        if (zoom_point->y < 0) return EINA_FALSE;
     }

   INF("zoom: %p - %g%%->%g%% over %gs: %s", obj, starting_zoom * 100.0, ending_zoom * 100.0, total_time, efx_speed_str[speed]);
   e = eina_hash_find(_efx_object_manager, obj);
   if (!e) e = efx_new(obj);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, EINA_FALSE);
   if (!total_time)
     {
        Evas_Coord x, y, w, h;

        if (!zoom_point)
          {
             evas_object_geometry_get(obj, &x, &y, &w, &h);
             x += (w / 2);
             y += (h / 2);
          }
        else
          {
             x = zoom_point->x;
             y = zoom_point->y;
          }
        if (e->zoom_data) efx_zoom_stop(obj);
        else
          {
             e->zoom_data = calloc(1, sizeof(Efx_Zoom_Data));
             evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_obj_del, e->zoom_data);
          }
        EINA_SAFETY_ON_NULL_RETURN_VAL(e->zoom_data, EINA_FALSE);
        _zoom(e, obj, ending_zoom, x, y);
        ezd = e->zoom_data;
        ezd->current_zoom = ending_zoom;
        if (zoom_point)
          ezd->focus.x = zoom_point->x, ezd->focus.y = zoom_point->y;
        else
          ezd->focus.x = ezd->focus.y = -1;
        return EINA_TRUE;
     }
   if (!e->zoom_data)
     {
        e->zoom_data = calloc(1, sizeof(Efx_Zoom_Data));
        EINA_SAFETY_ON_NULL_RETURN_VAL(e->zoom_data, EINA_FALSE);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE, (Evas_Object_Event_Cb)_obj_del, e->zoom_data);
     }
   ezd = e->zoom_data;
   ezd->e = e;
   ezd->speed = speed;
   ezd->ending_zoom = ending_zoom;
   ezd->starting_zoom = ezd->current_zoom = starting_zoom;
   if (zoom_point)
     ezd->focus.x = zoom_point->x, ezd->focus.y = zoom_point->y;
   else
     ezd->focus.x = ezd->focus.y = -1;
   ezd->cb = cb;
   ezd->data = (void*)data;
   if (ezd->anim) ecore_animator_del(ezd->anim);
   ecore_animator_timeline_add(total_time, (Ecore_Timeline_Cb)_zoom_cb, ezd);
   return EINA_TRUE;
}

void
efx_zoom_reset(Evas_Object *obj)
{
   _zoom_stop(obj, EINA_FALSE);
}

void
efx_zoom_stop(Evas_Object *obj)
{
   _zoom_stop(obj, EINA_FALSE);
}
