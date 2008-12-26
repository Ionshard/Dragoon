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

/* Screen mode parameters */
int r_width = 1024, r_height = 768, r_widthScaled, r_heightScaled, r_scale;
bool r_clear, r_fullscreen;

/* Keep track of the number of faces rendered per frame */
CCount r_countFaces, r_countLines;

/* World-space camera */
CVec r_camera;
bool r_cameraOn;

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
        int flags;

        /* Ensure a minimum render size or pre-rendering will crash */
        if (r_width < R_WIDTH)
                r_width = R_WIDTH;
        if (r_height < R_HEIGHT)
                r_height = R_HEIGHT;
        r_scale = r_height / R_HEIGHT;
        r_heightScaled = r_height / r_scale;
        r_heightScaled += (r_height - r_scale * r_heightScaled) / r_scale;
        r_widthScaled = r_width * r_heightScaled / r_height;

        /* Create a new window */
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
        flags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_ANYFORMAT;
        if (r_fullscreen)
                flags |= SDL_FULLSCREEN;
        if (!SDL_SetVideoMode(r_width, r_height, 0, flags)) {
                C_warning("Failed to set video mode: %s", SDL_GetError());
                return FALSE;
        }

        C_debug("Set mode %dx%d (%dx%d scaled), scale factor %d",
                r_width, r_height, r_widthScaled, r_heightScaled, r_scale);
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
        glOrtho(0.f, r_widthScaled, r_heightScaled, 0.f, 0.f, 1.f);

        /* Identity model matrix */
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Enable alpha-blending by default */
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /* Minimal alpha testing */
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 1 / 255.f);

        /* Depth test function */
        glDepthFunc(GL_LEQUAL);

        /* Background clear color */
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);

        /* No texture by default */
        glDisable(GL_TEXTURE_2D);

        /* We use lines to do 2D edge anti-aliasing although there is probably
           a better way so we need to always smooth lines (requires alpha
           blending to be on to work) */
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        /* Sprites are depth-tested */
        glEnable(GL_DEPTH_TEST);

        R_checkErrors();
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
        SDL_GL_SwapBuffers();
        R_checkErrors();
}

/******************************************************************************\
 Begin rendering in world-space coordinates.
\******************************************************************************/
void R_beginCam(void)
{
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

