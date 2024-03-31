//
// Created by Soren on 16/03/2024.
//

#include "WICTextureWriter.h"
#include <comdef.h>
#include <format>

WICTextureWriter::WICTextureWriter() {
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&factory);
    if (SUCCEEDED(hr))
    {
        hr = factory->CreateStream(&stream);
    }


    if (FAILED(hr))
    {
        _com_error error {hr};
        LPCSTR error_message = error.ErrorMessage();

        auto error_format = std::format("WICTextureWriter Initialisation Failed, Code: {0}", error_message);
        throw std::runtime_error(error_format);
    }
}

GUID WICTextureWriter::format_from_extension(std::filesystem::path out_path) {
    if (out_path.has_extension())
    {
        auto extension = out_path.extension();
        if (extension == L".png")
        {
            return GUID_ContainerFormatPng;
        }
        else if (extension == L".tif" || extension == L"tiff")
        {
            return GUID_ContainerFormatTiff;
        }
        else if (extension == std::filesystem::path{L".dds"})
        {
            return GUID_ContainerFormatDds;
        }
        else {
            return GUID_NULL;
        }
    }
    return GUID_NULL;
}

HRESULT WICTextureWriter::write_texture(std::filesystem::path out_path, size_t width, size_t height, size_t stride, size_t size,
                                        WICPixelFormatGUID in_px_format, WICPixelFormatGUID out_px_format, const void* buffer)
{
    if (buffer == nullptr)
    {
        return E_INVALIDARG;
    }

    GUID extension_id = format_from_extension(out_path);


    HRESULT hr = stream->InitializeFromFilename(out_path.wstring().c_str(), GENERIC_WRITE);
    if (SUCCEEDED(hr))
    {
        if (extension_id == GUID_NULL)
        {
            //throw std::invalid_argument("WICTextureWriter: Output filetype not supported.");
            return ERROR_FILE_NOT_SUPPORTED;
        }


        hr = factory->CreateEncoder(extension_id, nullptr, &encoder);
    }
    if (SUCCEEDED(hr))
    {
        hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    }

    if (SUCCEEDED(hr))
    {
        hr = encoder->CreateNewFrame(&bitmap_frame, &property_bag);
    }
    if (SUCCEEDED(hr))
    {
        if (extension_id == GUID_ContainerFormatTiff)
        {
            hr = TIFF_writer_initialise();

        }
        else if (extension_id == GUID_ContainerFormatPng)
        {
            hr = PNG_writer_initialise();
        }


        if (SUCCEEDED(hr))
        {
            hr = bitmap_frame->Initialize(property_bag.Get());

        }
        //write texture

        if (SUCCEEDED(hr))
        {
            hr = bitmap_frame->SetSize(width, height);
        }
        //if this != px_format after SetPixelFormat, then we must perform a conversion operation.
        WICPixelFormatGUID format = out_px_format;

        if (SUCCEEDED(hr))
        {

            hr = bitmap_frame->SetPixelFormat(&format);
#if DEBUG
            wchar_t short_name[256] = {0};
            uint32_t out_size = 0;
            HRESULT shortnameresult = WICMapGuidToShortName(format, 256, short_name, &out_size);
            printf("%lx: Output texture converted format is%s the same as out_px_format.\n",
                   shortnameresult, IsEqualGUID(format, out_px_format) ? "" : " not");
#endif
        }
        ComPtr<IWICBitmap> bit_map;
        hr = factory->CreateBitmapFromMemory(width, height, in_px_format, stride, size, (BYTE*)buffer, bit_map.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }


        if (SUCCEEDED(hr))
        {

            //SetPixelFormat places the format it's writing into its inoutparam, make sure it's the same
            //as what we gave it.
            if (!IsEqualGUID(in_px_format, format))
            {

                ComPtr<IWICFormatConverter> format_converter;
                hr = factory->CreateFormatConverter(format_converter.GetAddressOf());
                if (FAILED(hr))
                {
                    return hr;
                }
                ComPtr<IWICPalette> palette;
                bit_map->CopyPalette(palette.Get());
                hr = format_converter->Initialize(bit_map.Get(), format,  WICBitmapDitherTypeNone, palette.Get(), 0, WICBitmapPaletteTypeMedianCut);
                if (FAILED(hr))
                {
                    return hr;
                }
            }

        }

        if (SUCCEEDED(hr))
        {
            bitmap_frame->WriteSource(bit_map.Get(), nullptr);
        }

        if (SUCCEEDED(hr))
        {
            hr = bitmap_frame->Commit();
        }

        if (SUCCEEDED(hr))
        {
            hr = encoder->Commit();
        }
    }
    return hr;

}

HRESULT WICTextureWriter::TIFF_writer_initialise() {
    PROPBAG2 option = {0};
    OLECHAR option_name[] = L"TiffCompressionMethod";
    option.pstrName = option_name;
    VARIANT variant_value;
    VariantInit(&variant_value);
    variant_value.vt = VT_UI1;
    variant_value.bVal = WICTiffCompressionZIP;
    HRESULT hr = property_bag->Write(1, &option, &variant_value);

    return hr;

}

HRESULT WICTextureWriter::PNG_writer_initialise() {
    PROPBAG2 option = {0};
    OLECHAR option_name[] = L"InterlaceOption";
    option.pstrName = option_name;
    VARIANT variant_value;
    VariantInit(&variant_value);
    variant_value.vt = VT_BOOL;
    variant_value.boolVal = false;


    return S_OK;
}
