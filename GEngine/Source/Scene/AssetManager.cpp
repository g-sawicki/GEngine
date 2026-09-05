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
    std::shared_future<std::optional<Model>> loadFuture =
        std::async(std::launch::async, [this, path]() -> std::optional<Model> {
            // const std::filesystem::path cacheFile = path.filename().replace_extension(".gemb");
            // Timer cacheTimer;
            // if (auto cached = m_AssetCache.LoadBinaryModel(cacheFile); cached.has_value()) {
            //     const float cacheMs = cacheTimer.Elapsed<std::milli>();
            //     GE_CORE_INFO("Loaded {} from binary cache in {:.2f} ms", cacheFile.string(), cacheMs);
            //     return cached.value();
            // }

            Timer importTimer;
            ModelLoader modelLoader(path);
            std::expected<Model, std::string> result = modelLoader.Load();
            if (!result.has_value()) {
                GE_CORE_WARN("Failed to load model {}: {}", path.string(), result.error());
                return std::nullopt;
            }
            const float importMs = importTimer.Elapsed<std::milli>();
            GE_CORE_INFO("Loaded {} with assimp in {:.2f} ms", path.string(), importMs);

            // m_AssetCache.SaveBinaryModel(cacheFile, *result);
            return result.value();
        });
    m_Models[handle.Id] = std::move(loadFuture);
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
