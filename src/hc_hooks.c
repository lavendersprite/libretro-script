#include "hc_hooks.h"
#include "core.h"
#include "hashmap.h"

#include <hcdebug.h>

typedef struct breakpoint_entry
{
    retro_script_hc_breakpoint_userdata userdata;
    retro_script_breakpoint_cb cb;
} breakpoint_entry;

static struct retro_script_hashmap* breakpoint_hashmap = NULL;

ON_INIT()
{
    if (breakpoint_hashmap) retro_script_hashmap_destroy(breakpoint_hashmap);
    breakpoint_hashmap = retro_script_hashmap_create(sizeof(breakpoint_entry));
}

// clear all scripts when a core is unloaded
ON_DEINIT()
{
    if (breakpoint_hashmap) retro_script_hashmap_destroy(breakpoint_hashmap);
}

static void on_breakpoint(void* ud, hc_SubscriptionID id, hc_Event const* event)
{
    // check if this is one of ours...
    breakpoint_entry* entry = (breakpoint_entry*)(
        retro_script_hashmap_get(breakpoint_hashmap, id)
    );
    
    if (entry)
    {
        entry->cb(entry->userdata, id, event);
    }
    
    // otherwise, forward breakpoint callback to frontend
    else if (frontend_callbacks.breakpoint_cb)
    {
        frontend_callbacks.breakpoint_cb(ud, id, event);
    }
}

int retro_script_hc_register_breakpoint(retro_script_hc_breakpoint_userdata const* userdata, hc_SubscriptionID breakpoint_id, retro_script_breakpoint_cb cb)
{
    if (!cb) return 1;
    if (!retro_script_hc_get_debugger()) return 1;
    
    breakpoint_entry* entry = (breakpoint_entry*)(
        retro_script_hashmap_add(breakpoint_hashmap, breakpoint_id)
    );
    
    if (!entry) return 1;
    memcpy(&entry->userdata, userdata, sizeof(*userdata));
    entry->cb = cb;
    return 0;
}

int retro_script_hc_unregister_breakpoint(hc_SubscriptionID breakpoint_id)
{
    if (!retro_script_hc_get_debugger()) return 1;
    return !retro_script_hashmap_remove(breakpoint_hashmap, breakpoint_id);
}

static void init_debugger(hc_DebuggerIf* debugger)
{
    *(unsigned*)&debugger->frontend_api_version = HC_API_VERSION;
    *(breakpoint_cb_t*)&debugger->v1.handle_event = on_breakpoint;
}

hc_DebuggerIf* retro_script_hc_get_debugger()
{
    // obtain debugger if we don't have it yet
    if (!core.hc.debugger_is_init)
    {
        if (!core.hc.set_debugger)
        {
            if (core.retro_get_proc_address)
            {
                core.hc.set_debugger = (hc_Set)core.retro_get_proc_address("hc_set_debugger");
                if (!core.hc.set_debugger)
                {
                    // backup -- try with 3 gs, this is api v1
                    core.hc.set_debugger = (hc_Set)core.retro_get_proc_address("hc_set_debuggger");
                }
                if (!core.hc.set_debugger)
                {
                    // no debugger available
                    return NULL;
                }
            }
            else
            {
                // no debugger available.
                return NULL;
            }
        }
        
        // initialize debugger and also debug hashmap
        core.hc.debugger_is_init = true;
        init_debugger(&core.hc.debugger);
        core.hc.userdata = NULL;
        core.hc.set_debugger(&core.hc.debugger);
    }
    
    return &core.hc.debugger;
}
