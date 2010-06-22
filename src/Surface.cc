/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"
#include "os.h"
#include "Mode.h"
#include "Surface.h"

namespace dragoon {

Surface::Surface(const char* filename): lock_(0) {
  FILE* file = os::OpenRead(filename);
  if (!file)
    return;
  try {

    // Check the first few bytes of the file to see if it is PNG format
    png_byte png_header[8];
    fread(png_header, 1, sizeof(png_header), file);
    if (png_sig_cmp(png_header, 0, sizeof(png_header)))
      ERROR("'%s' is not in PNG format", filename);

    // Allocate the PNG structures
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL, NULL, NULL);
    if (!png_ptr)
      ERROR("Failed to allocate PNG read struct");
    png_infop info_ptr = NULL;
    try {

      // Tells libpng that we've read a bit of the header already
      png_set_sig_bytes(png_ptr, sizeof(png_header));

      // Set read callback before proceeding
      png_set_read_fn(png_ptr, file, NULL);

      // If an error occurs in libpng, it will longjmp back here
      if (setjmp(png_ptr->jmpbuf))
        ERROR("Error loading PNG '%s'", filename);

      // Allocate a PNG info struct
      info_ptr = png_create_info_struct(png_ptr);
      if (!info_ptr)
        ERROR("Failed to allocate PNG info struct");

      // Read the image info
      png_read_info(png_ptr, info_ptr);
      png_uint_32 width, height;
      int bit_depth, color_type;
      png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                   &color_type, NULL, NULL, NULL);

      // Convert paletted images to RGB
      if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

      // Convert transparent regions to alpha channel
      if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

      // Convert grayscale images to RGB
      if (color_type == PNG_COLOR_TYPE_GRAY
          || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_expand(png_ptr);
        png_set_gray_to_rgb(png_ptr);
      }

      // Crop 16-bit image data to 8-bit
      if (bit_depth == 16)
        png_set_strip_16(png_ptr);

      // Give opaque images an opaque alpha channel (ARGB)
      if (!(color_type & PNG_COLOR_MASK_ALPHA))
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

      // Convert 1-, 2-, and 4-bit samples to 8-bit
      png_set_packing(png_ptr);

      // Let libpng handle interlacing
      png_set_interlace_handling(png_ptr);

      // Update our image information
      png_read_update_info(png_ptr, info_ptr);
      png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                   &color_type, NULL, NULL, NULL);

      // Sanity check
      if (width < 1 || height < 1)
        ERROR("PNG image '%s' has invalid dimensions %dx%d",
              filename, width, height);

      // Allocate the SDL surface and get image data
      Alloc(width, height);
      if (!Lock())
        ERROR("Failed to lock surface");
      png_bytep row_ptrs[height];
      for (int i = 0; i < (int)height; i++)
        row_ptrs[i] = (png_bytep)ptr_->pixels + ptr_->pitch * i;
      png_read_image(png_ptr, row_ptrs);
      Unlock();

    } catch (log::Exception& e) {
      e.Print(log::LEVEL_WARN);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  } catch (log::Exception& e) {
    e.Print(log::LEVEL_WARN);
  }
  fclose(file);
}

Surface::Surface(int x, int y, int w, int h) {
  Alloc(w, h);
  Lock();
  glReadPixels(x, Mode::height() - h - y, w, h,
               GL_RGBA, GL_UNSIGNED_BYTE, ptr_->pixels);
  Unlock();
  Flip();
  Mode::Check();
}

void Surface::Put(int x, int y, Color color) {
  Validate(x, y);
  int bpp = ptr_->format->BytesPerPixel;
  Uint8* p = (Uint8*)ptr_->pixels + y * ptr_->pitch + x * bpp;
  Uint32 pixel = SDL_MapRGBA(ptr_->format, (Uint8)(255 * color[0]),
                             (Uint8)(255 * color[1]), (Uint8)(255 * color[2]),
                             (Uint8)(255 * color[3]));
  switch (bpp) {
  case 1:
    *p = pixel;
    break;
  case 2:
    *(Uint16 *)p = pixel;
    break;
  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
      p[0] = (pixel >> 16) & 0xff;
      p[1] = (pixel >> 8) & 0xff;
      p[2] = pixel & 0xff;
    } else {
      p[0] = pixel & 0xff;
      p[1] = (pixel >> 8) & 0xff;
      p[2] = (pixel >> 16) & 0xff;
    }
    break;
  case 4:
    *(Uint32 *)p = pixel;
    break;
  default:
    ERROR("Invalid surface format");
  }
}

