// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/model_textures.h"

#include <algorithm>

namespace reluva::newworlds {

void splitName(const std::string& name, std::string& folder, std::string& stem, std::string& ext) {
    const size_t slash = name.rfind('/');
    const size_t dot = name.rfind('.');
    const size_t start = slash == std::string::npos ? 0 : slash + 1;
    stem = name.substr(start, (dot == std::string::npos || dot < start ? name.size() : dot) - start);
    ext = dot == std::string::npos || dot < start ? std::string() : name.substr(dot + 1);
    folder.clear();
    if (slash != std::string::npos && slash > 0) {
        const size_t prev = name.rfind('/', slash - 1);
        const size_t from = prev == std::string::npos ? 0 : prev + 1;
        folder = name.substr(from, slash - from);
    }
}

std::vector<TextureFile> listTextureFiles(const Archives& archives) {
    std::vector<TextureFile> out;
    for (size_t a = 0; a < archives.size(); ++a) {
        const auto& entries = archives[a]->entries();
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].tag != cattag::Texture)
                continue;
            TextureFile t;
            std::string ext;
            splitName(entries[i].name, t.folder, t.stem, ext);
            if (ext == "lbp" || ext == "mp0") {
                t.ref = {a, i};
                out.push_back(std::move(t));
            }
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const TextureFile& x, const TextureFile& y) { return x.stem < y.stem; });
    return out;
}

// A model's textures are named after it: k_bop uses k_bop_a, k_bop_b and
// k_bop_g, in name order. A variant such as k_bop_l with no textures of its own
// uses those of the name without the suffix (except the flat _s outlines).
// Textures in the model's own folder (kln for geo/kln/k_bop.geo) are preferred.
std::vector<const TextureFile*> findModelTextures(const std::string& modelName,
                                                  const std::vector<TextureFile>& textures) {
    std::string folder, base, ext;
    splitName(modelName, folder, base, ext);
    std::vector<const TextureFile*> found;
    while (true) {
        auto matches = [&](const TextureFile& t) {
            return t.stem == base || (t.stem.size() > base.size() + 1 && t.stem.compare(0, base.size(), base) == 0 &&
                                      t.stem[base.size()] == '_' &&
                                      t.stem.find('_', base.size() + 1) == std::string::npos);
        };
        for (int sameFolder = 1; sameFolder >= 0 && found.empty(); --sameFolder)
            for (const TextureFile& t : textures)
                if ((!sameFolder || t.folder == folder) && matches(t))
                    found.push_back(&t);
        // Otherwise drop a short suffix (_l, _d, _ad, ...) and try again; but
        // not _s, the flat outlines, which don't use the full model's textures.
        const size_t us = base.rfind('_');
        if (!found.empty() || us == std::string::npos || base.size() - us > 3 || base.compare(us, 3, "_s") == 0)
            break;
        base.resize(us);
    }
    return found;
}

} // namespace reluva::newworlds
