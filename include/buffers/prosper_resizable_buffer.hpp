#ifndef __PROSPER_RESIZABLE_BUFFER_HPP__
#define __PROSPER_RESIZABLE_BUFFER_HPP__

#include "buffers/prosper_buffer.hpp"
#include "prosper_util.hpp"

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class DLLPROSPER ResizableBuffer
		: public Buffer
	{
	public:
		ResizableBuffer(Context &context,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf,const util::BufferCreateInfo &createInfo,uint64_t maxTotalSize);

		void AddReallocationCallback(const std::function<void()> &fCallback);
	protected:
		uint64_t m_baseSize = 0ull; // Un-aligned size of m_buffer
		uint64_t m_maxTotalSize = 0ull;
		util::BufferCreateInfo m_createInfo = {};

		std::vector<std::function<void()>> m_reallocationCallbacks;
	};
};
#pragma warning(pop)

#endif
