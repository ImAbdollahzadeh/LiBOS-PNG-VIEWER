#ifndef _LPD_RENDERER__H__
#define _LPD_RENDERER__H__

//***********************************************************************************************************

#include "LPD_ALIASES.h"
#include "LPD_PNG.h"
#include <Windows.h>

//***********************************************************************************************************

#define _LPD_RENDERER_STRECHED_IMAGE 600
#define _LPD_WINDOW_WIDTH_EDGE       16
#define _LPD_WINDOW_HEIGHT_EDGE      38

//***********************************************************************************************************

#define LPD_RENDERER_SWAP_BYTES(__LPD_RENDERER_BYTE1_, __LPD_RENDERER_BYTE2_) do { \
    UINT_8 __LPD_RENDERER_TMP_ = __LPD_RENDERER_BYTE1_;                            \
    __LPD_RENDERER_BYTE1_      = __LPD_RENDERER_BYTE2_;                            \
    __LPD_RENDERER_BYTE2_      = __LPD_RENDERER_TMP_;                              \
} while(0)

//***********************************************************************************************************

void lpd_render_buffer   (void* buffer, UINT_8 byte_per_pixel, IHDR* ihdr);
void lpd_convert_rgb2bgr (void* buffer, UINT_32 bytes);

//***********************************************************************************************************

/* windows functions prototypes */
HBITMAP           lpd_create_bmp  (HDC dc, void* fb);
void              lpd_blit        (HWND hwnd, void* buff);
void              lpd_strech_blit (HWND hwnd, void* buff);
LRESULT __stdcall lpd_WndProc     (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int     __stdcall lpd_WinMain     (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow);

//***********************************************************************************************************

#endif // !_LPD_RENDERER__H__