Color Surface::Get(int x, int y) const {
  Validate(x, y);
  int bpp = ptr_->format->BytesPerPixel;
  Uint8* p = (Uint8*)ptr_->pixels + y * ptr_->pitch + x * bpp;
  Uint32 pixel;
  switch (bpp) {
  case 1:
    pixel = *p;
    break;
  case 2:
    pixel = *(Uint16 *)p;
    break;
  case 3:
    pixel = SDL_BYTEORDER == SDL_BIG_ENDIAN ? p[0] << 16 | p[1] << 8 | p[2]
                                            : p[0] | p[1] << 8 | p[2] << 16;
    break;
  case 4:
    pixel = *(Uint32*)p;
    break;
  default:
    ERROR("Invalid surface format");
  }
  Uint8 r, g, b, a;
  SDL_GetRGBA(pixel, ptr_->format, &r, &g, &b, &a);
  return Color(r / 255., g / 255., b / 255., a / 255.);
}

bool Surface::Lock() {
  if (!ptr_)
    return false;
  if (!lock_++ && SDL_LockSurface(ptr_) < 0) {
    lock_ = 0;
    WARN("Failed to lock surface");
    return false;
  }
  return true;
}

void Surface::Unlock() {
  if (ptr_ && lock_ > 0 && !--lock_)
    SDL_UnlockSurface(ptr_);
}

bool Surface::Save(const char *filename) {
  if (!ptr_ || ptr_->w < 1 || ptr_->h < 1)
    return false;
  bool success = false;
  FILE* file;
  if (!(file = os::OpenWrite(filename)))
    return false;

  // Allocate the PNG structures
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL, NULL, NULL);
  png_infop info_ptr = NULL;
  if (!png_ptr) {
    WARN("Failed to allocate PNG write struct");
    return false;
  }

  // Set write callbacks before proceeding
  png_set_write_fn(png_ptr, file, NULL, NULL);
  try {

    // If an error occurs in libpng, it will longjmp back here
    if (setjmp(png_ptr->jmpbuf))
      ERROR("Error saving PNG '%s'", filename);

    // Allocate a PNG info struct
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
      ERROR("Failed to allocate PNG info struct");

    // Sanity check
    int height = ptr_->h;
    png_bytep row_ptrs[height];

    // Setup the info structure for writing our texture
    png_set_IHDR(png_ptr, info_ptr, ptr_->w, height, 8,
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Setup the comment text
    png_text text[2];
    text[0].key = (char*)"Title";
    text[0].text = (char*)PACKAGE_STRING;
    text[0].text_length = strlen(text[0].text);
    text[0].compression = PNG_TEXT_COMPRESSION_NONE;
    time_t msec;
    time(&msec);
    struct tm *local = localtime(&msec);
    text[1].key = (char*)"Creation Time";
    char buf[64];
    text[1].text_length = strftime(buf, sizeof(buf),
                                   "%d %b %Y %H:%M:%S GMT", local);
    text[1].text = buf;
    text[1].compression = PNG_TEXT_COMPRESSION_NONE;
    png_set_text(png_ptr, info_ptr, text, 2);

    // Set modified time
    png_time mod_time;
    mod_time.day = local->tm_mday;
    mod_time.hour = local->tm_hour;
    mod_time.minute = local->tm_min;
    mod_time.second = local->tm_sec;
    mod_time.year = local->tm_year + 1900;
    png_set_tIME(png_ptr, info_ptr, &mod_time);

    // Write image header
    png_write_info(png_ptr, info_ptr);

    // Write the image data
    if (!Lock())
      ERROR("Failed to lock surface");
    for (int i = 0; i < (int)height; i++)
      row_ptrs[i] = (png_bytep)ptr_->pixels + ptr_->pitch * i;
    png_write_image(png_ptr, row_ptrs);
    png_write_end(png_ptr, NULL);

    Unlock();
    success = true;
  } catch (log::Exception e) {
    e.Print(log::LEVEL_WARN);
  }
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(file);
  return success;
}

