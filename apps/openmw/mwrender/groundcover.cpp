#include "groundcover.hpp"

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/VertexAttribDivisor>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/terrain/quadtreenode.hpp>

#include "../mwworld/groundcoverstore.hpp"

#include "vismask.hpp"

namespace MWRender
{
    class InstancingVisitor : public osg::NodeVisitor
    {
    public:
        InstancingVisitor(std::vector<Groundcover::GroundcoverEntry>& instances, osg::Vec3f& chunkPosition)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mInstances(instances)
            , mChunkPosition(chunkPosition)
        {
        }

        void apply(osg::Geometry& geom) override
        {
            for (unsigned int i = 0; i < geom.getNumPrimitiveSets(); ++i)
            {
                geom.getPrimitiveSet(i)->setNumInstances(mInstances.size());
            }

            osg::ref_ptr<osg::Vec4Array> transforms = new osg::Vec4Array(mInstances.size());
            osg::BoundingBox box;
            float radius = geom.getBoundingBox().radius();
            for (unsigned int i = 0; i < transforms->getNumElements(); i++)
            {
                osg::Vec3f pos(mInstances[i].mPos.asVec3());
                osg::Vec3f relativePos = pos - mChunkPosition;
                (*transforms)[i] = osg::Vec4f(relativePos, mInstances[i].mScale);

                // Use an additional margin due to groundcover animation
                float instanceRadius = radius * mInstances[i].mScale * 1.1f;
                osg::BoundingSphere instanceBounds(relativePos, instanceRadius);
                box.expandBy(instanceBounds);
            }

            geom.setInitialBound(box);

            osg::ref_ptr<osg::Vec3Array> rotations = new osg::Vec3Array(mInstances.size());
            for (unsigned int i = 0; i < rotations->getNumElements(); i++)
            {
                (*rotations)[i] = mInstances[i].mPos.asRotationVec3();
            }

            // Display lists do not support instancing in OSG 3.4
            geom.setUseDisplayList(false);
            geom.setUseVertexBufferObjects(true);

            geom.setVertexAttribArray(6, transforms.get(), osg::Array::BIND_PER_VERTEX);
            geom.setVertexAttribArray(7, rotations.get(), osg::Array::BIND_PER_VERTEX);
        }

    private:
        std::vector<Groundcover::GroundcoverEntry> mInstances;
        osg::Vec3f mChunkPosition;
    };

    class DensityCalculator
    {
    public:
        DensityCalculator(float density)
            : mDensity(density)
        {
        }

        bool isInstanceEnabled()
        {
            if (mDensity >= 1.f)
                return true;

            mCurrentGroundcover += mDensity;
            if (mCurrentGroundcover < 1.f)
                return false;

            mCurrentGroundcover -= 1.f;

            return true;
        }
        void reset() { mCurrentGroundcover = 0.f; }

    private:
        float mCurrentGroundcover = 0.f;
        float mDensity = 0.f;
    };

    class ViewDistanceCallback : public SceneUtil::NodeCallback<ViewDistanceCallback>
    {
    public:
        ViewDistanceCallback(float dist, const osg::BoundingBox& box)
            : mViewDistance(dist)
            , mBox(box)
        {
        }
        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (Terrain::distance(mBox, nv->getEyePoint()) <= mViewDistance)
                traverse(node, nv);
        }

