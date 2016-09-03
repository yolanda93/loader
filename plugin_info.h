#ifndef _PLUGIN_INFO_H_
#define _PLUGIN_INFO_H_

/* Este fichero incluye la descripcion de la estructura usada para
   describir los servicios ofertados por el modulo dinamico. */

typedef struct plugin_info_st
{
  char*            banner;
  void           (*plugin_loop)   ();
  void           (*plugin_stop)   ();
  void           (*plugin_resume) ();
  int            (*plugin_finish) ();
} plugin_info_t;

extern plugin_info_t* plugin_info();

#endif
