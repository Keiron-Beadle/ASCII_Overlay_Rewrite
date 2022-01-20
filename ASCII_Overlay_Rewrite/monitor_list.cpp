#include "monitor_list.h"

std::vector<monitor_info> EnumerateAllMonitors(bool includeAllMonitors)
{
    std::vector<monitor_info> monitors;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
        {
            auto& monitors = *reinterpret_cast<std::vector<monitor_info>*>(lparam);
            monitors.push_back(monitor_info(hmon));

            return TRUE;
        }, reinterpret_cast<LPARAM>(&monitors));
    if (monitors.size() > 1 && includeAllMonitors)
    {
        monitors.push_back(monitor_info(nullptr, L"All Displays"));
    }
    return monitors;
}

monitor_list::monitor_list(bool includeAllMonitors)
{
    m_includeAllMonitors = includeAllMonitors;
    m_monitors = EnumerateAllMonitors(m_includeAllMonitors);
}

void monitor_list::Update()
{
    auto monitors = EnumerateAllMonitors(m_includeAllMonitors);
    std::map<HMONITOR, monitor_info> newMonitors;
    for (auto& monitor : monitors)
    {
        newMonitors.insert({ monitor.monitor_handle, monitor });
    }

    std::vector<int> monitorIndexesToRemove;
    auto index = 0;
    for (auto& monitor : m_monitors)
    {
        auto search = newMonitors.find(monitor.monitor_handle);
        if (search == newMonitors.end())
        {
            monitorIndexesToRemove.push_back(index);
        }
        else
        {
            newMonitors.erase(search);
        }
        index++;
    }

    // Remove old monitors
    std::sort(monitorIndexesToRemove.begin(), monitorIndexesToRemove.end(), std::greater<int>());
    for (const auto& removalIndex : monitorIndexesToRemove)
    {
        m_monitors.erase(m_monitors.begin() + removalIndex);
    }

    // Add new monitors
    for (const auto& pair : newMonitors)
    {
        auto monitor = pair.second;
        m_monitors.push_back(monitor);
    }
}