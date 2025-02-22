/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (property.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  https://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_PROPERTY_HPP
#define OPENMW_COMPONENTS_NIF_PROPERTY_HPP

#include "base.hpp"

namespace Nif
{

    struct Property : public Named
    {
    };

    struct NiTexturingProperty : public Property
    {
        unsigned short flags{ 0u };

        // A sub-texture
        struct Texture
        {
            /* Clamp mode
            0 - clampS clampT
            1 - clampS wrapT
            2 - wrapS clampT
            3 - wrapS wrapT
            */

            bool inUse;
            NiSourceTexturePtr texture;

            unsigned int clamp, uvSet;

            void read(NIFStream* nif);
            void post(Reader& nif);

            bool wrapT() const { return clamp & 1; }
            bool wrapS() const { return (clamp >> 1) & 1; }
        };

        /* Apply mode:
            0 - replace
            1 - decal
            2 - modulate
            3 - hilight  // These two are for PS2 only?
            4 - hilight2
        */
        unsigned int apply{ 0 };

        /*
         * The textures in this list are as follows:
         *
         * 0 - Base texture
         * 1 - Dark texture
         * 2 - Detail texture
         * 3 - Gloss texture
         * 4 - Glow texture
         * 5 - Bump map texture
         * 6 - Decal texture
         */
        enum TextureType
        {
            BaseTexture = 0,
            DarkTexture = 1,
            DetailTexture = 2,
            GlossTexture = 3,
            GlowTexture = 4,
            BumpTexture = 5,
            DecalTexture = 6,
        };

        std::vector<Texture> textures;
        std::vector<Texture> shaderTextures;

        osg::Vec2f envMapLumaBias;
        osg::Vec4f bumpMapMatrix;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiFogProperty : public Property
    {
        unsigned short mFlags;
        float mFogDepth;
        osg::Vec3f mColour;

        void read(NIFStream* nif) override;
    };

    // These contain no other data than the 'flags' field
    struct NiShadeProperty : public Property
    {
        unsigned short flags{ 0u };
        void read(NIFStream* nif) override
        {
            Property::read(nif);
            if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
                flags = nif->getUShort();
        }
    };

    enum class BSShaderType : unsigned int
    {
        ShaderType_TallGrass = 0,
        ShaderType_Default = 1,
        ShaderType_Sky = 10,
        ShaderType_Skin = 14,
        ShaderType_Water = 17,
        ShaderType_Lighting30 = 29,
        ShaderType_Tile = 32,
        ShaderType_NoLighting = 33
    };

    struct BSShaderProperty : public NiShadeProperty
    {
        unsigned int type{ 0u }, flags1{ 0u }, flags2{ 0u };
        float envMapIntensity{ 0.f };
        void read(NIFStream* nif) override;
    };

    struct BSShaderLightingProperty : public BSShaderProperty
    {
        unsigned int clamp{ 0u };
        void read(NIFStream* nif) override;

        bool wrapT() const { return clamp & 1; }
        bool wrapS() const { return (clamp >> 1) & 1; }
    };

    struct BSShaderPPLightingProperty : public BSShaderLightingProperty
    {
        BSShaderTextureSetPtr textureSet;
        struct RefractionSettings
        {
            float strength{ 0.f };
            int period{ 0 };
        };
        struct ParallaxSettings
        {
            float passes{ 0.f };
            float scale{ 0.f };
        };
        RefractionSettings refraction;
        ParallaxSettings parallax;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSShaderNoLightingProperty : public BSShaderLightingProperty
    {
        std::string filename;
        osg::Vec4f falloffParams;

        void read(NIFStream* nif) override;
    };

    enum class BSLightingShaderType : unsigned int
    {
        ShaderType_Default = 0,
        ShaderType_EnvMap = 1,
        ShaderType_Glow = 2,
        ShaderType_Parallax = 3,
        ShaderType_FaceTint = 4,
        ShaderType_SkinTint = 5,
        ShaderType_HairTint = 6,
        ShaderType_ParallaxOcc = 7,
        ShaderType_MultitexLand = 8,
        ShaderType_LODLand = 9,
        ShaderType_Snow = 10,
        ShaderType_MultiLayerParallax = 11,
        ShaderType_TreeAnim = 12,
        ShaderType_LODObjects = 13,
        ShaderType_SparkleSnow = 14,
        ShaderType_LODObjectsHD = 15,
        ShaderType_EyeEnvmap = 16,
        ShaderType_Cloud = 17,
        ShaderType_LODNoise = 18,
        ShaderType_MultitexLandLODBlend = 19,
        ShaderType_Dismemberment = 20
    };

    struct BSLightingShaderProperty : public BSShaderProperty
    {
        BSShaderTextureSetPtr mTextureSet;
        unsigned int mClamp{ 0u };
        float mAlpha;
        float mGlossiness;
        osg::Vec3f mEmissive, mSpecular;
        float mEmissiveMult, mSpecStrength;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSEffectShaderProperty : public BSShaderProperty
    {
        osg::Vec2f mUVOffset, mUVScale;
        std::string mSourceTexture;
        unsigned char mClamp;
        unsigned char mLightingInfluence;
        unsigned char mEnvMapMinLOD;
        osg::Vec4f mFalloffParams;
        osg::Vec4f mBaseColor;
        float mBaseColorScale;
        float mFalloffDepth;
        std::string mGreyscaleTexture;

        void read(NIFStream* nif) override;
    };

    struct NiDitherProperty : public Property
    {
        unsigned short flags;
        void read(NIFStream* nif) override
        {
            Property::read(nif);
            flags = nif->getUShort();
        }
    };

    struct NiZBufferProperty : public Property
    {
        unsigned short flags;
        unsigned int testFunction;
        void read(NIFStream* nif) override
        {
            Property::read(nif);
            flags = nif->getUShort();
            testFunction = (flags >> 2) & 0x7;
            if (nif->getVersion() >= NIFStream::generateVersion(4, 1, 0, 12)
                && nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
                testFunction = nif->getUInt();
        }

        bool depthTest() const { return flags & 1; }

        bool depthWrite() const { return (flags >> 1) & 1; }
    };

    struct NiSpecularProperty : public Property
    {
        unsigned short flags;
        void read(NIFStream* nif) override
        {
            Property::read(nif);
            flags = nif->getUShort();
        }

        bool isEnabled() const { return flags & 1; }
    };

    struct NiWireframeProperty : public Property
    {
        unsigned short flags;
        void read(NIFStream* nif) override
        {
            Property::read(nif);
            flags = nif->getUShort();
        }

        bool isEnabled() const { return flags & 1; }
    };

    // The rest are all struct-based
    template <typename T>
    struct StructPropT : Property
    {
        T data;
        unsigned short flags;

        void read(NIFStream* nif) override
        {
            Property::read(nif);
            flags = nif->getUShort();
            data.read(nif);
        }
    };

    struct S_MaterialProperty
    {
        // The vector components are R,G,B
        osg::Vec3f ambient{ 1.f, 1.f, 1.f }, diffuse{ 1.f, 1.f, 1.f };
        osg::Vec3f specular, emissive;
        float glossiness{ 0.f }, alpha{ 0.f }, emissiveMult{ 1.f };

        void read(NIFStream* nif);
    };

    struct S_AlphaProperty
    {
        /*
            NiAlphaProperty blend modes (glBlendFunc):
            0000 GL_ONE
            0001 GL_ZERO
            0010 GL_SRC_COLOR
            0011 GL_ONE_MINUS_SRC_COLOR
            0100 GL_DST_COLOR
            0101 GL_ONE_MINUS_DST_COLOR
            0110 GL_SRC_ALPHA
            0111 GL_ONE_MINUS_SRC_ALPHA
            1000 GL_DST_ALPHA
            1001 GL_ONE_MINUS_DST_ALPHA
            1010 GL_SRC_ALPHA_SATURATE

            test modes (glAlphaFunc):
            000 GL_ALWAYS
            001 GL_LESS
            010 GL_EQUAL
            011 GL_LEQUAL
            100 GL_GREATER
            101 GL_NOTEQUAL
            110 GL_GEQUAL
            111 GL_NEVER

            Taken from:
            http://niftools.sourceforge.net/doc/nif/NiAlphaProperty.html
        */

        // Tested against when certain flags are set (see above.)
        unsigned char threshold;

        void read(NIFStream* nif);
    };

    /*
        Docs taken from:
        http://niftools.sourceforge.net/doc/nif/NiStencilProperty.html
     */
    struct S_StencilProperty
    {
        // Is stencil test enabled?
        unsigned char enabled;

        /*
            0   TEST_NEVER
            1   TEST_LESS
            2   TEST_EQUAL
            3   TEST_LESS_EQUAL
            4   TEST_GREATER
            5   TEST_NOT_EQUAL
            6   TEST_GREATER_EQUAL
            7   TEST_NEVER (though nifskope comment says TEST_ALWAYS, but ingame it is TEST_NEVER)
         */
        int compareFunc;
        unsigned stencilRef;
        unsigned stencilMask;
        /*
            Stencil test fail action, depth test fail action and depth test pass action:
            0   ACTION_KEEP
            1   ACTION_ZERO
            2   ACTION_REPLACE
            3   ACTION_INCREMENT
            4   ACTION_DECREMENT
            5   ACTION_INVERT
         */
        int failAction;
        int zFailAction;
        int zPassAction;
        /*
            Face draw mode:
            0   DRAW_CCW_OR_BOTH
            1   DRAW_CCW        [default]
            2   DRAW_CW
            3   DRAW_BOTH
         */
        int drawMode;

        void read(NIFStream* nif);
    };

    struct NiAlphaProperty : public StructPropT<S_AlphaProperty>
    {
        bool useAlphaBlending() const { return flags & 1; }
        int sourceBlendMode() const { return (flags >> 1) & 0xF; }
        int destinationBlendMode() const { return (flags >> 5) & 0xF; }
        bool noSorter() const { return (flags >> 13) & 1; }

        bool useAlphaTesting() const { return (flags >> 9) & 1; }
        int alphaTestMode() const { return (flags >> 10) & 0x7; }
    };

    struct NiVertexColorProperty : public Property
    {
        enum class VertexMode : unsigned int
        {
            VertMode_SrcIgnore = 0,
            VertMode_SrcEmissive = 1,
            VertMode_SrcAmbDif = 2
        };

        enum class LightMode : unsigned int
        {
            LightMode_Emissive = 0,
            LightMode_EmiAmbDif = 1
        };

        unsigned short mFlags;
        VertexMode mVertexMode;
        LightMode mLightingMode;

        void read(NIFStream* nif) override;
    };

    struct NiStencilProperty : public Property
    {
        S_StencilProperty data;
        unsigned short flags{ 0u };

        void read(NIFStream* nif) override
        {
            Property::read(nif);
            if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
                flags = nif->getUShort();
            data.read(nif);
        }
    };

    struct NiMaterialProperty : public Property
    {
        S_MaterialProperty data;
        unsigned short flags{ 0u };

        void read(NIFStream* nif) override
        {
            Property::read(nif);
            if (nif->getVersion() >= NIFStream::generateVersion(3, 0, 0, 0)
                && nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
                flags = nif->getUShort();
            data.read(nif);
        }
    };

} // Namespace
#endif
