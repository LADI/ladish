/* lv2_data_access.h - C header file for the LV2 Data Access extension.
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

#ifndef LV2_POLYMORPHIC_PORT_H
#define LV2_POLYMORPHIC_PORT_H

#define LV2_POLYMORPHIC_PORT_URI "http://lv2plug.in/ns/ext/polymorphic-port"


/** @file
 * This header defines the LV2 Polymorphic Port extension with the URI
 * <http://lv2plug.in/ns/ext/polymorphic-port>.
 *
 * This extension defines a buffer format for ports that can take on
 * various types dynamically at runtime.
 */
	

/** The data field of the LV2_Feature for this extension.
 *
 * To support this feature the host must pass an LV2_Feature struct to the
 * instantiate method with URI "http://lv2plug.in/ns/ext/data-access"
 * and data pointed to an instance of this struct.
 */
typedef struct {
	
	/** A URI mapped to an integer with the LV2 URI map extension
	 * <http://lv2plug.in/ns/ext/uri-map> which described the type of
	 * data pointed to by buf.
	 */
	uint32_t type;

	/** Pointer to the actual port contents.
	 */
	void* buf;

} LV2_Polymorphic_Buffer;


#endif // LV2_POLYMORPHIC_PORT_H

