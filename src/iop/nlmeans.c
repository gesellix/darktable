/*
    This file is part of darktable,
    copyright (c) 2011 johannes hanika.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "develop/imageop.h"
#include "dtgtk/slider.h"
#include "gui/gtk.h"
#include <gtk/gtk.h>
#include <stdlib.h>

// this is the version of the modules parameters,
// and includes version information about compile-time dt
DT_MODULE(1)

typedef struct dt_iop_nlmeans_params_t
{
  // these are stored in db.
  float luma;
  float chroma;
}
dt_iop_nlmeans_params_t;

typedef struct dt_iop_nlmeans_gui_data_t
{
  GtkDarktableSlider *luma;
  GtkDarktableSlider *chroma;
}
dt_iop_nlmeans_gui_data_t;

typedef dt_iop_nlmeans_params_t dt_iop_nlmeans_data_t;

typedef struct dt_iop_nlmeans_global_data_t
{
  // this is optionally stored in self->global_data
  // and can be used to alloc globally needed stuff
  // which is needed in gui mode and during processing.

  // we don't need it for this example (as for most dt plugins)
}
dt_iop_nlmeans_global_data_t;

const char *name()
{
  return _("denoising (extra slow)");
}

int
groups ()
{
  return IOP_GROUP_CORRECT;
}

/** modify regions of interest (optional, per pixel ops don't need this) */
// void modify_roi_out(struct dt_iop_module_t *self, struct dt_dev_pixelpipe_iop_t *piece, dt_iop_roi_t *roi_out, const dt_iop_roi_t *roi_in);
// void modify_roi_in(struct dt_iop_module_t *self, struct dt_dev_pixelpipe_iop_t *piece, const dt_iop_roi_t *roi_out, dt_iop_roi_t *roi_in);

static float gh(const float const f)
{
  // return 0.0001f + dt_fast_expf(-fabsf(f)*800.0f);
  // return 1.0f/(1.0f + f*f);
  // make spread bigger: less smoothing
  const float spread = 100.f;
  return 1.0f/(1.0f + fabsf(f)*spread);
}

// TODO: this should be _a lot_ faster (perfectly suited, ppl report real-time performance numbers..)
// void process_cl 

