#include "util/ArrayBlock.h"
#include "core/FFDKeyUpdater.h"
#include "core/FFDKey.h"

namespace core
{
//-------------------------------------------------------------------------------------------------
class FFDResourceUpdater : public cmnd::Stable
{
    struct Target
    {
        Target(FFDKey* aKey) : key(aKey), pos() {}
        FFDKey* key;
        QVector<gl::Vector3> pos;
    };

    TimeLine& mTimeLine;
    QList<Target> mTargets;
    ResourceUpdatingWorkspacePtr mWorkspace;

public:
    FFDResourceUpdater(TimeLine& aTimeLine, const ResourceUpdatingWorkspacePtr& aWorkspace)
        : mTimeLine(aTimeLine)
        , mTargets()
        , mWorkspace(aWorkspace)
    {
    }

    virtual void exec()
    {
        // for each ffd keys
        auto& map = mTimeLine.map(TimeKeyType_FFD);
        for (auto itr = map.begin(); itr != map.end(); ++itr)
        {
            TimeKey* timeKey = itr.value();
            XC_PTR_ASSERT(timeKey);
            XC_ASSERT(timeKey->type() == TimeKeyType_FFD);
            FFDKey* ffdKey = (FFDKey*)timeKey;

            // find transition information
            auto transUnit = mWorkspace->findUnit(ffdKey->parent());
            if (!transUnit)
            {
                continue;
            }

            // calculate new ffd data
            util::ArrayBlock<const gl::Vector3> posArray(
                        ffdKey->data().positions(), ffdKey->data().count());
            auto newFFD = transUnit->mesh->createFFD(posArray, transUnit->trans);
            QScopedArrayPointer<gl::Vector3> newFFDScope(newFFD.array());

            // append new ffd data to targets
            mTargets.push_back(Target(ffdKey));
            auto& target = mTargets.back();
            target.pos.resize(newFFD.count());
            memcpy(target.pos.data(), newFFD.array(),
                   sizeof(gl::Vector3) * newFFD.count());
        }
        mWorkspace.reset(); // finish using of workspace

        redo();
    }

    virtual void redo()
    {
        for (auto& target : mTargets)
        {
            target.key->data().swap(target.pos);
        }
    }

    virtual void undo()
    {
        for (auto& target : mTargets)
        {
            target.key->data().swap(target.pos);
        }
    }
};


//-------------------------------------------------------------------------------------------------
cmnd::Stable* FFDKeyUpdater::createResourceUpdater(
        ObjectNode& aNode, const ResourceUpdatingWorkspacePtr& aWorkspace)
{
    if (!aNode.timeLine()) return nullptr;
    return new FFDResourceUpdater(*aNode.timeLine(), aWorkspace);
}

} // namespace core

