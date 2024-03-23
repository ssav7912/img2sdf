//
// Created by Soren on 16/03/2024.
//

#ifndef IMG2SDF_WICTEXTUREWRITER_H
#define IMG2SDF_WICTEXTUREWRITER_H
#include <wincodec.h>
#include <wincodecsdk.h>
#include <wrl.h>
#include <filesystem>
using namespace Microsoft::WRL;


class WICTextureWriter
{
public:
    WICTextureWriter();

    static GUID format_from_extension(std::filesystem::path out_path);

    ///Writes a strided DX resource to a file using WIC. Performs format conversions if desired pixel_format unsupported
    ///by file extension given.
    ///@param out_path the output file name & location.
    ///@param width the width of the resource in pixels
    ///@param height the height of the resource in pixels
    ///@param stride the 'stride' of the data, in bytes, from one line to the next.
    ///@param size the size of the resource, in bytes.
    ///@param px_format the desired output pixel format. This must match the format the resource is in. If the format is unsupported
    ///by the output container, a conversion is made to the 'nearest' type, which may be lossy.
    ///@param buffer pointer to raw pixel data.
    HRESULT write_texture(std::filesystem::path out_path, size_t width, size_t height, size_t stride, size_t size,
                          WICPixelFormatGUID px_format, const void* buffer);
private:
    ComPtr<IWICImagingFactory> factory = nullptr;
    ComPtr<IWICBitmapEncoder> encoder = nullptr;
    ComPtr<IWICBitmapFrameEncode> bitmap_frame = nullptr;
    ComPtr<IPropertyBag2> property_bag = nullptr;

    ComPtr<IWICStream> stream = nullptr;

    HRESULT TIFF_writer_initialise();
    HRESULT PNG_writer_initialise();
};

#endif //IMG2SDF_WICTEXTUREWRITER_H
