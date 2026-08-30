#pragma once

#include <filesystem>

namespace GEngine {

struct Model;

class AssetCache {
  public:
    explicit AssetCache(const std::filesystem::path& directory);

    bool SaveBinaryModel(const std::filesystem::path& filename, const Model& model);
    std::optional<Model> LoadBinaryModel(const std::filesystem::path& filename) const;

  private:
    std::filesystem::path m_Directory{};
};

} // namespace GEngine
