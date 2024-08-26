//***********************************************************************************************************

#include "LPD_RENDERER.h"

//***********************************************************************************************************

static void*   lpd_rgb_buffer   = LPD_NULL;
static UINT_32 lpd_width        = 0;
static UINT_32 lpd_height       = 0;
static UINT_8  lpd_bpp          = 0;
static BOOL    lpd_strech       = LPD_FALSE;
static INT_32  lpd_scale_factor = 1;

//***********************************************************************************************************

static UINT_32 lpd_renderer_reverse_a_dword(UINT_32 num)
{
	return ((num & 0xFF) << 24) | ((num & 0xFF00) << 8) | ((num & 0xFF0000) >> 8) | ((num & 0xFF000000) >> 24);
}

//***********************************************************************************************************

static UINT_32 lpd_find_max(UINT_32 num1, UINT_32 num2)
{
	return (num1 >= num2) ? num1 : num2;
}

//***********************************************************************************************************

void lpd_render_buffer(void* buffer, UINT_8 byte_per_pixel, IHDR* ihdr)
{
	// retrieve the width and height from IHDR
	UINT_32 width  = lpd_renderer_reverse_a_dword(ihdr->width);
	UINT_32 height = lpd_renderer_reverse_a_dword(ihdr->height);

	// load them into lpd_width and lpd_height
	lpd_width  = width;
	lpd_height = height;

	// load the lpd_rgb_buffer
	lpd_rgb_buffer = buffer;

	// load the lpd_bpp
	lpd_bpp = byte_per_pixel;

	// convert RGB to BGR
	lpd_convert_rgb2bgr(lpd_rgb_buffer, (lpd_width * lpd_height * lpd_bpp));

	// decide if the final image needs to be streched
	if ((lpd_width < 100) || (lpd_height < 100))
	{
		lpd_scale_factor = lpd_find_max((_LPD_RENDERER_STRECHED_IMAGE / lpd_width), (_LPD_RENDERER_STRECHED_IMAGE / lpd_height));
		lpd_strech = LPD_TRUE;
	}

	// call the window entry
	lpd_WinMain(0, 0, 0, 10);

	// unload the lpd_rgb_buffer, lpd_strech, lpd_width, lpd_height, and lpd_bpp
	lpd_rgb_buffer = LPD_NULL;
	lpd_strech     = LPD_FALSE;
	lpd_width      = 0;
	lpd_height     = 0;
	lpd_bpp        = 0;
}


//***********************************************************************************************************

HBITMAP lpd_create_bmp(HDC dc, void* fb)
{
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = lpd_width * lpd_scale_factor;
	bmi.bmiHeader.biHeight      = -(INT_32)(lpd_height * lpd_scale_factor);
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = lpd_bpp << 3;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* fb_ptr                = LPD_NULL;
	HBITMAP hbitmap             = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void**)&fb_ptr, 0, 0);
	UINT_64* dst                = (UINT_64*)fb_ptr;
	UINT_64* org                = (UINT_64*)fb;
	INT_32 step                 = LPD_WINDOW_ALIGN(lpd_width * lpd_scale_factor * lpd_height * lpd_scale_factor * lpd_bpp);
	do { *dst++ = *org++; } while (step -= 8);
	return hbitmap;
}

//***********************************************************************************************************

void lpd_blit(HWND hwnd, void* buff)
{
	PAINTSTRUCT ps;
	HDC hdc        = BeginPaint(hwnd, &ps);
	HDC hdcMem     = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, lpd_create_bmp(hdc, buff));
	BitBlt(hdc, 0, 0, lpd_width * lpd_scale_factor, lpd_height * lpd_scale_factor, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);
	EndPaint(hwnd, &ps);
}

//***********************************************************************************************************

LRESULT __stdcall lpd_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_PAINT:
		// decide which blitter function to use
		void(*blitter)(HWND, void*);
		blitter = (lpd_strech) ? &lpd_strech_blit : &lpd_blit;

		// call the blitter function
		blitter(hwnd, lpd_rgb_buffer);

		// break the case
		break;
	case WM_CLOSE: 
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcA(hwnd, message, wParam, lParam);
}

//***********************************************************************************************************