void Surface::Deseam() {
  if (Lock()) {
    for (int y = 0; y < ptr_->h; ++y)
      for (int x = 0; x < ptr_->w; ++x) {
        Color color = Get(x, y);
        if (!color[3]) {
          Color sum = Color::black();
          if (x > 0) {
            color = Get(x - 1, y);
            sum += color * color[3];
          }
          if (y > 0) {
            color = Get(x, y - 1);
            sum += color * color[3];
          }
          if (x < ptr_->w - 1) {
            color = Get(x + 1, y);
            sum += color * color[3];
          }
          if (y < ptr_->h - 1) {
            color = Get(x, y + 1);
            sum += color * color[3];
          }
          if (sum[3] > 0) {
            sum /= sum[3];
            sum[3] = 0;
            Put(x, y, sum);
          }
        }
      }
    Unlock();
  }
}

void Surface::Flip() {
  if (Lock()) {
    for (int y = 0; y < ptr_->h / 2; y++)
      for (int x = 0; x < ptr_->w; x++) {
        Color color_top = Get(x, y);
        Color color_bottom = Get(x, ptr_->h - y - 1);
        Put(x, y, color_bottom);
        Put(x, ptr_->h - y - 1, color_top);
      }
    Unlock();
  }
}

void Surface::Scale(Surface& dest, int scale_x, int scale_y, int dx, int dy) {
  if (Lock()) {
    if (dest.Lock()) {
      for (int y = 0; y < ptr_->h; y++)
        for (int x = 0; x < ptr_->w; x++) {
          Color color = Get(x, y);
          for (int ys = 0; ys < scale_y; ys++)
            for (int xs = 0; xs < scale_x; xs++)
              dest.Put(dx + scale_x * x + xs, dy + scale_y * y + ys, color);
        }
      dest.Unlock();
    }
    Unlock();
  }
}

void Surface::Blit(Surface& dest, int sx, int sy, int sw, int sh,
                   int dx, int dy) {
  if (Lock()) {
    if (dest.Lock()) {
      for (int y = 0; y < sh; y++)
        for (int x = 0; x < sw; x++)
          dest.Put(dx + x, dy + y, Get(sx + x, sy + y));
      dest.Unlock();
    }
    Unlock();
  }
}

void Surface::Blit(Surface& dest, int sx, int sy, int sw, int sh,
                   int dx, int dy, int dw, int dh) {
  if (Lock()) {
    if (dest.Lock()) {
      for (int y = 0; y < dh; y++)
        for (int x = 0; x < dw; x++)
          dest.Put(dx + x, dy + y, Get(sx + x * sw / dw, sy + y * sh / dh));
      dest.Unlock();
    }
    Unlock();
  }
}

void Surface::BlitShadowed(Surface& dest, int sx, int sy, int sw, int sh,
                           int dx, int dy, int sh_x, int sh_y, Color shadow) {
  ASSERT(sh_x >= 0 && sh_y >= 0);
  if (Lock()) {
    if (dest.Lock()) {
      for (int y = 0; y < sh; y++)
        for (int x = 0; x < sw; x++) {
          Color sc = Get(sx + x, sy + y);
          Color sh = Color::none();
          if (x > sh_x && y > sh_y)
            sh = Get(sx + x - sh_x, sy + y - sh_y) * shadow;
          dest.Put(dx + x, dy + y, sc.Blend(sh));
        }
      dest.Unlock();
    }
    Unlock();
  }
}

void Surface::Alloc(int width, int height) {
  if (ptr_)
    SDL_FreeSurface(ptr_);

  // Setup the texture pixel format, RGBA in 32 bits
  unsigned int mask[4]
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    = { 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff };
#else
    = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };
#endif

  // Allocate the surface
  ASSERT(width > 0 && height > 0);
  ptr_ = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, width, height, 32,
                              mask[0], mask[1], mask[2], mask[3]);
  SDL_SetAlpha(ptr_, SDL_RLEACCEL, SDL_ALPHA_OPAQUE);
  ASSERT(ptr_->format);
}

void Surface::Validate(int x, int y) const {
  ASSERT(ptr_);
  ASSERT(ptr_->format);
  ASSERT(x >= 0);
  ASSERT(y >= 0);
  ASSERT(x < ptr_->w);
  ASSERT(y < ptr_->h);
  ASSERT(lock_ >= 0);
}

} // namespace dragoon
