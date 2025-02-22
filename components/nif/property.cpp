#include "property.hpp"

#include "controlled.hpp"
#include "data.hpp"

namespace Nif
{

    void NiTexturingProperty::Texture::read(NIFStream* nif)
    {
        inUse = nif->getBoolean();
        if (!inUse)
            return;

        texture.read(nif);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            clamp = nif->getInt();
            nif->skip(4); // Filter mode. Ignoring because global filtering settings are more sensible
        }
        else
        {
            clamp = nif->getUShort() & 0xF;
        }
        // Max anisotropy. I assume we'll always only use the global anisotropy setting.
        if (nif->getVersion() >= NIFStream::generateVersion(20, 5, 0, 4))
            nif->getUShort();

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            uvSet = nif->getUInt();

        // Two PS2-specific shorts.
        if (nif->getVersion() < NIFStream::generateVersion(10, 4, 0, 2))
            nif->skip(4);
        if (nif->getVersion() <= NIFStream::generateVersion(4, 1, 0, 18))
            nif->skip(2); // Unknown short
        else if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            if (nif->getBoolean()) // Has texture transform
            {
                nif->getVector2(); // UV translation
                nif->getVector2(); // UV scale
                nif->getFloat(); // W axis rotation
                nif->getUInt(); // Transform method
                nif->getVector2(); // Texture rotation origin
            }
        }
    }

    void NiTexturingProperty::Texture::post(Reader& nif)
    {
        texture.post(nif);
    }

    void NiTexturingProperty::read(NIFStream* nif)
    {
        Property::read(nif);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD
            || nif->getVersion() >= NIFStream::generateVersion(20, 1, 0, 2))
            flags = nif->getUShort();
        if (nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 1))
            apply = nif->getUInt();

        unsigned int numTextures = nif->getUInt();

        if (!numTextures)
            return;

        textures.resize(numTextures);
        for (unsigned int i = 0; i < numTextures; i++)
        {
            textures[i].read(nif);
            if (i == 5 && textures[5].inUse) // Bump map settings
            {
                envMapLumaBias = nif->getVector2();
                bumpMapMatrix = nif->getVector4();
            }
            else if (i == 7 && textures[7].inUse && nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 5))
                /*float parallaxOffset = */ nif->getFloat();
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            unsigned int numShaderTextures = nif->getUInt();
            shaderTextures.resize(numShaderTextures);
            for (unsigned int i = 0; i < numShaderTextures; i++)
            {
                shaderTextures[i].read(nif);
                if (shaderTextures[i].inUse)
                    nif->getUInt(); // Unique identifier
            }
        }
    }

    void NiTexturingProperty::post(Reader& nif)
    {
        Property::post(nif);
        for (size_t i = 0; i < textures.size(); i++)
            textures[i].post(nif);
        for (size_t i = 0; i < shaderTextures.size(); i++)
            shaderTextures[i].post(nif);
    }

    void BSShaderProperty::read(NIFStream* nif)
    {
        NiShadeProperty::read(nif);
        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
        {
            type = nif->getUInt();
            flags1 = nif->getUInt();
            flags2 = nif->getUInt();
            envMapIntensity = nif->getFloat();
        }
    }

    void BSShaderLightingProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);
        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
            clamp = nif->getUInt();
    }

    void BSShaderPPLightingProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);
        textureSet.read(nif);
        if (nif->getBethVersion() <= 14)
            return;
        refraction.strength = nif->getFloat();
        refraction.period = nif->getInt();
        if (nif->getBethVersion() <= 24)
            return;
        parallax.passes = nif->getFloat();
        parallax.scale = nif->getFloat();
    }

    void BSShaderPPLightingProperty::post(Reader& nif)
    {
        BSShaderLightingProperty::post(nif);
        textureSet.post(nif);
    }

    void BSShaderNoLightingProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);
        filename = nif->getSizedString();
        if (nif->getBethVersion() >= 27)
            falloffParams = nif->getVector4();
    }

    void BSLightingShaderProperty::read(NIFStream* nif)
    {
        type = nif->getUInt();
        BSShaderProperty::read(nif);
        flags1 = nif->getUInt();
        flags2 = nif->getUInt();
        nif->skip(8); // UV offset
        nif->skip(8); // UV scale
        mTextureSet.read(nif);
        mEmissive = nif->getVector3();
        mEmissiveMult = nif->getFloat();
        mClamp = nif->getUInt();
        mAlpha = nif->getFloat();
        nif->getFloat(); // Refraction strength
        mGlossiness = nif->getFloat();
        mSpecular = nif->getVector3();
        mSpecStrength = nif->getFloat();
        nif->skip(8); // Lighting effects
        switch (static_cast<BSLightingShaderType>(type))
        {
            case BSLightingShaderType::ShaderType_EnvMap:
                nif->skip(4); // Environment map scale
                break;
            case BSLightingShaderType::ShaderType_SkinTint:
            case BSLightingShaderType::ShaderType_HairTint:
                nif->skip(12); // Tint color
                break;
            case BSLightingShaderType::ShaderType_ParallaxOcc:
                nif->skip(4); // Max passes
                nif->skip(4); // Scale
                break;
            case BSLightingShaderType::ShaderType_MultiLayerParallax:
                nif->skip(4); // Inner layer thickness
                nif->skip(4); // Refraction scale
                nif->skip(8); // Inner layer texture scale
                nif->skip(4); // Environment map strength
                break;
            case BSLightingShaderType::ShaderType_SparkleSnow:
                nif->skip(16); // Sparkle parameters
                break;
            case BSLightingShaderType::ShaderType_EyeEnvmap:
                nif->skip(4); // Cube map scale
                nif->skip(12); // Left eye cube map offset
                nif->skip(12); // Right eye cube map offset
                break;
            default:
                break;
        }
    }

    void BSLightingShaderProperty::post(Reader& nif)
    {
        BSShaderProperty::post(nif);
        mTextureSet.post(nif);
    }

    void BSEffectShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);
        flags1 = nif->getUInt();
        flags2 = nif->getUInt();
        mUVOffset = nif->getVector2();
        mUVScale = nif->getVector2();
        mSourceTexture = nif->getSizedString();
        unsigned int miscParams = nif->getUInt();
        mClamp = miscParams & 0xFF;
        mLightingInfluence = (miscParams >> 8) & 0xFF;
        mEnvMapMinLOD = (miscParams >> 16) & 0xFF;
        mFalloffParams = nif->getVector4();
        mBaseColor = nif->getVector4();
        mBaseColorScale = nif->getFloat();
        mFalloffDepth = nif->getFloat();
        mGreyscaleTexture = nif->getSizedString();
    }

    void NiFogProperty::read(NIFStream* nif)
    {
        Property::read(nif);
        mFlags = nif->getUShort();
        mFogDepth = nif->getFloat();
        mColour = nif->getVector3();
    }

    void S_MaterialProperty::read(NIFStream* nif)
    {
        if (nif->getBethVersion() < 26)
        {
            ambient = nif->getVector3();
            diffuse = nif->getVector3();
        }
        specular = nif->getVector3();
        emissive = nif->getVector3();
        glossiness = nif->getFloat();
        alpha = nif->getFloat();
        if (nif->getBethVersion() >= 22)
            emissiveMult = nif->getFloat();
    }

    void NiVertexColorProperty::read(NIFStream* nif)
    {
        Property::read(nif);
        mFlags = nif->getUShort();
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            mVertexMode = static_cast<VertexMode>(nif->getUInt());
            mLightingMode = static_cast<LightMode>(nif->getUInt());
        }
        else
        {
            mVertexMode = static_cast<VertexMode>((mFlags >> 4) & 0x3);
            mLightingMode = static_cast<LightMode>((mFlags >> 3) & 0x1);
        }
    }

    void S_AlphaProperty::read(NIFStream* nif)
    {
        threshold = nif->getChar();
    }

    void S_StencilProperty::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            enabled = nif->getChar();
            compareFunc = nif->getInt();
            stencilRef = nif->getUInt();
            stencilMask = nif->getUInt();
            failAction = nif->getInt();
            zFailAction = nif->getInt();
            zPassAction = nif->getInt();
            drawMode = nif->getInt();
        }
        else
        {
            unsigned short flags = nif->getUShort();
            enabled = flags & 0x1;
            failAction = (flags >> 1) & 0x7;
            zFailAction = (flags >> 4) & 0x7;
            zPassAction = (flags >> 7) & 0x7;
            drawMode = (flags >> 10) & 0x3;
            compareFunc = (flags >> 12) & 0x7;
            stencilRef = nif->getUInt();
            stencilMask = nif->getUInt();
        }
    }

}
