#include "creator.hpp"

#include <apps/opencs/model/world/scope.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <stdexcept>

namespace CSMDoc
{
    class Document;
}

CSVWorld::Creator::~Creator() {}

void CSVWorld::Creator::setScope(unsigned int scope)
{
    if (scope != CSMWorld::Scope_Content)
        throw std::logic_error("Invalid scope in creator");
}

CSVWorld::CreatorFactoryBase::~CreatorFactoryBase() {}

CSVWorld::Creator* CSVWorld::NullCreatorFactory::makeCreator(
    CSMDoc::Document& document, const CSMWorld::UniversalId& id) const
{
    return nullptr;
}
