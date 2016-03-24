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

#define  _ISOC99_SOURCE

#include <ladspa.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* returns an integer, -1 if x is negative, 1 otherwise */
#define signum(x) ((((x) >= 0) << 1) - 1)
/* returns 1 if the argument is a floating point zero */
#define fzero(x) (fpclassify(x) == FP_ZERO)

struct op {
  unsigned int count;
  LADSPA_Data   **ports;
  void        *extra;
};

LADSPA_Handle make_op(const LADSPA_Descriptor *descriptor, unsigned long sr) {
  struct op *new = malloc(sizeof(struct op));
  new->ports = malloc(sizeof(LADSPA_Data)*descriptor->PortCount);
  return new;
}

/* since we only store ports in our handle,
   we can treat it as an array  of ports */
void connect_port_op(LADSPA_Handle handle, unsigned long port,
		     LADSPA_Data *loc) {
  struct op *op = (struct op *)handle;
  op->ports[port] = loc;
}

void cleanup_op(LADSPA_Handle instance) {
  struct op * op = (struct op *) instance;
  if (op->extra) {
    free(op->extra);
  }
  free(instance);
}

/* data shared by the various ladspa descriptors */
const LADSPA_PortDescriptor binary_control_desc[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL
};


const LADSPA_PortRangeHint binary_control_hints[] =
  {{LADSPA_HINT_DEFAULT_1, HUGE_VAL*-1, HUGE_VAL},
   {LADSPA_HINT_DEFAULT_1, HUGE_VAL*-1, HUGE_VAL},
   0};

#define maker "Justin Smith (IMPURE DATA)"
#define copyright "NONE"
#define id_start 100
#define properties LADSPA_PROPERTY_HARD_RT_CAPABLE

/*      */
/* plus */
/*      */
void run_plus(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = *o->ports[0] + *o->ports[1];
}
const char *plus_ports[] = {"a", "b", "a+b"};
LADSPA_Descriptor plus = {
  /* UniqueID Label Properties  Name */
  id_start,    "+", properties, "plus (c, c) -> c",
  /* Maker Copyright  PortCount PortDescriptors      PortNames */
  maker,   copyright, 3,        binary_control_desc, plus_ports,
  /* PortRangeHints  ImplementationData instantiate connect_port    activate */
  binary_control_hints, NULL,           make_op,    connect_port_op, NULL,
  /* run      run_adding set_run_adding_gain deactivate cleanup */
  run_plus, NULL,      NULL,               NULL,      cleanup_op
};

/*               */
/* reverse minus */
/*               */
void run_rminus(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = *o->ports[1] - *o->ports[0];
}
const char *rminus_ports[] = {"a", "b", "b-a"};
LADSPA_Descriptor rminus = {
  /* UniqueID Label Properties  Name */
  id_start+1, "r-", properties, "reverse minus (c, c) -> c",
  /* Maker Copyright  PortCount PortDescriptors      PortNames */
  maker,   copyright, 3,        binary_control_desc, rminus_ports,
  /* PortRangeHints ImplementationData instantiate connect_port    activate */
  binary_control_hints, NULL,          make_op,    connect_port_op, NULL,
  /* run      run_adding set_run_adding_gain deactivate cleanup */
  run_rminus, NULL,      NULL,               NULL,      cleanup_op
};

/*       */
/* minus */
/*       */
void run_minus(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = *o->ports[0] - *o->ports[1];
}
const char *minus_ports[] = {"a", "b", "a-b"};
LADSPA_Descriptor minus = {
  /* UniqueID Label Properties  Name */
  id_start+2, "-",  properties, "minus (c, c) -> c",
  /* Maker Copyright  PortCount PortDescriptors      PortNames */
  maker,   copyright, 3,        binary_control_desc, minus_ports,
  /* PortRangeHints ImplementationData instantiate connect_port    activate */
  binary_control_hints, NULL,          make_op,    connect_port_op, NULL,
  /* run      run_adding set_run_adding_gain deactivate cleanup */
  run_minus, NULL,      NULL,               NULL,      cleanup_op
};

/*        */
/* divide */
/*        */

void run_divide(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = fzero(*o->ports[1]) ?
    HUGE_VAL*signum(*o->ports[0]) : *o->ports[0] / *o->ports[1];
}
const char *divide_ports[] = {"a", "b", "a/b"};
LADSPA_Descriptor divide = {
  id_start+3, "/", properties, "divide (c, c) -> c", maker, copyright, 3,
  binary_control_desc, divide_ports, binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_divide, NULL, NULL, NULL, cleanup_op
};

