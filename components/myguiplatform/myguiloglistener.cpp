#include "myguiloglistener.hpp"

#include <iomanip>

#include <components/debug/debuglog.hpp>

namespace osgMyGUI
{
    void CustomLogListener::open()
    {
        mStream.open(mFileName, std::ios_base::out);
        if (!mStream.is_open())
            Log(Debug::Error) << "Unable to create MyGUI log with path " << mFileName;
    }

    void CustomLogListener::close()
    {
        if (mStream.is_open())
            mStream.close();
    }

    void CustomLogListener::flush()
    {
        if (mStream.is_open())
            mStream.flush();
    }

    void CustomLogListener::log(const std::string& _section, MyGUI::LogLevel _level, const struct tm* _time,
        const std::string& _message, const char* _file, int _line)
    {
        if (mStream.is_open())
        {
            const char* separator = "  |  ";
            mStream << std::setw(2) << std::setfill('0') << _time->tm_hour << ":" << std::setw(2) << std::setfill('0')
                    << _time->tm_min << ":" << std::setw(2) << std::setfill('0') << _time->tm_sec << separator
                    << _section << separator << _level.print() << separator << _message << separator << _file
                    << separator << _line << std::endl;
        }
    }

    MyGUI::LogLevel LogFacility::getCurrentLogLevel() const
    {
        switch (Debug::CurrentDebugLevel)
        {
            case Debug::Error:
                return MyGUI::LogLevel::Error;
            case Debug::Warning:
                return MyGUI::LogLevel::Warning;
            default:
                return MyGUI::LogLevel::Info;
        }
    }
}
