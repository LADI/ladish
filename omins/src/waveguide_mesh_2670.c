/* This file is an audio plugin.  Copyright (C) 2005 Loki Davison. Based on code by Brook Eaton and a couple
 * of papers... 
 * 
 * Implements a Waveguide Mesh drum. FIXME to be extended, to have rimguides, power normalisation and all
 * manner of other goodies.
 *
 * Tension is well, drum tension
 * power is how hard you hit it. 
 *  
 * 
 * This plugin is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This plugin is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define _XOPEN_SOURCE 500 /* strdup */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ladspa.h"

#define MESH_BASE_ID 2670

#define MESH_NUM_PORTS 6

/* Port Numbers */
#define MESH_INPUT1   0
#define MESH_OUTPUT  1
#define MESH_TENSION 2
#define MESH_POWER 3
#define MESH_EX_X  4
#define MESH_EX_Y 5


#define LENGTH 8 // must be divisible by 4!!
#define WIDTH 8
#define SIZE LENGTH*WIDTH  // Size of mesh
#define INITIAL 0 // initial values stored at junctions
#define LOSS 0.2 // loss in wave propagation
#define INIT_DELTA 6 // initial values
#define INIT_T 0.1	// for the impedances
#define INIT_GAMMA 8 //
#define PORTS 8 //for the initialization of junction velocities from excitation

// 2D array of junctions.
// The important function of the junction is to store
// the velocities of travelling waves so that adjacent junction's
// velocities can be calculated.
typedef struct _junction
{
	LADSPA_Data v_junction; // junction velocity
	LADSPA_Data n_junction; // velocity heading north into junction
	LADSPA_Data s_junction; // velocity heading south into junction
	LADSPA_Data e_junction; // velocity heading east into junction
	LADSPA_Data w_junction; // velocity heading west into junction
	LADSPA_Data c_junction; // velocity heading back into the junction (delayed)
	LADSPA_Data s_temp; // these two 'temp' values are used because calculation
	LADSPA_Data e_temp; // of the mesh updates s/e values before they are used
} _junction;		// to calculate north and south velocities!

/* All state information for plugin */

typedef struct
{
    /* Ports */
    LADSPA_Data* input1;
    LADSPA_Data* output;
    LADSPA_Data* tension;
    LADSPA_Data* power;
    LADSPA_Data* ex_x;
    LADSPA_Data* ex_y;    

    /* vars */
    _junction mesh[LENGTH][WIDTH];
    LADSPA_Data last_trigger;
    
} WgMesh;


/* Construct a new plugin instance */
LADSPA_Handle
wgmesh_instantiate(const LADSPA_Descriptor* descriptor,
                   unsigned long            srate)
{
    WgMesh * plugin = (WgMesh *) malloc (sizeof (WgMesh));
    int i, j;

    //// Init Mesh
    for (i=0; i<LENGTH; i++) {
	for (j=0; j<WIDTH; j++) {
		plugin->mesh[i][j].v_junction = INITIAL;
		plugin->mesh[i][j].n_junction = INITIAL;
		plugin->mesh[i][j].s_junction = INITIAL;
		plugin->mesh[i][j].e_junction = INITIAL;
		plugin->mesh[i][j].w_junction = INITIAL;
		plugin->mesh[i][j].c_junction = INITIAL;
		plugin->mesh[i][j].s_temp = INITIAL;
		plugin->mesh[i][j].e_temp = INITIAL;
	}
    }
    plugin->last_trigger = 0.0; 
    return (LADSPA_Handle)plugin;
}


/* Connect a port to a data location */
void
wgmesh_connect_port(LADSPA_Handle instance,
             unsigned long port,
             LADSPA_Data*  location)
{
	WgMesh* plugin;

	plugin = (WgMesh*)instance;
	switch (port) {
	case MESH_INPUT1:
		plugin->input1 = location;
		break;
	case MESH_OUTPUT:
		plugin->output = location;
		break;
	case MESH_TENSION:
		plugin->tension = location;
		break;
	case MESH_POWER:
		plugin->power = location;
		break;
	case MESH_EX_X:
		plugin->ex_x = location;
		break;
	case MESH_EX_Y:
		plugin->ex_y = location;
		break;
	}
}


