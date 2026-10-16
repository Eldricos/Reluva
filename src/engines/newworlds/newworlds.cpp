// Star Trek: New Worlds engine (clean-room reimplementation).
//
// Everything in this engine must come from the game's data files, observed
// behaviour of the original, or documented reverse engineering of a legally
// owned copy. Never from leaked source code. See CONTRIBUTING.md.

#include "engines/engine.h"

namespace kestrel {

namespace {

// Detection table. Fill it in from a real install: pick 2-3 files that are
// unique to the game, then record their size and the MD5 of their first
// kDetectionHashBytes bytes (the `kestrel --hash <file>` tool prints these).
const std::vector<GameVariant> kVariants = {
    // {"newworlds", "Star Trek: New Worlds", "en", "windows",
    //  {{"<file1>", "<md5>", <size>}, {"<file2>", "<md5>", <size>}}},
};

class NewWorldsEngine : public Engine {
public:
    NewWorldsEngine(System& system, GameTarget target) : sys_(system), target_(std::move(target)) {}

    Error run() override {
        return Error::fail("The New Worlds engine is not implemented yet.");
    }

private:
    [[maybe_unused]] System& sys_;
    [[maybe_unused]] GameTarget target_;
};

class NewWorldsMetaEngine : public MetaEngine {
public:
    const char* id() const override { return "newworlds"; }
    const char* name() const override { return "New Worlds"; }

    std::vector<DetectedGame> detect(const std::filesystem::path& dir) const override {
        return detectFromTable(id(), kVariants, dir);
    }

    std::unique_ptr<Engine> create(System& system, const GameTarget& target) const override {
        return std::make_unique<NewWorldsEngine>(system, target);
    }
};

} // namespace

const MetaEngine& newWorldsMetaEngine() {
    static const NewWorldsMetaEngine instance;
    return instance;
}

} // namespace kestrel
