#include "custommarkerstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void CustomMarker::save(ESMWriter& esm) const
    {
        esm.writeHNT("POSX", mWorldX);
        esm.writeHNT("POSY", mWorldY);
        mCell.save(esm);
        if (!mNote.empty())
            esm.writeHNString("NOTE", mNote);
    }

    void CustomMarker::load(ESMReader& esm)
    {
        esm.getHNT(mWorldX, "POSX");
        esm.getHNT(mWorldY, "POSY");
        mCell.load(esm);
        mNote = esm.getHNOString("NOTE");
    }

}
