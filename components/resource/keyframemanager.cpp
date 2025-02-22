#include "keyframemanager.hpp"

#include <components/vfs/manager.hpp>

#include <osg/Stats>
#include <osgAnimation/Animation>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Channel>

#include <components/debug/debuglog.hpp>
#include <components/misc/pathhelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/nifosg/nifloader.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/osgacontroller.hpp>

#include "animation.hpp"
#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace Resource
{

    RetrieveAnimationsVisitor::RetrieveAnimationsVisitor(SceneUtil::KeyframeHolder& target,
        osg::ref_ptr<osgAnimation::BasicAnimationManager> animationManager, const std::string& normalized,
        const VFS::Manager* vfs)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mTarget(target)
        , mAnimationManager(animationManager)
        , mNormalized(normalized)
        , mVFS(vfs)
    {
    }

    bool RetrieveAnimationsVisitor::belongsToLeftUpperExtremity(const std::string& name)
    {
        static const std::array boneNames = { "bip01_l_clavicle", "left_clavicle", "bip01_l_upperarm", "left_upper_arm",
            "bip01_l_forearm", "bip01_l_hand", "left_hand", "left_wrist", "shield_bone", "bip01_l_pinky1",
            "bip01_l_pinky2", "bip01_l_pinky3", "bip01_l_ring1", "bip01_l_ring2", "bip01_l_ring3", "bip01_l_middle1",
            "bip01_l_middle2", "bip01_l_middle3", "bip01_l_pointer1", "bip01_l_pointer2", "bip01_l_pointer3",
            "bip01_l_thumb1", "bip01_l_thumb2", "bip01_l_thumb3", "left_forearm" };

        if (std::find(boneNames.begin(), boneNames.end(), name) != boneNames.end())
            return true;

        return false;
    }

    bool RetrieveAnimationsVisitor::belongsToRightUpperExtremity(const std::string& name)
    {
        static const std::array boneNames = { "bip01_r_clavicle", "right_clavicle", "bip01_r_upperarm",
            "right_upper_arm", "bip01_r_forearm", "bip01_r_hand", "right_hand", "right_wrist", "bip01_r_thumb1",
            "bip01_r_thumb2", "bip01_r_thumb3", "weapon_bone", "bip01_r_pinky1", "bip01_r_pinky2", "bip01_r_pinky3",
            "bip01_r_ring1", "bip01_r_ring2", "bip01_r_ring3", "bip01_r_middle1", "bip01_r_middle2", "bip01_r_middle3",
            "bip01_r_pointer1", "bip01_r_pointer2", "bip01_r_pointer3", "right_forearm" };

        if (std::find(boneNames.begin(), boneNames.end(), name) != boneNames.end())
            return true;

        return false;
    }

    bool RetrieveAnimationsVisitor::belongsToTorso(const std::string& name)
    {
        static const std::array boneNames
            = { "bip01_spine1", "bip01_spine2", "bip01_neck", "bip01_head", "head", "neck", "chest", "groin" };

        if (std::find(boneNames.begin(), boneNames.end(), name) != boneNames.end())
            return true;

        return false;
    }

    void RetrieveAnimationsVisitor::addKeyframeController(const std::string& name, const osg::Node& node)
    {
        osg::ref_ptr<SceneUtil::OsgAnimationController> callback = new SceneUtil::OsgAnimationController();

        callback->setName(name);

        std::vector<SceneUtil::EmulatedAnimation> emulatedAnimations;

        for (const auto& animation : mAnimationManager->getAnimationList())
        {
            if (animation)
            {
                //"Default" is osg dae plugin's default naming scheme for unnamed animations
                if (animation->getName() == "Default")
                {
                    animation->setName(std::string("idle"));
                }

                osg::ref_ptr<Resource::Animation> mergedAnimationTrack = new Resource::Animation;
                const std::string animationName = animation->getName();
                mergedAnimationTrack->setName(animationName);

                const osgAnimation::ChannelList& channels = animation->getChannels();
                for (const auto& channel : channels)
                {
                    if (name == "Bip01 R Clavicle")
                    {
                        if (!belongsToRightUpperExtremity(channel->getTargetName()))
                            continue;
                    }
                    else if (name == "Bip01 L Clavicle")
                    {
                        if (!belongsToLeftUpperExtremity(channel->getTargetName()))
                            continue;
                    }
                    else if (name == "Bip01 Spine1")
                    {
                        if (!belongsToTorso(channel->getTargetName()))
                            continue;
                    }
                    else if (belongsToRightUpperExtremity(channel->getTargetName())
                        || belongsToLeftUpperExtremity(channel->getTargetName())
                        || belongsToTorso(channel->getTargetName()))
                        continue;

                    mergedAnimationTrack->addChannel(channel.get()->clone());
                }

                callback->addMergedAnimationTrack(mergedAnimationTrack);

                float startTime = animation->getStartTime();
                float stopTime = startTime + animation->getDuration();

                SceneUtil::EmulatedAnimation emulatedAnimation;
                emulatedAnimation.mStartTime = startTime;
                emulatedAnimation.mStopTime = stopTime;
                emulatedAnimation.mName = animationName;
                emulatedAnimations.emplace_back(emulatedAnimation);
            }
        }

        // mTextKeys is a nif-thing, used by OpenMW's animation system
        // Format is likely "AnimationName: [Keyword_optional] [Start OR Stop]"
        // AnimationNames are keywords like idle2, idle3... AiPackages and various mechanics control which
        // animations are played Keywords can be stuff like Loop, Equip, Unequip, Block, InventoryHandtoHand,
        // InventoryWeaponOneHand, PickProbe, Slash, Thrust, Chop... even "Slash Small Follow" osgAnimation formats
        // should have a .txt file with the same name, each line holding a textkey and whitespace separated time
        // value e.g. idle: start 0.0333
        try
        {
            Files::IStreamPtr textKeysFile = mVFS->get(changeFileExtension(mNormalized, "txt"));
            std::string line;
            while (getline(*textKeysFile, line))
            {
                mTarget.mTextKeys.emplace(parseTimeSignature(line), parseTextKey(line));
            }
        }
        catch (std::exception&)
        {
            Log(Debug::Warning) << "No textkey file found for " << mNormalized;
        }

        callback->setEmulatedAnimations(emulatedAnimations);
        mTarget.mKeyframeControllers.emplace(name, callback);
    }

    void RetrieveAnimationsVisitor::apply(osg::Node& node)
    {
        if (node.libraryName() == std::string_view("osgAnimation") && node.className() == std::string_view("Bone")
            && Misc::StringUtils::lowerCase(node.getName()) == std::string_view("bip01"))
        {
            addKeyframeController("bip01", node); /* Character root */
            addKeyframeController("Bip01 Spine1", node); /* Torso */
            addKeyframeController("Bip01 L Clavicle", node); /* Left arm */
            addKeyframeController("Bip01 R Clavicle", node); /* Right arm */
        }

        traverse(node);
    }

    std::string RetrieveAnimationsVisitor::parseTextKey(const std::string& line)
    {
        size_t spacePos = line.find_last_of(' ');
        if (spacePos != std::string::npos)
            return line.substr(0, spacePos);
        return "";
    }

    double RetrieveAnimationsVisitor::parseTimeSignature(const std::string& line)
    {
        size_t spacePos = line.find_last_of(' ');
        double time = 0.0;
        if (spacePos != std::string::npos && spacePos + 1 < line.size())
            time = Misc::StringUtils::toNumeric<double>(line.substr(spacePos + 1), time);
        return time;
    }

    std::string RetrieveAnimationsVisitor::changeFileExtension(const std::string& file, const std::string& ext)
    {
        size_t extPos = file.find_last_of('.');
        if (extPos != std::string::npos && extPos + 1 < file.size())
        {
            return file.substr(0, extPos + 1) + ext;
        }
        return file;
    }

}