/** process, all real work is done here. */
void process (struct dt_iop_module_t *self, dt_dev_pixelpipe_iop_t *piece, void *i, void *o, const dt_iop_roi_t *roi_in, const dt_iop_roi_t *roi_out)
{
  // this is called for preview and full pipe separately, each with its own pixelpipe piece.
  // get our data struct:
  // dt_iop_nlmeans_params_t *d = (dt_iop_nlmeans_params_t *)piece->data;

  const int K = 7; // nbhood
  const int P = 3; // pixel filter size

  // TODO: adjust to Lab, make L more important
  // TODO: are these user parameters, or should we just use blending after the fact?
  const float norm[4] = { 1.0f/50.0f, 1.0f/256.0f, 1.0f/256.0f, 1.0f };

  float *S = dt_alloc_align(64, sizeof(float)*roi_out->width*roi_out->height);
  // we want to sum up weights in col[3], so need to init to 0:
  memset(o, 0x0, sizeof(float)*roi_out->width*roi_out->height*4);

  // for each shift vector
  for(int kj=-K;kj<=K;kj++)
  {
    for(int ki=-K;ki<=K;ki++)
    {
#if 0
      // TODO: don't construct summed area tables but use sliding window! (applies to cpu version res < 1k only)
#else
      // construct summed area table of weights:
#ifdef _OPENMP
  #pragma omp parallel for default(none) schedule(static) shared(i,S,roi_in,roi_out,kj,ki)
#endif
      for(int j=0; j<roi_out->height; j++)
      {
        const float *in  = ((float *)i) + 4* roi_in->width * j;
        const float *ins = ((float *)i) + 4*(roi_in->width *(j+kj) + ki);
        float *out = ((float *)S) + roi_out->width*j;
        if(j+kj < 0 || j+kj >= roi_out->height) memset(out, 0x0, sizeof(float)*roi_out->width);
        else for(int i=0; i<roi_out->width; i++)
        {
          if(i+ki < 0 || i+ki >= roi_out->width) out[0] = 0.0f;
          else
          {
            out[0]  = (in[0] - ins[0])*(in[0] - ins[0]) * norm[0] * norm[0];
            out[0] += (in[1] - ins[1])*(in[1] - ins[1]) * norm[1] * norm[1];
            out[0] += (in[2] - ins[2])*(in[2] - ins[2]) * norm[2] * norm[2];
          }
          in  += 4;
          ins += 4;
          out ++;
        }
      }
      // now sum up:
      // horizontal phase:
#ifdef _OPENMP
  #pragma omp parallel for default(none) schedule(static) shared(S,roi_out)
#endif
      for(int j=0; j<roi_out->height; j++)
      {
        int stride = 1;
        while(stride < roi_out->width)
        {
          float *out = ((float *)S) + roi_out->width*j;
          for(int i=0;i<roi_out->width-stride;i++)
          {
            out[0] += out[stride];
            out ++;
          }
          stride <<= 1;
        }
      }
      // vertical phase:
#ifdef _OPENMP
  #pragma omp parallel for default(none) schedule(static) shared(S,roi_out)
#endif
      for(int i=0; i<roi_out->width; i++)
      {
        int stride = 1;
        while(stride < roi_out->height)
        {
          float *out = S + i;
          for(int j=0;j<roi_out->height-stride;j++)
          {
            out[0] += out[roi_out->width*stride];
            out += roi_out->width;
          }
          stride <<= 1;
        }
      }
      // now the denoising loop:
#ifdef _OPENMP
  #pragma omp parallel for default(none) schedule(static) shared(i,o,S,kj,ki,roi_in,roi_out)
#endif
      for(int j=0; j<roi_out->height; j++)
      {
        if(j+kj < 0 || j+kj >= roi_out->height) continue;
        const float *in  = ((float *)i) + 4*(roi_in->width *(j+kj) + ki);
        float *out = ((float *)o) + 4*roi_out->width*j;
        const float *s = S + roi_out->width*j;
        const int offy = MIN(j, MIN(roi_out->height - j - 1, P)) * roi_out->width;
        for(int i=0; i<roi_out->width; i++)
        {
          if(i+ki >= 0 && i+ki < roi_out->width)
          {
            const int offx = MIN(i, MIN(roi_out->width - i - 1, P));
            const float m1 = s[offx - offy], m2 = s[- offx + offy], p1 = s[offx + offy], p2 = s[- offx - offy];
            const float w = gh(p1 + p2 - m1 - m2);
            for(int k=0;k<3;k++) out[k] += in[k] * w;
            out[3] += w;
          }
          s   ++;
          in  += 4;
          out += 4;
        }
      }
    }
#endif
  }
  // normalize:
#ifdef _OPENMP
  #pragma omp parallel for default(none) schedule(static) shared(o,roi_out)
#endif
  for(int j=0; j<roi_out->height; j++)
  {
    float *out = ((float *)o) + 4*roi_out->width*j;
    for(int i=0; i<roi_out->width; i++)
    {
      for(int k=0;k<3;k++) out[k] *= 1.0f/out[3];
      out += 4;
    }
  }
  // free the summed area table:
  free(S);
}

/** optional: if this exists, it will be called to init new defaults if a new image is loaded from film strip mode. */
void reload_defaults(dt_iop_module_t *module)
{
  // our module is disabled by default
  module->default_enabled = 0;
  // init defaults:
  dt_iop_nlmeans_params_t tmp = (dt_iop_nlmeans_params_t)
  {
    0.5f, 0.5f
  };
  memcpy(module->params, &tmp, sizeof(dt_iop_nlmeans_params_t));
  memcpy(module->default_params, &tmp, sizeof(dt_iop_nlmeans_params_t));
}

/** init, cleanup, commit to pipeline */
void init(dt_iop_module_t *module)
{
  // we don't need global data:
  module->data = NULL; //malloc(sizeof(dt_iop_nlmeans_global_data_t));
  module->params = malloc(sizeof(dt_iop_nlmeans_params_t));
  module->default_params = malloc(sizeof(dt_iop_nlmeans_params_t));
  // about the first thing to do in Lab space:
  module->priority = 310;
  module->params_size = sizeof(dt_iop_nlmeans_params_t);
  module->gui_data = NULL;
}

void cleanup(dt_iop_module_t *module)
{
  free(module->gui_data);
  module->gui_data = NULL; // just to be sure
  free(module->params);
  module->params = NULL;
  free(module->data); // just to be sure
  module->data = NULL;
}

