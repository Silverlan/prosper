// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.resizable_buffer;

export import :buffer.buffer;

export {
#pragma warning(push)
#pragma warning(disable : 4251)
	namespace prosper {
		class DLLPROSPER IResizableBuffer : virtual public IBuffer {
		  public:
			enum class ReallocationBehavior : uint8_t {
				DeviceWaitIdle = 0,
				SafelyFreeOldBuffer,
			};
			IResizableBuffer(IBuffer &parent);
			ReallocationBehavior GetReallocationBehavior() const { return m_reallocationBehavior; }
			void SetReallocationBehavior(ReallocationBehavior behavior) { m_reallocationBehavior = behavior; }
			void AddReallocationCallback(const std::function<void()> &fCallback);

			void SetResizable(bool resizable) { m_resizable = resizable; }
			bool IsResizable() const { return m_resizable; }

			// Should only be used for debugging purposes
			void ReallocateMemory();
		  protected:
			virtual void MoveInternalBuffer(IBuffer &other) = 0;
			virtual void ReleaseBufferSafely() = 0;
			bool ReallocateMemory(size_t requiredSize);
			void RunReallocationCallbacks();

			std::vector<IBuffer *> m_allocatedSubBuffers;
			uint64_t m_baseSize = 0ull; // Un-aligned size of m_buffer
			ReallocationBehavior m_reallocationBehavior = ReallocationBehavior::DeviceWaitIdle;
			bool m_resizable = true;

			std::vector<std::function<void()>> m_onReallocCallbacks;
		};
	};
#pragma warning(pop)
}
