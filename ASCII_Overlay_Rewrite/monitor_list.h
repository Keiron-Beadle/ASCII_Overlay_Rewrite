#pragma once
#include <winrt/base.h>

#include "OpenGLWindow.h"

struct monitor_info
{
	explicit monitor_info(const HMONITOR monitorHandle)
    {
        monitor_handle = monitorHandle;
        MONITORINFOEX monitorInfo = { sizeof(monitorInfo) };
        winrt::check_bool(GetMonitorInfo(monitor_handle, &monitorInfo));
        const std::wstring displayName(monitorInfo.szDevice);
        display_name = displayName;
    }
    monitor_info(HMONITOR monitorHandle, std::wstring const& displayName)
    {
        monitor_handle = monitorHandle;
        display_name = displayName;
    }

    HMONITOR monitor_handle;
    std::wstring display_name;

    bool operator==(const monitor_info& monitor) const { return monitor_handle == monitor.monitor_handle; }
    bool operator!=(const monitor_info& monitor) const { return !(*this == monitor); }
};

class monitor_list
{
public:
    monitor_list(bool includeAllMonitors);

    void Update();
    const std::vector<monitor_info> GetCurrentMonitors() const { return m_monitors; }

private:

private:
    std::vector<monitor_info> m_monitors;
    bool m_includeAllMonitors = false;
};
