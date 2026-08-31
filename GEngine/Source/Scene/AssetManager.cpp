#include "PCH.hpp"

#include "AssetManager.hpp"

#include "Core/Utility/Timer.hpp"
#include "ModelLoader.hpp"

namespace GEngine {

[[nodiscard]] ModelHandle AssetManager::LoadModel(const std::filesystem::path& path) {
    auto it = m_PathToHandle.find(path);
    if (it != m_PathToHandle.end())
        return it->second;

    const std::filesystem::path cacheFile = path.filename().replace_extension(".gemb");
    {
        GE_SCOPED_TIMER(std::format("Loaded {} from binary cache", cacheFile.string()));
        if (auto cached = m_AssetCache.LoadBinaryModel(cacheFile); cached.has_value()) {
            ModelHandle handle{m_NextModelId++};
            m_Models[handle.Id] = std::move(*cached);
            m_PathToHandle[path] = handle;
            return handle;
        }
    }

    std::expected<Model, std::string> result = ModelLoader::Load(path);
    if (!result.has_value())
        return {};

    m_AssetCache.SaveBinaryModel(cacheFile, *result);

    ModelHandle handle{m_NextModelId++};
    m_Models[handle.Id] = std::move(result.value());
    m_PathToHandle[path] = handle;
    return handle;
}

[[nodiscard]] ModelHandle AssetManager::AddModel(Model model) {
    ModelHandle handle{m_NextModelId++};
    m_Models[handle.Id] = std::move(model);
    return handle;
}

[[nodiscard]] const Model* AssetManager::GetModel(ModelHandle handle) const {
    auto it = m_Models.find(handle.Id);
    return (it != m_Models.end()) ? &it->second : nullptr;
}

} // namespace GEngine
