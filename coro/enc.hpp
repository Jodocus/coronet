#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <clocale>
#include <codecvt>
#include <locale>
#include <string>

#if _MSC_VER <= 1900
#define BUG_VS
std::locale::id std::codecvt<char16_t, char, struct _Mbstatet>::id;
#endif

template <typename To, typename From>
struct caster { static_assert(true, "no conversion specialization found"); };

template <typename T>
struct caster<T, T> { T operator()(const T& t) { return t; } };

template <typename ToTraits, typename FromTraits, typename ToAlloc, typename FromAlloc>
struct caster<std::basic_string<char, ToTraits, ToAlloc>, std::basic_string<wchar_t, FromTraits, FromAlloc>> {
	auto operator()(const std::basic_string<wchar_t, FromTraits, FromAlloc>& wstr) {
		using namespace std;
		basic_string<char, ToTraits, ToAlloc> res;
		if(wstr.empty()) return res;
#ifdef _MSC_VER
		const auto size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
			static_cast<int>(wstr.length()), nullptr, 0, 0, 0);
		if(size == 0) throw range_error{ "WideCharToMultiByte failure" };
		res.resize(size);
		if(::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()),
			&res[0], static_cast<int>(res.length()), 0, 0) == 0)
			throw range_error{ "WideCharToMultiByte failure" };
#else
		auto loc = setlocale(LC_ALL, "en_US.utf8");
		mbstate_t state{ };
		auto src = wstr.c_str();
		auto l = wcsrtombs(nullptr, &src, 0, &state);
		if(l == -1) throw range_error{ "wcsrtombs failure" };
		res.resize(l);
		if(wcsrtombs(&res[0], &src, res.length(), &state) == -1)
			throw range_error{ "wcsrtombs failure" };
		setlocale(LC_ALL, loc);
#endif
		return res;
	}
};

template <typename ToTraits, typename FromTraits, typename ToAlloc, typename FromAlloc>
struct caster<std::basic_string<wchar_t, ToTraits, ToAlloc>, std::basic_string<char, FromTraits, FromAlloc>> {
	auto operator()(const std::basic_string<char, FromTraits, FromAlloc>& str) {
		using namespace std;
		basic_string<wchar_t, ToTraits, ToAlloc> res;
		if(str.empty()) return res;
#ifdef _MSC_VER
		const auto size = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
			static_cast<int>(str.length()), nullptr, 0);
		if(size == 0) throw range_error{ "MultiByteToWideChar failure" };
		res.resize(size);
		if(::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()),
			&res[0], static_cast<int>(res.length())) == 0)
			throw range_error{ "MultiByteToWideChar failure" };
#else
		auto loc = setlocale(LC_ALL, "en_US.utf8");
		mbstate_t state{ };
		auto l = mbsrtowcs(NULL, &mbstr, 0, &state);
		if(l == -1) throw range_error{ "mbsrtowcs failure" };
		res.resize(l);
		auto src = str.c_str();
		if(mbsrtowcs(&wstr[0], &src, wstr.length(), &state) == -1)
			throw range_error{ "mbsrtowcs failure" };
		setlocale(LC_ALL, loc);
#endif
		return res;
	}
};

template <typename ToTraits, typename FromTraits, typename ToAlloc, typename FromAlloc>
struct caster<std::basic_string<char16_t, ToTraits, ToAlloc>, std::basic_string<char, FromTraits, FromAlloc>> {
	auto operator ()(const std::basic_string<char, FromTraits, FromAlloc>& str) {
#ifdef BUG_VS
		std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
		auto conv = convert.from_bytes(str);
		return std::basic_string<char16_t, ToTraits, ToAlloc>{ reinterpret_cast<const char16_t*>(conv.c_str()), conv.length() };
#else
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
		return convert.from_bytes(str);
#endif
	}
};

template <typename ToTraits, typename FromTraits, typename ToAlloc, typename FromAlloc>
struct caster<std::basic_string<char, ToTraits, ToAlloc>, std::basic_string<char16_t, FromTraits, FromAlloc>> {
	auto operator ()(const std::basic_string<char16_t, FromTraits, FromAlloc>& str) {
#ifdef BUG_VS
		std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
		auto p = reinterpret_cast<const int16_t *>(str.data());
		return convert.to_bytes(p, p + str.size());
#else
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
		return convert.to_bytes(str);
#endif
	}
};


// Exceptions: std::range_error, std::bad_alloc
template <typename To, typename From>
auto code_cast(const From& str) noexcept(noexcept(std::declval<caster<To, From>>()(str))) {
	return caster<To, From>()(str);
}
