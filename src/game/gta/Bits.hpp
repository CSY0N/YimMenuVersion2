#pragma once
namespace Bits
{
	template<typename T>
	inline void ClearBit(T* address, int pos)
	{
		*address &= ~(1 << pos);
	}
	
	template<typename T>
	inline void ClearBits(T* address, int bits)
	{
		*address &= ~(bits);
	}

	template<typename T>
	inline bool HasBitsSet(T* address, int pos)
	{
		return *address & 1 << pos;
	}

	template<typename T>
	inline bool HasBitsSets(T* address, T bits)
	{
		return (*address & bits) == bits;
	}

	template<typename T>
	inline void SetBit(T* address, int pos)
	{
		*address |= 1 << pos;
	}

	template<typename T>
	inline void SetBits(T* address, int bits)
	{
		*address |= bits;
	}
}
