#pragma once

#include "AssetCache.hpp"
#include "Model.hpp"

#include <filesystem>
#include <unordered_map>

namespace GEngine {

class AssetManager {
  public:
    AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

    [[nodiscard]] ModelHandle LoadModel(const std::filesystem::path& path);
    [[nodiscard]] ModelHandle AddModel(Model model);
    [[nodiscard]] const Model* GetModel(ModelHandle handle) const;

  private:
    uint32_t m_NextModelId{1u};
    std::unordered_map<std::filesystem::path, ModelHandle> m_PathToHandle;
    std::unordered_map<uint32_t, Model> m_Models;

    AssetCache m_AssetCache{"AssetCache"};
};

} // namespace GEngine
