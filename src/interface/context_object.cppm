// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:context_object;

export import std;

export {
#pragma warning(push)
#pragma warning(disable : 4251)
	namespace prosper {
		class IPrContext;
		class DLLPROSPER ContextObject {
		  public:
			ContextObject(IPrContext &context);
			virtual ~ContextObject() = default;

			IPrContext &GetContext() const;

			virtual void SetDebugName(const std::string &name);
			const std::string &GetDebugName() const;

			// For internal use only
			virtual void OnRelease() {}
		  private:
			mutable std::weak_ptr<IPrContext> m_wpContext = {};
			std::string m_dbgName;
		};
	};
#pragma warning(pop)
}