    private:
        float mViewDistance;
        osg::BoundingBox mBox;
    };

    inline bool isInChunkBorders(ESM::CellRef& ref, osg::Vec2f& minBound, osg::Vec2f& maxBound)
    {
        osg::Vec2f size = maxBound - minBound;
        if (size.x() >= 1 && size.y() >= 1)
            return true;

        osg::Vec3f pos = ref.mPos.asVec3();
        osg::Vec3f cellPos = pos / ESM::Land::REAL_SIZE;
        if ((minBound.x() > std::floor(minBound.x()) && cellPos.x() < minBound.x())
            || (minBound.y() > std::floor(minBound.y()) && cellPos.y() < minBound.y())
            || (maxBound.x() < std::ceil(maxBound.x()) && cellPos.x() >= maxBound.x())
            || (maxBound.y() < std::ceil(maxBound.y()) && cellPos.y() >= maxBound.y()))
            return false;

        return true;
    }

    osg::ref_ptr<osg::Node> Groundcover::getChunk(float size, const osg::Vec2f& center, unsigned char lod,
        unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile)
    {
        if (lod > getMaxLodLevel())
            return nullptr;
        GroundcoverChunkId id = std::make_tuple(center, size);
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(id);
        if (obj)
            return static_cast<osg::Node*>(obj.get());
        else
        {
            InstanceMap instances;
            collectInstances(instances, size, center);
            osg::ref_ptr<osg::Node> node = createChunk(instances, center);
            mCache->addEntryToObjectCache(id, node.get());
            return node;
        }
    }

    Groundcover::Groundcover(
        Resource::SceneManager* sceneManager, float density, float viewDistance, const MWWorld::GroundcoverStore& store)
        : GenericResourceManager<GroundcoverChunkId>(nullptr)
        , mSceneManager(sceneManager)
        , mDensity(density)
        , mStateset(new osg::StateSet)
        , mGroundcoverStore(store)
    {
        setViewDistance(viewDistance);
        // MGE uses default alpha settings for groundcover, so we can not rely on alpha properties
        // Force a unified alpha handling instead of data from meshes
        osg::ref_ptr<osg::AlphaFunc> alpha = new osg::AlphaFunc(osg::AlphaFunc::GEQUAL, 128.f / 255.f);
        mStateset->setAttributeAndModes(alpha.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        mStateset->setAttributeAndModes(new osg::BlendFunc, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        mStateset->setRenderBinDetails(0, "RenderBin", osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
        mStateset->setAttribute(new osg::VertexAttribDivisor(6, 1));
        mStateset->setAttribute(new osg::VertexAttribDivisor(7, 1));

        mProgramTemplate = mSceneManager->getShaderManager().getProgramTemplate()
            ? Shader::ShaderManager::cloneProgram(mSceneManager->getShaderManager().getProgramTemplate())
            : osg::ref_ptr<osg::Program>(new osg::Program);
        mProgramTemplate->addBindAttribLocation("aOffset", 6);
        mProgramTemplate->addBindAttribLocation("aRotation", 7);
    }

    Groundcover::~Groundcover() {}

    void Groundcover::collectInstances(InstanceMap& instances, float size, const osg::Vec2f& center)
    {
        if (mDensity <= 0.f)
            return;

        osg::Vec2f minBound = (center - osg::Vec2f(size / 2.f, size / 2.f));
        osg::Vec2f maxBound = (center + osg::Vec2f(size / 2.f, size / 2.f));
        DensityCalculator calculator(mDensity);
        ESM::ReadersCache readers;
        osg::Vec2i startCell = osg::Vec2i(std::floor(center.x() - size / 2.f), std::floor(center.y() - size / 2.f));
        for (int cellX = startCell.x(); cellX < startCell.x() + size; ++cellX)
        {
            for (int cellY = startCell.y(); cellY < startCell.y() + size; ++cellY)
            {
                ESM::Cell cell;
                mGroundcoverStore.initCell(cell, cellX, cellY);
                if (cell.mContextList.empty())
                    continue;

                calculator.reset();
                std::map<ESM::RefNum, ESM::CellRef> refs;
                for (size_t i = 0; i < cell.mContextList.size(); ++i)
                {
                    const std::size_t index = static_cast<std::size_t>(cell.mContextList[i].index);
                    const ESM::ReadersCache::BusyItem reader = readers.get(index);
                    cell.restore(*reader, i);
                    ESM::CellRef ref;
                    bool deleted = false;
                    while (cell.getNextRef(*reader, ref, deleted))
                    {
                        if (!deleted && refs.find(ref.mRefNum) == refs.end() && !calculator.isInstanceEnabled())
                            deleted = true;
                        if (!deleted && !isInChunkBorders(ref, minBound, maxBound))
                            deleted = true;

                        if (deleted)
                        {
                            refs.erase(ref.mRefNum);
                            continue;
                        }
                        refs[ref.mRefNum] = std::move(ref);
                    }
                }

                for (auto& pair : refs)
                {
                    ESM::CellRef& ref = pair.second;
                    const std::string& model = mGroundcoverStore.getGroundcoverModel(ref.mRefID);
                    if (!model.empty())
                        instances[model].emplace_back(std::move(ref));
                }
            }
        }
    }

    osg::ref_ptr<osg::Node> Groundcover::createChunk(InstanceMap& instances, const osg::Vec2f& center)
    {
        osg::ref_ptr<osg::Group> group = new osg::Group;
        osg::Vec3f worldCenter = osg::Vec3f(center.x(), center.y(), 0) * ESM::Land::REAL_SIZE;
        for (auto& pair : instances)
        {
            const osg::Node* temp = mSceneManager->getTemplate(pair.first);
            osg::ref_ptr<osg::Node> node = static_cast<osg::Node*>(temp->clone(osg::CopyOp::DEEP_COPY_NODES
                | osg::CopyOp::DEEP_COPY_DRAWABLES | osg::CopyOp::DEEP_COPY_USERDATA | osg::CopyOp::DEEP_COPY_ARRAYS
                | osg::CopyOp::DEEP_COPY_PRIMITIVES));

            // Keep link to original mesh to keep it in cache
            group->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(temp));

            InstancingVisitor visitor(pair.second, worldCenter);
            node->accept(visitor);
            group->addChild(node);
        }

        osg::ComputeBoundsVisitor cbv;
        group->accept(cbv);
        osg::BoundingBox box = cbv.getBoundingBox();
        group->addCullCallback(new ViewDistanceCallback(getViewDistance(), box));

        group->setStateSet(mStateset);
        group->setNodeMask(Mask_Groundcover);
        if (mSceneManager->getLightingMethod() != SceneUtil::LightingMethod::FFP)
            group->addCullCallback(new SceneUtil::LightListCallback);
        mSceneManager->recreateShaders(group, "groundcover", true, mProgramTemplate);
        mSceneManager->shareState(group);
        group->getBound();
        return group;
    }

    unsigned int Groundcover::getNodeMask()
    {
        return Mask_Groundcover;
    }

    void Groundcover::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Groundcover Chunk", mCache->getCacheSize());
    }
}
