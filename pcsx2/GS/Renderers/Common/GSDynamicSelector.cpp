#include <algorithm>

#include "common/Assertions.h"

#include "GSDynamicSelector.h"

void GSDynamicSelector::AddSelector(const std::string& name, u32 bits)
{
	u32 index = static_cast<u32>(m_selectors.size());
	m_selectors.emplace_back(index, m_curr_bit, bits, name);
	m_curr_bit += bits;
}

template<typename T>
T GSDynamicSelector::GetSelectorValue(const std::string& name)
{
	const auto it = std::find_if(m_selectors.begin(), m_selectors.end(),
		[](const Selector& sel) { return sel.name == name });
	if (it != m_selectors.end())
		return static_cast<T>(it->value);
	pxFail("Unknown selector");
	return static_cast<T>(0);
}

template<typename T>
T GSDynamicSelector::GetSelectorValue(u32 index)
{
	if (index < static_cast<u32>(m_selectors.size()))
		return static_cast<T>(m_selectors[index].value);
	pxFail("Selector index out of range");
	return static_cast<T>(0);
}