inline void excite_mesh(WgMesh* plugin, LADSPA_Data power, LADSPA_Data ex_x, LADSPA_Data ex_y)
{
	int i=ex_x,j=ex_y;
	LADSPA_Data temp;
	LADSPA_Data Yj;
		
	Yj = 2*(INIT_DELTA*INIT_DELTA/((INIT_T*INIT_T)*(INIT_GAMMA*INIT_GAMMA))); // junction admittance
	temp = power*2/(LENGTH+WIDTH);
	plugin->mesh[i][j].v_junction = plugin->mesh[i][j].v_junction +  temp;
	plugin->mesh[i][j].n_junction = plugin->mesh[i][j].n_junction + Yj*temp/PORTS; 
	// All velocities leaving the junction are equal to
	plugin->mesh[i][j].s_junction = plugin->mesh[i][j].s_junction + Yj*temp/PORTS; 
			// the total velocity in the junction * the admittance
			plugin->mesh[i][j].e_junction = plugin->mesh[i][j].e_junction + Yj*temp/PORTS; 
			// divided by the number of outgoing ports.
			plugin->mesh[i][j].w_junction = plugin->mesh[i][j].w_junction + Yj*temp/PORTS;
			//mesh[i][j].c_junction = 0;
}

void
wgmesh_run_cr(LADSPA_Handle instance, unsigned long nframes)
{	
	WgMesh*  plugin = (WgMesh*)instance;
	LADSPA_Data tension = *(plugin->tension);
	LADSPA_Data ex_x = *(plugin->ex_x);
	LADSPA_Data ex_y = *(plugin->ex_y);
	LADSPA_Data* input = plugin->input1;
	LADSPA_Data* out = plugin->output;
	LADSPA_Data* power = plugin->power;
	LADSPA_Data last_trigger = plugin->last_trigger;
	size_t i,j,k;
	LADSPA_Data filt, trg, oldfilt;
	LADSPA_Data Yc,Yj,tempN,tempS,tempE,tempW;
	
	// Set input variables //
	oldfilt = plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction;

	for (k=0; k<nframes; k++) {
		if (tension==0)
			tension=0.0001;
		trg = input[k];
		    
		if (trg > 0.0f && !(last_trigger > 0.0f))
		{
		    //printf("got trigger, exciting mesh, %f \n", tension);
		    excite_mesh(plugin, power[k], ex_x, ex_y);
		}

		//junction admitance	
		Yj = 2*INIT_DELTA*INIT_DELTA/(( (tension)*((tension) )*(INIT_GAMMA*INIT_GAMMA))); 
		Yc = Yj-4; // junction admittance (left shift is for multiply by 2!)
		//plugin->v_power = power[k];

		for (i=1; i<LENGTH-1; i++) {
			// INNER MESH //
			for (j=1; j<WIDTH-1; j++) { // to multiply by 2 - simply shift to the left by 1!
				plugin->mesh[i][j].v_junction = 2.0*(plugin->mesh[i][j].n_junction + plugin->mesh[i][j].s_junction
					+ plugin->mesh[i][j].e_junction + plugin->mesh[i][j].w_junction + Yc*plugin->mesh[i][j].c_junction)/Yj; 

				plugin->mesh[i][j+1].s_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].n_junction; 
				plugin->mesh[i][j-1].n_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].s_temp; 

				plugin->mesh[i+1][j].e_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].w_junction; 
				plugin->mesh[i-1][j].w_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].e_temp; 

				plugin->mesh[i][j].c_junction = plugin->mesh[i][j].v_junction - plugin->mesh[i][j].c_junction; 

				plugin->mesh[i][j].s_temp = plugin->mesh[i][j].s_junction; //
				plugin->mesh[i][j].e_temp = plugin->mesh[i][j].e_junction; // update current values in the temp slots!
			}
			// BOUNDARY //
			tempS = plugin->mesh[i][0].s_junction;
			plugin->mesh[i][0].s_junction = -plugin->mesh[i][0].n_junction;
			plugin->mesh[i][1].s_junction = plugin->mesh[i][1].s_temp = tempS;
			tempN = plugin->mesh[i][WIDTH-1].n_junction;
			plugin->mesh[i][WIDTH-1].n_junction = -plugin->mesh[i][WIDTH-1].s_junction;
			plugin->mesh[i][WIDTH-2].n_junction = tempN;
			// 'i's in the neplugint few lines are really 'j's!
			tempE = plugin->mesh[0][i].e_junction;
			plugin->mesh[0][i].e_junction = -plugin->mesh[0][i].w_junction;
			plugin->mesh[1][i].e_junction = plugin->mesh[1][i].e_temp = tempE;
			tempW = plugin->mesh[WIDTH-1][i].w_junction;
			plugin->mesh[WIDTH-1][i].w_junction = -plugin->mesh[WIDTH-1][i].e_junction;
			plugin->mesh[WIDTH-2][i].w_junction = tempW;			
		}

		filt = LOSS*(plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction + oldfilt);
		oldfilt = plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction;
		plugin->mesh[LENGTH-LENGTH/4][WIDTH-WIDTH/4].v_junction = filt;

		out[k] = plugin->mesh[WIDTH/4][WIDTH/4-1].v_junction;
	       	last_trigger = trg;
	}
	plugin->last_trigger = last_trigger;

}

