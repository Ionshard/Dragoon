/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_private.h"

/* Camera motion speed in pixels per second */
#define CAMERA_SPEED 10

/* Camera shake params */
#define SHAKE_DRAG 5

/* Screen mode parameters */
int r_width = 1024, r_height = 768, r_widthScaled, r_heightScaled, r_scale,
    r_initFrame;
bool r_clear, r_fullscreen;

/* Keep track of the number of faces rendered per frame */
CCount r_countFaces, r_countLines;

/* World-space camera */
CVec r_camera, r_cameraTo;
float r_cameraSec, r_cameraShake;
bool r_cameraOn;

/* Screenshots */
static char screenshot[256];

/******************************************************************************\
 See if there were any OpenGL errors.
\******************************************************************************/
#if CHECKED
void R_checkErrors_full(const char *func)
{
        int error;

        if ((error = glGetError()) == GL_NO_ERROR)
                return;
        C_log(CLL_ERROR, func, "OpenGL error %d: %s",
              error, gluErrorString(error));
}
#endif

/******************************************************************************\
 Sets the video mode. SDL creates a window in windowed mode.
\******************************************************************************/
bool R_setVideoMode(void)
{
        const SDL_Surface *video;
        int flags;

        /* Ensure a minimum render size or pre-rendering will crash */
        if (r_width < R_WIDTH)
                r_width = R_WIDTH;
        if (r_height < R_HEIGHT)
                r_height = R_HEIGHT;

        /* Reset textures for resolution change */
        R_resetTextures();

        /* Create a new window */
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
        flags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_ANYFORMAT | SDL_RESIZABLE;
        if (r_fullscreen)
                flags |= SDL_FULLSCREEN;
        if (!(video = SDL_SetVideoMode(r_width, r_height, 0, flags))) {
                C_warning("Failed to set video mode: %s", SDL_GetError());
                return FALSE;
        }

        /* Get the actual screen size */
        r_scale = video->h / R_HEIGHT;
        r_heightScaled = video->h / r_scale;
        r_heightScaled += (video->h - r_scale * r_heightScaled) / r_scale;
        r_widthScaled = video->w * r_heightScaled / video->h;
        C_debug("Set mode %dx%d (%dx%d scaled), scale factor %d",
                video->w, video->h, r_widthScaled, r_heightScaled, r_scale);

        /* Update reset frame so we reinitialize textures as necessary */
        r_initFrame = c_frame;

        R_checkErrors();
        return TRUE;
}

/******************************************************************************\
 Initialize OpenGL for 2D rendering.
\******************************************************************************/
void R_initGl(void)
{
        /* Screen viewport */
        glViewport(0, 0, r_width, r_height);

        /* Orthogonal projection */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.f, r_widthScaled, r_heightScaled, 0.f, 0.f, -1.f);

        /* Identity model matrix */
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Enable alpha-blending by default */
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /* Minimal alpha testing */
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 1 / 255.f);

        /* Background clear color */
        glClearColor(1.0f, 0.0f, 1.0f, 1.f);

        /* No texture by default */
        glDisable(GL_TEXTURE_2D);

        /* We use lines to do 2D edge anti-aliasing although there is probably
           a better way so we need to always smooth lines (requires alpha
           blending to be on to work) */
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        /* Sprites are depth-tested */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        R_checkErrors();
}

/******************************************************************************\
 Mark this frame for saving a screenshot when the buffer flips. Returns the
 full path and filename of the image saved or NULL if there was a problem.
\******************************************************************************/
const char *R_screenshot(void)
{
        time_t msec;
        struct tm *local;
        int i;
        const char *filename;
        char dir[256];

        /* Make sure the directory exists */
        snprintf(dir, sizeof (dir), "%s/screenshots", C_userDir());
        if (!C_mkdir(dir)) {
                C_warning("Failed to create screenshot directory '%s'", dir);
                return NULL;
        }

        /* Can't take two screenshots per frame */
        if (screenshot[0]) {
                C_warning("Can't save screenshot, '%s' queued", screenshot);
                return NULL;
        }

        /* Start off with a path based on the current date and time */
        time(&msec);
        local = localtime(&msec);
        filename = C_va("%s/%d-%02d-%02d--%02d%02d.png",
                        dir, local->tm_year + 1900,
                        local->tm_mon + 1, local->tm_mday, local->tm_hour,
                        local->tm_min);

        /* If this is taken, start adding letters to the end of it */
        for (i = 0; C_fileExists(filename) && i < 26; i++)
                filename = C_va("%s/%d-%02d-%02d--%02d%02d%c.png",
                                dir, local->tm_year + 1900,
                                local->tm_mon + 1, local->tm_mday,
                                local->tm_hour, local->tm_min, 'a' + i);

        C_strncpy_buf(screenshot, filename);
        return filename;
}

/******************************************************************************\
 Clears the buffer and starts rendering the scene.
\******************************************************************************/
void R_begin(void)
{
        int clearFlags;

        clearFlags = GL_DEPTH_BUFFER_BIT;
        if (r_clear)
                clearFlags |= GL_COLOR_BUFFER_BIT;
        glClear(clearFlags);
}

/******************************************************************************\
 Finishes rendering the scene and flips the buffer.
\******************************************************************************/
void R_end(void)
{
        /* Before flipping the buffer, save any pending screenshots */
        if (screenshot[0]) {
                SDL_Surface *surf;

                C_debug("Saving screenshot '%s'", screenshot);
                surf = R_screenSurface(0, 0, r_width, r_height);
                R_saveSurface(surf, screenshot);
                R_freeSurface(surf);
                screenshot[0] = NUL;
        }

        SDL_GL_SwapBuffers();
        R_checkErrors();
}

/******************************************************************************\
 Begin rendering in world-space coordinates.
\******************************************************************************/
void R_beginCam(void)
{
        float prop;

        /* Move the current camera view toward the target view */
        if ((prop = CAMERA_SPEED * r_cameraSec) > 1)
                prop = 1;
        r_camera = CVec_lerp(r_camera, prop, r_cameraTo);

        /* Camera shakes */
        if (r_cameraShake)
                r_camera = CVec_add(r_camera, C_shake(&r_cameraShake,
                                                      SHAKE_DRAG, r_cameraSec));

        C_assert(!r_cameraOn);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(-r_camera.x, -r_camera.y, 0);
        r_cameraOn = TRUE;
}

/******************************************************************************\
 Stop rendering in world-space coordinates.
\******************************************************************************/
void R_endCam(void)
{
        C_assert(r_cameraOn);
        glPopMatrix();
        R_checkErrors();
        r_cameraOn = FALSE;
}

