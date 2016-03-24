/*
  this file is a part of ladosc, which in turn is a part of the noisesmith
  package: <http://code.google.com/p/noisesmith-linux-audio/>
  Copyright (C) 2008  Justin Smith
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ladspa.h>
#include <stdlib.h>
#include <lo/lo.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

/*******************/
/*******************/
/*******************/
/****           ****/
/****           ****/
/**** universal ****/
/****           ****/
/****           ****/
/*******************/
/*******************/
/*******************/

#define maker "Justin Smith (LADOSC)"
#define copyright "GPL, v3 or Newer"
#define id_start 150
#define properties LADSPA_PROPERTY_HARD_RT_CAPABLE
#define dest "/lad_osc"
#define port_min 49152.0
#define port_max 65535.0

/******************/
/******************/
/***            ***/
/*** osc output ***/
/***            ***/
/******************/
/******************/

struct out {
  LADSPA_Data *input;
  LADSPA_Data *index;
  LADSPA_Data old_input;
  LADSPA_Data old_index;
  LADSPA_Data *connect;
  LADSPA_Data *port;
  LADSPA_Data *ip[4];
  int connect_state;
  lo_address addr;
  LADSPA_Data *status;
};

LADSPA_Handle make_out(const LADSPA_Descriptor *descriptor,
			 unsigned long sr) {
  struct out *new = malloc(sizeof(struct out));
  if(new == NULL) {
    return NULL;
  }
  new->connect_state = 0;
  new->addr = NULL;
  return new;
}

void connect_port_out(LADSPA_Handle handle, unsigned long port,
		     LADSPA_Data *loc) {
  struct out *o = (struct out *)handle;
  switch(port) {
  case 0: o->input = loc; break;
  case 1: o->index = loc; break;
  case 2: o->connect = loc; break;
  case 3: o->port = loc; break;
  case 4: o->ip[0] = loc; break;
  case 5: o->ip[1] = loc; break;
  case 6: o->ip[2] = loc; break;
  case 7: o->ip[3] = loc; break;
  case 8: o->status = loc;
  }
}

void cleanup_out(LADSPA_Handle instance) {
  struct out *o = (struct out *)instance;
  if(o->connect_state) {
    lo_address_free(o->addr);
  }
  free(instance);
}

const LADSPA_PortDescriptor out_desc[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL
};


const LADSPA_PortRangeHint out_hints[] = {
  {LADSPA_HINT_DEFAULT_0, HUGE_VAL*-1, HUGE_VAL},
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_INTEGER|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, 0.0, 999.0},
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_TOGGLED, 0.0, 1.0},
  {LADSPA_HINT_DEFAULT_MINIMUM|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, port_min, port_max},
  /* defaults to 127.0.0.1 : localhost */
  {LADSPA_HINT_DEFAULT_MIDDLE|LADSPA_HINT_BOUNDED_BELOW| /* defaults 127 */
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, 0.0, 255.0},
  {LADSPA_HINT_DEFAULT_MINIMUM|LADSPA_HINT_BOUNDED_BELOW| /* defaults 0 */
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, 0.0, 255.0},
  {LADSPA_HINT_DEFAULT_MINIMUM|LADSPA_HINT_BOUNDED_BELOW| /* defaults 0 */
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, 0.0, 255.0},
  {LADSPA_HINT_DEFAULT_1|LADSPA_HINT_BOUNDED_BELOW|     /* defaults 1 */
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, 0.0, 255.0},
  {LADSPA_HINT_INTEGER, INT_MIN, INT_MAX}
};

