#include "mod/getPlayerSkin.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/memory/Hook.h"
#include "mc/network/packet/PlayerSkinPacket.h"
#include "mc/world/actor/player/SerializedSkin.h"
#include "mc/world/actor/player/SerializedSkinImpl.h"
#include "mc/deps/core/image/Image.h"
#include <fstream>

namespace get_skin {

static ll::io::Logger* g_logger = nullptr;

// 监听皮肤包，获取皮肤图片数据
// 注意：read 是虚函数，需要加 $
LL_TYPE_INSTANCE_HOOK(
    PlayerSkinPacketHook,
    HookPriority::Normal,
    PlayerSkinPacket,
    &PlayerSkinPacket::$read,  // ← 加 $ 前缀！
    ::Bedrock::Result<void>,
    ::ReadOnlyBinaryStream& stream
) {
    auto result = origin(stream);
    
    if (result && g_logger) {
        auto& uuid = this->mUUID;
        auto& skin = this->mSkin;
        
        // 获取 UUID 字符串
        std::string uuidStr = uuid.toString();  // ← 用 toString()
        
        // 获取皮肤内部实现 - 需要从 TypedStorage 中获取实际指针
        // skin.mSkinImpl 是 TypedStorage，需要调用 get() 或解引用
        auto* skinImpl = skin.mSkinImpl.get();  // 这行应该没问题
        
        if (skinImpl) {
            g_logger->info("玩家 {} 皮肤 ID: {}", uuidStr, skinImpl->mId);
            g_logger->info("皮肤图片尺寸: {}x{}", 
                skinImpl->mSkinImage.mWidth, 
                skinImpl->mSkinImage.mHeight);
            g_logger->info("是否自定义角色: {}", skinImpl->mIsPersona);
            
            // 获取皮肤图片数据
            auto& imageBytes = skinImpl->mSkinImage.mImageBytes;
            size_t dataSize = imageBytes.size();
            
            if (dataSize > 0) {
                g_logger->info("皮肤数据大小: {} 字节", dataSize);
                
                // 保存到文件
                std::string filename = "skin_" + uuidStr + ".png";
                std::ofstream file(filename, std::ios::binary);
                if (file.is_open()) {
                    file.write(reinterpret_cast<const char*>(imageBytes.data()), dataSize);
                    file.close();
                    g_logger->info("皮肤已保存到: {}", filename);
                } else {
                    g_logger->error("无法保存皮肤文件: {}", filename);
                }
            } else {
                g_logger->warn("皮肤数据为空");
            }
        } else {
            g_logger->warn("无法获取皮肤实现数据");
        }
    }
    
    return result;
}

// 插件实现
getSkin& getSkin::getInstance() {
    static getSkin instance;
    return instance;
}

bool getSkin::load() {
    g_logger = &getSelf().getLogger();
    getSelf().getLogger().info("getSkin 加载中...");
    return true;
}

bool getSkin::enable() {
    getSelf().getLogger().info("getSkin 已启用，皮肤监听已激活");
    static ll::memory::HookRegistrar<PlayerSkinPacketHook> registrar;
    return true;
}

bool getSkin::disable() {
    getSelf().getLogger().info("getSkin 已禁用");
    PlayerSkinPacketHook::unhook();
    return true;
}

} // namespace get_skin

LL_REGISTER_MOD(get_skin::getSkin, get_skin::getSkin::getInstance());