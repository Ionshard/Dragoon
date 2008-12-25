/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_public.h"

/* Number of milliseconds spent sleeping this frame */
CCount c_throttled;

/* The minimum duration of frame in milliseconds calculated from [c_max_fps] by
   the [C_throttle_fps()] function */
int c_throttleMsec;

/* Duration of time since the program started */
int c_timeMsec;

/* The current frame number */
int c_frame;

/* Duration of the last frame */
int c_frameMsec;
float c_frameSec;

/* Highest desired frames per second rate */
int c_maxFps;

/******************************************************************************\
 Initializes counters and timers.
\******************************************************************************/
void C_initTime(void)
{
        c_maxFps = 125;
        c_frame = 1;
        c_timeMsec = SDL_GetTicks();
        CCount_reset(&c_throttled);
}

/******************************************************************************\
 Updates the current time. This needs to be called exactly once per frame.
\******************************************************************************/
void C_updateTime(void)
{
        static unsigned int lastMsec;

        c_timeMsec = SDL_GetTicks();
        c_frameMsec = c_timeMsec - lastMsec;
        c_frameSec = c_frameMsec / 1000.f;
        lastMsec = c_timeMsec;
        c_frame++;

        /* Report when a frame takes an unusually long time */
        if (c_frameMsec > 500)
                C_debug("Frame %d lagged, %d msec", c_frame, c_frameMsec);
}

/******************************************************************************\
 Returns the time since the last call to C_timer(). Useful for measuring the
 efficiency of sections of code.
\******************************************************************************/
unsigned int C_timer(void)
{
        static unsigned int lastMsec;
        unsigned int elapsed;

        elapsed = SDL_GetTicks() - lastMsec;
        lastMsec += elapsed;
        return elapsed;
}

/******************************************************************************\
 Initializes a counter structure.
\******************************************************************************/
void CCount_reset(CCount *counter)
{
        counter->lastTime = c_timeMsec;
        counter->startFrame = c_frame;
        counter->startTime = c_timeMsec;
        counter->value = 0.f;
}

/******************************************************************************\
 Polls a counter to see if [interval] msec have elapsed since the last poll.
 Returns TRUE if the counter is ready to be polled and sets the poll time.
\******************************************************************************/
bool CCount_poll(CCount *counter, int interval)
{
        if (c_timeMsec - counter->lastTime < interval)
                return FALSE;
        counter->lastTime = c_timeMsec;
        return TRUE;
}

/******************************************************************************\
 Returns the per-frame count of a counter.
\******************************************************************************/
float CCount_perFrame(const CCount *counter)
{
        int frames;

        frames = c_frame - counter->startFrame;
        if (frames < 1)
                return 0.f;
        return counter->value / frames;
}

/******************************************************************************\
 Returns the per-second count of a counter.
\******************************************************************************/
float CCount_perSec(const CCount *counter)
{
        float seconds;

        seconds = (c_timeMsec - counter->startTime) / 1000.f;
        if (seconds <= 0.f)
                return 0.f;
        return counter->value / seconds;
}

/******************************************************************************\
 Returns the average frames-per-second while counter was running.
\******************************************************************************/
float CCount_fps(const CCount *counter)
{
        float seconds;

        seconds = (c_timeMsec - counter->startTime) / 1000.f;
        if (seconds <= 0.f)
                return 0.f;
        return (c_frame - counter->startFrame) / seconds;
}

/******************************************************************************\
 Throttle framerate if vsync is off or broken so we don't burn the CPU for no
 reason. SDL_Delay() will only work in 10ms increments on some systems so it
 is necessary to break down our delays into bite-sized chunks ([waitMsec]).
\******************************************************************************/
void C_throttleFps(void)
{
        static int waitMsec;
        int msec;

        if (c_maxFps < 1)
                return;
        c_throttleMsec = 1000 / c_maxFps;
        if (c_frameMsec > c_throttleMsec)
                return;
        waitMsec += c_throttleMsec - c_frameMsec;
        msec = (waitMsec / 10) * 10;
        if (msec > 0) {
                SDL_Delay(msec);
                waitMsec -= msec;
                CCount_add(&c_throttled, msec);
        }
}