void run_out(LADSPA_Handle instance, unsigned long count) {
  struct out *o = (struct out *)instance;
  int connect = (int)*o->connect;
  if(connect && !o->connect_state) {
      char addr[16]; /* big enough for "255.255.255.255" */
      char oct1[3];
      char oct2[3];
      char oct3[3];
      char oct4[3];
      char port[6]; /* big enough for "65535" */
      snprintf(oct1, 4,  "%d", (int) *o->ip[0]);
      snprintf(oct2, 4,  "%d", (int) *o->ip[1]);
      snprintf(oct3, 4,  "%d", (int) *o->ip[2]);
      snprintf(oct4, 4,  "%d", (int) *o->ip[3]);
      snprintf(addr, 16, "%s.%s.%s.%s", oct1, oct2, oct3, oct4);
      snprintf(port, 6, "%d", (int) *o->port);
#ifdef DEBUG
      fprintf(stderr, "osc_out: connecting to %s:%s\n", addr, port);
#endif
      o->addr = lo_address_new(addr, port);
      if(lo_address_errno(o->addr) != 0) {
	fprintf(stderr, "osc_out: connection error: %d\n",
		lo_address_errno(o->addr));
      }
      o->connect_state  = 1;
  } else if (!connect && o->connect_state) {
#ifdef DEBUG
    fprintf(stderr, "osc_out: disconnecting\n");
#endif
    lo_address_free(o->addr);
    o->addr = NULL;
    o->connect_state = 0;
  }
  if(o->connect_state) {
    int send = 0;
    if(*o->index != o->old_index) {
      o->old_index = *o->index;
      send = 1;
    }
    if(*o->input != o->old_input) {
      o->old_input = *o->input;
      send = 1;
    } /* we are lazy about sending, because sending takes precious time! */
    if (send && (o->addr != NULL)) {
#ifdef DEBUG
      fprintf(stderr, "osc_out sending %f\n", *o->input);
#endif
      lo_send(o->addr, dest, "ff", *o->index, *o->input);
      if(lo_address_errno(o->addr) != 0) {
	fprintf(stderr, "LADSPA osc.so: osc_out: error sending message: %s\n",
		lo_address_errstr(o->addr));
      }
    }
    if(o->status) {
      *o->status = (float)lo_address_errno(o->addr);
    }
  }
}
      

const char *out_ports[] = {"input", "index", "connect", "port", "octet_1",
			   "octet_2", "octet_3", "octet_4", "status"};
LADSPA_Descriptor out = {
 UniqueID: id_start, Label: "osc_out", Properties: properties,
 Name: "osc output",  Maker: maker, Copyright: copyright, PortCount: 9,
 PortDescriptors: out_desc, PortNames: out_ports, PortRangeHints: out_hints,
 ImplementationData: NULL, instantiate: make_out,
 connect_port:  connect_port_out, activate: NULL, run: run_out,
 run_adding: NULL, set_run_adding_gain: NULL, deactivate: NULL,
 cleanup: cleanup_out
};


/*****************/
/*****************/
/***           ***/
/*** osc input ***/
/***           ***/
/*****************/
/*****************/

struct in {
  LADSPA_Data *index_start;
  LADSPA_Data *output[16];
   LADSPA_Data *connect;
  LADSPA_Data *port;
  LADSPA_Data *status;
  lo_server_thread thread;
  int connect_state;
};

LADSPA_Handle make_in(const LADSPA_Descriptor *descriptor,
			 unsigned long sr) {
  struct in *new = calloc(1, sizeof(struct in));
  if(new == NULL) {
    return NULL;
  }
  /* made redundant by calloc above */
/*   new->thread = NULL; */
/*   new->connect_state = 0; */
/*   new->status = NULL; */
/*   new->output[0] = NULL; */
/*   new->output[1] = NULL; */
/*   new->output[2] = NULL; */
/*   new->output[3] = NULL; */
/*   new->output[4] = NULL; */
/*   new->output[5] = NULL; */
/*   new->output[6] = NULL; */
/*   new->output[7] = NULL; */
/*   new->output[8] = NULL; */
/*   new->output[9] = NULL; */
/*   new->output[10] = NULL; */
/*   new->output[11] = NULL; */
/*   new->output[12] = NULL; */
/*   new->output[13] = NULL; */
/*   new->output[14] = NULL; */
/*   new->output[15] = NULL; */
  return new;
}

void connect_port_in(LADSPA_Handle handle, unsigned long port,
		     LADSPA_Data *loc) {
  struct in *i = (struct in *)handle;
  switch(port) {
  case 0: i->index_start = loc; break;
  case 1 ... 16: i->output[port-1] = loc; break;
  case 17: i->connect = loc; break;
  case 18: i->port = loc; break;
  case 19: i->status = loc;
  }
}

