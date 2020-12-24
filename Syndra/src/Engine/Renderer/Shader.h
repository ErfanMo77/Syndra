#pragma once

#include <string>

namespace Syndra {

	class Shader {

	public:
		Shader(const std::string& vertexSrc,const std::string& fragSrc);
		~Shader();

		void Bind() const;
		void Unbind() const;
	
	private:
		uint32_t m_RendererID;
	};

}