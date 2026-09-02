#pragma once

#include "AssetCache.hpp"
#include "Core/Utility/Defines.hpp"
#include "Model.hpp"

#include <filesystem>
#include <future>
#include <mutex>
#include <unordered_map>

namespace GEngine {

class AssetManager {
  public:
    AssetManager() = default;

    GE_NO_COPY_NO_MOVE(AssetManager)

    [[nodiscard]] ModelHandle LoadModel(const std::filesystem::path& path);
    [[nodiscard]] ModelHandle AddModel(Model model);
    [[nodiscard]] const Model* GetModel(ModelHandle handle) const;

  private:
    uint32_t m_NextModelId{1u};
    std::unordered_map<std::filesystem::path, ModelHandle> m_PathToHandle;
    std::unordered_map<uint32_t, std::shared_future<std::optional<Model>>> m_Models;

    AssetCache m_AssetCache{"AssetCache"};

    mutable std::mutex m_Mutex{};
};

} // namespace GEngine