void cleanup_in(LADSPA_Handle instance) {
  struct in *i = (struct in *)instance;
  if(i->connect_state) {
    lo_server_thread_free(i->thread);
  }
  free(instance);
}

const LADSPA_PortDescriptor in_desc[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL, /* index_start */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[0] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[1] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[2] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[3] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[4] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[5] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[6] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[7] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[8] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[9] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[10] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[11] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[12] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[13] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[14] */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL, /* output[15] */
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL, /* connect */
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL, /* port */
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL /* status */
};


const LADSPA_PortRangeHint in_hints[] = {
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_INTEGER|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, 0.0, 999.0}, /* index_start */
  /* specify infinite bounded below and above, because buggy hosts all
     act like I specified bounds even if I didn't */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[0] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[1] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[2] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[3] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[4] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[5] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[6] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[7] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[8] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[9] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[10] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[11] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[12] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[13] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[14] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|
   LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL}, /* output[15] */
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_TOGGLED, 0.0, 1.0}, /* connect */
  {LADSPA_HINT_DEFAULT_MINIMUM|LADSPA_HINT_BOUNDED_BELOW| /* port */
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, port_min, port_max},
  {LADSPA_HINT_DEFAULT_0|LADSPA_HINT_BOUNDED_BELOW|     /* status */
   LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER, INT_MIN, INT_MAX}
};

void server_init_error(int n, const char *msg, const char *path) {
  fprintf(stderr, "lad-osc server error %d in path %s: %s\n", n, path, msg);
}

int in_handler(const char *path, const char *types, lo_arg **argv, int argc,
		void *data, void *user_data) {
  struct in *i = (struct in *)user_data;
  int index = (int) (argv[0]->f - *i->index_start);
  float value = argv[1]->f;
  if((index >= 0) && (index < 16) && i->output[index]) {
    *i->output[index] = value;
  }
  return 0;
}

void run_in(LADSPA_Handle instance, unsigned long count) {
  struct in *i = (struct in *)instance;
  int connect = (int)*i->connect;
  if(connect && !i->connect_state) {
      char port[6]; /* big enough for "65535" */
      snprintf(port, 6, "%d", (int) *i->port);
#ifdef DEBUG
      fprintf(stderr, "osc_in: starting server on %s\n", port);
#endif
      i->thread = lo_server_thread_new(port, server_init_error);
      lo_server_thread_add_method(i->thread, dest, "ff", in_handler, i);
      lo_server_thread_start(i->thread);
      i->connect_state  = 1;
  } else if (!connect && i->connect_state && i->thread) {
#ifdef DEBUG
    fprintf(stderr, "osc_in: halting server on %d\n",
	    lo_server_trhead_get_port(i->thread));
#endif
    lo_server_thread_stop(i->thread);
    lo_server_thread_free(i->thread);
    i->thread = NULL;
    i->connect_state = 0;
  }
}
      

const char *in_ports[] = {"index_start", "out_0", "out_1", "out_2", "out_3",
			  "out_4", "out_5", "out_6", "out_7", "out_8", "out_9",
			  "out_10", "out_11", "out_12", "out_13", "out_14",
			  "out_15", "connect", "port", "status", "bug"};
LADSPA_Descriptor in = {
  UniqueID: id_start+1, Label: "osc_in", Properties: properties,
  Name: "osc input", Maker: maker,   Copyright: copyright, PortCount: 20,
  PortDescriptors: in_desc, PortNames: in_ports, PortRangeHints: in_hints,
  ImplementationData: NULL, instantiate: make_in,
  connect_port: connect_port_in, activate: NULL, run: run_in, run_adding: NULL,
  set_run_adding_gain: NULL, deactivate: NULL, cleanup: cleanup_in
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
  switch(index) {
  case 0: return &out;
  case 1: return &in;
  default: return NULL;
  }
}

void _init() {
}

void _fini() {
}
