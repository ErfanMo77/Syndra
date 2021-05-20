#pragma once

#include <string>

#include "Engine/Core/Core.h"

namespace Syndra{

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool operator==(const Texture& other) const = 0;

		virtual std::string GetPath() const = 0;

		static void BindTexture(uint32_t rendererID, uint32_t slot);
	};

	class Texture1D : public Texture
	{
	public:
		static Ref<Texture1D> Create(uint32_t size);
		static Ref<Texture1D> Create(uint32_t size, void* data);
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height);
		static Ref<Texture2D> Create(const std::string& path, bool sRGB = false);
		static Ref<Texture2D> Create(uint32_t width, uint32_t height, const unsigned char* data, bool sRGB = false);
	};

}