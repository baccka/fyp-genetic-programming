#pragma once

namespace genetic {
	
template <typename It>
class IteratorRange {
public:
	IteratorRange(It first, It last ): begin_(first), end_(last) {
	}

	It begin() const {
		return begin_;
	}

	It end() const {
		return end_;
	}
	
private:
	It begin_;
	It end_;
};
	
template <typename It>
IteratorRange<It> makeIteratorRange(It f, It l) {
	return IteratorRange<It>(f, l);
}

} // end namespace genetic
