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
 * 'objects' of any type (defined by a URI).  This is useful for large/shared
 * data structures (like images or waveforms) and for non-POD event payloads
 * (allowing plugins to process events too large/expensive to copy).
 *
 * This extension requires the host to support the LV2 URI Map extension.
 */


/** Function to free an LV2 object (destructor).
 *
 * Any creator of an object must provide a pointer to this function to the
 * host so the object can be freed.  This function must 'deinitialize' the
 * contents of the object, NOT free the LV2_Object struct itself.
 * For POD objects this function does not need to be provided, as freeing
 * the LV2_Object struct will completely free the object. */
typedef void (*LV2_Object_Destructor)(LV2_Object* object);


/** The data field of the LV2_Feature for the LV2 Object extension.
 *
 * A host which supports this extension must pass an LV2_Feature struct to the
 * plugin's instantiate method with 'URI' "http://lv2plug.in/ns/ext/object" and
 * 'data' pointing to an instance of this struct. */
typedef struct {
	
	/** Create (allocate) a new LV2 Object.
	 *
	 * The returned value has size bytes of memory immediately following the
	 * LV2_Object header, which the caller must initialize appropriately.
	 *
	 * @param callback_data Must be the callback_data member of this struct.
	 * @param destructor Function to clean up after any initialisation which
	 *        is done after this call.  This function is not responsible for
	 *        freeing the returned LV2_Object struct itself; NULL may be passed
	 *        if the object does not require any destruction (e.g. POD objects).
	 * @param context The calling context, mapped to a URI (see lv2_context.h)
	 * @param type Type of the new object, mapped to a URI as in LV2_Object.
	 * @param size Size in bytes of the new object, not including header. */
	LV2_Object* (*lv2_object_new)(LV2_Object_Callback_Data callback_data,
	                              LV2_Object_Destructor    destructor,
	                              uint32_t                 context,
	                              uint32_t                 type,
	                              uint32_t                 size);
	
	/** Reference an LV2 Object.
	 *
	 * This function can be called to increment the reference count of a
	 * object (preventing destruction while a reference is held).
	 * 
	 * Note that when an object is the payload of an event sent to a plugin,
	 * the plugin implicitly already has a reference to the object so this
	 h* function is not required for typical event processing situations.
	 *
	 * @param callback_data Must be the callback_data member of this struct.
	 * @param context The calling context, mapped to a URI (see lv2_context.h)
	 * @param object The object to obtain a reference to. */
	LV2_Object* (*lv2_object_ref)(LV2_Object_Callback_Data callback_data,
	                                uint32_t                 context,
	                                LV2_Object*              object);
	
	
	/** Unreference an LV2 Object.
	 *
	 * Plugins must call this function whenever they drop a reference to an
	 * object.  When objects are the payload of an event sent to a plugin,
	 * the plugin implicitly has a reference to the object and must call this
	 * function if the event is not stored somehow or passed to an output.
	 *
	 * @param callback_data Must be the callback_data member of this struct.
	 * @param context The calling context, mapped to a URI (see lv2_context.h)
	 * @param object The object to drop a reference to. */
	LV2_Object* (*lv2_object_unref)(LV2_Object_Callback_Data callback_data,
	                                uint32_t                 context,
	                                LV2_Object*              object);

	/** Opaque pointer to host data.
	 *
	 * The plugin MUST pass this to any call to functions in this struct.
	 * Otherwise, it must not be interpreted in any way.
	 */
	LV2_Object_Callback_Data callback_data;

} LV2_Object_Feature;



/** An LV2 object.
 *
 * LV2 objects are (like ports and events) generic 'blobs' of any type.
 * The type field defines the format of a given object's contents.
 *
 * This struct defines the header of an LV2 object.  Immediately following
 * the header in memory is the contents of the object (though the object
 * itself may not be POD, i.e. objects may contain pointers). */
typedef struct {
	
	/** Opaque host data associated with this object.
	 *
	 * Plugins must not interpret this data in any way.  Hosts may store
	 * whatever information they need to associate with object instances
	 * here, via the lv2_object_new method (which is the only way for a
	 * plugin to allocate a new LV2 Object. */
	LV2_Object_Host_Data host_data;
	
	/** Destructor function for this object.
	 */
	LV2_Object_Destructor destructor;

	/** The type of this object.  This number represents a URI, mapped to an
	 * integer by from some call to the uri_to_id function (defined in the
	 * LV2 URI Map extension), with the URI of this extension as the 'map'
	 * argument.  0 is never the type of a valid object, if this field is 0
	 * any reader must ignore the object's contents entirely. */
	uint32_t type;

	/** The size of this object, not including this header. */
	uint32_t size;

	/* size bytes of data follow here */

} LV2_Object;


#endif // LV2_OBJECT_H

