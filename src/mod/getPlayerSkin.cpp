#include "mod/getPlayerSkin.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/memory/Hook.h"
#include "mc/network/packet/PlayerSkinPacket.h"
#include "mc/world/actor/player/SerializedSkin.h"
#include "mc/world/actor/player/SerializedSkinImpl.h"
#include "mc/deps/core/image/Image.h"
#include "ll/api/service/Bedrock.h"
#include "mc/world/level/Level.h"
#include <fstream>
#include <filesystem>

namespace get_skin {

static ll::io::Logger* g_logger = nullptr;
static std::string g_savePath = "skins/";  // 保存皮肤的文件夹

// 保存皮肤到文件
void saveSkinToFile(const mce::UUID& uuid, SerializedSkinImpl* skinImpl) {
    if (!skinImpl) return;
    
    std::string uuidStr = uuid.asString();
    auto& imageBytes = skinImpl->mSkinImage.mImageBytes;
    size_t dataSize = imageBytes.size();
    
    if (dataSize == 0) {
        if (g_logger) g_logger->warn("玩家 {} 皮肤数据为空", uuidStr);
        return;
    }
    
    // 创建保存目录
    std::filesystem::create_directories(g_savePath);
    
    // 保存皮肤文件
    std::string filename = g_savePath + "skin_" + uuidStr + ".png";
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(imageBytes.data()), dataSize);
        file.close();
        if (g_logger) {
            g_logger->info("玩家 {} 皮肤已保存: {} ({}x{}, {} 字节)", 
                uuidStr, filename, 
                skinImpl->mSkinImage.mWidth, 
                skinImpl->mSkinImage.mHeight, 
                dataSize);
        }
    } else {
        if (g_logger) g_logger->error("无法保存玩家 {} 皮肤文件: {}", uuidStr, filename);
    }
}

// 监听皮肤包，获取皮肤图片数据
// 注意：read 是虚函数，需要加 $ 前缀
LL_TYPE_INSTANCE_HOOK(
    PlayerSkinPacketHook,
    HookPriority::Normal,
    PlayerSkinPacket,
    &PlayerSkinPacket::$read,
    ::Bedrock::Result<void>,
    ::ReadOnlyBinaryStream& stream
) {
    auto result = origin(stream);
    
    if (result && g_logger) {
        auto& uuid = this->mUUID;
        auto& skin = this->mSkin;
        
        // 获取皮肤内部实现
        auto* skinImpl = skin.mSkinImpl.get();
        if (skinImpl) {
            // 保存皮肤到文件
            saveSkinToFile(uuid, skinImpl);
        } else {
            g_logger->warn("无法获取玩家 {} 的皮肤实现数据", uuid.asString());
        }
    }
    
    return result;
}

// 主动获取所有在线玩家的皮肤
void fetchAllOnlinePlayersSkin() {
    auto level = ll::service::getLevel();
    if (!level) {
        if (g_logger) g_logger->error("无法获取 Level 服务");
        return;
    }
    
    auto& players = level->getPlayers();
    if (g_logger) g_logger->info("开始获取 {} 个在线玩家的皮肤", players.size());
    
    for (auto& player : players) {
        if (!player) continue;
        
        auto& skin = player->mSkin;
        auto* skinImpl = skin.mSkinImpl.get();
        if (skinImpl) {
            saveSkinToFile(player->getUuid(), skinImpl);
        } else {
            if (g_logger) g_logger->warn("玩家 {} 的皮肤数据不可用", player->getRealName());
        }
    }
}

// 插件实现
getSkin& getSkin::getInstance() {
    static getSkin instance;
    return instance;
}

bool getSkin::load() {
    g_logger = &getSelf().getLogger();
    getSelf().getLogger().info("getSkin 加载中...");
    getSelf().getLogger().info("皮肤保存目录: {}", g_savePath);
    return true;
}

bool getSkin::enable() {
    getSelf().getLogger().info("getSkin 已启用，皮肤监听已激活");
    
    // 注册 Hook
    static ll::memory::HookRegistrar<PlayerSkinPacketHook> registrar;
    
    // 可选：主动获取当前所有在线玩家的皮肤
    // fetchAllOnlinePlayersSkin();
    
    return true;
}

bool getSkin::disable() {
    getSelf().getLogger().info("getSkin 已禁用");
    PlayerSkinPacketHook::unhook();
    return true;
}

} // namespace get_skin

LL_REGISTER_MOD(get_skin::getSkin, get_skin::getSkin::getInstance());