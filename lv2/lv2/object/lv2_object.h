/* lv2_object.h - C header file for the LV2 Object extension.
 * 
 * Copyright (C) 2008 Dave Robillard <dave@drobilla.net>
 * 
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

#ifndef LV2_OBJECT_H
#define LV2_OBJECT_H

#define LV2_OBJECT_URI "http://lv2plug.in/ns/ext/object"

#include <stdint.h>

/** @file
 * This header defines the code portion of the LV2 Object extension with URI
 * <http://lv2plug.in/ns/ext/object> (preferred prefix 'lv2obj').
 *
 * This extension allows hosts and plugins to manage dynamically allocated
 * 'objects' of any type (defined by a URI, naturally).  This is useful for
 * large/shared data structures (like images or waveforms) and for non-POD
 * event payloads (allowing the use of events much too large to copy).
 *
 * This extension requires the host to support the LV2 URI Map extension and
 * the LV2 Context extension.
 */


/** Function to free an LV2 object (destructor).
 *
 * Any creator of an object must provide a pointer to this function to the
 * host so the object can be freed.  This function must 'deinitialize' the
 * contents of the object, NOT free the LV2_Object struct itself.
 * For POD objects this function does not need to be provided, as freeing
 * the LV2_Object struct will completely free the object.
 */
typedef void (*LV2_Object_Destructor)(LV2_Object* object);


/** The data field of the LV2_Feature for the LV2 Object extension.
 *
 * The host MUST pass an LV2_Feature struct to the plugin's instantiate
 * method with URI set to "http://lv2plug.in/ns/ext/object" and data
 * set to an instance of this struct.
 *
 * The plugin MUST pass callback_data to any invocation of the functions
 * defined in this struct.  Otherwise, callback_data is opaque and the
 * plugin MUST NOT interpret its value in any way.
 */
typedef struct {
	
	/** Destroy an object.
	 *
	 * This function is provided by the creator of an object to allow the host
	 * to destroy the object when the last reference to it is dropped.
	 *
	 * @param context The calling context, mapped to a URI as in LV2_Object.
	 *        NOT necessarily equal to the context member of object.
	 * @param object The object to be destroyed.
	 */
	LV2_Object* (*lv2_object_delete)(LV2_Object_Callback_Data callback_data,
	                                 uint32_t                 context,
	                                 LV2_Object*              object);


	/** Create (allocate) a new object.
	 *
	 * The returned value has size bytes of memory immediately following the
	 * LV2_Object header, which the caller must initialize appropriately.
	 *
	 * @param destructor Function to clean up after any initialisation which
	 *        is done after this call.  This function is not responsible to
	 *        free the returned LV2_Object struct itself; NULL may be passed
	 *        if the object does not require any destruction (POD objects).
	 */
	LV2_Object* (*lv2_object_new)(LV2_Object_Callback_Data callback_data,
	                              LV2_Object_Destructor    destructor,
	                              uint32_t                 context,
	                              uint32_t                 type,
	                              uint32_t                 size);

	LV2_Object_Callback_Data callback_data;

} LV2_Object_Feature;



/** An LV2 object.
 *
 * LV2 objects are (like ports and events) generic 'blobs' of any type.
 * The type field defines the format of a given object's contents.
 *
 * This struct defines the header of an LV2 object.  Immediately following
 * the header in memory is the contents of the object (though the object
 * itself may not be POD, i.e. objects can contain pointers).
 */
typedef struct {
	
	/** Destructor function for this object.
	 */
	LV2_Object_Destructor destructor;

	/** The context this object was allocated in.  This number represents a
	 * URI, mapped to an integer by some call to the uri_to_id function
	 * (defined in the LV2 URI Map extension), with the URI of the LV2 context
	 * extension (<http://lv2plug.in/ns/ext/context>) as the 'map' argument.
	 */
	uint32_t context;

	/** The type of this object.  This number represents a URI, mapped to an
	 * integer by from some call to the uri_to_id function (defined in the
	 * LV2 URI Map extension), with the URI of this extension as the 'map'
	 * argument.
	 */
	uint32_t type;

	/** The size of this object, not including this header.
	 */
	uint32_t size;

	/* size bytes of data follow here */

} LV2_Object;


#endif // LV2_OBJECT_H

