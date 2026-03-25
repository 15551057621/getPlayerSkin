#include "mod/getPlayerSkin.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/memory/Hook.h"
#include "mc/network/packet/PlayerSkinPacket.h"
#include "mc/world/actor/player/SerializedSkin.h"
#include "mc/world/actor/player/SerializedSkinImpl.h"
#include <fstream>

namespace get_skin {

static ll::io::Logger* g_logger = nullptr;

// 监听皮肤包，获取皮肤图片数据
LL_TYPE_INSTANCE_HOOK(
    PlayerSkinPacketHook,
    HookPriority::Normal,
    PlayerSkinPacket,
    &PlayerSkinPacket::read,
    ::Bedrock::Result<void>,
    ::ReadOnlyBinaryStream& stream
) {
    auto result = origin(stream);
    
    if (result && g_logger) {
        auto& uuid = this->mUUID;
        auto& skin = this->mSkin;
        
        // 获取内部实现
        auto* skinImpl = skin.mSkinImpl.get();
        if (skinImpl) {
            // 获取 UUID 字符串
            std::string uuidStr = uuid.asString();  // 如果还报错，改用 uuid.toString()
            
            g_logger->info("玩家 {} 皮肤 ID: {}", uuidStr, skinImpl->mId);
            
            // 尝试获取皮肤图片数据（需要查看 mce::Image 的结构）
            // 可能需要用 skinImpl->mSkinImage.getData() 或其他方法
            
            // 简化：先只输出日志，不保存文件
            g_logger->info("皮肤图片尺寸: {}x{}", 
                skinImpl->mSkinImage.mWidth, 
                skinImpl->mSkinImage.mHeight);
            g_logger->info("是否自定义角色: {}", skinImpl->mIsPersona);
            
            // TODO: 保存皮肤文件需要先了解 mce::Image 的结构
            // 暂时注释掉文件保存部分
            /*
            // 获取皮肤图片原始数据
            auto& imageData = skinImpl->mSkinImage.mData;
            g_logger->info("皮肤数据大小: {} 字节", imageData.size());
            
            std::string filename = "skin_" + uuidStr + ".png";
            std::ofstream file(filename, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<char*>(imageData.data()), imageData.size());
                file.close();
                g_logger->info("皮肤已保存到: {}", filename);
            } else {
                g_logger->error("无法保存皮肤文件: {}", filename);
            }
            */
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