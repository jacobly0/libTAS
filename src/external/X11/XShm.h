/************************************************************

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

********************************************************/

/* THIS IS NOT AN X CONSORTIUM STANDARD OR AN X PROJECT TEAM SPECIFICATION */

#ifndef _XSHM_H_
#define _XSHM_H_

// #include <X11/Xfuncproto.h>
// #include <X11/extensions/shm.h>
#include <X11/Xlib.h>

#ifndef _XSHM_SERVER_
typedef unsigned long ShmSeg;

typedef struct {
    int	type;		    /* of event */
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event;	    /* true if this came frome a SendEvent request */
    Display *display;	    /* Display the event was read from */
    Drawable drawable;	    /* drawable of request */
    int major_code;	    /* ShmReqCode */
    int minor_code;	    /* X_ShmPutImage */
    ShmSeg shmseg;	    /* the ShmSeg used in the request */
    unsigned long offset;   /* the offset into ShmSeg used in the request */
} XShmCompletionEvent;

typedef struct {
    ShmSeg shmseg;	/* resource id */
    int shmid;		/* kernel id */
    char *shmaddr;	/* address in client */
    Bool readOnly;	/* how the server should attach it */
} XShmSegmentInfo;

#endif /* _XSHM_SERVER_ */

#endif
