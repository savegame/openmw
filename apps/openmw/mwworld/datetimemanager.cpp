#include "datetimemanager.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "duration.hpp"
#include "esmstore.hpp"
#include "globals.hpp"
#include "timestamp.hpp"

namespace
{
    static int getDaysPerMonth(int month)
    {
        switch (month)
        {
            case 0:
                return 31;
            case 1:
                return 28;
            case 2:
                return 31;
            case 3:
                return 30;
            case 4:
                return 31;
            case 5:
                return 30;
            case 6:
                return 31;
            case 7:
                return 31;
            case 8:
                return 30;
            case 9:
                return 31;
            case 10:
                return 30;
            case 11:
                return 31;
        }

        throw std::runtime_error("month out of range");
    }
}

namespace MWWorld
{
    void DateTimeManager::setup(Globals& globalVariables)
    {
        mGameHour = globalVariables[Globals::sGameHour].getFloat();
        mDaysPassed = globalVariables[Globals::sDaysPassed].getInteger();
        mDay = globalVariables[Globals::sDay].getInteger();
        mMonth = globalVariables[Globals::sMonth].getInteger();
        mYear = globalVariables[Globals::sYear].getInteger();
        mTimeScale = globalVariables[Globals::sTimeScale].getFloat();
    }

    void DateTimeManager::setHour(double hour)
    {
        if (hour < 0)
            hour = 0;

        const Duration duration = Duration::fromHours(hour);

        mGameHour = duration.getHours();

        if (const int days = duration.getDays(); days > 0)
            setDay(days + mDay);
    }

    void DateTimeManager::setDay(int day)
    {
        if (day < 1)
            day = 1;

        int month = mMonth;
        while (true)
        {
            int days = getDaysPerMonth(month);
            if (day <= days)
                break;

            if (month < 11)
            {
                ++month;
            }
            else
            {
                month = 0;
                mYear++;
            }

            day -= days;
        }

        mDay = day;
        mMonth = month;
    }

    TimeStamp DateTimeManager::getTimeStamp() const
    {
        return TimeStamp(mGameHour, mDaysPassed);
    }

    float DateTimeManager::getTimeScaleFactor() const
    {
        return mTimeScale;
    }

    ESM::EpochTimeStamp DateTimeManager::getEpochTimeStamp() const
    {
        ESM::EpochTimeStamp timeStamp;
        timeStamp.mGameHour = mGameHour;
        timeStamp.mDay = mDay;
        timeStamp.mMonth = mMonth;
        timeStamp.mYear = mYear;
        return timeStamp;
    }

    void DateTimeManager::setMonth(int month)
    {
        if (month < 0)
            month = 0;

        int years = month / 12;
        month = month % 12;

        int days = getDaysPerMonth(month);
        if (mDay > days)
            mDay = days;

        mMonth = month;

        if (years > 0)
            mYear += years;
    }

    void DateTimeManager::advanceTime(double hours, Globals& globalVariables)
    {
        hours += mGameHour;
        setHour(hours);

        int days = static_cast<int>(hours / 24);
        if (days > 0)
            mDaysPassed += days;

        globalVariables[Globals::sGameHour].setFloat(mGameHour);
        globalVariables[Globals::sDaysPassed].setInteger(mDaysPassed);
        globalVariables[Globals::sDay].setInteger(mDay);
        globalVariables[Globals::sMonth].setInteger(mMonth);
        globalVariables[Globals::sYear].setInteger(mYear);
    }

    std::string_view DateTimeManager::getMonthName(int month) const
    {
        if (month == -1)
            month = mMonth;

        const int months = 12;
        if (month < 0 || month >= months)
            return {};

        static const std::string_view monthNames[months] = { "sMonthMorningstar", "sMonthSunsdawn", "sMonthFirstseed",
            "sMonthRainshand", "sMonthSecondseed", "sMonthMidyear", "sMonthSunsheight", "sMonthLastseed",
            "sMonthHeartfire", "sMonthFrostfall", "sMonthSunsdusk", "sMonthEveningstar" };

        const ESM::GameSetting* setting
            = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(monthNames[month]);
        return setting->mValue.getString();
    }

    bool DateTimeManager::updateGlobalFloat(GlobalVariableName name, float value)
    {
        if (name == Globals::sGameHour)
        {
            setHour(value);
            return true;
        }
        else if (name == Globals::sDay)
        {
            setDay(static_cast<int>(value));
            return true;
        }
        else if (name == Globals::sMonth)
        {
            setMonth(static_cast<int>(value));
            return true;
        }
        else if (name == Globals::sYear)
        {
            mYear = static_cast<int>(value);
        }
        else if (name == Globals::sTimeScale)
        {
            mTimeScale = value;
        }
        else if (name == Globals::sDaysPassed)
        {
            mDaysPassed = static_cast<int>(value);
        }

        return false;
    }

    bool DateTimeManager::updateGlobalInt(GlobalVariableName name, int value)
    {
        if (name == Globals::sGameHour)
        {
            setHour(static_cast<float>(value));
            return true;
        }
        else if (name == Globals::sDay)
        {
            setDay(value);
            return true;
        }
        else if (name == Globals::sMonth)
        {
            setMonth(value);
            return true;
        }
        else if (name == Globals::sYear)
        {
            mYear = value;
        }
        else if (name == Globals::sTimeScale)
        {
            mTimeScale = static_cast<float>(value);
        }
        else if (name == Globals::sDaysPassed)
        {
            mDaysPassed = value;
        }

        return false;
    }
}
