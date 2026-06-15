/**
 * \file
 * \author Rudy Castan
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "FrameBuffer.hpp"

#include "Environment.hpp"
#include "GL.hpp"
#include "util/Logger.hpp"

namespace opengl
{

    FrameBuffer::~FrameBuffer() noexcept
    {
        delete_resources();
    }

    FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept : frameBufferHandle(other.frameBufferHandle), depthTexture(std::move(other.depthTexture)), colorTexture(std::move(other.colorTexture))
    {
        other.frameBufferHandle = 0;
    }

    FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept
    {
        std::swap(frameBufferHandle, other.frameBufferHandle);
        std::swap(depthTexture, other.depthTexture);
        std::swap(colorTexture, other.colorTexture);
        return *this;
    }

    void FrameBuffer::Use(bool bind) const
    {
        GL::BindFramebuffer(GL_FRAMEBUFFER, bind ? frameBufferHandle : 0);
    }

    void FrameBuffer::LoadWithSpecification(Specification spec)
    {
        delete_resources();

        const int width  = spec.Width;
        const int height = spec.Height;

        if (spec.DepthFormat != Texture::DepthComponentSize::None)
        {
            if (const auto loaded = depthTexture.LoadAsDepthTexture(width, height, spec.DepthFormat); !loaded)
            {
                util::log_error("Failed to create depth texture");
                return;
            }
        }
        else
        {
            depthTexture = Texture();
        }

        GLenum draw_buffers[] = { GL_NONE };

        if (spec.ColorFormat != ColorComponent::None)
        {
            const auto color_format = static_cast<Texture::ColorFormat>(spec.ColorFormat);

            if (const auto loaded = colorTexture.LoadAsFormat(width, height, color_format); !loaded)
                {
                util::log_error("Failed to create color texture");
                return;
            }
            draw_buffers[0] = GL_COLOR_ATTACHMENT0;
        }
        else
        {
            colorTexture = Texture();
        }


        GLenum status_result{};

        GL::GenFramebuffers(1, &frameBufferHandle);
        GL::BindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);

        if (spec.DepthFormat != Texture::DepthComponentSize::None)
        {
            GL::FramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_2D,
                depthTexture.GetHandle(),
                0
            );
        }

        if (spec.ColorFormat != ColorComponent::None)
        {
            GL::FramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                colorTexture.GetHandle(),
                0
            );
        }

        GL::DrawBuffers(1, draw_buffers);

        status_result = GL::CheckFramebufferStatus(GL_FRAMEBUFFER);

        GL::BindFramebuffer(GL_FRAMEBUFFER, 0);

        if (status_result != GL_FRAMEBUFFER_COMPLETE)
        {
            std::string message = "Framebuffer is not complete.\n";
            switch (status_result)
            {
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: message += "\tERROR: Framebuffer incomplete: Attachment is NOT complete.\n"; break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: message += "\tERROR: Framebuffer incomplete: No image is attached to FBO.\n"; break;
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: message += "\tERROR: Framebuffer incomplete: Draw buffer.\n"; break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: message += "\tERROR: Framebuffer incomplete: Read buffer.\n"; break;
                case GL_FRAMEBUFFER_UNSUPPORTED: message += "\tERROR: Framebuffer incomplete: Unsupported by FBO implementation.\n"; break;
            }
            util::log_error("{}", message);
            throw std::runtime_error{ message };
        }
    }

    void FrameBuffer::delete_resources() noexcept
    {
        if (frameBufferHandle != 0)
        {
            GL::DeleteFramebuffers(1, &frameBufferHandle);
            frameBufferHandle = 0;
        }

        depthTexture = Texture();
        colorTexture = Texture();
    }
}
