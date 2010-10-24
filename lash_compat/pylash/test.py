#!/usr/bin/env python
#
# This file shows example usage of python bindings for LASH
# As such code here is public domain you can use it as you wish and in
# particular use this code as template for adding LASH support to your program.
#

import sys
import time
import lash

def lash_check_events(lash_client):
    event = lash.lash_get_event(lash_client)

    while event:
        print repr(event)

        event_type = lash.lash_event_get_type(event)
        if event_type == lash.LASH_Quit:
            print "LASH ordered quit."
            return False
        elif event_type == lash.LASH_Save_File:
            print "LASH ordered to save data in directory %s" % lash.lash_event_get_string(event)
            lash.lash_send_event(lash_client, event)
        elif event_type == lash.LASH_Save_Data_Set:
            print "LASH ordered to save data"
            lash.lash_send_event(lash_client, event)
        elif event_type == lash.LASH_Restore_Data_Set:
            print "LASH ordered to restore data"
            lash.lash_event_destroy(event)
        elif event_type == lash.LASH_Restore_File:
            print "LASH ordered to restore data from directory %s" % lash.lash_event_get_string(event)
            lash.lash_event_destroy(event)
        else:
            print "Got unhandled LASH event, type " + str(event_type)
            lash.lash_event_destroy(event)
            return True

        event = lash.lash_get_event(lash_client)

    return True

# sys.argv is modified by this call
lash_client = lash.init(sys.argv, "pylash test", lash.LASH_Config_Data_Set | lash.LASH_Terminal)
if not lash_client:
    print "Cannot connect to LASH server"
    sys.exit(1)

print "Successfully connected to LASH server at " +  lash.lash_get_server_name(lash_client)

# Send our client name to server
lash_event = lash.lash_event_new_with_type(lash.LASH_Client_Name)
lash.lash_event_set_string(lash_event, "pylash test")
lash.lash_send_event(lash_client, lash_event)

# loop until we receive quit order from LASH server
while lash_check_events(lash_client):
    time.sleep(1)
