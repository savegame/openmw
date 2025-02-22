#ifndef OPENMW_ESM_WEAP_H
#define OPENMW_ESM_WEAP_H

#include <string>

#include "components/esm/refid.hpp"
#include "loadskil.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Weapon definition
     */

    struct Weapon
    {
        constexpr static RecNameInts sRecordId = REC_WEAP;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Weapon"; }

        enum Type
        {
            PickProbe = -4,
            HandToHand = -3,
            Spell = -2,
            None = -1,
            ShortBladeOneHand = 0,
            LongBladeOneHand = 1,
            LongBladeTwoHand = 2,
            BluntOneHand = 3,
            BluntTwoClose = 4,
            BluntTwoWide = 5,
            SpearTwoWide = 6,
            AxeOneHand = 7,
            AxeTwoHand = 8,
            MarksmanBow = 9,
            MarksmanCrossbow = 10,
            MarksmanThrown = 11,
            Arrow = 12,
            Bolt = 13
        };

        enum AttackType
        {
            AT_Chop,
            AT_Slash,
            AT_Thrust
        };

        enum Flags
        {
            Magical = 0x01,
            Silver = 0x02
        };

#pragma pack(push)
#pragma pack(1)
        struct WPDTstruct
        {
            float mWeight;
            int mValue;
            short mType;
            unsigned short mHealth;
            float mSpeed, mReach;
            unsigned short mEnchant; // Enchantment points. The real value is mEnchant/10.f
            unsigned char mChop[2], mSlash[2], mThrust[2]; // Min and max
            int mFlags;
        }; // 32 bytes
#pragma pack(pop)

        WPDTstruct mData;

        unsigned int mRecordFlags;
        RefId mId, mEnchant, mScript;
        std::string mName, mModel, mIcon;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };

    struct WeaponType
    {
        enum Flags
        {
            TwoHanded = 0x01,
            HasHealth = 0x02
        };

        enum Class
        {
            Melee = 0,
            Ranged = 1,
            Thrown = 2,
            Ammo = 3
        };

        // std::string mDisplayName; // TODO: will be needed later for editor
        std::string mShortGroup;
        std::string mLongGroup;
        ESM::RefId mSoundIdDown;
        ESM::RefId mSoundIdUp;
        std::string mAttachBone;
        std::string mSheathingBone;
        Skill::SkillEnum mSkill;
        Class mWeaponClass;
        int mAmmoType;
        int mFlags;

        WeaponType(std::string shortGroup, std::string longGroup, const std::string& soundId, std::string attachBone,
            std::string sheathingBone, Skill::SkillEnum skill, Class weaponClass, int ammoType, int flags)
            : mShortGroup(std::move(shortGroup))
            , mLongGroup(std::move(longGroup))
            , mSoundIdDown(ESM::RefId::stringRefId(soundId + " Down"))
            , mSoundIdUp(ESM::RefId::stringRefId(soundId + " Up"))
            , mAttachBone(std::move(attachBone))
            , mSheathingBone(std::move(sheathingBone))
            , mSkill(skill)
            , mWeaponClass(weaponClass)
            , mAmmoType(ammoType)
            , mFlags(flags)
        {
        }
    };

}
#endif
