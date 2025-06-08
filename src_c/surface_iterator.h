#include "pygame.h"

/*
 * This file provides an interface for consistent and performant pixel iteration
 * on Surfaces.
 *
 * These surface iterators can be read or written, do not try to read and write
 * the same iterator at once.
 * 
 * For performance reasons, the returned pixel buffer might not always have 4
 * valid pixels, like at the end of a row it might only have 0-3 pixels to
 * populate from.
*/

typedef struct {
    SDL_Color p1;
    SDL_Color p2;
    SDL_Color p3;
    SDL_Color p4;
} pg_surface_iterator_buffer;

typedef struct {
    /* public */
    pg_surface_iterator_buffer pixels;
    bool done;  // readonly

    /* private */
    PG_PixelFormat *_pxfmt;
    SDL_Palette *_palette;
    int _surf_w_num_batches4;  // number of batches of 4 pixels
    int _surf_w_num_post4;  // number of pixels at the end of each row outside
                            // a batch
    int _surf_w_post_skip;  // number of bytes at the end of each row after all
                            // pixels
    int _surf_bpp;
    uint8_t *_px_ptr;
    int _remaining_rows;
    int _remaining_width_batches;
} pg_surface_iterator_context;

bool
pg_surface_iterator_create(SDL_Surface *surface,
                           pg_surface_iterator_context *context)
{
    context->done = false;
    return _pg_surface_iterator_create_generic(surface, context);
}

void
pg_surface_iterator_read(pg_surface_iterator_context *context)
{
    if (!context->done) {
        _pg_surface_iterator_read_generic(context);
    }
}

void
pg_surface_iterator_write(pg_surface_iterator_context *context)
{
    if (!context->done) {
        _pg_surface_iterator_write_generic(context);
    }
}

bool
_pg_surface_iterator_create_generic(SDL_Surface *surface,
                                    pg_surface_iterator_context *context)
{
    if (!PG_GetSurfaceDetails(surface, &(context->_pxfmt),
                              &(context->_palette))) {
        return false;
    }

    int surf_bpp = PG_FORMAT_BytesPerPixel(context->_pxfmt);

    if (surf_bpp < 1 || surf_bpp > 4) {
        SDL_SetError("Unsupported surface type for this operation");
        return false;
    }

    context->_surf_w_num_batches4 = surface->w % 4;
    context->_surf_w_num_post4 = surface->w / 4;
    context->_surf_w_post_skip = surface->pitch - (surface->w * surf_bpp);
    context->_surf_bpp = surf_bpp;
    context->_px_ptr = (uint8_t *)surface->pixels;
    context->_remaining_rows = surface->h;
    context->_remaining_width_batches = context->_surf_w_num_batches4;
}