/** commit is the synch point between core and gui, so it copies params to pipe data. */
void commit_params (struct dt_iop_module_t *self, dt_iop_params_t *params, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  dt_iop_nlmeans_params_t *p = (dt_iop_nlmeans_params_t *)params;
  dt_iop_nlmeans_data_t *d = (dt_iop_nlmeans_data_t *)piece->data;
  d->luma   = p->luma;
  d->chroma = p->chroma;
}

void init_pipe     (struct dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  piece->data = malloc(sizeof(dt_iop_nlmeans_data_t));
  self->commit_params(self, self->default_params, pipe, piece);
}

void cleanup_pipe  (struct dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  free(piece->data);
}

static void
luma_callback(GtkRange *range, dt_iop_module_t *self)
{
  // this is important to avoid cycles!
  if(darktable.gui->reset) return;
  dt_iop_nlmeans_gui_data_t *g = (dt_iop_nlmeans_gui_data_t *)self->gui_data;
  dt_iop_nlmeans_params_t *p = (dt_iop_nlmeans_params_t *)self->params;
  p->luma = dtgtk_slider_get_value(g->luma);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void
chroma_callback(GtkRange *range, dt_iop_module_t *self)
{
  // this is important to avoid cycles!
  if(darktable.gui->reset) return;
  dt_iop_nlmeans_gui_data_t *g = (dt_iop_nlmeans_gui_data_t *)self->gui_data;
  dt_iop_nlmeans_params_t *p = (dt_iop_nlmeans_params_t *)self->params;
  p->chroma = dtgtk_slider_get_value(g->chroma);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

/** gui callbacks, these are needed. */
void gui_update    (dt_iop_module_t *self)
{
  // let gui slider match current parameters:
  dt_iop_nlmeans_gui_data_t *g = (dt_iop_nlmeans_gui_data_t *)self->gui_data;
  dt_iop_nlmeans_params_t *p = (dt_iop_nlmeans_params_t *)self->params;
  dtgtk_slider_set_value(g->luma,   p->luma);
  dtgtk_slider_set_value(g->chroma, p->chroma);
}

void gui_init     (dt_iop_module_t *self)
{
  // init the slider (more sophisticated layouts are possible with gtk tables and boxes):
  self->gui_data = malloc(sizeof(dt_iop_nlmeans_gui_data_t));
  dt_iop_nlmeans_gui_data_t *g = (dt_iop_nlmeans_gui_data_t *)self->gui_data;
  self->widget = gtk_vbox_new(TRUE, DT_GUI_IOP_MODULE_CONTROL_SPACING);
  // TODO: adjust defaults:
  g->luma   = DTGTK_SLIDER(dtgtk_slider_new_with_range(DARKTABLE_SLIDER_BAR, 0.0f, 1.0f, 0.01, 0.5f, 3));
  g->chroma = DTGTK_SLIDER(dtgtk_slider_new_with_range(DARKTABLE_SLIDER_BAR, 0.0f, 1.0f, 0.01, 0.5f, 3));
  dtgtk_slider_set_default_value(g->luma,   0.5f);
  dtgtk_slider_set_default_value(g->chroma, 0.5f);
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(g->luma), TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(g->chroma), TRUE, TRUE, 0);
  dtgtk_slider_set_label(g->luma, _("luma"));
  dtgtk_slider_set_unit (g->luma, "%");
  dtgtk_slider_set_label(g->chroma, _("chroma"));
  dtgtk_slider_set_unit (g->chroma, "%");
  g_object_set (GTK_OBJECT(g->luma),   "tooltip-text", _("how much to smooth brightness"), (char *)NULL);
  g_object_set (GTK_OBJECT(g->chroma), "tooltip-text", _("how much to smooth colors"), (char *)NULL);
  g_signal_connect (G_OBJECT (g->luma),   "value-changed", G_CALLBACK (luma_callback),   self);
  g_signal_connect (G_OBJECT (g->chroma), "value-changed", G_CALLBACK (chroma_callback), self);
}

void gui_cleanup  (dt_iop_module_t *self)
{
  // nothing else necessary, gtk will clean up the slider.
  free(self->gui_data);
  self->gui_data = NULL;
}

/** additional, optional callbacks to capture darkroom center events. */
// void gui_post_expose(dt_iop_module_t *self, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery);
// int mouse_moved(dt_iop_module_t *self, double x, double y, int which);
// int button_pressed(dt_iop_module_t *self, double x, double y, int which, int type, uint32_t state);
// int button_released(struct dt_iop_module_t *self, double x, double y, int which, uint32_t state);
// int scrolled(dt_iop_module_t *self, double x, double y, int up, uint32_t state);

// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;