#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    int Race::MaleFemale::getValue(bool male) const
    {
        return male ? mMale : mFemale;
    }

    float Race::MaleFemaleF::getValue(bool male) const
    {
        return male ? mMale : mFemale;
    }

    void Race::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mPowers.mList.clear();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("RADT"):
                    esm.getHTSized<140>(mData);
                    hasData = true;
                    break;
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                case fourCC("NPCS"):
                    mPowers.add(esm);
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing RADT subrecord");
    }
    void Race::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("RADT", mData, 140);
        mPowers.save(esm);
        esm.writeHNOString("DESC", mDescription);
    }

    void Race::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mDescription.clear();

        mPowers.mList.clear();

        for (int i = 0; i < 7; ++i)
        {
            mData.mBonus[i].mSkill = -1;
            mData.mBonus[i].mBonus = 0;
        }

        for (int i = 0; i < 8; ++i)
            mData.mAttributeValues[i].mMale = mData.mAttributeValues[i].mFemale = 1;

        mData.mHeight.mMale = mData.mHeight.mFemale = 1;
        mData.mWeight.mMale = mData.mWeight.mFemale = 1;

        mData.mFlags = 0;
    }
}