void
_pg_surface_iterator_read_generic(pg_surface_iterator_context *context)
{
    if (context->_remaining_width_batches != 0) {
        switch (context->_surf_bpp) {
            case 1:
                PG_GetRGBA(*(context->_px_ptr++), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                PG_GetRGBA(*(context->_px_ptr++), context->_pxfmt,
                           context->_palette, &(context->pixels.p2.r),
                           &(context->pixels.p2.g), &(context->pixels.p2.b),
                           &(context->pixels.p2.a));
                PG_GetRGBA(*(context->_px_ptr++), context->_pxfmt,
                           context->_palette, &(context->pixels.p3.r),
                           &(context->pixels.p3.g), &(context->pixels.p3.b),
                           &(context->pixels.p3.a));
                PG_GetRGBA(*(context->_px_ptr++), context->_pxfmt,
                           context->_palette, &(context->pixels.p4.r),
                           &(context->pixels.p4.g), &(context->pixels.p4.b),
                           &(context->pixels.p4.a));
                break;
            case 2:
                PG_GetRGBA(*((uint16_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                PG_GetRGBA(*((uint16_t *)context->_px_ptr[2]), context->_pxfmt,
                           context->_palette, &(context->pixels.p2.r),
                           &(context->pixels.p2.g), &(context->pixels.p2.b),
                           &(context->pixels.p2.a));
                PG_GetRGBA(*((uint16_t *)context->_px_ptr[4]), context->_pxfmt,
                           context->_palette, &(context->pixels.p3.r),
                           &(context->pixels.p3.g), &(context->pixels.p3.b),
                           &(context->pixels.p3.a));
                PG_GetRGBA(*((uint16_t *)context->_px_ptr[6]), context->_pxfmt,
                           context->_palette, &(context->pixels.p4.r),
                           &(context->pixels.p4.g), &(context->pixels.p4.b),
                           &(context->pixels.p4.a));
                context->_px_ptr += 8;
                break;
            case 3:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                PG_GetRGBA(context->_px_ptr[0] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[2] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
                PG_GetRGBA(context->_px_ptr[3] + context->_px_ptr[4]
                               << 8 + context->_px_ptr[5] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p2.r), &(context->pixels.p2.g),
                           &(context->pixels.p2.b), &(context->pixels.p2.a));
                PG_GetRGBA(context->_px_ptr[6] + context->_px_ptr[7]
                               << 8 + context->_px_ptr[8] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p3.r), &(context->pixels.p3.g),
                           &(context->pixels.p3.b), &(context->pixels.p3.a));
                PG_GetRGBA(context->_px_ptr[9] + context->_px_ptr[10]
                               << 8 + context->_px_ptr[11] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p4.r), &(context->pixels.p4.g),
                           &(context->pixels.p4.b), &(context->pixels.p4.a));
#else
                PG_GetRGBA(context->_px_ptr[2] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[0] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
                PG_GetRGBA(context->_px_ptr[5] + context->_px_ptr[4]
                               << 8 + context->_px_ptr[3] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p2.r), &(context->pixels.p2.g),
                           &(context->pixels.p2.b), &(context->pixels.p2.a));
                PG_GetRGBA(context->_px_ptr[8] + context->_px_ptr[7]
                               << 8 + context->_px_ptr[6] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p3.r), &(context->pixels.p3.g),
                           &(context->pixels.p3.b), &(context->pixels.p3.a));
                PG_GetRGBA(context->_px_ptr[11] + context->_px_ptr[10]
                               << 8 + context->_px_ptr[9] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p4.r), &(context->pixels.p4.g),
                           &(context->pixels.p4.b), &(context->pixels.p4.a));
#endif
                context->_px_ptr += 12;
                break;
            default: /* case 4 */
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[4]), context->_pxfmt,
                           context->_palette, &(context->pixels.p2.r),
                           &(context->pixels.p2.g), &(context->pixels.p2.b),
                           &(context->pixels.p2.a));
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[8]), context->_pxfmt,
                           context->_palette, &(context->pixels.p3.r),
                           &(context->pixels.p3.g), &(context->pixels.p3.b),
                           &(context->pixels.p3.a));
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[12]), context->_pxfmt,
                           context->_palette, &(context->pixels.p4.r),
                           &(context->pixels.p4.g), &(context->pixels.p4.b),
                           &(context->pixels.p4.a));
                context->_px_ptr += 16;
                break;
        }
        context->_remaining_width_batches--;
        return;
    }

    // If no remaining batches of 4, process any remaining and prepare for next
    // row
    for (int i = 0; i < context->_surf_w_num_post4; i++) {
        switch (context->_surf_bpp) {
            case 1:
                PG_GetRGBA(*(context->_px_ptr++), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                break;
            case 2:
                PG_GetRGBA(*((uint16_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                context->_px_ptr += 2;
                break;
            case 3:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                PG_GetRGBA(context->_px_ptr[0] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[2] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
#else
                PG_GetRGBA(context->_px_ptr[2] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[0] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
#endif
                context->_px_ptr += 3;
                break;
            default: /* case 4 */
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                context->_px_ptr += 4;
                break;
        }
    }
    context->_px_ptr += context->_surf_w_post_skip;
    context->_remaining_rows--;
    context->_remaining_width_batches = context->_surf_w_num_batches4;

    if (context->_remaining_rows < 0) {
        context->done = true;
    }
}

void
_pg_surface_iterator_write_generic(pg_surface_iterator_context *context)
{
    if (context->_remaining_width_batches != 0) {
        switch (context->_surf_bpp) {
            case 1:
                context->_px_ptr[0] = (uint8_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p1.r,
                    context->pixels.p1.g, context->pixels.p1.b,
                    context->pixels.p1.a);
                context->_px_ptr[1] = (uint8_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p2.r,
                    context->pixels.p2.g, context->pixels.p2.b,
                    context->pixels.p2.a);
                context->_px_ptr[2] = (uint8_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p3.r,
                    context->pixels.p3.g, context->pixels.p3.b,
                    context->pixels.p3.a);
                context->_px_ptr[3] = (uint8_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p4.r,
                    context->pixels.p4.g, context->pixels.p4.b,
                    context->pixels.p4.a);
                context->_px_ptr += 4;
                break;
            case 2:
                *(uint16_t*)context->_px_ptr[0] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p1.r,
                    context->pixels.p1.g, context->pixels.p1.b,
                    context->pixels.p1.a);
                *(uint16_t*)context->_px_ptr[2] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p2.r,
                    context->pixels.p2.g, context->pixels.p2.b,
                    context->pixels.p2.a);
                *(uint16_t*)context->_px_ptr[4] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p3.r,
                    context->pixels.p3.g, context->pixels.p3.b,
                    context->pixels.p3.a);
                *(uint16_t*)context->_px_ptr[6] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p4.r,
                    context->pixels.p4.g, context->pixels.p4.b,
                    context->pixels.p4.a);
                context->_px_ptr += 8;
                break;
            case 3:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                PG_GetRGBA(context->_px_ptr[0] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[2] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
                PG_GetRGBA(context->_px_ptr[3] + context->_px_ptr[4]
                               << 8 + context->_px_ptr[5] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p2.r), &(context->pixels.p2.g),
                           &(context->pixels.p2.b), &(context->pixels.p2.a));
                PG_GetRGBA(context->_px_ptr[6] + context->_px_ptr[7]
                               << 8 + context->_px_ptr[8] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p3.r), &(context->pixels.p3.g),
                           &(context->pixels.p3.b), &(context->pixels.p3.a));
                PG_GetRGBA(context->_px_ptr[9] + context->_px_ptr[10]
                               << 8 + context->_px_ptr[11] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p4.r), &(context->pixels.p4.g),
                           &(context->pixels.p4.b), &(context->pixels.p4.a));
#else
                PG_GetRGBA(context->_px_ptr[2] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[0] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
                PG_GetRGBA(context->_px_ptr[5] + context->_px_ptr[4]
                               << 8 + context->_px_ptr[3] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p2.r), &(context->pixels.p2.g),
                           &(context->pixels.p2.b), &(context->pixels.p2.a));
                PG_GetRGBA(context->_px_ptr[8] + context->_px_ptr[7]
                               << 8 + context->_px_ptr[6] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p3.r), &(context->pixels.p3.g),
                           &(context->pixels.p3.b), &(context->pixels.p3.a));
                PG_GetRGBA(context->_px_ptr[11] + context->_px_ptr[10]
                               << 8 + context->_px_ptr[9] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p4.r), &(context->pixels.p4.g),
                           &(context->pixels.p4.b), &(context->pixels.p4.a));