namespace Resource
{

    KeyframeManager::KeyframeManager(const VFS::Manager* vfs, SceneManager* sceneManager)
        : ResourceManager(vfs)
        , mSceneManager(sceneManager)
    {
    }

    KeyframeManager::~KeyframeManager() {}

    osg::ref_ptr<const SceneUtil::KeyframeHolder> KeyframeManager::get(const std::string& name)
    {
        const std::string normalized = mVFS->normalizeFilename(name);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const SceneUtil::KeyframeHolder>(static_cast<SceneUtil::KeyframeHolder*>(obj.get()));
        else
        {
            osg::ref_ptr<SceneUtil::KeyframeHolder> loaded(new SceneUtil::KeyframeHolder);
            if (Misc::getFileExtension(normalized) == "kf")
            {
                auto file = std::make_shared<Nif::NIFFile>(normalized);
                Nif::Reader reader(*file);
                reader.parse(mVFS->getNormalized(normalized));
                NifOsg::Loader::loadKf(*file, *loaded.get());
            }
            else
            {
                osg::ref_ptr<osg::Node> scene = const_cast<osg::Node*>(mSceneManager->getTemplate(normalized).get());
                osg::ref_ptr<osgAnimation::BasicAnimationManager> bam
                    = dynamic_cast<osgAnimation::BasicAnimationManager*>(scene->getUpdateCallback());
                if (bam)
                {
                    Resource::RetrieveAnimationsVisitor rav(*loaded.get(), bam, normalized, mVFS);
                    scene->accept(rav);
                }
            }
            mCache->addEntryToObjectCache(normalized, loaded);
            return loaded;
        }
    }

    void KeyframeManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Keyframe", mCache->getCacheSize());
    }

}
