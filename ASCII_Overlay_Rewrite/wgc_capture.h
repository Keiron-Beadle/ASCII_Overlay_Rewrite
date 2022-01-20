#pragma once
#include <Unknwn.h>
#include <inspectable.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <mutex>

#include "monitor_list.h"

class wgc_capture
{
public:
	wgc_capture();

	~wgc_capture() { close(); }

	void start_capture();

	winrt::Windows::UI::Composition::ICompositionSurface create_surface(
		winrt::Windows::UI::Composition::Compositor const& compositor);

	void set_pixel_format(winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixel_format)
	{
		CheckClosed();
		m_lock.lock();
		m_pixelFormatUpdate = pixel_format;
		m_lock.unlock();
	}

	bool is_cursor_enabled() { CheckClosed(); return m_session.IsCursorCaptureEnabled(); }
	void init(std::string& ascii_text, std::mutex& ascii_mutex);
	void close();
private:
	void on_frame_arrived(
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
		winrt::Windows::Foundation::IInspectable const& args);

	void start_capture_from_item(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item);
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem try_start_capture_from_hmonitor(HMONITOR handle);
	void resize_swap_chain() const;
	bool try_resize_swap_chain(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame);
	bool try_update_pixel_format();
	void parse_data_for_ascii_text(D3D11_MAPPED_SUBRESOURCE& data);

private:
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
	winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
	//winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
	winrt::Windows::Graphics::SizeInt32 m_lastSize;

	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };

	ID3D11Texture2D *m_cpu_tex = NULL;
	winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
	winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
	winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat;

	std::string* ascii_text;
	std::unique_ptr<monitor_list> m_monitors;
	std::unique_lock<std::mutex> m_lock;
	std::optional<winrt::Windows::Graphics::DirectX::DirectXPixelFormat> m_pixelFormatUpdate = std::nullopt;

	std::atomic<bool> m_closed = false;
	std::atomic<bool> m_captureNextImage = false;

private:
	static inline auto get_grey_value(const unsigned char* const data, const int row, const int stride, const int col)
	{
		const int row_base = row * stride;
		const int blue = data[row_base + col];
		const int green = data[row_base + col + 1];
		const int red = data[row_base + col + 2];
		const int grey = (red * 0.299) / 3 + (green * 0.587) / 3 + (blue * 0.114) / 3;
		return constants::brightness * grey;
	}
	inline auto CreateDirect3DDevice(IDXGIDevice* dxgi_device)
	{
		winrt::com_ptr<::IInspectable> d3d_device;
		winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, d3d_device.put()));
		return d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
	}
	inline void CheckClosed() const
	{
		if (m_closed.load() == true)
		{
			throw winrt::hresult_error(RO_E_CLOSED);
		}
	}
	inline auto create_capture_item_for_monitor(HMONITOR handle)
	{
		const auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
		winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
		winrt::check_hresult(interop_factory->CreateForMonitor(handle, winrt::guid("79c3f95b-31f7-4ec2-a464-632ef5d30760"), winrt::put_abi(item)));
		return item;
	}
};