int __stdcall lpd_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
	WNDCLASS wndclass;
	memset(&wndclass, 0, sizeof(WNDCLASS));
	wndclass.lpfnWndProc   = lpd_WndProc;
	wndclass.lpszClassName = "LPD_PNG_DECODER";
	RegisterClass(&wndclass);
	HWND hwnd = CreateWindow(
		wndclass.lpszClassName, 
		"decoded_png",
		WS_SYSMENU | WS_CAPTION , 
		0, 
		0,
		_LPD_WINDOW_WIDTH_EDGE  + lpd_width  * lpd_scale_factor, 
		_LPD_WINDOW_HEIGHT_EDGE + lpd_height * lpd_scale_factor,
		0,
		0,
		0,
		0
	);
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

//***********************************************************************************************************

void lpd_strech_blit(HWND hwnd, void* buffer)
{
	// find out the width and height of the streched image
	UINT_32 new_width  = lpd_width  * lpd_scale_factor;
	UINT_32 new_height = lpd_height * lpd_scale_factor;

	// allocate memory for the streched image
	UINT_8* streched_image = (UINT_8*)lpd_zero_allocation(LPD_WINDOW_ALIGN(new_width * new_height * lpd_bpp));

	// in each row get the pixel r, g, and b values and extend it to a block of (scale_factor * scale_factor)
	UINT_8* image = (UINT_8*)buffer;

	// the height of a scaled block
	UINT_32 block_height = lpd_scale_factor;

	// the row bytes in a scaled block
	UINT_32 block_width_bytes = lpd_scale_factor * lpd_bpp;

	// how many blocks are in a row
	UINT_32 max_horizontal_blocks = lpd_width;

	// make an alias for height
	UINT_32 height = lpd_height;

	// which pixel in unscaled row, we are in
	UINT_32 row_pixels = 0;

	// which row in a scaled block, we are in
	UINT_32 vertical_block_counter = 0;

	// which column in a scaled block, we are in
	UINT_32 horizontal_block_counter = 0;

	// do until all rows are consumed
	while (height--)
	{
		// in a row of unscaled image, go ahead
		while (row_pixels < (lpd_width * lpd_bpp))
		{
			// retrieve R, G, and B
			UINT_8 red = *(image + row_pixels + 0);
			UINT_8 grn = *(image + row_pixels + 1);
			UINT_8 blu = *(image + row_pixels + 2);

			// find out the starting memory of only this scaled block
			UINT_8* this_block_start = streched_image + (((vertical_block_counter * block_height * max_horizontal_blocks) + horizontal_block_counter) * block_width_bytes);

			// define a row counter in this block
			UINT_32 current_block_height = 0;

			// loop until the last row of the current block is reached
			while (current_block_height < block_height)
			{
				// make an alias for the beginning of a row in the current scaled block
				UINT_8* ptr = this_block_start + (current_block_height * max_horizontal_blocks * block_width_bytes);

				// do the actual streching of a single pixel into a row of scaled block
				for (UINT_32 i = 0; i < block_width_bytes; i += lpd_bpp)
				{
					ptr[i + 0] = red;
					ptr[i + 1] = grn;
					ptr[i + 2] = blu;
				}

				// increment the row counter in the current block
				current_block_height++;
			}

			// go one scaled block ahead 
			horizontal_block_counter++;

			// go the next pixel in row of unscaled image
			row_pixels += lpd_bpp;
		}

		// go back to the beginning of a unscaled row
		row_pixels = 0;

		// find the next row memory of the unscaled image
		image += (lpd_width * lpd_bpp);

		// set back the horizontal row counter to zero
		horizontal_block_counter = 0;

		// increment the vertical block counter
		vertical_block_counter++;
	}

	// now the streching is over, so that, blit the new scaled image
	lpd_blit(hwnd, streched_image);
}

//***********************************************************************************************************

void lpd_convert_rgb2bgr(void* buffer, UINT_32 bytes)
{
	// define a byte counter
	UINT_32 i = 0;

	// make an alias for bytes in buffer
	UINT_8* p = (UINT_8*)buffer;

	// swap R and B bytes
	while (i < bytes)
	{
		LPD_RENDERER_SWAP_BYTES(p[i],  p[i + 2]);
		i += lpd_bpp;
	}
}

//***********************************************************************************************************
