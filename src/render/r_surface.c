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

/* Estimated video memory usage */
int r_videoMem, r_videoMemMax;

/******************************************************************************\
 Gets a pixel from an SDL surface. Lock the surface before calling.
\******************************************************************************/
CColor R_getPixel(const SDL_Surface *surf, int x, int y)
{
        Uint8 *p, r, g, b, a;
        Uint32 pixel;
        int bpp;

        bpp = surf->format->BytesPerPixel;
        p = (Uint8 *)surf->pixels + y * surf->pitch + x * bpp;
        switch (bpp) {
        case 1: pixel = *p;
                break;
        case 2: pixel = *(Uint16 *)p;
                break;
        case 3: pixel = SDL_BYTEORDER == SDL_BIG_ENDIAN ?
                        p[0] << 16 | p[1] << 8 | p[2] :
                        p[0] | p[1] << 8 | p[2] << 16;
                break;
        case 4: pixel = *(Uint32 *)p;
                break;
        default:
                C_error("Invalid surface format");
        }
        SDL_GetRGBA(pixel, surf->format, &r, &g, &b, &a);
        return CColor_bytes(r, g, b, a);
}

/******************************************************************************\
 Sets the color of a pixel on an SDL surface. Lock the surface before
 calling.
\******************************************************************************/
void R_putPixel(SDL_Surface *surf, int x, int y, CColor color)
{
        Uint32 pixel;
        Uint8 *p;
        int bpp;

        bpp = surf->format->BytesPerPixel;
        p = (Uint8 *)surf->pixels + y * surf->pitch + x * bpp;
        pixel = SDL_MapRGBA(surf->format, (Uint8)(255 * color.r),
                            (Uint8)(255 * color.g), (Uint8)(255 * color.b),
                            (Uint8)(255 * color.a));
        switch (bpp) {
        case 1: *p = pixel;
                break;
        case 2: *(Uint16 *)p = pixel;
                break;
        case 3: if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                        p[0] = (pixel >> 16) & 0xff;
                        p[1] = (pixel >> 8) & 0xff;
                        p[2] = pixel & 0xff;
                } else {
                        p[0] = pixel & 0xff;
                        p[1] = (pixel >> 8) & 0xff;
                        p[2] = (pixel >> 16) & 0xff;
                }
                break;
        case 4: *(Uint32 *)p = pixel;
                break;
        default:
                C_error("Invalid surface format");
        }
}

/******************************************************************************\
 Whole-integer scale blit a surface.
\******************************************************************************/
void R_scaleSurface(SDL_Surface *src, SDL_Surface *dest,
                    int scale, int dx, int dy)
{
        CColor color;
        int x, y, xs, ys;

        if (SDL_LockSurface(src) < 0 || SDL_LockSurface(dest) < 0) {
                C_warning("Failed to lock source or destination surface");
                return;
        }
        for (y = 0; y < src->h; y++)
                for (x = 0; x < src->w; x++) {
                        color = R_getPixel(src, x, y);
                        for (ys = 0; ys < scale; ys++)
                                for (xs = 0; xs < scale; xs++)
                                        R_putPixel(dest, dx + scale * x + xs,
                                                   dy + scale * y + ys, color);
                }
        SDL_UnlockSurface(src);
        SDL_UnlockSurface(dest);
}

/******************************************************************************\
 Wrapper around surface allocation that keeps track of memory allocated.
\******************************************************************************/
SDL_Surface *R_allocSurface(int width, int height)
{
        SDL_Surface *surface;
        unsigned int mask[4];

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        /* Setup the texture pixel format, RGBA in 32 bits */
        mask[0] = 0xff000000;
        mask[1] = 0x00ff0000;
        mask[2] = 0x0000ff00;
        mask[3] = 0x000000ff;
#else
        /* Setup the texture pixel format, ABGR in 32 bits */
        mask[0] = 0x000000ff;
        mask[1] = 0x0000ff00;
        mask[2] = 0x00ff0000;
        mask[3] = 0xff000000;
#endif

        /* Allocate the surface */
        surface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA,
                                       width, height, 32,
                                       mask[0], mask[1], mask[2], mask[3]);
        SDL_SetAlpha(surface, SDL_RLEACCEL, SDL_ALPHA_OPAQUE);

        /* Keep track of allocated memory */
        r_videoMem += width * height * 4;
        if (r_videoMem > r_videoMemMax)
                r_videoMemMax = r_videoMem;

        return surface;
}

