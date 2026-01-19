#include "ComponentFactory.h"

const std::unordered_map<std::string, createCompFunc>& ComponentFactory::GetRegisteredComponents()
{
    return registeredComponents;
}