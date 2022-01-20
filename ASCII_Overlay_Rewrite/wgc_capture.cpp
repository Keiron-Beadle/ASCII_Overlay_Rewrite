#include "wgc_capture.h"

#include "d3dHelpers.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::Graphics;
    using namespace Capture;
    using namespace DirectX;
    using namespace Direct3D11;
    using namespace Numerics;
    using namespace Windows::UI;
    using namespace Composition;
}

template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
	const auto access = object.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

wgc_capture::wgc_capture()
{
    auto ddDevice = util::uwp::CreateD3DDevice();
    auto dxgiDevice = ddDevice.as<IDXGIDevice>();
    m_device = CreateDirect3DDevice(dxgiDevice.get());
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    d3dDevice->GetImmediateContext(m_d3dContext.put());
    m_pixelFormat = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R8G8B8A8UIntNormalized;

    m_monitors = std::make_unique<monitor_list>(true);
    const auto monitor_to_record = m_monitors->GetCurrentMonitors()[1];

    m_item = create_capture_item_for_monitor(monitor_to_record.monitor_handle);

	D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = static_cast<uint32_t>(m_item.Size().Width);
    texDesc.Height = static_cast<uint32_t>(m_item.Size().Height);
    texDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.BindFlags = NULL;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texDesc.MiscFlags = NULL;
    texDesc.Usage = D3D11_USAGE_STAGING;

    ddDevice->CreateTexture2D(&texDesc, 0, &m_cpu_tex);

	m_swapChain = util::uwp::CreateDXGISwapChain(d3dDevice, static_cast<uint32_t>(m_item.Size().Width),
        static_cast<uint32_t>(m_item.Size().Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat),2);

    winrt::SizeInt32 itemSize(m_item.Size());
    m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, 
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R8G8B8A8UIntNormalized, 1, itemSize);
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();
    m_framePool.FrameArrived({ this, &wgc_capture::on_frame_arrived });
    WINRT_ASSERT(m_session != nullptr);

}

void wgc_capture::start_capture()
{
    CheckClosed();
    m_session.StartCapture();
}

winrt::Windows::UI::Composition::ICompositionSurface wgc_capture::create_surface(
	winrt::Windows::UI::Composition::Compositor const& compositor)
{
    CheckClosed();
    return util::uwp::CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

void wgc_capture::init(std::string& ascii_text, std::mutex& ascii_mutex)
{
    //Initialise recording
    this->ascii_text = &ascii_text;
    m_lock = std::unique_lock(ascii_mutex, std::defer_lock);

    start_capture_from_item(m_item);
}

void wgc_capture::close()
{
    bool expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_session.Close();
        m_framePool.Close();

        m_swapChain = nullptr;
        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void wgc_capture::on_frame_arrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
	winrt::Windows::Foundation::IInspectable const& args)
{
    bool swapChainResizedToFrame = false;
    {
        auto frame = sender.TryGetNextFrame();
        swapChainResizedToFrame = try_resize_swap_chain(frame);
        winrt::com_ptr<ID3D11Texture2D> backBuffer;
        winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
        auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());
        m_d3dContext->CopyResource(m_cpu_tex, surfaceTexture.get());
        D3D11_MAPPED_SUBRESOURCE cpu_subresource;
        m_d3dContext->Map(m_cpu_tex, 0, D3D11_MAP_READ, NULL, &cpu_subresource);

        m_lock.lock();
        parse_data_for_ascii_text(cpu_subresource);
        m_lock.unlock();

    }
    //DXGI_PRESENT_PARAMETERS present_parameters{};
    //m_swapChain->Present1(1, 0, &present_parameters);
    //swapChainResizedToFrame = swapChainResizedToFrame || try_update_pixel_format();

    //if (swapChainResizedToFrame)
    //{
    //    m_framePool.Recreate(m_device, m_pixelFormat, 2, m_lastSize);
    //}
}

void wgc_capture::start_capture_from_item(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item)
{
    start_capture();
    //auto surface = create_surface(m_compositor);
    
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem wgc_capture::try_start_capture_from_hmonitor(HMONITOR handle)
{
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
    try
    {
        item = create_capture_item_for_monitor(handle);
        start_capture_from_item(item);
    }
    catch (winrt::hresult_error const& error)
    {
        std::cout << "Error in try start capture for monitor";
    }
    return item;
}

void wgc_capture::resize_swap_chain() const
{
    winrt::check_hresult(m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width),
        static_cast<uint32_t>(m_lastSize.Height), static_cast<DXGI_FORMAT>(m_pixelFormat), 0));
}

bool wgc_capture::try_resize_swap_chain(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame)
{
    auto const contentSize = frame.ContentSize();
    if ((contentSize.Width != m_lastSize.Width) ||
        (contentSize.Height != m_lastSize.Height))
    {
        m_lastSize = contentSize;
        resize_swap_chain();
        return true;
    }
    return false;
}

bool wgc_capture::try_update_pixel_format()
{
    m_lock.lock();
    if (m_pixelFormatUpdate.has_value())
    {
        auto pixelFormat = m_pixelFormatUpdate.value();
        m_pixelFormatUpdate = std::nullopt;
        if (pixelFormat != m_pixelFormat)
        {
            m_pixelFormat = pixelFormat;
            resize_swap_chain();
            m_lock.unlock();
            return true;
        }
    }
    m_lock.unlock();
    return false;
}

void wgc_capture::parse_data_for_ascii_text(D3D11_MAPPED_SUBRESOURCE& resource)
{
    const unsigned char* data = static_cast<unsigned char*>(resource.pData);
    //For automatically adjusting brightness I keep a count of some characters, so I can adjust
//brightness to keep a ratio of these on-screen, start at 1 so no divide by 0 error
    int space_counter = 1;
    int period_counter = 1;

    ascii_text->clear();
    ascii_text->reserve(18000);
    constexpr int stride = (constants::capture_width * (32 / 8) + 3) & ~3;
    for (int row = 1; row < constants::capture_height; row++) {
        for (int col = 1; col < stride; col += 4) {

            //This part averages pixels from 4 ordinal directions and the middle to get an average brightness of that area
            constexpr int ascii_size = sizeof(constants::ascii_scale);
            const int grey_up = get_grey_value(data, row - 1, stride, col);
            const int grey_down = get_grey_value(data, row + 1, stride, col);
            const int grey_left = get_grey_value(data, row, stride, col - 4);
            const int grey_mid = get_grey_value(data, row, stride, col);
            const int grey_right = get_grey_value(data, row, stride, col + 4);
            const int avg_grey = (grey_left + grey_mid + grey_right + grey_up + grey_down) / 5;
            const char pixel_value = constants::ascii_scale[avg_grey / (ascii_size - 1)];
            if (pixel_value == '\0') { continue; }
            else if (pixel_value == ' ')
            {
                space_counter++;
            }
            else if (pixel_value == '.') { period_counter++; }

            ascii_text->push_back(pixel_value);
            col += constants::capture_pixelsize;
        }
        row += constants::capture_pixelsize;
        ascii_text->push_back(-1);
    }
    //adjust_brightness(space_counter, period_counter);
}