/*                */
/* reverse divide */
/*                */
void run_rdivide(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = fzero(*o->ports[0]) ?
    HUGE_VAL*signum(*o->ports[1]) : *o->ports[1] / *o->ports[0];
}
const char *rdivide_ports[] = {"a", "b", "b/a"};
LADSPA_Descriptor rdivide = {
  id_start+4, "rdiv", properties, "reverse divide (c, c) -> c", maker,
  copyright, 3,
  binary_control_desc, rdivide_ports, binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_divide, NULL, NULL, NULL, cleanup_op
};

/*          */
/* multiply */
/*          */
void run_multiply(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = *o->ports[0] * *o->ports[1];
}
const char *multiply_ports[] = {"a", "b", "a*b"};
LADSPA_Descriptor multiply = {
  id_start+5, "*", properties, "multiply (c, c) -> c", maker, copyright, 3,
  binary_control_desc, multiply_ports, binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_multiply, NULL, NULL, NULL, cleanup_op
};

/*        */
/* modulo */
/*        */
void run_modulo(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = fmodf(*o->ports[0], *o->ports[1]);
}
const char *modulo_ports[] = {"a", "b", "a%b"};
LADSPA_Descriptor modulo = {
  id_start+6, "%", properties, "modulo (c, c) -> c", maker, copyright, 3,
  binary_control_desc, modulo_ports, binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_modulo, NULL, NULL, NULL, cleanup_op
};

/* bitwise operations */
const LADSPA_PortRangeHint bitwise_binary_control_hints[] =
  {{LADSPA_HINT_DEFAULT_1|LADSPA_HINT_INTEGER|
    LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE,
    0.0, 4294967296.0},
   {LADSPA_HINT_DEFAULT_1|LADSPA_HINT_INTEGER|
    LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE,
    0.0, 4294967296.0},
   {LADSPA_HINT_DEFAULT_1|LADSPA_HINT_INTEGER|
    LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE,
    0.0, 4294967296.0}};

/*           */
/* binary or */
/*           */
void run_bor(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] | (int)*o->ports[1]);
}
const char *bor_ports[] = {"a", "b", "a|b"};
LADSPA_Descriptor bor = {
  id_start+7, "|", properties, "bitwise or (c, c) -> c", maker, copyright, 3,
  binary_control_desc, bor_ports, bitwise_binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_bor, NULL, NULL, NULL, cleanup_op
};

/*     */
/* xor */
/*     */
void run_xor(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] ^ (int)*o->ports[1]);
}
const char *xor_ports[] = {"a", "b", "a^b"};
LADSPA_Descriptor xor = {
  id_start+8, "^", properties, "exclusive or (c, c) -> c", maker, copyright, 3,
  binary_control_desc, xor_ports, bitwise_binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_xor, NULL, NULL, NULL, cleanup_op
};

/*            */
/* binary and */
/*            */
void run_band(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] & (int)*o->ports[1]);
}
const char *band_ports[] = {"a", "b", "a&b"};
LADSPA_Descriptor band = {
  id_start+9, "&", properties, "binary and (c, c) -> c", maker, copyright, 3,
  binary_control_desc, band_ports, bitwise_binary_control_hints, NULL, make_op,
  connect_port_op, NULL, run_band, NULL, NULL, NULL, cleanup_op
};

/*             */
/* right shift */
/*             */
void run_shiftr(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] >> (int)*o->ports[1]);
}
const char *shiftr_ports[] = {"a", "b", "a>>b"};
LADSPA_Descriptor shiftr = {
  id_start+10, ">>", properties, "shift right (c, c) -> c", maker, copyright, 3,
  binary_control_desc, shiftr_ports, bitwise_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_shiftr, NULL, NULL, NULL, cleanup_op
};

/*            */
/* left shift */
/*            */
void run_shiftl(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] << (int)*o->ports[1]);
}
const char *shiftl_ports[] = {"a", "b", "a<<b"};
LADSPA_Descriptor shiftl = {
  id_start+11, "<<", properties, "shift left (c, c) -> c", maker, copyright, 3,
  binary_control_desc, shiftl_ports, bitwise_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_shiftl, NULL, NULL, NULL, cleanup_op
};

/* logical operators */

const LADSPA_PortRangeHint logical_binary_control_hints[] =
  {{LADSPA_HINT_TOGGLED},
   {LADSPA_HINT_TOGGLED},
   {LADSPA_HINT_TOGGLED}};