/******************************************************************************\
 Frees a surface, updating video memory estimate.
\******************************************************************************/
void R_freeSurface(SDL_Surface *surface)
{
        if (!surface)
                return;

        /* Keep track of free'd memory */
        r_videoMem -= surface->w * surface->h * surface->format->BytesPerPixel;

        SDL_FreeSurface(surface);
}

/******************************************************************************\
 Loads a PNG file and allocates a new SDL surface for it. Returns NULL on
 failure.

 Based on tutorial implementation in the libpng manual:
 http://www.libpng.org/pub/png/libpng-1.2.5-manual.html
\******************************************************************************/
SDL_Surface *R_loadSurface(const char *filename)
{
        SDL_Surface *surface;
        png_byte pngHeader[8];
        png_bytep rowPointers[4096];
        png_infop infoPtr;
        png_structp pngPtr;
        png_uint_32 width, height;
        FILE *file;
        int i, bitDepth, colorType;

        surface = NULL;

        /* We have to read everything ourselves for libpng */
        if (!(file = C_fopen_read(filename)))
                return NULL;

        /* Check the first few bytes of the file to see if it is PNG format */
        C_fread(file, pngHeader, sizeof (pngHeader));
        if (png_sig_cmp(pngHeader, 0, sizeof (pngHeader))) {
                C_warning("'%s' is not in PNG format", filename);
                fclose(file);
                return NULL;
        }

        /* Allocate the PNG structures */
        pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                        NULL, NULL, NULL);
        if (!pngPtr) {
                C_warning("Failed to allocate PNG read struct");
                return NULL;
        }

        /* Tells libpng that we've read a bit of the header already */
        png_set_sig_bytes(pngPtr, sizeof (pngHeader));

        /* Set read callback before proceeding */
        png_set_read_fn(pngPtr, file, NULL);

        /* If an error occurs in libpng, it will longjmp back here */
        if (setjmp(pngPtr->jmpbuf)) {
                C_warning("Error loading PNG '%s'", filename);
                goto cleanup;
        }

        /* Allocate a PNG info struct */
        infoPtr = png_create_info_struct(pngPtr);
        if (!infoPtr) {
                C_warning("Failed to allocate PNG info struct");
                goto cleanup;
        }

        /* Read the image info */
        png_read_info(pngPtr, infoPtr);
        png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth,
                     &colorType, NULL, NULL, NULL);

        /* Convert paletted images to RGB */
        if (colorType == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(pngPtr);

        /* Convert transparent regions to alpha channel */
        if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(pngPtr);

        /* Convert grayscale images to RGB */
        if (colorType == PNG_COLOR_TYPE_GRAY ||
            colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
                png_set_expand(pngPtr);
                png_set_gray_to_rgb(pngPtr);
        }

        /* Crop 16-bit image data to 8-bit */
        if (bitDepth == 16)
                png_set_strip_16(pngPtr);

        /* Give opaque images an opaque alpha channel (ARGB) */
        if (!(colorType & PNG_COLOR_MASK_ALPHA))
                png_set_filler(pngPtr, 0xff, PNG_FILLER_AFTER);

        /* Convert 1-, 2-, and 4-bit samples to 8-bit */
        png_set_packing(pngPtr);

        /* Let libpng handle interlacing */
        png_set_interlace_handling(pngPtr);

        /* Update our image information */
        png_read_update_info(pngPtr, infoPtr);
        png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth,
                     &colorType, NULL, NULL, NULL);

        /* Sanity check */
        if (width < 1 || height < 1) {
                C_warning("PNG image '%s' has invalid dimensions %dx%d",
                          filename, width, height);
                goto cleanup;
        }
        if (height > sizeof (rowPointers)) {
                C_warning("PNG image '%s' is too tall (%dpx), cropping",
                          filename, height);
                height = sizeof (rowPointers);
        }

        /* Allocate the SDL surface and get image data */
        surface = R_allocSurface(width, height);
        if (SDL_LockSurface(surface) < 0) {
                C_warning("Failed to lock surface");
                goto cleanup;
        }
        for (i = 0; i < (int)height; i++)
                rowPointers[i] = (png_bytep)surface->pixels +
                                  surface->pitch * i;
        png_read_image(pngPtr, rowPointers);
        SDL_UnlockSurface(surface);