void
wgmesh_cleanup(LADSPA_Handle instance)
{
	free(instance);
}


LADSPA_Descriptor* wg_mesh_cr_desc = NULL;


/* Called automatically when the plugin library is first loaded. */
void
_init()
{
	char**                 port_names;
	LADSPA_PortDescriptor* port_descriptors;
	LADSPA_PortRangeHint*  port_range_hints;

	wg_mesh_cr_desc = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (wg_mesh_cr_desc) {

		wg_mesh_cr_desc->UniqueID = MESH_BASE_ID;
		wg_mesh_cr_desc->Label = strdup("wg_mesh_cr");
		wg_mesh_cr_desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		wg_mesh_cr_desc->Name = strdup("Simple waveguide mesh (CR Controls)");
		wg_mesh_cr_desc->Maker = strdup("Loki Davison");
		wg_mesh_cr_desc->Copyright = strdup("GPL");
		wg_mesh_cr_desc->PortCount = MESH_NUM_PORTS;
		port_descriptors = (LADSPA_PortDescriptor*)calloc(MESH_NUM_PORTS, sizeof(LADSPA_PortDescriptor));
		wg_mesh_cr_desc->PortDescriptors = (const LADSPA_PortDescriptor*)port_descriptors;
		port_descriptors[MESH_TENSION] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[MESH_POWER] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[MESH_INPUT1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_descriptors[MESH_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_descriptors[MESH_EX_X] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_descriptors[MESH_EX_Y] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names = (char**)calloc(MESH_NUM_PORTS, sizeof(char*));
		wg_mesh_cr_desc->PortNames = (const char**)port_names;
		port_names[MESH_TENSION] = strdup("Tension");
		port_names[MESH_POWER] = strdup("Power");
		port_names[MESH_INPUT1] = strdup("Trigger");
		port_names[MESH_OUTPUT] = strdup("Output");
		port_names[MESH_EX_X] = strdup("Excitation X");
		port_names[MESH_EX_Y] = strdup("Excitation Y");
		port_range_hints = ((LADSPA_PortRangeHint *)
		                    calloc(MESH_NUM_PORTS, sizeof(LADSPA_PortRangeHint)));
		wg_mesh_cr_desc->PortRangeHints = (const LADSPA_PortRangeHint*)port_range_hints;
		port_range_hints[MESH_TENSION].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE
| LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[MESH_TENSION].LowerBound = 0.0001f;
                port_range_hints[MESH_TENSION].UpperBound = 0.22f;
		port_range_hints[MESH_POWER].HintDescriptor = LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[MESH_POWER].LowerBound = 0.000f;
		port_range_hints[MESH_POWER].UpperBound = 20.000f;
		
		port_range_hints[MESH_EX_X].HintDescriptor = LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER;
		port_range_hints[MESH_EX_X].LowerBound = 0.950f; //1
		port_range_hints[MESH_EX_X].UpperBound = LENGTH-0.99;//length-1 ish

		port_range_hints[MESH_EX_Y].HintDescriptor = LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER;
		port_range_hints[MESH_EX_Y].LowerBound = 0.950f; //1
		port_range_hints[MESH_EX_Y].UpperBound = LENGTH-0.99;//length-1 ish
		
		port_range_hints[MESH_INPUT1].HintDescriptor = 0;
		port_range_hints[MESH_OUTPUT].HintDescriptor = 0;
		wg_mesh_cr_desc->instantiate = wgmesh_instantiate;
		wg_mesh_cr_desc->connect_port = wgmesh_connect_port;
		wg_mesh_cr_desc->activate = NULL;
		wg_mesh_cr_desc->run = wgmesh_run_cr;
		wg_mesh_cr_desc->run_adding = NULL;
		wg_mesh_cr_desc->set_run_adding_gain = NULL;
		wg_mesh_cr_desc->deactivate = NULL;
		wg_mesh_cr_desc->cleanup = wgmesh_cleanup;
	}
}


void
wgmesh_delete_descriptor(LADSPA_Descriptor* psDescriptor)
{
	unsigned long lIndex;
	if (psDescriptor) {
		free((char*)psDescriptor->Label);
		free((char*)psDescriptor->Name);
		free((char*)psDescriptor->Maker);
		free((char*)psDescriptor->Copyright);
		free((LADSPA_PortDescriptor *)psDescriptor->PortDescriptors);
		for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
			free((char*)(psDescriptor->PortNames[lIndex]));
		free((char**)psDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
		free(psDescriptor);
	}
}


/* Called automatically when the library is unloaded. */
void
_fini()
{
	wgmesh_delete_descriptor(wg_mesh_cr_desc);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor*
ladspa_descriptor(unsigned long Index)
{
	switch (Index) {
	case 0:
		return wg_mesh_cr_desc;
	default:
		return NULL;
	}
}

