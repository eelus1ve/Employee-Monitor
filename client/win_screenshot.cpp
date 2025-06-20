#include "win_screenshot.h"

std::vector<BYTE> CaptureScreenToMemory() {
    HWND hDesktopWnd = GetDesktopWindow();
    HDC hDesktopDC = GetDC(hDesktopWnd);
    HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, width, height);
    SelectObject(hCaptureDC, hCaptureBitmap);
    BitBlt(hCaptureDC, 0, 0, width, height, hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);

    BITMAP bmp;
    GetObject(hCaptureBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    int bmpSize = ((bmp.bmWidth * 3 + 3) & ~3) * bmp.bmHeight;
    std::vector<BYTE> bmpBuffer(bmpSize);

    GetDIBits(hCaptureDC, hCaptureBitmap, 0, bmp.bmHeight, bmpBuffer.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    bmfHeader.bfType = 0x4D42; // 'BM'
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpBuffer.size();
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    std::vector<BYTE> output;
    output.resize(bmfHeader.bfSize);

    memcpy(output.data(), &bmfHeader, sizeof(bmfHeader));
    memcpy(output.data() + sizeof(bmfHeader), &bi, sizeof(bi));
    memcpy(output.data() + sizeof(bmfHeader) + sizeof(bi), bmpBuffer.data(), bmpBuffer.size());

    DeleteObject(hCaptureBitmap);
    DeleteDC(hCaptureDC);
    ReleaseDC(hDesktopWnd, hDesktopDC);

    return output;
}
