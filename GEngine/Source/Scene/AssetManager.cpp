#include "PCH.hpp"

#include "AssetManager.hpp"

#include "Core/Utility/Timer.hpp"
#include "ModelLoader.hpp"

namespace GEngine {

[[nodiscard]] ModelHandle AssetManager::LoadModel(const std::filesystem::path& path) {
    std::lock_guard lock{m_Mutex};
    auto it = m_PathToHandle.find(path);
    if (it != m_PathToHandle.end())
        return it->second;

    ModelHandle handle{m_NextModelId++};
    m_PathToHandle[path] = handle;
    m_Models[handle.Id] = std::async(std::launch::async, [this, path]() -> std::optional<Model> {
        const std::filesystem::path cacheFile = path.filename().replace_extension(".gemb");
        {
            GE_SCOPED_TIMER(std::format("Loaded {} from binary cache", cacheFile.string()));
            if (auto cached = m_AssetCache.LoadBinaryModel(cacheFile); cached.has_value()) {
                return cached.value();
            }
        }

        std::expected<Model, std::string> result = ModelLoader::Load(path);
        if (!result.has_value()) {
            GE_CORE_WARN("Failed to load model {}: {}", path.string(), result.error());
            return std::nullopt;
        }

        m_AssetCache.SaveBinaryModel(cacheFile, *result);
        return result.value();
    }).share();
    return handle;
}

[[nodiscard]] ModelHandle AssetManager::AddModel(Model model) {
    std::lock_guard lock{m_Mutex};
    ModelHandle handle{m_NextModelId++};
    std::promise<std::optional<Model>> ready;
    ready.set_value(std::move(model));
    m_Models[handle.Id] = ready.get_future().share();
    return handle;
}

[[nodiscard]] const Model* AssetManager::GetModel(ModelHandle handle) const {
    std::lock_guard lock{m_Mutex};
    auto it = m_Models.find(handle.Id);
    if (it == m_Models.end())
        return nullptr;
    const std::optional<Model>& model = it->second.get();
    return model.has_value() ? &model.value() : nullptr;
}

} // namespace GEngine
