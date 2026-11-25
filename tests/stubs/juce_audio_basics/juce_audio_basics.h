// Minimal stand-in for juce::AudioBuffer so the DSP core can be tested without JUCE.
#pragma once
#include <vector>
#include <algorithm>
namespace juce {
template <typename T> class AudioBuffer {
public:
    void setSize (int ch, int n, bool = false, bool = false, bool = false) { d.assign (ch, std::vector<T> (n, T (0))); }
    void clear() { for (auto& v : d) std::fill (v.begin(), v.end(), T (0)); }
    T* getWritePointer (int c) { return d[c].data(); }
    const T* getReadPointer (int c) const { return d[c].data(); }
private:
    std::vector<std::vector<T>> d;
};
}
