// filepath: png2ico_gdiplus.cpp
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <memory>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

class ImageConverter {
private:
    static const std::vector<int> TARGET_SIZES;
    ULONG_PTR gdiplusToken;

public:
    ImageConverter() {
        GdiplusStartupInput startupInput;
        GdiplusStartup(&gdiplusToken, &startupInput, nullptr);
    }
    ~ImageConverter() {
        GdiplusShutdown(gdiplusToken);
    }

    // Crop the source image to a centered square (keep center region).
    std::unique_ptr<Bitmap> MakeSquareFromCenter(Bitmap* source) {
        UINT w = source->GetWidth();
        UINT h = source->GetHeight();
        UINT squareSize = (w < h ? w : h);
        UINT startX = (w - squareSize) / 2;
        UINT startY = (h - squareSize) / 2;

        auto result = std::make_unique<Bitmap>(squareSize, squareSize, PixelFormat32bppARGB);
        Graphics g(result.get());
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetCompositingQuality(CompositingQualityHighQuality);
        Rect targetRect(0, 0, squareSize, squareSize);
        g.DrawImage(source, targetRect, startX, startY, squareSize, squareSize, UnitPixel);
        return result;
    }

    // Resize the (square) image to targetSize x targetSize.
    std::unique_ptr<Bitmap> CreateResizedVersion(Bitmap* source, int targetSize) {
        auto result = std::make_unique<Bitmap>(targetSize, targetSize, PixelFormat32bppARGB);
        Graphics g(result.get());
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetCompositingQuality(CompositingQualityHighQuality);
        g.SetSmoothingMode(SmoothingModeHighQuality);
        Rect targetRect(0, 0, targetSize, targetSize);
        g.DrawImage(source, targetRect);
        return result;
    }

    // Encode a GDI+ Bitmap into an in-memory PNG byte array.
    std::vector<BYTE> ConvertToPngBytes(Bitmap* image) {
        CLSID encoderClsid;
        FindPngEncoder(&encoderClsid);

        IStream* memoryStream = nullptr;
        CreateStreamOnHGlobal(nullptr, TRUE, &memoryStream);

        image->Save(memoryStream, &encoderClsid, nullptr);

        STATSTG stats;
        memoryStream->Stat(&stats, STATFLAG_NONAME);
        std::vector<BYTE> data(static_cast<size_t>(stats.cbSize.QuadPart));

        LARGE_INTEGER seekPos = { 0 };
        memoryStream->Seek(seekPos, STREAM_SEEK_SET, nullptr);

        ULONG bytesRead = 0;
        memoryStream->Read(data.data(), static_cast<ULONG>(data.size()), &bytesRead);
        memoryStream->Release();
        return data;
    }

    // Convert one PNG file to a multi-size ICO file.
    bool ProcessPngToIco(const std::wstring& inputFile, const std::wstring& outputFile) {
        auto sourceImage = std::make_unique<Bitmap>(inputFile.c_str());
        if (sourceImage->GetLastStatus() != Ok) {
            std::wcerr << L"Load failed: " << inputFile << std::endl;
            return false;
        }

        auto squareImage = MakeSquareFromCenter(sourceImage.get());
        std::vector<std::vector<BYTE>> imageDataList;
        imageDataList.reserve(TARGET_SIZES.size());

        for (int size : TARGET_SIZES) {
            auto resized = CreateResizedVersion(squareImage.get(), size);
            imageDataList.push_back(ConvertToPngBytes(resized.get()));
        }
        return WriteIcoFile(imageDataList, outputFile);
    }

private:
    // Locate the PNG encoder CLSID.
    void FindPngEncoder(CLSID* clsid) {
        UINT count = 0, bytes = 0;
        GetImageEncodersSize(&count, &bytes);
        std::vector<BYTE> buffer(bytes);
        ImageCodecInfo* info = reinterpret_cast<ImageCodecInfo*>(buffer.data());
        GetImageEncoders(count, bytes, info);
        for (UINT i = 0; i < count; ++i) {
            if (wcscmp(info[i].MimeType, L"image/png") == 0) {
                *clsid = info[i].Clsid;
                return;
            }
        }
    }

