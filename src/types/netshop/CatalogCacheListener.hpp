#pragma once
#include "types/netshop/netCatalogBaseItem.hpp"
namespace rage
{
	class CatalogCacheListener
	{
	public:
		virtual ~CatalogCacheListener() = default;
		virtual void OnCacheFileLoaded(/*catalog*/) = 0;
	};
	static_assert(sizeof(CatalogCacheListener) == 0x8);

	struct netCatalogNode
	{
		std::uint32_t m_KeyHash;        // 0x00
		std::uint32_t m_Unknown04;      // 0x04
		void* m_Unknown08;              // 0x08
		netCatalogNode* m_Next;         // 0x10
		std::uint8_t m_Unknown18[0x08]; // 0x18
		netCatalogBaseItem m_Item;      // 0x20

		netCatalogBaseItem* Item()
		{
			return &m_Item;
		}
		const netCatalogBaseItem* Item() const
		{
			return &m_Item;
		}
	};

	static_assert(offsetof(netCatalogNode, m_Next) == 0x10);
	static_assert(offsetof(netCatalogNode, m_Item) == 0x20);

	class netCatalog : public CatalogCacheListener
	{
	public:
		char pad_0008[0x28];                 // 0x08 to 0x2F
		netCatalogNode** m_BucketArray;      // 0x30
		std::uint16_t m_BucketCount;         // 0x38
		char pad_003A[0x16];                 // 0x3A to 0x4F
		std::uint32_t m_crc;                 // 0x50
		std::uint32_t m_Unknown54;           // 0x54
		std::uint32_t m_version;             // 0x58
		std::uint32_t m_latestServerVersion; // 0x5C

		template<typename Fn>
		void ForEachItem(Fn&& fn) const
		{
			for (std::uint16_t i = 0; i < m_BucketCount; ++i)
			{
				for (auto* node = m_BucketArray[i]; node; node = node->m_Next)
				{
					fn(*node->Item());
				}
			}
		}
	};

}
