// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:gli;

export import std.compat;

export {
	namespace prosper {
		enum class Format : uint32_t;
	};

	// We have to separate gli as a wrapper because we can't
	// include both glm and gli at the same time.
	// (Because we're using glm as a module and gli is not compatible with that)
	namespace gli_wrapper {
		uint32_t get_block_size(prosper::Format format);
		uint32_t get_component_count(prosper::Format format);
		struct GliTexture;
		struct GliTextureWrapper {
			GliTextureWrapper(uint32_t width, uint32_t height, prosper::Format format, uint32_t numLevels);
			~GliTextureWrapper();
			std::shared_ptr<GliTexture> texture;

			size_t size() const;
			size_t size(uint32_t imimap) const;
			void *data(uint32_t layer, uint32_t face, uint32_t mipmap);
			const void *data(uint32_t layer, uint32_t face, uint32_t mipmap) const;

			bool save_dds(const std::string &fileName) const;
			bool save_ktx(const std::string &fileName) const;
		};
	};
}
