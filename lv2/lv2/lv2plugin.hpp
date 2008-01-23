/****************************************************************************
    
    lv2plugin.hpp - support file for writing LV2 plugins in C++
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
        Modified by Dave Robillard, 2008
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef LV2PLUGIN_HPP
#define LV2PLUGIN_HPP

#include <unistd.h>
#include <stdarg.h>

#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <lv2.h>
#include <lv2/lv2types.hpp>
#include <lv2/uri_map/lv2_uri_map.h>


namespace LV2 {
  

  /** @internal
      A thin wrapper around std::vector<LV2_Descriptor> that frees the URI
      members of the descriptors. */
  class DescList : public std::vector<LV2_Descriptor> {
  public:
    ~DescList();
  };


  /** @internal
      This returns a list of all registered plugins. It is only used 
      internally. */
  DescList& get_lv2_descriptors();

  
  /** This is a template base class for LV2 plugins. It has default 
      implementations for all functions, so you only have to implement the 
      functions that you need (for example run()). All subclasses must have 
      a constructor whose signature matches the one in the example code below, 
      otherwise it will not work with the template class LV2::Register. The 
      host will use these parameter to pass the sample rate, the path to the 
      bundle directory and the list of features passed from the host when it 
      creates a new instance of the plugin. 
      
      This is a template so that simulated dynamic binding can be used for
      the callbacks. This is not all that useful for simple plugins but it may
      come in handy for extensions and it doesn't add any additional vtable
      lookup and function call costs, like real dynamic binding would.
      @code
      #include <lv2plugin.hpp>
      
      class TestLV2 : public LV2::Plugin<TestLV2> {
      public:
        TestLV2(double, const char*, const LV2::Feature* const*) : LV2::Plugin<TestLV2>(2) { }
        void run(uint32_t sample_count) {
          memcpy(p(1), p(0), sample_count * sizeof(float));
        }
      };
      
      static unsigned _ = TestLV2::register_class<TestLV2>("http://ll-plugins.sf.net/plugins/TestLV2#0.0.0");
      @endcode
      
      If the above code is compiled and linked with @c -llv2_plugin into a 
      shared module, it could form the shared object part of a fully 
      functional (but not very useful) LV2 plugin with one audio input port
      and one audio output port that just copies the input to the output.
  */
  template <class Derived, 
            class Ext1 = End, class Ext2 = End, class Ext3 = End,
            class Ext4 = End, class Ext5 = End, class Ext6 = End, 
            class Ext7 = End, class Ext8 = End, class Ext9 = End>
  class Plugin : public MixinTree<Derived, 
                                  Ext1, Ext2, Ext3, Ext4, Ext5, 
                                  Ext6, Ext7, Ext8, Ext9> {
  public:
    
    /** This constructor is needed to initialise the port vector with the
        correct number of ports, and to check if all the required features
        are provided. */
    Plugin(uint32_t ports) 
      : m_ports(ports, 0) {
      m_features = s_features;
      m_bundle_path = s_bundle_path;
      s_features = 0;
      s_bundle_path = 0;
      if (m_features) {
        FeatureHandlerMap hmap;
        Derived::map_feature_handlers(hmap);
        for (const Feature* const* iter = m_features; *iter != 0; ++iter) {
          FeatureHandlerMap::iterator miter;
          miter = hmap.find((*iter)->URI);
          if (miter != hmap.end())
            miter->second(static_cast<Derived*>(this), (*iter)->data);
        }
      }
    }
    
    /** Connects the ports. You shouldn't have to override this, just use
        p(port) to access the port buffers. 
    
        If you do override this function, remember that if you want your plugin
        to be realtime safe this function may not block, allocate memory or
        otherwise take a long time to return. */
    void connect_port(uint32_t port, void* data_location) {
      m_ports[port] = data_location;
    }
    
    /** Override this function if you need to do anything on activation. 
        This is always called before the host starts using the run() function.
        You should reset your plugin to it's initial state here. */
    void activate() { }
  
    /** This is the process callback which should fill all output port buffers. 
        You most likely want to override it. 
        
        Remember that if you want your plugin to be realtime safe, this function
        may not block, allocate memory or take more than O(sample_count) time
        to execute. */
    void run(uint32_t sample_count) { }
  
    /** Override this function if you need to do anything on deactivation. 
        The host calls this when it does not plan to make any more calls to 
        run() (unless it calls activate() again). */
    void deactivate() { }
    
    /** Use this function to register your plugin class so that the host
        can find it. You need to do this when the shared library is loaded 
        by the host. One way of doing that is to put the function call in 
        the initialiser for a global variable, like this:
        
        @code
unsigned _ =  MyPluginClass::register_class("http://my.plugin.class");
        @endcode
    */
	static unsigned register_class(const std::string& uri) {
		LV2_Descriptor desc;
		std::memset(&desc, 0, sizeof(LV2_Descriptor));
		char* c_uri = new char[uri.size() + 1];
		std::memcpy(c_uri, uri.c_str(), uri.size() + 1);
		desc.URI = c_uri;
		desc.instantiate = &Derived::_create_plugin_instance;
		desc.connect_port = &Derived::_connect_port;
		desc.activate = &Derived::_activate;
		desc.run = &Derived::_run;
		desc.deactivate = &Derived::_deactivate;
		desc.cleanup = &Derived::_delete_plugin_instance;
		desc.extension_data = &Derived::extension_data;
		get_lv2_descriptors().push_back(desc);
		return get_lv2_descriptors().size() - 1;
	}

  protected:
  
    /** Use this function to access and cast port buffers, for example
        like this:
    
        @code
LV2_MIDI* midibuffer = p<LV2_MIDI>(midiport_index);
        @endcode
        
        If you want to access a port buffer as a pointer-to-float (i.e. an audio
        or control port) you can use the non-template version instead. */
    template <typename T> T*& p(uint32_t port) {
      return reinterpret_cast<T*&>(m_ports[port]);
    }
  
    /** Use this function to access data buffers for control or audio ports. */
    float*& p(uint32_t port) {
      return reinterpret_cast<float*&>(m_ports[port]);
    }
    
    /** Returns the filesystem path to the bundle that contains this plugin. */
    const char* bundle_path() const {
      return m_bundle_path;
    }
    
    /** @internal
        This vector contains pointers to all port buffers. You don't need to
        access it directly, use the p() function instead. */
    std::vector<void*> m_ports;

  private:

    static void _connect_port(LV2_Handle instance, uint32_t port, 
                             void* data_location) {
      reinterpret_cast<Derived*>(instance)->connect_port(port, data_location);
    }
  
    static void _activate(LV2_Handle instance) {
      reinterpret_cast<Derived*>(instance)->activate();
    }
  
    static void _run(LV2_Handle instance, uint32_t sample_count) {
      reinterpret_cast<Derived*>(instance)->run(sample_count);
    }
  
    static void _deactivate(LV2_Handle instance) {
      reinterpret_cast<Derived*>(instance)->deactivate();
    }
  
    /** @internal
        This function creates an instance of a plugin. It is used as
        the instantiate() callback in the LV2 descriptor. You should not use
        it directly. */
    static LV2_Handle _create_plugin_instance(const LV2_Descriptor* descriptor,
                                              double sample_rate,
                                              const char* bundle_path,
                                              const LV2_Feature* const* 
                                              features) {

      // copy some data to static variables so the subclasses don't have to
      // bother with it
      s_features = features;
      s_bundle_path = bundle_path;

      Derived* t = new Derived(sample_rate);
      if (t->check_ok())
        return reinterpret_cast<LV2_Handle>(t);
      delete t;
      return 0;
    }
            
    /** @internal
        This function destroys an instance of a plugin. It is used as the
        cleanup() callback in the LV2 descriptor. You should not use it
        directly. */
    static void _delete_plugin_instance(LV2_Handle instance) {
      delete reinterpret_cast<Derived*>(instance);
    }


  private:

    LV2::Feature const* const* m_features;
    char const* m_bundle_path;
    
    static LV2::Feature const* const* s_features;
    static char const* s_bundle_path;

  };


  template<class Derived, class Ext1, class Ext2, class Ext3, class Ext4,
           class Ext5, class Ext6, class Ext7, class Ext8, class Ext9>
  LV2::Feature const* const* 
  Plugin<Derived, Ext1, Ext2, Ext3, Ext4, 
         Ext5, Ext6, Ext7, Ext8, Ext9>::s_features = 0;
  
  template<class Derived, class Ext1, class Ext2, class Ext3, class Ext4,
           class Ext5, class Ext6, class Ext7, class Ext8, class Ext9>
  char const* 
  Plugin<Derived, Ext1, Ext2, Ext3, Ext4, 
         Ext5, Ext6, Ext7, Ext8, Ext9>::s_bundle_path = 0;

  /** The fixed buffer size extension. A host that supports this will always
      call the plugin's run() function with the same @c sample_count parameter,
      which will be equal to the uint32_t variable pointed to by the data
      pointer for this feature. */
  template <bool Required>
  struct FixedExt {
    
    template <class Derived> struct I : Extension<Required> {
      
      I() : m_buffer_size(0) { }
      
      static void map_feature_handlers(FeatureHandlerMap& hmap) {
        hmap["http://tapas.affenbande.org/lv2/ext/fixed-buffersize"] = 
          &I<Derived>::handle_feature;
      }
      
      static void handle_feature(void* instance, void* data) { 
        Derived* d = reinterpret_cast<Derived*>(instance);
        I<Derived>* fe = static_cast<I<Derived>*>(d);
        fe->m_buffer_size = *reinterpret_cast<uint32_t*>(data);
        fe->m_ok = true;
      }
      
    protected:
      
      /** This returns the buffer size that the host has promised to use.
          If the host does not support this extension this function will
          return 0. */
      uint32_t get_buffer_size() const { return m_buffer_size; }
      
      uint32_t m_buffer_size;
      
    };
    
  };
  
  /** The fixed power-of-2 buffer size extension. This works just like 
      FixedExt with the additional requirement that the buffer size must
      be a power of 2. */
  template <bool Required>
  struct FixedP2Ext {
    
    template <class Derived> struct I : Extension<Required> {
      
      I() : m_buffer_size(0) { }
      
      static void map_feature_handlers(FeatureHandlerMap& hmap) {
        hmap["http://tapas.affenbande.org/lv2/ext/power-of-two-buffersize"] = 
          &I<Derived>::handle_feature;
      }
      
      static void handle_feature(void* instance, void* data) { 
        Derived* d = reinterpret_cast<Derived*>(instance);
        I<Derived>* fe = static_cast<I<Derived>*>(d);
        fe->m_buffer_size = *reinterpret_cast<uint32_t*>(data);
        fe->m_ok = true;
      }
      
    protected:
      
      /** This returns the buffer size that the host has promised to use.
          If the host does not support this extension this function will
          return 0. */
      uint32_t get_buffer_size() const { return m_buffer_size; }
      
      uint32_t m_buffer_size;
      
    };
  };
  
	
	/** The URI map extension. */
  template <bool Required>
  struct UriMapExt {
    
    template <class Derived> struct I : Extension<Required> {
      
      I() : m_uri_map_feature(0) { }
      
	  static void map_feature_handlers(FeatureHandlerMap& hmap) {
		  hmap[LV2_URI_MAP_URI] = &I<Derived>::handle_feature;
	  }
      
	  static void handle_feature(void* instance, void* data) { 
		  Derived* d = reinterpret_cast<Derived*>(instance);
		  I<Derived>* fe = static_cast<I<Derived>*>(d);
		  fe->m_uri_map_feature = reinterpret_cast<LV2_URI_Map_Feature*>(data);
		  fe->m_ok = true;
	  }
      
    protected:
      
      /** This returns the buffer size that the host has promised to use.
          If the host does not support this extension this function will
          return 0. */
      uint32_t uri_to_id(const char* map, const char* uri) const {
		  return m_uri_map_feature->uri_to_id(
				  m_uri_map_feature->callback_data, map, uri);
	  }
      
      LV2_URI_Map_Feature* m_uri_map_feature;
      
    };
  
  };
 
  
}


#endif
