#include <pch.h>
#include <game/rtech/assets/particle_script.h>

#undef max
void LoadParticleScriptAsset(CAssetContainer* const container, CAsset* const asset)
{
    UNUSED(container);
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    ParticleScriptAsset* particleScriptAsset = nullptr;

    // [rika]: this was only ever version 8, and is now cut from the game
    switch (pakAsset->version())
    {
    case 8:
    {
        const ParticleScriptAssetHeader_v8_t* const hdr = reinterpret_cast<const ParticleScriptAssetHeader_v8_t* const>(pakAsset->header());
        particleScriptAsset = new ParticleScriptAsset(hdr);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    assertm(particleScriptAsset->name, "particle script had invalid name");
    const std::string name = std::format("particle_script/{}.rpak", particleScriptAsset->name);

#if _DEBUG
    const uint64_t guidFromName = RTech::StringToGuid(name.c_str());
    assertm(guidFromName == pakAsset->guid(), "incorrect name for asset");
#endif

    pakAsset->setExtraData(particleScriptAsset);
    pakAsset->SetAssetName(name, true);
}

// [rika]: todo preview textures and shaders ?

void InitParticleScriptAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'kspr',
        .headerAlignment = 8,
        .loadFunc = LoadParticleScriptAsset,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { nullptr, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}