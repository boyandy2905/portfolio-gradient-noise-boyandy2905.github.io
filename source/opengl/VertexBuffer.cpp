/**
 * \file
 * \author Rudy Castan
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#include "VertexBuffer.hpp"

#include "GL.hpp"
#include <GL/glew.h>

namespace opengl
{

    VertexBuffer::VertexBuffer(GLsizei size_in_bytes) : size(size_in_bytes)
    {
        constexpr const void* no_data = nullptr;

        GL::GenBuffers(1, &buffer_handle);
        GL::BindBuffer(GL_ARRAY_BUFFER, buffer_handle);
        GL::BufferData(GL_ARRAY_BUFFER, size, no_data, GL_DYNAMIC_DRAW);
        GL::BindBuffer(GL_ARRAY_BUFFER, 0);
    }

    VertexBuffer::~VertexBuffer()
    {
        if (buffer_handle != 0)
        {
            GL::DeleteBuffers(1, &buffer_handle);
        }
    }

    VertexBuffer::VertexBuffer(VertexBuffer&& temp) noexcept : size(temp.size), buffer_handle(temp.buffer_handle)
    {
        temp.buffer_handle = 0;
        temp.size          = 0;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer&& temp) noexcept
    {
        std::swap(buffer_handle, temp.buffer_handle);
        std::swap(size, temp.size);

        return *this;
    }

    void VertexBuffer::Use(bool bind) const
    {
        GL::BindBuffer(GL_ARRAY_BUFFER, bind ? buffer_handle : 0);
    }

    void VertexBuffer::send_buffer_data_to_gpu(const void* data, GLsizei size_bytes, GLsizei starting_offset) const
    {
        GL::BindBuffer(GL_ARRAY_BUFFER, buffer_handle);
        GL::BufferSubData(GL_ARRAY_BUFFER, starting_offset, size_bytes, data);
        GL::BindBuffer(GL_ARRAY_BUFFER, 0);
    }
}
