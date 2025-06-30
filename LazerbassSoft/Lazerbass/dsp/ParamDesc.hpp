#pragma once
#include <cstdint>
#include <type_traits>

namespace dsp {

inline static constexpr int32_t ClampUncheck(int32_t value, int32_t min, int32_t max) {
    return value < min ? min : value > max ? max : value;
}

inline static constexpr float ClampUncheck(float value, float min, float max) {
    return value < min ? min : value > max ? max : value;
}

static constexpr float LerpUncheck(float a, float b, float t) {
    return a * (1 - t) + b * t;
}

struct IntParamDesc {
    const char* const name;
    const int32_t min;
    const int32_t max;
    const int32_t defaultValue;
    int32_t value{};
    const int32_t altMul;

    constexpr IntParamDesc(const char* name, int32_t min, int32_t max, int32_t defaultValue, int32_t altMul)
        : name(name), min(min), max(max), defaultValue(defaultValue), value(defaultValue), altMul(altMul) {}

    constexpr void Add(int32_t dvalue, bool alt) {
        if (alt) {
            value = ClampUncheck(value + dvalue * altMul, min, max);
        }
        else {
            value = ClampUncheck(value + dvalue, min, max);
        }
    }

    constexpr void Reset() { value = defaultValue; }

    constexpr int32_t Get() const { return value; }

    constexpr float GetValueAsNormalized() const {
        return static_cast<float>(value - min) / static_cast<float>(max - min);
    }
};

struct FloatParamDesc {
    static constexpr int32_t kScale = 10000;
    const char* const name;
    const int32_t min;
    const int32_t max;
    const int32_t step;
    const int32_t defaultValue;
    int32_t value{};
    const int32_t altMul;

    float modulationValue; // -1~1

    constexpr FloatParamDesc(const char* name, float min, float max, float step, float defaultValue, int32_t altMul)
        : name(name), min(min * kScale), max(max * kScale), step(step * kScale), defaultValue(defaultValue * kScale), value(defaultValue * kScale), altMul(altMul) {}

    constexpr void Add(int32_t dvalue, bool alt) {
        if (alt) {
            value = ClampUncheck(value + dvalue * step * altMul, min, max);
        }
        else {
            value = ClampUncheck(value + dvalue * step, min, max);
        }
    }

    constexpr float Get() const {
        return value / static_cast<float>(kScale); 
    }

    constexpr float GetWithModulation() const {
        auto map0 = (max - min) * modulationValue + value;
        auto clamp0 = map0 < min ? min : map0 > max ? max : map0;
        return clamp0 / static_cast<float>(kScale);
    }

    constexpr float GetFloatValue(float offset) const {
        auto map0 = (max - min) * offset + value;
        auto clamp0 = map0 < min ? min : map0 > max ? max : map0;
        return clamp0 / static_cast<float>(kScale);
    }

    constexpr void Reset() {
        value = defaultValue;
    }

    constexpr float GetValueAsNormalized() const {
        return static_cast<float>(value - min) / static_cast<float>(max - min);
    }
};

struct BoolParamDesc {
    const char* const name;
    bool value{};
    const bool defaultValue{};

    constexpr BoolParamDesc(const char* name, float defaultValue) 
        : name(name), value(defaultValue), defaultValue(defaultValue) {}

    constexpr void Add(int32_t dvalue) {
        if (dvalue > 0) {
            value = true;
        }
        else {
            value = false;
        }
    }

    constexpr bool Get() const {
        return value; 
    }

    constexpr void Reset() {
        value = defaultValue;
    }
};

template<class E> requires std::is_enum_v<E>
struct EnumNumTrait {
    static constexpr int32_t GetEnumNumber() {
        static_assert(requires { E::kCount; });
        return static_cast<int32_t>(E::kCount) - 1;
    }

    static constexpr int32_t kValue = GetEnumNumber();
};

template<class T> requires std::is_enum_v<T>
struct EnumParamDesc {
    const char* const name;
    const int32_t min = 0;
    const int32_t max = EnumNumTrait<T>::kValue;
    int32_t value{};
    const int32_t defaultValue{};

    constexpr EnumParamDesc(const char* name, T defaultValue) 
        : name(name), value(static_cast<int32_t>(defaultValue)), defaultValue(static_cast<int32_t>(defaultValue)) {}

    void Add(int32_t dvalue) {
        value = ClampUncheck(value + dvalue, min, max);
    }

    T Get() const {
        return static_cast<T>(value);
    }

    int32_t GetInt() const { return value; }

    void Reset() { value = defaultValue; }

    template<std::size_t N>
    constexpr const char* GetName(const char* const (&names)[N]) const {
        static_assert(N == EnumNumTrait<T>::kValue + 1);
        return names[value];
    }
};

}