cleanup:
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        fclose(file);
        return surface;
}

/******************************************************************************\
 Write the contents of a surface as a PNG file. Returns TRUE if a file was
 written.
\******************************************************************************/
int R_saveSurface(SDL_Surface *surface, const char *filename)
{
        struct tm *local;
        time_t msec;
        png_bytep rowPointers[4096];
        png_infop infoPtr;
        png_structp pngPtr;
        png_text text[2];
        png_time modTime;
        FILE *file;
        int i, height, success;
        char buf[64];

        if (!surface || surface->w < 1 || surface->h < 1)
                return FALSE;
        success = FALSE;

        /* We have to read everything ourselves for libpng */
        if (!(file = C_fopen_write(filename)))
                return FALSE;

        /* Allocate the PNG structures */
        pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                         NULL, NULL, NULL);
        if (!pngPtr) {
                C_warning("Failed to allocate PNG write struct");
                return FALSE;
        }

        /* Set write callbacks before proceeding */
        png_set_write_fn(pngPtr, file, NULL, NULL);

        /* If an error occurs in libpng, it will longjmp back here */
        if (setjmp(pngPtr->jmpbuf)) {
                C_warning("Error saving PNG '%s'", filename);
                goto cleanup;
        }

        /* Allocate a PNG info struct */
        infoPtr = png_create_info_struct(pngPtr);
        if (!infoPtr) {
                C_warning("Failed to allocate PNG info struct");
                goto cleanup;
        }

        /* Sanity check */
        height = surface->h;
        if (height > 4096) {
                C_warning("Image too tall (%dpx), cropping", height);
                height = 4096;
        }

        /* Setup the info structure for writing our texture */
        png_set_IHDR(pngPtr, infoPtr, surface->w, height, 8,
                     PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        /* Setup the comment text */
        text[0].key = "Title";
        text[0].text = PACKAGE_STRING;
        text[0].text_length = strlen(text[0].text);
        text[0].compression = PNG_TEXT_COMPRESSION_NONE;
        time(&msec);
        local = localtime(&msec);
        text[1].key = "Creation Time";
        text[1].text = buf;
        text[1].text_length = strftime(buf, sizeof (buf),
                                       "%d %b %Y %H:%M:%S GMT", local);
        text[1].compression = PNG_TEXT_COMPRESSION_NONE;
        png_set_text(pngPtr, infoPtr, text, 2);

        /* Set modified time */
        modTime.day = local->tm_mday;
        modTime.hour = local->tm_hour;
        modTime.minute = local->tm_min;
        modTime.second = local->tm_sec;
        modTime.year = local->tm_year + 1900;
        png_set_tIME(pngPtr, infoPtr, &modTime);

        /* Write image header */
        png_write_info(pngPtr, infoPtr);

        /* Write the image data */
        if (SDL_LockSurface(surface) < 0) {
                C_warning("Failed to lock surface");
                goto cleanup;
        }
        for (i = 0; i < (int)height; i++)
                rowPointers[i] = (png_bytep)surface->pixels +
                                  surface->pitch * i;
        png_write_image(pngPtr, rowPointers);
        png_write_end(pngPtr, NULL);
        SDL_UnlockSurface(surface);
        success = TRUE;

cleanup:
        png_destroy_write_struct(&pngPtr, &infoPtr);
        fclose(file);
        return success;
}

