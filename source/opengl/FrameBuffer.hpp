/**
 * \file
 * \author Rudy Castan
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <array>
#include <filesystem>

#include "Handle.hpp"
#include "Texture.hpp"

namespace opengl
{

    class [[nodiscard]] FrameBuffer
    {
    public:
        FrameBuffer() = default;
        ~FrameBuffer() noexcept;

        FrameBuffer(const FrameBuffer& other) = delete;
        FrameBuffer(FrameBuffer&& other) noexcept;
        FrameBuffer& operator=(const FrameBuffer& other) = delete;
        FrameBuffer& operator=(FrameBuffer&& other) noexcept;

        void Use(bool bind = true) const;

        [[nodiscard]] Handle GetHandle() const noexcept
        {
            return frameBufferHandle;
        }

        const Texture& DepthTexture() const
        {
            return depthTexture;
        }

        const Texture& ColorTexture() const
        {
            return colorTexture;
        }

        Texture& ColorTexture()
        {
            return colorTexture;
        }

        enum ColorComponent
        {
            None,
            RGBA8   = Texture::RGBA8,
            RGBA32F = Texture::RGBA32F,
            R32F    = Texture::R32F
        };

        struct Specification
        {
            int                         Width = 0, Height = 0;
            Texture::DepthComponentSize DepthFormat = Texture::DepthComponentSize::None;
            ColorComponent              ColorFormat = ColorComponent::None;
        };

        void LoadWithSpecification(Specification spec);


    private:
        void delete_resources() noexcept;

    private:
        Handle  frameBufferHandle = 0;
        Texture depthTexture;
        Texture colorTexture;
    };
}
