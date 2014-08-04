#include <node.h>
#include <v8.h>

#include <stdio.h>
#include <iso646.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdarg.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

using namespace v8;
using namespace std;

typedef enum _name_kind {
    name_none = 0,
    name_string = (1 << 0),
    name_xid = (1 << 1),
    name_index = (1 << 2),
    name_preferred = (1 << 3),
} name_kind_t;

typedef struct {
    name_kind_t	    kind;
    char    	    *string;
    XID	    	    xid;
    int		    index;
} name_t;

static void
init_name (name_t *name)
{
    name->kind = name_none;
}

static void
set_name_xid (name_t *name, XID xid)
{
    name->kind |= name_xid;
    name->xid = xid;
}

static double
mode_refresh (XRRModeInfo *mode_info)
{
    double rate;
    unsigned int vTotal = mode_info->vTotal;

    if (mode_info->modeFlags & RR_DoubleScan) {
        /* doublescan doubles the number of lines */
        vTotal *= 2;
    }

    if (mode_info->modeFlags & RR_Interlace) {
        /* interlace splits the frame into two fields */
        /* the field rate is what is typically reported by monitors */
        vTotal /= 2;
    }

    if (mode_info->hTotal && vTotal)
        rate = ((double) mode_info->dotClock /
                ((double) mode_info->hTotal * (double) vTotal));
    else
        rate = 0;
    return rate;
}

static XRRModeInfo *
find_mode (name_t *name, double refresh, XRRScreenResources *res)
{
    int m;
    XRRModeInfo *best = NULL;
    double bestDist = 0;

    for (m = 0; m < res->nmode; m++)
    {
	XRRModeInfo *mode = &res->modes[m];
        if ((name->kind & name_xid) && name->xid == mode->id)
        {
            best = mode;
            break;
        }
        if ((name->kind & name_string) && !strcmp (name->string, mode->name))
        {
            double dist;

            if (refresh)
                dist = fabs (mode_refresh (mode) - refresh);
            else
                dist = 0;
            if (!best || dist < bestDist)
            {
                bestDist = dist;
                best = mode;
            }
      	}
    }
    return best;
}

static XRRModeInfo *
find_mode_by_xid (RRMode mode, XRRScreenResources *res)
{
    name_t  mode_name;

    init_name (&mode_name);
    set_name_xid (&mode_name, mode);
    return find_mode (&mode_name, 0, res);
}

Handle<Value> getDisplays(const Arguments& args) {
    HandleScope scope;

    Display *dpy;
    Window  root;
    XRRScreenResources *res;
    XRRScreenConfiguration *sc;

    v8::Handle<v8::Array> result = v8::Array::New();

    char *display_name = NULL;
    int screen = -1;

    dpy = XOpenDisplay (display_name);
    if (dpy == NULL) {
      fprintf (stderr, "Can't open display %s\n", XDisplayName(display_name));
      exit (1);
    }
    if (screen < 0)
      screen = DefaultScreen (dpy);
    if (screen >= ScreenCount (dpy)) {
      fprintf (stderr, "Invalid screen number %d (display has %d)\n",
      	       screen, ScreenCount (dpy));
      exit (1);
    }

    root = RootWindow (dpy, screen);

    // get current resolution
    //
    sc = XRRGetScreenInfo (dpy, root);
    Rotation  current_rotation;
    SizeID    current_size;
    XRRScreenSize *sizes;
    int nsize;
    current_size = XRRConfigCurrentConfiguration (sc, &current_rotation);
    sizes = XRRConfigSizes(sc, &nsize);

    v8::Handle<v8::Object> resolution_object = v8::Object::New();

    res = XRRGetScreenResourcesCurrent (dpy, root);
    for (int i=0; i<res->noutput; i++) {
        XRROutputInfo *output_info;

        output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
        v8::Handle<v8::Object> output = v8::Object::New();

        output->Set(String::New("name"),  String::New(output_info->name));
        switch (output_info->connection) {
            case RR_Connected:
                output->Set(String::New("status"), String::New("connected"));

                resolution_object->Set(String::New("width"),
                                      Integer::New(sizes[current_size].width));
                resolution_object->Set(String::New("height"),
                                      Integer::New(sizes[current_size].height));

                resolution_object->Set(String::New("mwidth"),
                                      Integer::New(sizes[current_size].mwidth));
                resolution_object->Set(String::New("mheight"),
                                      Integer::New(sizes[current_size].mheight));

                output->Set(String::New("resolution"), resolution_object);
                break;
            case RR_Disconnected:
                output->Set(String::New("status"), String::New("disconnected"));
                break;
        }

        v8::Handle<v8::Array> available_resolutions = v8::Array::New();
        for (int j=0; j<output_info->nmode; j++) {
            char *resolution;
            XRRModeInfo *mode = find_mode_by_xid (output_info->modes[j], res);
            asprintf(&resolution, "%dx%d", mode->width, mode->height);
            available_resolutions->Set(j,String::New(resolution));
        }
        output->Set(String::New("available_resolutions"),
                available_resolutions);

        result->Set(i,output);
    }

    return scope.Close(result);
}

