#include <vector>

#include <fmt/core.h>

class GSDynamicSelector
{
public:
	void AddSelector(const std::string& name, u32 bits);

	template<typename T>
	T GetSelectorValue(const std::string& name);
	template<typename T>
	T GetSelectorValue(u32 index);

	// CONTIUE HERE
	void Encode(u32* out, u32 num_values);
	void Decode(u32* out, u32 num_values);
private:
	struct Selector
	{
		u32 index;
		u32 start;
		u32 bits;
		u32 value;
		std::string name;
	};

	std::vector<Selector> m_selectors;
	u32 m_curr_bit = 0;
};