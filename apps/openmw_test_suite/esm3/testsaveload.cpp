#include <components/esm/fourcc.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/player.hpp>
#include <components/esm3/quickkeys.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <random>
#include <type_traits>

namespace ESM
{
    namespace
    {
        auto tie(const ContItem& value)
        {
            return std::tie(value.mCount, value.mItem);
        }

        auto tie(const ESM::Region::SoundRef& value)
        {
            return std::tie(value.mSound, value.mChance);
        }

        auto tie(const ESM::QuickKeys::QuickKey& value)
        {
            return std::tie(value.mType, value.mId);
        }
    }

    inline bool operator==(const ESM::ContItem& lhs, const ESM::ContItem& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline std::ostream& operator<<(std::ostream& stream, const ESM::ContItem& value)
    {
        return stream << "ESM::ContItem {.mCount = " << value.mCount << ", .mItem = '" << value.mItem << "'}";
    }

    inline bool operator==(const ESM::Region::SoundRef& lhs, const ESM::Region::SoundRef& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline std::ostream& operator<<(std::ostream& stream, const ESM::Region::SoundRef& value)
    {
        return stream << "ESM::Region::SoundRef {.mSound = '" << value.mSound << "', .mChance = " << value.mChance
                      << "}";
    }

    inline bool operator==(const ESM::QuickKeys::QuickKey& lhs, const ESM::QuickKeys::QuickKey& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline std::ostream& operator<<(std::ostream& stream, const ESM::QuickKeys::QuickKey& value)
    {
        return stream << "ESM::QuickKeys::QuickKey {.mType = '" << static_cast<std::uint32_t>(value.mType)
                      << "', .mId = " << value.mId << "}";
    }

    namespace
    {
        using namespace ::testing;

        constexpr std::array formats = {
            MaxLimitedSizeStringsFormatVersion,
            MaxStringRefIdFormatVersion,
            CurrentSaveGameFormatVersion,
        };

        constexpr std::uint32_t fakeRecordId = fourCC("FAKE");

        template <class T>
        void save(const T& record, ESMWriter& writer)
        {
            record.save(writer);
        }

        void save(const CellRef& record, ESMWriter& writer)
        {
            record.save(writer, true);
        }

        template <typename T>
        std::unique_ptr<std::istream> makeEsmStream(const T& record, FormatVersion formatVersion)
        {
            ESMWriter writer;
            auto stream = std::make_unique<std::stringstream>();
            writer.setFormatVersion(formatVersion);
            writer.save(*stream);
            writer.startRecord(fakeRecordId);
            save(record, writer);
            writer.endRecord(fakeRecordId);
            return stream;
        }

        template <class T, class = std::void_t<>>
        struct HasLoad : std::false_type
        {
        };

        template <class T>
        struct HasLoad<T, std::void_t<decltype(std::declval<T>().load(std::declval<ESMReader&>()))>> : std::true_type
        {
        };

        template <class T>
        auto load(ESMReader& reader, T& record) -> std::enable_if_t<HasLoad<std::decay_t<T>>::value>
        {
            record.load(reader);
        }

        template <class T, class = std::void_t<>>
        struct HasLoadWithDelete : std::false_type
        {
        };

        template <class T>
        struct HasLoadWithDelete<T,
            std::void_t<decltype(std::declval<T>().load(std::declval<ESMReader&>(), std::declval<bool&>()))>>
            : std::true_type
        {
        };

        template <class T>
        auto load(ESMReader& reader, T& record) -> std::enable_if_t<HasLoadWithDelete<std::decay_t<T>>::value>
        {
            bool deleted = false;
            record.load(reader, deleted);
        }

        void load(ESMReader& reader, CellRef& record)
        {
            bool deleted = false;
            record.load(reader, deleted, true);
        }

        template <typename T>
        void saveAndLoadRecord(const T& record, FormatVersion formatVersion, T& result)
        {
            ESMReader reader;
            reader.open(makeEsmStream(record, formatVersion), "stream");
            ASSERT_TRUE(reader.hasMoreRecs());
            ASSERT_EQ(reader.getRecName().toInt(), fakeRecordId);
            reader.getRecHeader();
            load(reader, result);
        }

        struct Esm3SaveLoadRecordTest : public TestWithParam<FormatVersion>
        {
            std::minstd_rand mRandom;
            std::uniform_int_distribution<short> mRefIdDistribution{ 'a', 'z' };

            std::string generateRandomString(std::size_t size)
            {
                std::string value;
                while (value.size() < size)
                    value.push_back(static_cast<char>(mRefIdDistribution(mRandom)));
                return value;
            }

            RefId generateRandomRefId(std::size_t size = 33) { return RefId::stringRefId(generateRandomString(size)); }
        };

        TEST_F(Esm3SaveLoadRecordTest, headerShouldNotChange)
        {
            const std::string author = generateRandomString(33);
            const std::string description = generateRandomString(257);

            auto stream = std::make_unique<std::stringstream>();

            ESMWriter writer;
            writer.setAuthor(author);
            writer.setDescription(description);
            writer.setFormatVersion(CurrentSaveGameFormatVersion);
            writer.save(*stream);
            writer.close();

            ESMReader reader;
            reader.open(std::move(stream), "stream");
            EXPECT_EQ(reader.getAuthor(), author);
            EXPECT_EQ(reader.getDesc(), description);
        }

        TEST_F(Esm3SaveLoadRecordTest, containerContItemShouldSupportRefIdLongerThan32)
        {
            Container record;
            record.blank();
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 42, .mItem = generateRandomRefId(33) });
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 13, .mItem = generateRandomRefId(33) });
            Container result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mInventory.mList, record.mInventory.mList);
        }

        TEST_F(Esm3SaveLoadRecordTest, regionSoundRefShouldSupportRefIdLongerThan32)
        {
            Region record;
            record.blank();
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(33), .mChance = 42 });
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(33), .mChance = 13 });
            Region result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mSoundList, record.mSoundList);
        }

        TEST_F(Esm3SaveLoadRecordTest, scriptSoundRefShouldSupportRefIdLongerThan32)
        {
            Script record;
            record.blank();
            record.mId = generateRandomRefId(33);
            record.mData.mNumShorts = 42;
            Script result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mData.mNumShorts, record.mData.mNumShorts);
        }

        TEST_P(Esm3SaveLoadRecordTest, playerShouldNotChange)
        {
            std::minstd_rand random;
            Player record{};
            record.mObject.blank();
            record.mBirthsign = generateRandomRefId();
            record.mObject.mRef.mRefID = generateRandomRefId();
            std::generate_n(std::inserter(record.mPreviousItems, record.mPreviousItems.end()), 2,
                [&] { return std::make_pair(generateRandomRefId(), generateRandomRefId()); });
            Player result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mBirthsign, result.mBirthsign);
            EXPECT_EQ(record.mPreviousItems, result.mPreviousItems);
        }

        TEST_P(Esm3SaveLoadRecordTest, cellRefShouldNotChange)
        {
            CellRef record;
            record.blank();
            record.mRefID = generateRandomRefId();
            record.mOwner = generateRandomRefId();
            record.mSoul = generateRandomRefId();
            record.mFaction = generateRandomRefId();
            record.mKey = generateRandomRefId();
            CellRef result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mRefID, result.mRefID);
            EXPECT_EQ(record.mOwner, result.mOwner);
            EXPECT_EQ(record.mSoul, result.mSoul);
            EXPECT_EQ(record.mFaction, result.mFaction);
            EXPECT_EQ(record.mKey, result.mKey);
        }

        TEST_P(Esm3SaveLoadRecordTest, creatureStatsShouldNotChange)
        {
            CreatureStats record;
            record.blank();
            record.mLastHitAttemptObject = generateRandomRefId();
            record.mLastHitObject = generateRandomRefId();
            CreatureStats result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mLastHitAttemptObject, result.mLastHitAttemptObject);
            EXPECT_EQ(record.mLastHitObject, result.mLastHitObject);
        }

        TEST_P(Esm3SaveLoadRecordTest, containerShouldNotChange)
        {
            Container record;
            record.blank();
            record.mId = generateRandomRefId();
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 42, .mItem = generateRandomRefId(32) });
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 13, .mItem = generateRandomRefId(32) });
            Container result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mInventory.mList, record.mInventory.mList);
        }

        TEST_P(Esm3SaveLoadRecordTest, regionShouldNotChange)
        {
            Region record;
            record.blank();
            record.mId = generateRandomRefId();
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(32), .mChance = 42 });
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(32), .mChance = 13 });
            Region result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mSoundList, record.mSoundList);
        }

        TEST_P(Esm3SaveLoadRecordTest, scriptShouldNotChange)
        {
            Script record;
            record.blank();
            record.mId = generateRandomRefId(32);
            record.mData.mNumShorts = 42;
            Script result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mData.mNumShorts, record.mData.mNumShorts);
        }

        TEST_P(Esm3SaveLoadRecordTest, quickKeysShouldNotChange)
        {
            const QuickKeys record {
                .mKeys = {
                    {
                        .mType = QuickKeys::Type::Magic,
                        .mId = generateRandomRefId(32),
                    },
                    {
                        .mType = QuickKeys::Type::MagicItem,
                        .mId = generateRandomRefId(32),
                    },
                },
            };
            QuickKeys result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mKeys, record.mKeys);
        }

        INSTANTIATE_TEST_SUITE_P(FormatVersions, Esm3SaveLoadRecordTest, ValuesIn(formats));
    }
}