#endif
                context->_px_ptr += 12;
                break;
            default: /* case 4 */


                *(uint32_t*)context->_px_ptr[0] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p1.r,
                    context->pixels.p1.g, context->pixels.p1.b,
                    context->pixels.p1.a);
                *(uint32_t*)context->_px_ptr[4] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p2.r,
                    context->pixels.p2.g, context->pixels.p2.b,
                    context->pixels.p2.a);
                *(uint32_t*)context->_px_ptr[8] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p3.r,
                    context->pixels.p3.g, context->pixels.p3.b,
                    context->pixels.p3.a);
                *(uint32_t*)context->_px_ptr[12] = (uint16_t)PG_MapRGBA(
                    context->_pxfmt, context->_palette, context->pixels.p4.r,
                    context->pixels.p4.g, context->pixels.p4.b,
                    context->pixels.p4.a);
                context->_px_ptr += 16;


                PG_GetRGBA(*((uint32_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[4]), context->_pxfmt,
                           context->_palette, &(context->pixels.p2.r),
                           &(context->pixels.p2.g), &(context->pixels.p2.b),
                           &(context->pixels.p2.a));
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[8]), context->_pxfmt,
                           context->_palette, &(context->pixels.p3.r),
                           &(context->pixels.p3.g), &(context->pixels.p3.b),
                           &(context->pixels.p3.a));
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[12]), context->_pxfmt,
                           context->_palette, &(context->pixels.p4.r),
                           &(context->pixels.p4.g), &(context->pixels.p4.b),
                           &(context->pixels.p4.a));
                context->_px_ptr += 16;
                break;
        }
        context->_remaining_width_batches--;
        return;
    }

    // If no remaining batches of 4, process any remaining and prepare for next
    // row
    for (int i = 0; i < context->_surf_w_num_post4; i++) {
        switch (context->_surf_bpp) {
            case 1:
                PG_GetRGBA(*(context->_px_ptr++), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                break;
            case 2:
                PG_GetRGBA(*((uint16_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                context->_px_ptr += 2;
                break;
            case 3:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                PG_GetRGBA(context->_px_ptr[0] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[2] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
#else
                PG_GetRGBA(context->_px_ptr[2] + context->_px_ptr[1]
                               << 8 + context->_px_ptr[0] << 16,
                           context->_pxfmt, context->_palette,
                           &(context->pixels.p1.r), &(context->pixels.p1.g),
                           &(context->pixels.p1.b), &(context->pixels.p1.a));
#endif
                context->_px_ptr += 3;
                break;
            default: /* case 4 */
                PG_GetRGBA(*((uint32_t *)context->_px_ptr[0]), context->_pxfmt,
                           context->_palette, &(context->pixels.p1.r),
                           &(context->pixels.p1.g), &(context->pixels.p1.b),
                           &(context->pixels.p1.a));
                context->_px_ptr += 4;
                break;
        }
    }
    context->_px_ptr += context->_surf_w_post_skip;
    context->_remaining_rows--;
    context->_remaining_width_batches = context->_surf_w_num_batches4;

    if (context->_remaining_rows < 0) {
        context->done = true;
    }
}
