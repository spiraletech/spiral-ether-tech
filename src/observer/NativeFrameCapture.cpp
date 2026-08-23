#include "observer/NativeFrameCapture.hpp"

#include <SDL3/SDL.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <propidl.h>
#include <gdiplus.h>

#include <vector>
#endif

namespace hakui::observer {

#if defined(_WIN32)
namespace {

bool pngEncoderClsid(CLSID& result)
{
    UINT count = 0;
    UINT bytes = 0;
    if (Gdiplus::GetImageEncodersSize(&count, &bytes) != Gdiplus::Ok ||
        bytes == 0) {
        return false;
    }
    std::vector<unsigned char> storage(bytes);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(storage.data());
    if (Gdiplus::GetImageEncoders(count, bytes, codecs) != Gdiplus::Ok) {
        return false;
    }
    for (UINT index = 0; index < count; ++index) {
        if (codecs[index].MimeType &&
            wcscmp(codecs[index].MimeType, L"image/png") == 0) {
            result = codecs[index].Clsid;
            return true;
        }
    }
    return false;
}

} // namespace
#endif

bool captureWindowPng(
    SDL_Window* window,
    const std::filesystem::path& destination,
    std::string& error
)
{
#if defined(_WIN32)
    if (!window) {
        error = "native frame capture received no SDL window";
        return false;
    }
    auto* hwnd = static_cast<HWND>(SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr
    ));
    if (!hwnd) {
        error = "SDL did not expose a Win32 window handle";
        return false;
    }

    RECT client{};
    if (!GetClientRect(hwnd, &client)) {
        error = "could not inspect client frame bounds";
        return false;
    }
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (width <= 0 || height <= 0) {
        error = "client frame is not visible";
        return false;
    }

    HDC windowDc = GetDC(hwnd);
    HDC memoryDc = windowDc ? CreateCompatibleDC(windowDc) : nullptr;
    HBITMAP bitmap = windowDc ? CreateCompatibleBitmap(windowDc, width, height) : nullptr;
    if (!windowDc || !memoryDc || !bitmap) {
        if (bitmap) DeleteObject(bitmap);
        if (memoryDc) DeleteDC(memoryDc);
        if (windowDc) ReleaseDC(hwnd, windowDc);
        error = "could not allocate native frame surface";
        return false;
    }

    HGDIOBJ previous = SelectObject(memoryDc, bitmap);
    constexpr UINT captureFlags = 0x00000001U | 0x00000002U;
    BOOL copied = PrintWindow(hwnd, memoryDc, captureFlags);
    if (!copied) {
        copied = BitBlt(
            memoryDc, 0, 0, width, height,
            windowDc, 0, 0, SRCCOPY | CAPTUREBLT
        );
    }
    SelectObject(memoryDc, previous);
    DeleteDC(memoryDc);
    ReleaseDC(hwnd, windowDc);
    if (!copied) {
        DeleteObject(bitmap);
        error = "native frame copy failed";
        return false;
    }

    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok) {
        DeleteObject(bitmap);
        error = "GDI+ startup failed";
        return false;
    }
    CLSID encoder{};
    bool saved = false;
    if (pngEncoderClsid(encoder)) {
        Gdiplus::Bitmap image(bitmap, nullptr);
        saved = image.Save(destination.wstring().c_str(), &encoder, nullptr) ==
            Gdiplus::Ok;
    }
    Gdiplus::GdiplusShutdown(token);
    DeleteObject(bitmap);
    if (!saved) {
        error = "could not encode FrameSnapshot.png";
        return false;
    }
    return true;
#else
    (void)window;
    (void)destination;
    error = "native frame capture is not implemented on this platform";
    return false;
#endif
}

} // namespace hakui::observer