    // Write ICO file with PNG-compressed icon images.
    bool WriteIcoFile(const std::vector<std::vector<BYTE>>& imageList, const std::wstring& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        struct {
            uint16_t reserved = 0;
            uint16_t type = 1;
            uint16_t count;
        } header;
        header.count = static_cast<uint16_t>(imageList.size());
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        size_t dataStart = sizeof(header) + imageList.size() * 16;
        size_t offset = dataStart;

        for (size_t i = 0; i < imageList.size(); ++i) {
            struct {
                uint8_t width;
                uint8_t height;
                uint8_t colorCount = 0;
                uint8_t reserved = 0;
                uint16_t planes = 1;
                uint16_t bitCount = 32;
                uint32_t imageSize;
                uint32_t imageOffset;
            } entry;
            int size = TARGET_SIZES[i];
            entry.width = (size >= 256) ? 0 : static_cast<uint8_t>(size);
            entry.height = (size >= 256) ? 0 : static_cast<uint8_t>(size);
            entry.imageSize = static_cast<uint32_t>(imageList[i].size());
            entry.imageOffset = static_cast<uint32_t>(offset);
            file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
            offset += imageList[i].size();
        }

        for (auto const& data : imageList) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
        return file.good();
    }
};

const std::vector<int> ImageConverter::TARGET_SIZES = {16, 24, 32, 48, 64, 128, 256};

// Get the directory where the executable resides (with trailing slash).
static std::wstring GetExeDirectory() {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring full(path, len);
    size_t pos = full.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? L".\\" : full.substr(0, pos + 1);
}

// Check if file name ends with ".png" (case-insensitive).
static bool HasPngExtension(const std::wstring& name) {
    size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    std::wstring ext = name.substr(dot + 1);
    for (auto& c : ext) c = towlower(c);
    return ext == L"png";
}

// Replace .png extension with .ico (or append if none).
static std::wstring ReplaceWithIco(const std::wstring& pngPath) {
    size_t dot = pngPath.find_last_of(L'.');
    if (dot == std::wstring::npos) return pngPath + L".ico";
    return pngPath.substr(0, dot) + L".ico";
}

int wmain(int argc, wchar_t* argv[]) {
    ImageConverter converter;

    // No arguments: convert all PNG files in executable directory.
    if (argc == 1) {
        std::wstring dir = GetExeDirectory();
        std::wstring pattern = dir + L"*.png";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) {
            std::wcout << L"No PNG files found in: " << dir << std::endl;
            std::wcout << L"Usage: " << argv[0] << L" <file.png>" << std::endl;
            return 1;
        }

        int total = 0, ok = 0;
        std::wcout << L"Batch converting PNG files in: " << dir << std::endl;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && HasPngExtension(fd.cFileName)) {
                std::wstring inFile = dir + fd.cFileName;
                std::wstring outFile = ReplaceWithIco(inFile);
                std::wcout << L"[Convert] " << inFile << L" -> " << outFile << std::endl;
                ++total;
                if (converter.ProcessPngToIco(inFile, outFile)) {
                    std::wcout << L"  OK" << std::endl;
                    ++ok;
                } else {
                    std::wcout << L"  FAIL" << std::endl;
                }
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);

        std::wcout << L"Done. Total: " << total << L", Success: " << ok << L", Failed: " << (total - ok) << std::endl;
        if (total == 0) return 1;
        return (ok == total) ? 0 : (ok > 0 ? 2 : 1);
    }

    // Mode 2: One argument -> convert that single PNG, output name auto (.ico).
    if (argc == 2) {
        std::wstring input = argv[1];
        if (!HasPngExtension(input)) {
            std::wcout << L"Input must be a .png file." << std::endl;
            return 1;
        }
        std::wstring output = ReplaceWithIco(input);
        std::wcout << L"Converting: " << input << L" -> " << output << std::endl;
        if (converter.ProcessPngToIco(input, output)) {
            std::wcout << L"Success: " << output << std::endl;
            return 0;
        } else {
            std::wcout << L"Failed." << std::endl;
            return 1;
        }
    }

    // Invalid argument count: show usage.
    std::wcout << L"Usage:\n  " << argv[0] << L"\n    Convert all PNGs in exe directory.\n  " << argv[0]
               << L" <file.png>\n    Convert single PNG (output auto: same name .ico)" << std::endl;
    return 1;
}