Handle<Value> getBrightness(const Arguments& args) {
    HandleScope scope;
    Display *dpy;
    static Atom backlight;
    int screen = 0, o = 0;
    Window root;
    XRRScreenResources *resources;
    RROutput output;
    XRRPropertyInfo *info;
    double min, max;

    dpy = XOpenDisplay(NULL);
    backlight = XInternAtom (dpy, "Backlight", True);
    root = RootWindow(dpy, screen);
    resources = XRRGetScreenResources(dpy, root);
    output = resources->outputs[o];

    unsigned char *prop;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom actual_type;

    XRRGetOutputProperty (dpy, output, backlight,
                          0, 100, False, False,
                          AnyPropertyType,
                          &actual_type, &actual_format,
                          &nitems, &bytes_after, &prop);

    info = XRRQueryOutputProperty(dpy, output, backlight);

    const int32_t *val = (const int32_t *) prop;
    char backlight_value[11];
    snprintf( backlight_value, sizeof backlight_value, "%" PRId32, *val);

    min = info->values[0];
    max = info->values[1];

    v8::Handle<v8::Object> result = v8::Object::New();
    result->Set(String::New("value"), Int32::New(*val));
    result->Set(String::New("max"), Number::New(max));
    result->Set(String::New("min"), Number::New(min));

    return scope.Close(result);
}

Handle<Value> setBrightness(const Arguments& args) {
    HandleScope scope;
    Display *dpy;
    static Atom backlight;
    int screen = 0, o = 0;
    Window root;
    XRRScreenResources *resources;
    RROutput output;
    XRRPropertyInfo *info;
    double min, max;
    long value;
    float argValue;
    argValue = args[0]->ToNumber()->Value();

    dpy = XOpenDisplay(NULL);
    backlight = XInternAtom (dpy, "Backlight", True);
    root = RootWindow(dpy, screen);
    resources = XRRGetScreenResources(dpy, root);
    output = resources->outputs[o];
    info = XRRQueryOutputProperty(dpy, output, backlight);
    min = info->values[0];
    max = info->values[1];
    XFree(info); // Don't need this anymore
    XRRFreeScreenResources(resources); // or this

    value = argValue;

    if (value>max) {
        value = max;
    } else if (value<min) {
        value = min;
    }

    XRRChangeOutputProperty(dpy, output, backlight, XA_INTEGER,
                    32, PropModeReplace, (unsigned char *) &value, 1);
    XFlush(dpy);
    XSync(dpy, False);

    return scope.Close(Boolean::New(True));
}

Handle<Value> setScreenResolution(const Arguments& args) {
    HandleScope scope;
    Display *dpy;

    int width = args[0]->ToInteger()->Value();
    int height = args[1]->ToInteger()->Value();

    Rotation rotation = 1;
    int reflection = 0;

    char *displayname = NULL;

    dpy = XOpenDisplay (displayname);

    if (dpy == NULL) {
        printf ("Cannot open display %s\n", displayname);
        return scope.Close(Boolean::New(False));
    }

    int screen = DefaultScreen (dpy);
    Window root = RootWindow (dpy, screen);

    XSelectInput (dpy, root, StructureNotifyMask);
    XRRSelectInput (dpy, root, RRScreenChangeNotifyMask);
    int eventbase;
    int errorbase;
    XRRQueryExtension(dpy, &eventbase, &errorbase);

    XRRScreenConfiguration *sc = XRRGetScreenInfo (dpy, root);
    if (sc == NULL) {
        printf ("Cannot get screen info\n");
        return scope.Close(Boolean::New(False));
    }

    int nsize;
    XRRScreenSize *sizes = XRRConfigSizes(sc, &nsize);
    int sizeindex = 0;

    while (sizeindex < nsize) {
        if (sizes[sizeindex].width == width &&
        sizes[sizeindex].height == height)
            break;
        sizeindex++;
    }

    if (sizeindex >= nsize) {
        printf ("%dx%d resolution not available\n", width, height);
        return scope.Close(Boolean::New(False));
    }

    Status status = XRRSetScreenConfig (dpy, sc,
                                        DefaultRootWindow (dpy),
                                        (SizeID) sizeindex,
                                        (Rotation) (rotation | reflection),
                                        CurrentTime);

    XEvent event;
    bool rcvdrrnotify = false;
    bool rcvdconfignotify = false;

    if (status == RRSetConfigSuccess) {
        while (1) {
            XNextEvent(dpy, (XEvent *) &event);
            //printf ("Event received, type = %d\n", event.type);
            XRRUpdateConfiguration (&event) ;
            switch (event.type - eventbase) {
                case RRScreenChangeNotify:
                    rcvdrrnotify = true;
                    break;
                default:
                    if (event.type == ConfigureNotify) {
                        //printf("Rcvd ConfigureNotify Event!\n");
                        rcvdconfignotify = true;
                    } else {
                        //printf("unknown event = %d!\n", event.type);
                    }
                    break;
            }

            if (rcvdrrnotify && rcvdrrnotify) {
                break;
            }
        }
    }

    return scope.Close(Boolean::New(True));
}

void init(Handle<Object> target) {
    target->Set(String::NewSymbol("getDisplays"),
              FunctionTemplate::New(getDisplays)->GetFunction());
    target->Set(String::NewSymbol("getBrightness"),
              FunctionTemplate::New(getBrightness)->GetFunction());
    target->Set(String::NewSymbol("setBrightness"),
              FunctionTemplate::New(setBrightness)->GetFunction());
    target->Set(String::NewSymbol("setScreenResolution"),
              FunctionTemplate::New(setScreenResolution)->GetFunction());
}
NODE_MODULE(nodexrandr, init)