/*       */
/* equal */
/*       */
void run_equal(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)(*o->ports[0] == *o->ports[1]);
}
const char *equal_ports[] = {"a", "b", "a=b"};
LADSPA_Descriptor equal = {
  id_start+12, "=", properties, "equal (c, c) -> c", maker, copyright, 3,
  binary_control_desc, equal_ports, logical_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_equal, NULL, NULL, NULL, cleanup_op
};

/*         */
/* greater */
/*         */
void run_greater(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)(*o->ports[0] > *o->ports[1]);
}
const char *greater_ports[] = {"a", "b", "a>b"};
LADSPA_Descriptor greater = {
  id_start+13, ">", properties, "greater (c, c) -> c", maker, copyright, 3,
  binary_control_desc, greater_ports, logical_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_greater, NULL, NULL, NULL, cleanup_op
};

/*      */
/* less */
/*      */
void run_less(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)(*o->ports[0] < *o->ports[1]);
}
const char *less_ports[] = {"a", "b", "a<b"};
LADSPA_Descriptor less = {
  id_start+14, "<", properties, "less (c, c) -> c", maker, copyright, 3,
  binary_control_desc, less_ports, logical_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_less, NULL, NULL, NULL, cleanup_op
};

/*          */
/* notequal */
/*          */
void run_notequal(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)(*o->ports[0] != *o->ports[1]);
}
const char *notequal_ports[] = {"a", "b", "a!=b"};
LADSPA_Descriptor notequal = {
  id_start+15, "!=", properties, "not equal (c, c) -> c", maker, copyright, 3,
  binary_control_desc, notequal_ports, logical_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_notequal, NULL, NULL, NULL, cleanup_op
};

/*        */
/* logand */
/*        */
void run_logand(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] && (int)*o->ports[1]);
}
const char *logand_ports[] = {"a", "b", "a&&b"};
LADSPA_Descriptor logand = {
  id_start+16, "&&", properties, "logical and (c, c) -> c", maker, copyright, 3,
  binary_control_desc, logand_ports, logical_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_logand, NULL, NULL, NULL, cleanup_op
};

/*        */
/* logior */
/*        */
void run_logior(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  *o->ports[2] = (float)((int)*o->ports[0] || (int)*o->ports[1]);
}
const char *logior_ports[] = {"a", "b", "a||b"};
LADSPA_Descriptor logior = {
  id_start+17, "||", properties, "logical or (c, c) -> c", maker, copyright, 3,
  binary_control_desc, logior_ports, logical_binary_control_hints, NULL,
  make_op, connect_port_op, NULL, run_logior, NULL, NULL, NULL, cleanup_op
};

const LADSPA_PortRangeHint switch_hints[] =
  {{LADSPA_HINT_TOGGLED},
   {LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1, HUGE_VAL},
   {LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE, HUGE_VAL*-1,
    HUGE_VAL}};
const LADSPA_PortDescriptor switch_desc[] = {
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
  LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL
};
void run_switch(LADSPA_Handle instance, unsigned long count) {
  struct op *o = (struct op *)instance;
  if(*o->ports[0] > 0.0) {
    *o->ports[3] = *o->ports[1];
  } else {
    *o->ports[3] = *o->ports[2];
  }
}
const char *switch_ports[] = {"predicate", "a", "b", "result"};
LADSPA_Descriptor bswitch = {
  id_start+19, "switch", properties, "switch (c, c, c) -> c", maker, copyright,
  4, switch_desc, switch_ports, switch_hints, NULL, make_op, connect_port_op,
  NULL, run_switch, NULL, NULL, NULL, cleanup_op
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
  switch(index) {
  case 0:  return &plus;
  case 1:  return &minus;
  case 2:  return &rminus;
  case 3:  return &divide;
  case 4:  return &rdivide;
  case 5:  return &multiply;
  case 6:  return &modulo;
  case 7:  return &bor;
  case 8:  return &xor;
  case 9:  return &band;
  case 10: return &shiftr;
  case 11: return &shiftl;
  case 12: return &equal;
  case 13: return &greater;
  case 14: return &less;
  case 15: return &notequal;
  case 16: return &logand;
  case 17: return &logior;
/*   case 18: return &spigot; */
  case 18: return &bswitch;
  default: return NULL;
  }
}

LADSPA_PortRangeHint *hints;

void _init() {
}

void _fini() {
}

